#if defined(_WIN32) || defined(__linux__)
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "prx/libc/include/CallTrace.hpp"
#include "prx/libc/include/general/AsmMacros.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>

extern "C" void CallTraceEnter_nid_no_patch();

namespace {

struct ImportRecord {
    void* target;
    std::string name;
    std::string library;
    mutable std::atomic<std::uint64_t> calls{0};
};

struct RingSlot {
    std::atomic<std::uint64_t> sequence{0};
    std::atomic<std::uint64_t> thread{0};
    std::atomic<std::uint64_t> returnAddress{0};
    std::atomic<const ImportRecord*> record{nullptr};
};

constexpr std::size_t RingCapacity = 65536;
constexpr std::size_t ThunkBytes = 24;
constexpr std::size_t ThunkAreaBytes = 1024 * 1024;

std::mutex g_mutex;
std::deque<ImportRecord> g_records;
std::unordered_map<void*, void*> g_thunkByTarget;
std::unique_ptr<RingSlot[]> g_ring;
std::atomic<std::uint64_t> g_sequence{0};
std::uint8_t* g_writable = nullptr;
std::uint8_t* g_executable = nullptr;
std::size_t g_thunks = 0;

bool Selected(std::string_view library) {
    const char* value = std::getenv("APS5_TRACE_CALLS");
    if (value == nullptr || *value == '\0') return false;
    std::string_view rest(value);
    if (library.ends_with(".prx")) library.remove_suffix(4);
    while (!rest.empty()) {
        const auto comma = rest.find(',');
        auto token = rest.substr(0, comma);
        rest = comma == std::string_view::npos ? std::string_view{} : rest.substr(comma + 1);
        if (token.ends_with(".prx")) token.remove_suffix(4);
        if (token == "all" || token == library) return true;
    }
    return false;
}

std::uint64_t CurrentThread() {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return static_cast<std::uint64_t>(gettid());
#endif
}

#ifdef _WIN32
void MapThunkArea() {
    const HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_EXECUTE_READWRITE, 0, static_cast<DWORD>(ThunkAreaBytes), nullptr);
    if (section == nullptr) throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "call trace: CreateFileMappingW");
    g_writable = static_cast<std::uint8_t*>(MapViewOfFile(section, FILE_MAP_WRITE, 0, 0, ThunkAreaBytes));
    g_executable = static_cast<std::uint8_t*>(MapViewOfFile(section, FILE_MAP_READ | FILE_MAP_EXECUTE, 0, 0, ThunkAreaBytes));
    const DWORD code = GetLastError();
    CloseHandle(section);
    if (g_writable == nullptr || g_executable == nullptr) throw std::system_error(static_cast<int>(code), std::system_category(), "call trace: MapViewOfFile");
}
#else
void MapThunkArea() {
    const int file = memfd_create("aps5-call-trace", MFD_CLOEXEC);
    if (file < 0) throw std::system_error(errno, std::generic_category(), "call trace: memfd_create");
    void* writable = MAP_FAILED;
    void* executable = MAP_FAILED;
    if (ftruncate(file, ThunkAreaBytes) == 0) {
        writable = mmap(nullptr, ThunkAreaBytes, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
        if (writable != MAP_FAILED) executable = mmap(nullptr, ThunkAreaBytes, PROT_READ | PROT_EXEC, MAP_SHARED, file, 0);
    }
    const int code = errno;
    close(file);
    if (writable == MAP_FAILED || executable == MAP_FAILED) {
        if (writable != MAP_FAILED) munmap(writable, ThunkAreaBytes);
        throw std::system_error(code, std::generic_category(), "call trace: mmap");
    }
    g_writable = static_cast<std::uint8_t*>(writable);
    g_executable = static_cast<std::uint8_t*>(executable);
}

struct sigaction g_previousAbort{};

void DescribeAddress(std::uint64_t address, char* buffer, std::size_t size) {
    Dl_info info{};
    if (dladdr(reinterpret_cast<const void*>(address), &info) != 0 && info.dli_fname != nullptr && info.dli_fbase != nullptr) {
        const char* slash = std::strrchr(info.dli_fname, '/');
        const char* name = slash == nullptr ? info.dli_fname : slash + 1;
        std::snprintf(buffer, size, "0x%016llx %s+0x%llx", static_cast<unsigned long long>(address), *name == '\0' ? "?" : name,
            static_cast<unsigned long long>(address - reinterpret_cast<std::uint64_t>(info.dli_fbase)));
        return;
    }
    std::snprintf(buffer, size, "0x%016llx", static_cast<unsigned long long>(address));
}

void ReportCallTrace() {
    static CallTraceEntry entries[64];
    const std::size_t count = CopyCallTrace_nid_no_patch(entries, std::size(entries));
    if (count == 0) return;
    char line[4096 + 64];
    std::fprintf(stderr, "  last traced system library calls (APS5_TRACE_CALLS):\n");
    for (std::size_t index = 0; index < count; ++index) {
        DescribeAddress(entries[index].returnAddress, line, sizeof(line));
        std::fprintf(stderr, "    #%llu thread %llu %s %s from %s\n", static_cast<unsigned long long>(entries[index].sequence),
            static_cast<unsigned long long>(entries[index].thread), entries[index].library, entries[index].name, line);
    }
    static CallTraceCount counts[32];
    std::uint64_t total = 0;
    const std::size_t counted = CopyCallCounts_nid_no_patch(counts, std::size(counts), &total);
    if (counted == 0) return;
    std::fprintf(stderr, "  most called traced imports (%llu calls in total):\n", static_cast<unsigned long long>(total));
    for (std::size_t index = 0; index < counted; ++index) {
        std::fprintf(stderr, "    %12llu %s %s\n", static_cast<unsigned long long>(counts[index].calls), counts[index].library, counts[index].name);
    }
}

void ReportOnAbort(int number, siginfo_t* info, void* context) {
    std::fprintf(stderr, "[libc] SIGABRT\n");
    ReportCallTrace();
    std::fflush(stderr);
    if ((g_previousAbort.sa_flags & SA_SIGINFO) != 0) {
        if (g_previousAbort.sa_sigaction != nullptr) g_previousAbort.sa_sigaction(number, info, context);
        return;
    }
    if (g_previousAbort.sa_handler == SIG_IGN) return;
    if (g_previousAbort.sa_handler != SIG_DFL) {
        g_previousAbort.sa_handler(number);
        return;
    }
    std::signal(number, SIG_DFL);
    std::raise(number);
}

void InstallAbortReport() {
    struct sigaction action{};
    action.sa_sigaction = ReportOnAbort;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGABRT, &action, &g_previousAbort) != 0) throw std::system_error(errno, std::generic_category(), "call trace: sigaction");
}
#endif

void* WriteThunk(const ImportRecord& record) {
    if ((g_thunks + 1) * ThunkBytes > ThunkAreaBytes) throw std::runtime_error("call trace: thunk area exhausted");
    const std::size_t offset = g_thunks * ThunkBytes;
    const std::uint8_t prefix[] = {0x49, 0xbb};
    const std::uint8_t jump[] = {0xff, 0x25, 0, 0, 0, 0};
    const auto recordAddress = reinterpret_cast<std::uint64_t>(&record);
    const auto handler = reinterpret_cast<std::uint64_t>(&CallTraceEnter_nid_no_patch);
    std::uint8_t* thunk = g_writable + offset;
    std::memcpy(thunk, prefix, sizeof(prefix));
    std::memcpy(thunk + 2, &recordAddress, sizeof(recordAddress));
    std::memcpy(thunk + 10, jump, sizeof(jump));
    std::memcpy(thunk + 16, &handler, sizeof(handler));
#ifdef _WIN32
    FlushInstructionCache(GetCurrentProcess(), g_executable + offset, ThunkBytes);
#else
    __builtin___clear_cache(reinterpret_cast<char*>(g_executable + offset), reinterpret_cast<char*>(g_executable + offset + ThunkBytes));
#endif
    ++g_thunks;
    return g_executable + offset;
}

}

static_assert(offsetof(ImportRecord, target) == 0);

extern "C" {

void APS5_VABI CallTraceRecord_nid_no_patch(const void* record, std::uint64_t returnAddress) {
    static_cast<const ImportRecord*>(record)->calls.fetch_add(1, std::memory_order_relaxed);
    const std::uint64_t sequence = g_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    RingSlot& slot = g_ring[sequence % RingCapacity];
    slot.sequence.store(0, std::memory_order_release);
    slot.thread.store(CurrentThread(), std::memory_order_relaxed);
    slot.returnAddress.store(returnAddress, std::memory_order_relaxed);
    slot.record.store(static_cast<const ImportRecord*>(record), std::memory_order_relaxed);
    slot.sequence.store(sequence, std::memory_order_release);
}

void* TraceImport_nid_no_patch(void* target, const char* name, const char* library) {
    if (target == nullptr || name == nullptr || library == nullptr || !Selected(library)) return target;
    std::lock_guard lock(g_mutex);
    if (g_ring == nullptr) {
        MapThunkArea();
        g_ring = std::make_unique<RingSlot[]>(RingCapacity);
#ifdef __linux__
        InstallAbortReport();
#endif
    }
    if (const auto existing = g_thunkByTarget.find(target); existing != g_thunkByTarget.end()) return existing->second;
    g_records.emplace_back(target, name, library);
    void* thunk = WriteThunk(g_records.back());
    g_thunkByTarget.emplace(target, thunk);
    return thunk;
}

std::size_t CopyCallTrace_nid_no_patch(CallTraceEntry* entries, std::size_t capacity) {
    if (entries == nullptr || capacity == 0 || g_ring == nullptr) return 0;
    const std::uint64_t last = g_sequence.load(std::memory_order_acquire);
    const std::uint64_t span = std::min<std::uint64_t>({last, capacity, RingCapacity});
    std::size_t count = 0;
    for (std::uint64_t sequence = last - span + 1; sequence <= last; ++sequence) {
        const RingSlot& slot = g_ring[sequence % RingCapacity];
        if (slot.sequence.load(std::memory_order_acquire) != sequence) continue;
        const auto* record = slot.record.load(std::memory_order_relaxed);
        const CallTraceEntry entry{sequence, slot.thread.load(std::memory_order_relaxed), slot.returnAddress.load(std::memory_order_relaxed), record->name.c_str(), record->library.c_str()};
        if (slot.sequence.load(std::memory_order_acquire) != sequence) continue;
        entries[count++] = entry;
    }
    return count;
}

std::size_t CopyCallCounts_nid_no_patch(CallTraceCount* entries, std::size_t capacity, std::uint64_t* total) {
    if (total != nullptr) *total = g_sequence.load(std::memory_order_acquire);
    if (entries == nullptr || capacity == 0) return 0;
    std::unique_lock lock(g_mutex, std::try_to_lock);
    if (!lock.owns_lock()) return 0;
    std::size_t count = 0;
    for (const auto& record : g_records) {
        const std::uint64_t calls = record.calls.load(std::memory_order_relaxed);
        if (calls == 0 || (count == capacity && calls <= entries[count - 1].calls)) continue;
        std::size_t position = count < capacity ? count++ : count - 1;
        while (position > 0 && entries[position - 1].calls < calls) {
            entries[position] = entries[position - 1];
            --position;
        }
        entries[position] = {calls, record.name.c_str(), record.library.c_str()};
    }
    return count;
}

}

asm(".text\n"
    APS5_ASM_FUNCTION("CallTraceEnter_nid_no_patch")
    "    push %rdi\n"
    "    push %rsi\n"
    "    push %rdx\n"
    "    push %rcx\n"
    "    push %r8\n"
    "    push %r9\n"
    "    push %rax\n"
    "    push %r10\n"
    "    push %r11\n"
    "    sub $128, %rsp\n"
    "    movdqu %xmm0, 0(%rsp)\n"
    "    movdqu %xmm1, 16(%rsp)\n"
    "    movdqu %xmm2, 32(%rsp)\n"
    "    movdqu %xmm3, 48(%rsp)\n"
    "    movdqu %xmm4, 64(%rsp)\n"
    "    movdqu %xmm5, 80(%rsp)\n"
    "    movdqu %xmm6, 96(%rsp)\n"
    "    movdqu %xmm7, 112(%rsp)\n"
    "    mov %r11, %rdi\n"
    "    mov 200(%rsp), %rsi\n"
    "    call CallTraceRecord_nid_no_patch\n"
    "    movdqu 0(%rsp), %xmm0\n"
    "    movdqu 16(%rsp), %xmm1\n"
    "    movdqu 32(%rsp), %xmm2\n"
    "    movdqu 48(%rsp), %xmm3\n"
    "    movdqu 64(%rsp), %xmm4\n"
    "    movdqu 80(%rsp), %xmm5\n"
    "    movdqu 96(%rsp), %xmm6\n"
    "    movdqu 112(%rsp), %xmm7\n"
    "    add $128, %rsp\n"
    "    pop %r11\n"
    "    pop %r10\n"
    "    pop %rax\n"
    "    pop %r9\n"
    "    pop %r8\n"
    "    pop %rcx\n"
    "    pop %rdx\n"
    "    pop %rsi\n"
    "    pop %rdi\n"
    "    jmp *(%r11)\n"
    APS5_ASM_FUNCTION_END("CallTraceEnter_nid_no_patch"));
#endif
