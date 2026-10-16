#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/KernelErrors.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

extern "C" {
int APS5_VABI sceKernelOpen(const char* path, int flags, std::uint16_t mode);
int APS5_VABI sceKernelClose(int d);
std::int64_t APS5_VABI sceKernelLseek(int d, std::int64_t offset, int whence);
std::int64_t APS5_VABI lseek_nid_postfix(int d, std::int64_t offset, int whence);
int* APS5_VABI __error_nid_postfix();
int APS5_VABI pipe_nid_postfix(int* descriptors);
int APS5_VABI close_nid_postfix(int d);
}

namespace {

void Require(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "lseek whence check failed at line %d\n", line);
        std::exit(1);
    }
}
#define Check(value) Require((value), __LINE__)

constexpr int GuestEbadf = 9;
constexpr int GuestEinval = 22;
constexpr int GuestEspipe = 29;

std::filesystem::path EnterFreshRoot() {
    const auto root = std::filesystem::temp_directory_path() /
        ("anyps5-lseek-whence-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    std::filesystem::current_path(root);
    return root;
}

void UnknownWhence() {
    { std::ofstream file("data.bin", std::ios::binary); file << "0123456789"; }
    const int fd = sceKernelOpen("data.bin", SCE_KERNEL_O_RDONLY, 0);
    Check(fd >= 0);
    Check(sceKernelLseek(fd, 4, 0) == 4);
    for (const int whence : {-1, 5, 6, 0x7fffffff}) {
        Check(sceKernelLseek(fd, 0, whence) == SCE_KERNEL_ERROR_EINVAL);
        *__error_nid_postfix() = 0;
        Check(lseek_nid_postfix(fd, 0, whence) == -1 && *__error_nid_postfix() == GuestEinval);
    }
    Check(sceKernelLseek(fd, 0, 1) == 4);
    Check(sceKernelClose(fd) == 0);

    Check(sceKernelLseek(fd, 0, 9) == SCE_KERNEL_ERROR_EBADF);
    *__error_nid_postfix() = 0;
    Check(lseek_nid_postfix(fd, 0, 9) == -1 && *__error_nid_postfix() == GuestEbadf);
    int pipe[2] = {-1, -1};
    Check(pipe_nid_postfix(pipe) == 0);
    for (const int end : pipe) {
        Check(sceKernelLseek(end, 0, 9) == SCE_KERNEL_ERROR_ESPIPE);
        *__error_nid_postfix() = 0;
        Check(lseek_nid_postfix(end, 0, -1) == -1 && *__error_nid_postfix() == GuestEspipe);
    }
    Check(close_nid_postfix(pipe[0]) == 0 && close_nid_postfix(pipe[1]) == 0);
}


}

int main() {
    const auto root = EnterFreshRoot();
    UnknownWhence();
    std::error_code error;
    std::filesystem::current_path(std::filesystem::temp_directory_path(), error);
    std::filesystem::remove_all(root, error);
    return 0;
}
