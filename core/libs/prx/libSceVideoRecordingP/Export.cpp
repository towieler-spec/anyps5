#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::mutex g_infoMutex;
std::map<int32_t, std::vector<unsigned char>> g_info;

unsigned char ClampByte(int value) {
    return static_cast<unsigned char>(value < 0 ? 0 : value > 255 ? 255 : value);
}

int FloorDivide256(int value) {
    return value >= 0 ? value / 256 : -((-value + 255) / 256);
}

unsigned char Luma(int red, int green, int blue) {
    return ClampByte(FloorDivide256(66 * red + 129 * green + 25 * blue + 128) + 16);
}

unsigned char ChromaU(int red, int green, int blue) {
    return ClampByte(FloorDivide256(-38 * red - 74 * green + 112 * blue + 128) + 128);
}

unsigned char ChromaV(int red, int green, int blue) {
    return ClampByte(FloorDivide256(112 * red - 94 * green - 18 * blue + 128) + 128);
}
}

extern "C" {

void APS5_VABI _sceVideoRecordingQueryParam_nid_postfix(void* param) {
 (void)param;
 NotImplemented_nid_no_patch("_sceVideoRecordingQueryParam");
}

int APS5_VABI sceVideoRecordingClose(int discard) {
 (void)discard;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingCloseByHandle(int handle, int discard) {
 (void)handle;
 (void)discard;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingCopyBGRAtoNV12(void* dst, const void* src, uint32_t width, uint32_t height) {
    if (dst == nullptr || src == nullptr || width == 0 || height == 0) APS5_INVALID_ARG_EX;
    const auto maxSize = std::numeric_limits<std::size_t>::max();
    if (static_cast<std::size_t>(width) > maxSize / height) APS5_INVALID_ARG_EX;
    const auto pixelCount = static_cast<std::size_t>(width) * height;
    if (pixelCount > maxSize / 4) APS5_INVALID_ARG_EX;
    const auto ySize = pixelCount;
    const auto chromaWidth = static_cast<std::size_t>(width / 2 + width % 2);
    const auto chromaHeight = static_cast<std::size_t>(height / 2 + height % 2);
    if (chromaWidth > maxSize / chromaHeight || chromaWidth * chromaHeight > maxSize / 2) APS5_INVALID_ARG_EX;
    const auto uvSize = chromaWidth * chromaHeight * 2;
    if (ySize > maxSize - uvSize) APS5_INVALID_ARG_EX;

    const auto* input = static_cast<const unsigned char*>(src);
    auto* output = static_cast<unsigned char*>(dst);
    auto* uv = output + ySize;
    for (std::size_t row = 0; row < height; ++row) {
        for (std::size_t column = 0; column < width; ++column) {
            const auto* pixel = input + (row * width + column) * 4;
            output[row * width + column] = Luma(pixel[2], pixel[1], pixel[0]);
        }
    }
    for (std::size_t row = 0; row < chromaHeight; ++row) {
        for (std::size_t column = 0; column < chromaWidth; ++column) {
            int red = 0;
            int green = 0;
            int blue = 0;
            int samples = 0;
            for (std::size_t dy = 0; dy < 2; ++dy) {
                const auto sourceRow = row * 2 + dy;
                if (sourceRow >= height) continue;
                for (std::size_t dx = 0; dx < 2; ++dx) {
                    const auto sourceColumn = column * 2 + dx;
                    if (sourceColumn >= width) continue;
                    const auto* pixel = input + (sourceRow * width + sourceColumn) * 4;
                    blue += pixel[0];
                    green += pixel[1];
                    red += pixel[2];
                    ++samples;
                }
            }
            red = (red + samples / 2) / samples;
            green = (green + samples / 2) / samples;
            blue = (blue + samples / 2) / samples;
            const auto offset = (row * chromaWidth + column) * 2;
            uv[offset] = ChromaU(red, green, blue);
            uv[offset + 1] = ChromaV(red, green, blue);
        }
    }
    return 0;
}

int APS5_VABI sceVideoRecordingGetInfo(int32_t info, void* data, size_t size) {
    if (data == nullptr) APS5_INVALID_ARG_EX;
    std::lock_guard lock(g_infoMutex);
    const auto found = g_info.find(info);
    if (found == g_info.end()) throw std::logic_error(std::string(__func__) + ": info " + std::to_string(info) + " was never set");
    if (size < found->second.size()) APS5_INVALID_ARG_EX;
    std::memcpy(data, found->second.data(), found->second.size());
    return 0;
}

int APS5_VABI sceVideoRecordingGetInfoByHandle(int handle, int32_t info, void* data, size_t size) {
 (void)handle;
 (void)info;
 (void)data;
 (void)size;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingGetStatus(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingGetSupportInfo(void* info) {
 (void)info;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingGetVideoOutMode(void* mode) {
 (void)mode;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingInit(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingInitialize(void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingOpen(const char* path, const void* param, void* heap, int heapSize) {
 (void)path;
 (void)param;
 (void)heap;
 (void)heapSize;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingOpen2(const char* path, const void* param, void* heap, int heapSize) {
 (void)path;
 (void)param;
 (void)heap;
 (void)heapSize;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingQueryHddSize2(const void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingQueryMemSize(const void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingQueryMemSize2(const void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingRestModeProcessGetStatus(void* status) {
 (void)status;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingRestModeProcessStart(void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingSendCommand(int cmd, const void* param) {
 (void)cmd;
 (void)param;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingSetInfo(int32_t info, const void* data, size_t size) {
    if (data == nullptr && size != 0) APS5_INVALID_ARG_EX;
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::lock_guard lock(g_infoMutex);
    g_info[info].assign(bytes, bytes + size);
    return 0;
}

int APS5_VABI sceVideoRecordingStart(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingStop(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceVideoRecordingTerm(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

APS5_EXPORT("iQS6DUtLybE", videoRecordingUnknown_iQS6DUtLybE);
int APS5_VABI videoRecordingUnknown_iQS6DUtLybE(void) {
 NotImplemented_nid_no_patch("iQS6DUtLybE");
 return 0;
}

}
