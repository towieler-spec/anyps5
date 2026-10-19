#include "SceTypes.hpp"
#ifdef __linux__
#include "prx/libc/include/GuestAllocations.hpp"
#include <atomic>
#include <cstdio>
#include <chrono>
#include <thread>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>
#endif
#include <cstddef>
#include <cstdint>
#include <cstdlib>

extern "C" {
int APS5_VABI scePthreadCreate(Pthread* thread, const PthreadAttr* attr, PthreadEntry entry, void* arg, const char* name);
int APS5_VABI scePthreadJoin(Pthread thread, void** retval);
int APS5_VABI scePthreadAttrInit(PthreadAttr* attr);
int APS5_VABI scePthreadAttrDestroy(PthreadAttr* attr);
int APS5_VABI scePthreadAttrGet(Pthread thread, PthreadAttr* attr);
int APS5_VABI scePthreadAttrSetstacksize(PthreadAttr* attr, std::size_t stackSize);
int APS5_VABI scePthreadAttrGetstack(const PthreadAttr* attr, void** stackAddress, std::size_t* stackSize);
Pthread APS5_VABI scePthreadSelf();
#ifdef __linux__
int APS5_VABI scePthreadDetach(Pthread thread);
#endif
}

static constexpr int SCE_OK = 0;
static constexpr std::size_t FRAME_SIZE = 4096;
static constexpr std::size_t FRAME_MARGIN = 2 * FRAME_SIZE;
static constexpr std::size_t HOST_CALL_STACK = 256 * 1024;
static constexpr std::size_t STACK_SIZES[] = {16384, 65536, 1u << 20, 16u << 20};

thread_local unsigned char threadLocalBlock[512 * 1024];

static void Require(bool value) { if (!value) std::abort(); }

static std::uintptr_t Descend(std::uintptr_t floor);
static std::uintptr_t (*volatile descend)(std::uintptr_t) = Descend;

static std::uintptr_t Descend(std::uintptr_t floor) {
    volatile unsigned char frame[FRAME_SIZE];
    frame[0] = 0x5a;
    frame[FRAME_SIZE - 1] = 0xa5;
    const auto here = reinterpret_cast<std::uintptr_t>(&frame[0]);
    const auto deepest = here < floor + FRAME_MARGIN ? here : descend(floor);
    Require(frame[0] == 0x5a && frame[FRAME_SIZE - 1] == 0xa5);
    return deepest;
}

static void* APS5_VABI Worker(void* arg) {
    const auto requested = *static_cast<const std::size_t*>(arg);
    threadLocalBlock[0] = 1;
    threadLocalBlock[sizeof(threadLocalBlock) - 1] = 2;

    PthreadAttr attr = nullptr;
    Require(scePthreadAttrInit(&attr) == SCE_OK);
    Require(scePthreadAttrGet(scePthreadSelf(), &attr) == SCE_OK);
    void* address = nullptr;
    std::size_t size = 0;
    Require(scePthreadAttrGetstack(&attr, &address, &size) == SCE_OK);
    Require(scePthreadAttrDestroy(&attr) == SCE_OK);
    Require(size == requested);

    const auto begin = reinterpret_cast<std::uintptr_t>(address);
    const auto local = reinterpret_cast<std::uintptr_t>(&size);
    Require(local > begin && local < begin + size);
    Require(local - begin + FRAME_SIZE >= requested);
    const auto deepest = Descend(begin);
    Require(deepest >= begin && deepest < begin + FRAME_MARGIN);
    const auto hostDeepest = descend(begin - HOST_CALL_STACK);
    Require(hostDeepest < begin - HOST_CALL_STACK + FRAME_MARGIN);

    Require(threadLocalBlock[0] == 1 && threadLocalBlock[sizeof(threadLocalBlock) - 1] == 2);
    return arg;
}

#ifdef __linux__
struct StackProbe {
    std::size_t requested = 0;
    std::uintptr_t stack = 0;
    std::uintptr_t markerAddress = 0;
    std::uintptr_t marker = 0;
    std::atomic<bool> done{false};
    int hostTid = 0;
};

static void* APS5_VABI RetainedStackWorker(void* opaque) {
    auto& probe = *static_cast<StackProbe*>(opaque);
    PthreadAttr attr = nullptr;
    Require(scePthreadAttrInit(&attr) == SCE_OK);
    Require(scePthreadAttrGet(scePthreadSelf(), &attr) == SCE_OK);
    void* address = nullptr;
    std::size_t size = 0;
    Require(scePthreadAttrGetstack(&attr, &address, &size) == SCE_OK);
    Require(scePthreadAttrDestroy(&attr) == SCE_OK);
    Require(size == probe.requested);
    probe.stack = reinterpret_cast<std::uintptr_t>(address);
    probe.markerAddress = probe.stack + size - 4096;
    *reinterpret_cast<std::uintptr_t*>(probe.markerAddress) = probe.marker;
    probe.hostTid = static_cast<int>(syscall(SYS_gettid));
    probe.done.store(true, std::memory_order_release);
    return nullptr;
}

static void WaitForProbe(StackProbe& probe) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!probe.done.load(std::memory_order_acquire)) {
        Require(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const auto task = "/proc/self/task/" + std::to_string(probe.hostTid);
    while (access(task.c_str(), F_OK) == 0) {
        Require(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

static void RequireRetained(const StackProbe& probe) {
    const auto range = GuestAllocations::GuestAllocationsLookup_nid_postfix(probe.markerAddress, false);
    Require(range != nullptr && range->readable && range->writable);
    Require(*reinterpret_cast<const std::uintptr_t*>(probe.markerAddress) == probe.marker);
}

static bool ReusesOneOf(const StackProbe& probe, const StackProbe* previous, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) if (probe.stack == previous[i].stack) return true;
    return false;
}
#endif

int main() {
    for (auto stackSize : STACK_SIZES) {
        PthreadAttr attr = nullptr;
        Require(scePthreadAttrInit(&attr) == SCE_OK);
        Require(scePthreadAttrSetstacksize(&attr, stackSize) == SCE_OK);
        Pthread thread = nullptr;
        Require(scePthreadCreate(&thread, &attr, Worker, &stackSize, nullptr) == SCE_OK);
        Require(scePthreadAttrDestroy(&attr) == SCE_OK);
        void* result = nullptr;
        Require(scePthreadJoin(thread, &result) == SCE_OK);
        Require(result == &stackSize);
    }
#ifdef __linux__
    constexpr std::size_t JoinedStackSize = 81920;
    StackProbe joined[6];
    for (std::size_t i = 0; i < 6; ++i) {
        joined[i].requested = JoinedStackSize;
        joined[i].marker = 0x77000000u + i;
        PthreadAttr attr = nullptr;
        Require(scePthreadAttrInit(&attr) == SCE_OK);
        Require(scePthreadAttrSetstacksize(&attr, JoinedStackSize) == SCE_OK);
        Pthread thread = nullptr;
        Require(scePthreadCreate(&thread, &attr, RetainedStackWorker, &joined[i], nullptr) == SCE_OK);
        Require(scePthreadAttrDestroy(&attr) == SCE_OK);
        Require(scePthreadJoin(thread, nullptr) == SCE_OK);
        RequireRetained(joined[i]);
    }
    StackProbe joinedReuse;
    joinedReuse.requested = JoinedStackSize;
    joinedReuse.marker = 0x7700ffffu;
    PthreadAttr joinedAttr = nullptr;
    Require(scePthreadAttrInit(&joinedAttr) == SCE_OK);
    Require(scePthreadAttrSetstacksize(&joinedAttr, JoinedStackSize) == SCE_OK);
    Pthread joinedThread = nullptr;
    Require(scePthreadCreate(&joinedThread, &joinedAttr, RetainedStackWorker, &joinedReuse, nullptr) == SCE_OK);
    Require(scePthreadAttrDestroy(&joinedAttr) == SCE_OK);
    Require(scePthreadJoin(joinedThread, nullptr) == SCE_OK);
    Require(ReusesOneOf(joinedReuse, joined, 6));
    Require(*reinterpret_cast<const std::uintptr_t*>(joinedReuse.markerAddress) == joinedReuse.marker);

    constexpr std::size_t DetachedStackSize = 114688;
    StackProbe detached[5];
    for (std::size_t i = 0; i < 5; ++i) {
        detached[i].requested = DetachedStackSize;
        detached[i].marker = 0x77300000u + i;
        PthreadAttr attr = nullptr;
        Require(scePthreadAttrInit(&attr) == SCE_OK);
        Require(scePthreadAttrSetstacksize(&attr, DetachedStackSize) == SCE_OK);
        Pthread thread = nullptr;
        Require(scePthreadCreate(&thread, &attr, RetainedStackWorker, &detached[i], nullptr) == SCE_OK);
        Require(scePthreadAttrDestroy(&attr) == SCE_OK);
        Require(scePthreadDetach(thread) == SCE_OK);
        WaitForProbe(detached[i]);
        RequireRetained(detached[i]);
    }
    StackProbe detachedReuse;
    detachedReuse.requested = DetachedStackSize;
    detachedReuse.marker = 0x7730ffffu;
    PthreadAttr detachedAttr = nullptr;
    Require(scePthreadAttrInit(&detachedAttr) == SCE_OK);
    Require(scePthreadAttrSetstacksize(&detachedAttr, DetachedStackSize) == SCE_OK);
    Pthread detachedThread = nullptr;
    Require(scePthreadCreate(&detachedThread, &detachedAttr, RetainedStackWorker, &detachedReuse, nullptr) == SCE_OK);
    Require(scePthreadAttrDestroy(&detachedAttr) == SCE_OK);
    Require(scePthreadJoin(detachedThread, nullptr) == SCE_OK);
    Require(ReusesOneOf(detachedReuse, detached, 5));
    Require(*reinterpret_cast<const std::uintptr_t*>(detachedReuse.markerAddress) == detachedReuse.marker);
#endif
}
