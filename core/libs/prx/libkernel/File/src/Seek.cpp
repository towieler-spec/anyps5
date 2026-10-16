#include "prx/libkernel/File/include/Seek.hpp"
#include "prx/libkernel/File/include/NativeStat.hpp"
#include "SceTypes.hpp"

#include <cerrno>
#include <cstdint>

namespace {

constexpr int GuestEinval = 22;
constexpr int GuestEspipe = 29;
constexpr std::uint16_t TypeMask = 0170000;
constexpr std::uint16_t Fifo = 0010000;

}

namespace File {

int UnknownWhenceError(int fd) {
    FileStat status{};
    if (!FillFileStatFromDescriptor(fd, &status)) return errno;
    return (status.st_mode & TypeMask) == Fifo ? GuestEspipe : GuestEinval;
}

}
