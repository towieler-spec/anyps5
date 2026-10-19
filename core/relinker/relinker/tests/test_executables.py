from pathlib import Path
import os
import stat
import struct
import subprocess
import sys
import tempfile

from test_guest_host_libc import importing
from test_guest_intel_trampolines import main_fixture, pe_sections
from test_guest_module_dir import pe_export_names
from test_guest_module_directories import module_with_symbol, needed_libraries
from test_guest_needed_modules import executable_with_needed


LOAD_RVA = 0x10000
LAYOUTS = ((0x1050, 0x2840, 0x1000), (0x1060, 0x2850, 0x2000))
FLAG = 0x2820
STORE = b"\x48\xc7\x80"


def initialized_module(init, slots, size):
    image = bytearray(0x2000 + size)
    image[:16] = b"\x7fELF\x02\x01\x01" + bytes(9)
    struct.pack_into("<HHIQQQIHHHHHH", image, 16, 3, 62, 1, 0, 64, 0, 0, 64, 56, 4, 0, 0, 0)
    struct.pack_into("<IIQQQQQQ", image, 64, 1, 5, 0, 0, 0, 0x2000, 0x2000, 0x1000)
    struct.pack_into("<IIQQQQQQ", image, 176, 0x6474E551, 6, 0, 0, 0, 0, 0, 16)
    struct.pack_into("<IIQQQQQQ", image, 232, 1, 6, 0x2000, 0x2000, 0x2000, size, size, 0x1000)
    strings = b"\0shared\0"
    tags = [(5, 0x2600), (10, len(strings)), (6, 0x2400), (11, 24), (4, 0x2500),
            (7, 0x2700), (8, 48), (9, 24), (12, init), (25, slots), (27, 16), (0, 0)]
    struct.pack_into("<IIQQQQQQ", image, 120, 2, 6, 0x2000, 0x2000, 0x2000, len(tags) * 16, len(tags) * 16, 8)
    for index, tag in enumerate(tags):
        struct.pack_into("<qQ", image, 0x2000 + index * 16, *tag)
    image[0x2600:0x2600 + len(strings)] = strings
    struct.pack_into("<IBBHQQ", image, 0x2418, 1, 0x12, 0, 1, 0x1000, 6)
    struct.pack_into("<IIII", image, 0x2500, 1, 2, 0, 0)
    image[0x1000:0x1006] = b"\xb8\x2a\x00\x00\x00\xc3"
    code = b"\xc7\x05" + struct.pack("<i", FLAG - (init + 10)) + b"\x01\x00\x00\x00\xc3"
    image[init:init + len(code)] = code
    for index, entry in enumerate((0x1010, 0x1030)):
        struct.pack_into("<QQq", image, 0x2700 + index * 24, slots + index * 8, 8, entry)
        code = b"\x8b\x05" + struct.pack("<i", FLAG - (entry + 6)) + b"\x83\xf8\x01\x74\x02\x0f\x0b\xc3"
        image[entry:entry + len(code)] = code
    return image


def lifecycle_calls(init, slots):
    return [b"\xb9" + struct.pack("<I", LOAD_RVA + address) for address in (init, slots, slots + 8)]


def handle_loads(data, instructions):
    _, rva, size, raw, _ = next(section for section in pe_sections(data) if section[0] == b".entry")
    code = data[raw:raw + size]
    loads = []
    for instruction in instructions:
        position = code.find(instruction)
        while position >= 0:
            if code[position - 7:position - 4] == b"\x48\x8b\x05":
                loads.append((position, rva + position + struct.unpack_from("<i", code, position - 4)[0]))
            position = code.find(instruction, position + 1)
    return sorted(loads)


def check_startup_initialization(needing, deferring, modules):
    cleared, called = [], []
    for module, (init, slots, _) in zip(modules, LAYOUTS):
        sections = {name: (rva, size, raw, flags) for name, rva, size, raw, flags in pe_sections(module)}
        table, _, raw, flags = sections[b".ginit"]
        assert flags & 0x80000000, f"the shared module's .ginit section is not writable: {flags:#x}"
        assert "__aps5_guest_initialize" in pe_export_names(module), pe_export_names(module)
        table_words = struct.unpack_from("<IIII", module, raw)
        assert table_words == (LOAD_RVA + init, 2, LOAD_RVA + slots, LOAD_RVA + slots + 8), table_words
        clear = handle_loads(needing, [STORE + struct.pack("<I", table) + bytes(4)])
        calls = handle_loads(needing, lifecycle_calls(init, slots))
        assert len(clear) == 1 and [handle for _, handle in calls] == [clear[0][1]] * 3, (init, clear, calls)
        cleared += clear
        called += calls
    stores = handle_loads(needing, [STORE])
    assert stores == sorted(cleared), (stores, cleared)
    assert len({handle for _, handle in cleared}) == len(modules), cleared
    assert max(cleared)[0] < min(called)[0], (cleared, called)
    calls = [call for init, slots, _ in LAYOUTS for call in lifecycle_calls(init, slots)]
    assert not handle_loads(deferring, [STORE]) and not handle_loads(deferring, calls), "the deferring executable initializes a shared module at startup"


def main():
    relinker = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="anyps5-executables-") as directory:
        work = Path(directory)

        def convert(case, windows, options, picker=None, eboot=None):
            (case / "sce_module").mkdir(parents=True, exist_ok=True)
            (case / "sce_module" / "bundled.prx").write_bytes(module_with_symbol(True))
            (case / "picker").mkdir(exist_ok=True)
            (case / "picker" / "picker.bin").write_bytes(main_fixture() if picker is None else picker)
            source = case / "eboot.bin"
            source.write_bytes(main_fixture() if eboot is None else eboot)
            output = case / "out" / ("app.exe" if windows else "app.elf")
            output.parent.mkdir(exist_ok=True)
            result = subprocess.run([str(relinker), *(["--windows"] if windows else []), "--registry", *options, str(source), str(output)],
                                    capture_output=True, text=True, timeout=60)
            return result, output

        for windows in (False, True):
            suffix = ".exe" if windows else ".elf"
            case = work / f"{windows}-picker"
            result, output = convert(case, windows, ["--executable", "picker/picker.bin"])
            assert result.returncode == 0, (result.stdout, result.stderr)
            secondary = output.parent / ("picker.picker.bin" + suffix)
            assert secondary.read_bytes().startswith(b"MZ" if windows else b"\x7fELF"), secondary
            assert "Executable: picker/picker.bin\n" in result.stdout, result.stdout
            assert f"Executable file: {secondary}\n" in result.stdout, result.stdout
            assert f"  |-- app{suffix}\n  |-- picker.picker.bin{suffix}\n  |-- libs/\n" in result.stdout, result.stdout
            assert (output.parent / "picker.picker.bin.registry.json").is_file()
            assert sorted(path.name for path in output.parent.rglob("*.guest.prx")) == ["bundled.prx.guest.prx"]
            if not windows:
                assert "$ORIGIN/app0/sce_module/bundled.prx.guest.prx" in needed_libraries(secondary.read_bytes())
                if os.name != "nt":
                    for executable in (output, secondary):
                        assert stat.S_IMODE(executable.stat().st_mode) & 0o111 == 0o111, (executable, oct(executable.stat().st_mode))
            if windows and os.name == "nt":
                for executable in (output, secondary):
                    run = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
                    assert run.returncode == 42, (executable, run.returncode, run.stdout, run.stderr)

            for options, message in ((["--executable", "../eboot.bin"], "inside the guest module parent directory"),
                                     (["--executable", "picker"], "is not a file"),
                                     (["--executable", "picker/missing.bin"], "is not a file"),
                                     (["--executable", "eboot.bin"], "converted twice"),
                                     (["--executable", "picker/picker.bin", "--executable", "picker/./picker.bin"], "converted twice"),
                                     (["--module-dir", "picker", "--executable", "picker/picker.bin"], "in a guest module directory"),
                                     (["--module-dir", "picker/", "--executable", "picker/picker.bin"], "in a guest module directory")):
                case = work / f"{windows}-rejected-{len(message)}-{len(options)}"
                result, output = convert(case, windows, options)
                assert result.returncode == 2 and message in result.stderr, (options, result.stderr)
                assert not output.exists(), options

            for index, (directory, executable) in enumerate((("Picker", "picker/picker.bin"), ("picker", "PICKER/picker.bin"), ("linked", "picker/picker.bin"))):
                case = work / f"{windows}-identity-{index}"
                case.mkdir()
                insensitive = (work / f"{windows}-IDENTITY-{index}").exists()
                if (directory == "linked" and os.name == "nt") or (executable != "picker/picker.bin" and not insensitive):
                    continue
                if directory == "linked":
                    (case / "linked").symlink_to("picker", target_is_directory=True)
                elif not insensitive:
                    (case / directory).mkdir()
                result, output = convert(case, windows, ["--module-dir", directory, "--executable", executable])
                if insensitive or directory == "linked":
                    assert result.returncode == 2 and "in a guest module directory" in result.stderr, (directory, executable, result.stderr)
                    assert not output.exists(), (directory, executable)
                else:
                    assert result.returncode == 0, (directory, executable, result.stdout, result.stderr)

            case = work / f"{windows}-needed"
            (case / "Media" / "Modules").mkdir(parents=True)
            (case / "Media" / "Modules" / "needed.prx").write_bytes(module_with_symbol(True))
            result, output = convert(case, windows, ["--module-dir", "Media/Modules", "--executable", "picker/picker.bin"], executable_with_needed())
            if windows:
                assert result.returncode == 0, (result.stdout, result.stderr)
                assert "__aps5_guest_initialize" in pe_export_names((output.parent / "app0" / "Media" / "Modules" / "needed.prx.guest.prx").read_bytes())
            else:
                assert result.returncode == 0, (result.stdout, result.stderr)
                secondary = output.parent / "picker.picker.bin.elf"
                assert "$ORIGIN/app0/Media/Modules/needed.prx.guest.prx" in needed_libraries(secondary.read_bytes())
                assert "$ORIGIN/app0/Media/Modules/needed.prx.guest.prx" not in needed_libraries(output.read_bytes())

        shared = []
        for reverse in (False, True):
            case = work / f"windows-initialized-{reverse}"
            (case / "Media" / "Modules").mkdir(parents=True)
            for name, layout in zip(("needed.prx", "second.prx"), LAYOUTS):
                (case / "Media" / "Modules" / name).write_bytes(initialized_module(*layout))
            both = executable_with_needed(others=(b"second.prx",))
            picker, eboot = (None, both) if reverse else (both, None)
            result, output = convert(case, True, ["--module-dir", "Media/Modules", "--executable", "picker/picker.bin"], picker, eboot)
            assert result.returncode == 0, (reverse, result.stdout, result.stderr)
            secondary = output.parent / "picker.picker.bin.exe"
            shared.append([(output.parent / "app0" / "Media" / "Modules" / name).read_bytes() for name in ("needed.prx.guest.prx", "second.prx.guest.prx")])
            assert "__aps5_guest_initialize" not in pe_export_names((output.parent / "app0" / "sce_module" / "bundled.prx.guest.prx").read_bytes())
            needing, deferring = (output, secondary) if reverse else (secondary, output)
            check_startup_initialization(needing.read_bytes(), deferring.read_bytes(), shared[-1])
            if os.name == "nt":
                for executable in (output, secondary):
                    run = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
                    assert run.returncode == 42, (reverse, executable, run.returncode, run.stdout, run.stderr)
        assert shared[0] == shared[1], "a shared module depends on which executable defers it"

        for label, windows, target in (("linux", False, []), ("macos", False, ["--macos"]), ("windows", True, [])):
            for name, picker in (("subset", importing("libScePad.prx")), ("order", importing("libSceVideoOut.prx", "libScePad.prx"))):
                case = work / f"{label}-host-{name}"
                result, output = convert(case, windows, [*target, "--executable", "picker/picker.bin"], picker, importing("libScePad.prx", "libSceVideoOut.prx"))
                assert result.returncode == 0, (label, name, result.stdout, result.stderr)
                modules = sorted(output.parent.rglob("*.guest.prx"))
                assert [path.name for path in modules] == ["bundled.prx.guest.prx"], (label, name, modules)
                if label == "linux":
                    secondary = output.parent / "picker.picker.bin.elf"
                    assert needed_libraries(modules[0].read_bytes()) == [], needed_libraries(modules[0].read_bytes())
                    assert "libSceVideoOut.prx" in needed_libraries(output.read_bytes())
                    assert "libScePad.prx" in needed_libraries(secondary.read_bytes())

        if os.name != "nt":
            result, output = convert(work / "macos-picker", False, ["--macos", "--executable", "picker/picker.bin"])
            assert result.returncode == 0, (result.stdout, result.stderr)
            for executable in (output, output.parent / "picker.picker.bin.elf"):
                assert executable.read_bytes().startswith(bytes.fromhex("cffaedfe")), executable
                assert stat.S_IMODE(executable.stat().st_mode) & 0o111 == 0o111, (executable, oct(executable.stat().st_mode))


if __name__ == "__main__":
    main()
