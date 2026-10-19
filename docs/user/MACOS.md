# macOS

Running a title on macOS (Apple silicon, Rosetta 2, MoltenVK) with the scripts in [tools/macos](../../tools/macos).

## Build

```
cmake -S . -B ../build-macos -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 -DBUILD_TESTING=ON
cmake --build ../build-macos -- -k 0
```

The scripts look for the build in `../build-macos`; set `ANYPS5_BUILD` for another directory. The Vulkan SDK is found in `~/VulkanSDK/<version>/macOS`; set `VULKAN_SDK` for another one.

## Set up a title

```
tools/macos/setup-game.sh <dump directory> <game directory> [module directory ...]
```

The dump directory holds `eboot.bin`. Module directories are the ones the title loads besides `sce_module`, relative to the dump, for example `Media/Modules Media/Plugins` for Unity titles. The script unwraps the SELF files into `<game directory>/.relink-input`, relinks them, links the rest of the dump into `app0/`, links `libs/` to the build and copies the launch scripts. It does not modify the dump.

Run it again after a change to the relinker. A change to the system libraries only needs the build.

## Run

- `<game directory>/run.sh`: plays the title. Background pipeline builds (`APS5_ASYNC_PIPELINES`) are on; `APS5_ASYNC_PIPELINES=0 ./run.sh` turns them off.
- `<game directory>/run-profile.sh`: the same with `APS5_PROFILE_DRAW` and `APS5_PROFILE_GPU`, logged to `logs/profile-<date>.log`.
- `python3 -I tools/macos/profile_summary.py <log> [--windows]`: frame rate and per-draw cost by scene weight, and the longest stalls.

Save data is in `<game directory>/_sd`.
