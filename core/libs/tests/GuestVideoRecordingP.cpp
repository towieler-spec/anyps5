#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <vector>

extern "C" int APS5_VABI sceVideoRecordingCopyBGRAtoNV12(void*, const void*, std::uint32_t, std::uint32_t);

namespace {

void Require(bool value) { if (!value) std::abort(); }

template <typename TAction>
bool ThrowsInvalidArgument(TAction action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

void TestReferenceColors() {
    std::vector<std::uint8_t> input(2 * 2 * 4, 0xFF);
    std::vector<std::uint8_t> output(2 * 2 + 2, 0);
    Require(sceVideoRecordingCopyBGRAtoNV12(output.data(), input.data(), 2, 2) == 0);
    Require(std::all_of(output.begin(), output.begin() + 4, [](std::uint8_t value) { return value == 235; }));
    Require(output[4] == 128 && output[5] == 128);

    std::fill(input.begin(), input.end(), 0);
    Require(sceVideoRecordingCopyBGRAtoNV12(output.data(), input.data(), 2, 2) == 0);
    Require(std::all_of(output.begin(), output.begin() + 4, [](std::uint8_t value) { return value == 16; }));
    Require(output[4] == 128 && output[5] == 128);
}

void TestInvalidPointers() {
    std::uint8_t input[4]{};
    std::uint8_t output[3]{};
    Require(ThrowsInvalidArgument([&] { sceVideoRecordingCopyBGRAtoNV12(nullptr, input, 1, 1); }));
    Require(ThrowsInvalidArgument([&] { sceVideoRecordingCopyBGRAtoNV12(output, nullptr, 1, 1); }));
}

}

int main() {
    TestReferenceColors();
    TestInvalidPointers();
}
