#ifdef __APPLE__
#include "prx/libc/include/specifics/linux/ElfTypes.hpp"
#include <cstddef>
#include <cstdint>
#endif

extern "C" {
unsigned char guestFlexibleMemoryModuleData[512u * 1024u * 1024u] = {};
}

#ifdef __APPLE__
namespace {

struct GuestModuleMetadata {
    std::uint64_t version;
    std::uint64_t bias;
    std::byte reserved[0x48];
    std::uint64_t count;
    Elf64_Phdr headers[1];
};
static_assert(offsetof(GuestModuleMetadata, headers) == 0x60);

[[gnu::section("__APS5DATA,__meta"), gnu::used]] GuestModuleMetadata moduleMetadata{};

[[gnu::constructor]] void DescribeModuleSegments() {
    moduleMetadata.version = 2;
    moduleMetadata.bias = 0;
    moduleMetadata.count = 1;
    auto& data = moduleMetadata.headers[0];
    data.p_type = PT_LOAD;
    data.p_flags = 4 | 2;
    data.p_offset = 1;
    data.p_vaddr = reinterpret_cast<std::uintptr_t>(guestFlexibleMemoryModuleData);
    data.p_filesz = 0;
    data.p_memsz = sizeof(guestFlexibleMemoryModuleData);
}

}
#endif
