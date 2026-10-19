#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>

extern "C" {
int APS5_VABI sceConvertKeycodeGetImeKeyboardType(std::int32_t, std::uint32_t*);
int APS5_VABI sceConvertKeycodeGetVirtualKeycode(std::uint16_t, std::uint32_t, std::uint16_t*);
int APS5_VABI sceConvertKeycodeGetCharacter(std::uint16_t, std::uint32_t, std::uint32_t, char16_t*);
}

static void Require(bool value) {
    if (!value) std::abort();
}

int main() {
    constexpr int invalidAddress = static_cast<int>(0x80BC0031u);
    constexpr int invalidUserId = static_cast<int>(0x80BC0010u);
    constexpr int invalidType = static_cast<int>(0x80BC0011u);

    Require(sceConvertKeycodeGetImeKeyboardType(0, nullptr) == invalidAddress);
    Require(sceConvertKeycodeGetImeKeyboardType(-1, nullptr) == invalidAddress);

    std::uint32_t type = 0xFFFFFFFFu;
    Require(sceConvertKeycodeGetImeKeyboardType(-1, &type) == invalidUserId);
    Require(type == 0xFFFFFFFFu);

    Require(sceConvertKeycodeGetImeKeyboardType(0, &type) == 0);
    Require(type == 0);

    type = 0xFFFFFFFFu;
    Require(sceConvertKeycodeGetImeKeyboardType(1, &type) == 0);
    Require(type == 0);

    Require(sceConvertKeycodeGetVirtualKeycode(0x04, 0, nullptr) == invalidAddress);
    std::uint16_t vk = 0xFFFFu;
    Require(sceConvertKeycodeGetVirtualKeycode(0x04, 7, &vk) == invalidType);
    Require(vk == 0xFFFFu);

    for (std::uint16_t code = 0x04; code <= 0x1D; ++code) {
        vk = 0;
        Require(sceConvertKeycodeGetVirtualKeycode(code, 0, &vk) == 0);
        Require(vk == static_cast<std::uint16_t>('A' + (code - 0x04)));
    }

    for (std::uint16_t code = 0x1E; code <= 0x26; ++code) {
        vk = 0;
        Require(sceConvertKeycodeGetVirtualKeycode(code, 0, &vk) == 0);
        Require(vk == static_cast<std::uint16_t>('1' + (code - 0x1E)));
    }
    vk = 0;
    Require(sceConvertKeycodeGetVirtualKeycode(0x27, 0, &vk) == 0);
    Require(vk == static_cast<std::uint16_t>('0'));

    Require(sceConvertKeycodeGetVirtualKeycode(0x28, 0, &vk) == 0 && vk == 0x0D);
    Require(sceConvertKeycodeGetVirtualKeycode(0x29, 0, &vk) == 0 && vk == 0x1B);
    Require(sceConvertKeycodeGetVirtualKeycode(0x2A, 0, &vk) == 0 && vk == 0x08);
    Require(sceConvertKeycodeGetVirtualKeycode(0x2B, 0, &vk) == 0 && vk == 0x09);
    Require(sceConvertKeycodeGetVirtualKeycode(0x2C, 0, &vk) == 0 && vk == 0x20);

    for (std::uint16_t code = 0x3A; code <= 0x45; ++code) {
        vk = 0;
        Require(sceConvertKeycodeGetVirtualKeycode(code, 0, &vk) == 0);
        Require(vk == static_cast<std::uint16_t>(0x70 + (code - 0x3A)));
    }
    for (std::uint16_t code = 0x68; code <= 0x73; ++code) {
        vk = 0;
        Require(sceConvertKeycodeGetVirtualKeycode(code, 0, &vk) == 0);
        Require(vk == static_cast<std::uint16_t>(0x7C + (code - 0x68)));
    }

    Require(sceConvertKeycodeGetVirtualKeycode(0x4F, 0, &vk) == 0 && vk == 0x27);
    Require(sceConvertKeycodeGetVirtualKeycode(0x50, 0, &vk) == 0 && vk == 0x25);
    Require(sceConvertKeycodeGetVirtualKeycode(0x51, 0, &vk) == 0 && vk == 0x28);
    Require(sceConvertKeycodeGetVirtualKeycode(0x52, 0, &vk) == 0 && vk == 0x26);

    Require(sceConvertKeycodeGetVirtualKeycode(0x53, 0, &vk) == 0 && vk == 0x90);
    Require(sceConvertKeycodeGetVirtualKeycode(0x54, 0, &vk) == 0 && vk == 0x6F);
    Require(sceConvertKeycodeGetVirtualKeycode(0x55, 0, &vk) == 0 && vk == 0x6A);
    Require(sceConvertKeycodeGetVirtualKeycode(0x56, 0, &vk) == 0 && vk == 0x6D);
    Require(sceConvertKeycodeGetVirtualKeycode(0x57, 0, &vk) == 0 && vk == 0x6B);
    Require(sceConvertKeycodeGetVirtualKeycode(0x58, 0, &vk) == 0 && vk == 0x0D);
    for (std::uint16_t code = 0x59; code <= 0x61; ++code) {
        vk = 0;
        Require(sceConvertKeycodeGetVirtualKeycode(code, 0, &vk) == 0);
        Require(vk == static_cast<std::uint16_t>(0x61 + (code - 0x59)));
    }
    Require(sceConvertKeycodeGetVirtualKeycode(0x62, 0, &vk) == 0 && vk == 0x60);
    Require(sceConvertKeycodeGetVirtualKeycode(0x63, 0, &vk) == 0 && vk == 0x6E);

    Require(sceConvertKeycodeGetVirtualKeycode(0xE0, 0, &vk) == 0 && vk == 0xA2);
    Require(sceConvertKeycodeGetVirtualKeycode(0xE1, 0, &vk) == 0 && vk == 0xA0);
    Require(sceConvertKeycodeGetVirtualKeycode(0xE2, 0, &vk) == 0 && vk == 0xA4);
    Require(sceConvertKeycodeGetVirtualKeycode(0xE3, 0, &vk) == 0 && vk == 0x5B);
    Require(sceConvertKeycodeGetVirtualKeycode(0xE4, 0, &vk) == 0 && vk == 0xA3);
    Require(sceConvertKeycodeGetVirtualKeycode(0xE5, 0, &vk) == 0 && vk == 0xA1);
    Require(sceConvertKeycodeGetVirtualKeycode(0xE6, 0, &vk) == 0 && vk == 0xA5);
    Require(sceConvertKeycodeGetVirtualKeycode(0xE7, 0, &vk) == 0 && vk == 0x5C);

    Require(sceConvertKeycodeGetVirtualKeycode(0x1C, 3, &vk) == 0 && vk == static_cast<std::uint16_t>('Z'));
    Require(sceConvertKeycodeGetVirtualKeycode(0x16, 3, &vk) == 0 && vk == static_cast<std::uint16_t>('Y'));
    Require(sceConvertKeycodeGetVirtualKeycode(0x2D, 3, &vk) == 0 && vk == 0xDB);
    Require(sceConvertKeycodeGetVirtualKeycode(0x2E, 3, &vk) == 0 && vk == 0xDD);
    Require(sceConvertKeycodeGetVirtualKeycode(0x2F, 3, &vk) == 0 && vk == 0xBA);
    Require(sceConvertKeycodeGetVirtualKeycode(0x30, 3, &vk) == 0 && vk == 0xBB);
    Require(sceConvertKeycodeGetVirtualKeycode(0x33, 3, &vk) == 0 && vk == 0xC0);
    Require(sceConvertKeycodeGetVirtualKeycode(0x38, 3, &vk) == 0 && vk == 0xBD);

    Require(sceConvertKeycodeGetVirtualKeycode(0x04, 4, &vk) == 0 && vk == static_cast<std::uint16_t>('Q'));
    Require(sceConvertKeycodeGetVirtualKeycode(0x14, 4, &vk) == 0 && vk == static_cast<std::uint16_t>('A'));
    Require(sceConvertKeycodeGetVirtualKeycode(0x1A, 4, &vk) == 0 && vk == static_cast<std::uint16_t>('Z'));
    Require(sceConvertKeycodeGetVirtualKeycode(0x1D, 4, &vk) == 0 && vk == static_cast<std::uint16_t>('W'));
    Require(sceConvertKeycodeGetVirtualKeycode(0x10, 4, &vk) == 0 && vk == 0xBC);
    Require(sceConvertKeycodeGetVirtualKeycode(0x33, 4, &vk) == 0 && vk == static_cast<std::uint16_t>('M'));
    Require(sceConvertKeycodeGetVirtualKeycode(0x35, 4, &vk) == 0 && vk == 0xDF);

    Require(sceConvertKeycodeGetVirtualKeycode(0x87, 1, &vk) == 0 && vk == 0xE2);
    Require(sceConvertKeycodeGetVirtualKeycode(0x88, 1, &vk) == 0 && vk == 0x15);
    Require(sceConvertKeycodeGetVirtualKeycode(0x89, 1, &vk) == 0 && vk == 0xDC);
    Require(sceConvertKeycodeGetVirtualKeycode(0x8A, 1, &vk) == 0 && vk == 0x1C);
    Require(sceConvertKeycodeGetVirtualKeycode(0x8B, 1, &vk) == 0 && vk == 0x1D);
    Require(sceConvertKeycodeGetVirtualKeycode(0x2E, 1, &vk) == 0 && vk == 0xDD);
    Require(sceConvertKeycodeGetVirtualKeycode(0x2F, 1, &vk) == 0 && vk == 0xC0);
    Require(sceConvertKeycodeGetVirtualKeycode(0x34, 1, &vk) == 0 && vk == 0xBA);

    vk = 0xFFFFu;
    Require(sceConvertKeycodeGetVirtualKeycode(0x00, 0, &vk) == 0 && vk == 0);
    Require(sceConvertKeycodeGetVirtualKeycode(0xFF, 0, &vk) == 0 && vk == 0);

    char16_t ch[4] = {0};
    Require(sceConvertKeycodeGetCharacter(0x04, 0, 0, nullptr) == invalidAddress);
    Require(sceConvertKeycodeGetCharacter(0x04, 0, 7, ch) == invalidType);


    for (std::uint32_t state : {0u, 0x4000000u}) {
        Require(sceConvertKeycodeGetCharacter(0x04, state, 0, ch) == 0 && ch[0] == u'a');
        Require(sceConvertKeycodeGetCharacter(0x1D, state, 0, ch) == 0 && ch[0] == u'z');
        Require(sceConvertKeycodeGetCharacter(0x1E, state, 0, ch) == 0 && ch[0] == u'1');
        Require(sceConvertKeycodeGetCharacter(0x27, state, 0, ch) == 0 && ch[0] == u'0');
        Require(sceConvertKeycodeGetCharacter(0x28, state, 0, ch) == 0 && ch[0] == u'\n');
        Require(sceConvertKeycodeGetCharacter(0x2A, state, 0, ch) == 0 && ch[0] == u'\b');
        Require(sceConvertKeycodeGetCharacter(0x2B, state, 0, ch) == 0 && ch[0] == u'\t');
        Require(sceConvertKeycodeGetCharacter(0x2C, state, 0, ch) == 0 && ch[0] == u' ');
        Require(sceConvertKeycodeGetCharacter(0x2D, state, 0, ch) == 0 && ch[0] == u'-');
        Require(sceConvertKeycodeGetCharacter(0x2E, state, 0, ch) == 0 && ch[0] == u'=');
        Require(sceConvertKeycodeGetCharacter(0x2F, state, 0, ch) == 0 && ch[0] == u'[');
        Require(sceConvertKeycodeGetCharacter(0x30, state, 0, ch) == 0 && ch[0] == u']');
        Require(sceConvertKeycodeGetCharacter(0x31, state, 0, ch) == 0 && ch[0] == u'\\');
        Require(sceConvertKeycodeGetCharacter(0x33, state, 0, ch) == 0 && ch[0] == u';');
        Require(sceConvertKeycodeGetCharacter(0x34, state, 0, ch) == 0 && ch[0] == u'\'');
        Require(sceConvertKeycodeGetCharacter(0x35, state, 0, ch) == 0 && ch[0] == u'`');
        Require(sceConvertKeycodeGetCharacter(0x36, state, 0, ch) == 0 && ch[0] == u',');
        Require(sceConvertKeycodeGetCharacter(0x37, state, 0, ch) == 0 && ch[0] == u'.');
        Require(sceConvertKeycodeGetCharacter(0x38, state, 0, ch) == 0 && ch[0] == u'/');
        Require(sceConvertKeycodeGetCharacter(0x3A, state, 0, ch) == 0 && ch[0] == 0);
        Require(sceConvertKeycodeGetCharacter(0x64, state, 0, ch) == 0 && ch[0] == u'\\');
        Require(sceConvertKeycodeGetCharacter(0x54, state, 0, ch) == 0 && ch[0] == u'/');
        Require(sceConvertKeycodeGetCharacter(0x55, state, 0, ch) == 0 && ch[0] == u'*');
        Require(sceConvertKeycodeGetCharacter(0x56, state, 0, ch) == 0 && ch[0] == u'-');
        Require(sceConvertKeycodeGetCharacter(0x57, state, 0, ch) == 0 && ch[0] == u'+');
        Require(sceConvertKeycodeGetCharacter(0x58, state, 0, ch) == 0 && ch[0] == u'\n');
        Require(sceConvertKeycodeGetCharacter(0x59, state, 0, ch) == 0 && ch[0] == u'1');
        Require(sceConvertKeycodeGetCharacter(0x62, state, 0, ch) == 0 && ch[0] == u'0');
        Require(sceConvertKeycodeGetCharacter(0x63, state, 0, ch) == 0 && ch[0] == u'.');
    }

    for (std::uint32_t shift : {0x01u, 0x02u, 0x20u}) {
        Require(sceConvertKeycodeGetCharacter(0x04, shift, 0, ch) == 0 && ch[0] == u'A');
        Require(sceConvertKeycodeGetCharacter(0x1D, shift, 0, ch) == 0 && ch[0] == u'Z');
        Require(sceConvertKeycodeGetCharacter(0x1E, shift, 0, ch) == 0 && ch[0] == u'!');
        Require(sceConvertKeycodeGetCharacter(0x1F, shift, 0, ch) == 0 && ch[0] == u'@');
        Require(sceConvertKeycodeGetCharacter(0x20, shift, 0, ch) == 0 && ch[0] == u'#');
        Require(sceConvertKeycodeGetCharacter(0x21, shift, 0, ch) == 0 && ch[0] == u'$');
        Require(sceConvertKeycodeGetCharacter(0x22, shift, 0, ch) == 0 && ch[0] == u'%');
        Require(sceConvertKeycodeGetCharacter(0x23, shift, 0, ch) == 0 && ch[0] == u'^');
        Require(sceConvertKeycodeGetCharacter(0x24, shift, 0, ch) == 0 && ch[0] == u'&');
        Require(sceConvertKeycodeGetCharacter(0x25, shift, 0, ch) == 0 && ch[0] == u'*');
        Require(sceConvertKeycodeGetCharacter(0x26, shift, 0, ch) == 0 && ch[0] == u'(');
        Require(sceConvertKeycodeGetCharacter(0x27, shift, 0, ch) == 0 && ch[0] == u')');
        Require(sceConvertKeycodeGetCharacter(0x2D, shift, 0, ch) == 0 && ch[0] == u'_');
        Require(sceConvertKeycodeGetCharacter(0x2E, shift, 0, ch) == 0 && ch[0] == u'+');
        Require(sceConvertKeycodeGetCharacter(0x2F, shift, 0, ch) == 0 && ch[0] == u'{');
        Require(sceConvertKeycodeGetCharacter(0x30, shift, 0, ch) == 0 && ch[0] == u'}');
        Require(sceConvertKeycodeGetCharacter(0x31, shift, 0, ch) == 0 && ch[0] == u'|');
        Require(sceConvertKeycodeGetCharacter(0x33, shift, 0, ch) == 0 && ch[0] == u':');
        Require(sceConvertKeycodeGetCharacter(0x34, shift, 0, ch) == 0 && ch[0] == u'"');
        Require(sceConvertKeycodeGetCharacter(0x35, shift, 0, ch) == 0 && ch[0] == u'~');
        Require(sceConvertKeycodeGetCharacter(0x36, shift, 0, ch) == 0 && ch[0] == u'<');
        Require(sceConvertKeycodeGetCharacter(0x37, shift, 0, ch) == 0 && ch[0] == u'>');
        Require(sceConvertKeycodeGetCharacter(0x38, shift, 0, ch) == 0 && ch[0] == u'?');
        Require(sceConvertKeycodeGetCharacter(0x64, shift, 0, ch) == 0 && ch[0] == u'|');
    }

    for (std::uint32_t caps : {0x0200u, 0x10000u}) {
        Require(sceConvertKeycodeGetCharacter(0x04, caps, 0, ch) == 0 && ch[0] == u'A');
        Require(sceConvertKeycodeGetCharacter(0x1D, caps, 0, ch) == 0 && ch[0] == u'Z');
        Require(sceConvertKeycodeGetCharacter(0x1E, caps, 0, ch) == 0 && ch[0] == u'1');
        Require(sceConvertKeycodeGetCharacter(0x27, caps, 0, ch) == 0 && ch[0] == u'0');
        Require(sceConvertKeycodeGetCharacter(0x2D, caps, 0, ch) == 0 && ch[0] == u'-');

        const std::uint32_t shiftCaps = caps | 0x02u;
        Require(sceConvertKeycodeGetCharacter(0x04, shiftCaps, 0, ch) == 0 && ch[0] == u'a');
        Require(sceConvertKeycodeGetCharacter(0x1D, shiftCaps, 0, ch) == 0 && ch[0] == u'z');
        Require(sceConvertKeycodeGetCharacter(0x1E, shiftCaps, 0, ch) == 0 && ch[0] == u'!');
        Require(sceConvertKeycodeGetCharacter(0x2D, shiftCaps, 0, ch) == 0 && ch[0] == u'_');
    }

    Require(sceConvertKeycodeGetCharacter(0x1F, 0x02, 1, ch) == 0 && ch[0] == u'"');
    Require(sceConvertKeycodeGetCharacter(0x23, 0x02, 1, ch) == 0 && ch[0] == u'&');
    Require(sceConvertKeycodeGetCharacter(0x24, 0x02, 1, ch) == 0 && ch[0] == u'\'');
    Require(sceConvertKeycodeGetCharacter(0x2E, 0, 1, ch) == 0 && ch[0] == u'^');
    Require(sceConvertKeycodeGetCharacter(0x2E, 0x02, 1, ch) == 0 && ch[0] == u'~');
    Require(sceConvertKeycodeGetCharacter(0x2F, 0, 1, ch) == 0 && ch[0] == u'@');
    Require(sceConvertKeycodeGetCharacter(0x2F, 0x02, 1, ch) == 0 && ch[0] == u'`');
    Require(sceConvertKeycodeGetCharacter(0x30, 0, 1, ch) == 0 && ch[0] == u'[');
    Require(sceConvertKeycodeGetCharacter(0x30, 0x02, 1, ch) == 0 && ch[0] == u'{');
    Require(sceConvertKeycodeGetCharacter(0x31, 0, 1, ch) == 0 && ch[0] == u']');
    Require(sceConvertKeycodeGetCharacter(0x31, 0x02, 1, ch) == 0 && ch[0] == u'}');
    Require(sceConvertKeycodeGetCharacter(0x33, 0, 1, ch) == 0 && ch[0] == u';');
    Require(sceConvertKeycodeGetCharacter(0x33, 0x02, 1, ch) == 0 && ch[0] == u'+');
    Require(sceConvertKeycodeGetCharacter(0x34, 0, 1, ch) == 0 && ch[0] == u':');
    Require(sceConvertKeycodeGetCharacter(0x34, 0x02, 1, ch) == 0 && ch[0] == u'*');
    Require(sceConvertKeycodeGetCharacter(0x87, 0, 1, ch) == 0 && ch[0] == u'\\');
    Require(sceConvertKeycodeGetCharacter(0x87, 0x02, 1, ch) == 0 && ch[0] == u'_');
    Require(sceConvertKeycodeGetCharacter(0x89, 0, 1, ch) == 0 && ch[0] == u'\\');
    Require(sceConvertKeycodeGetCharacter(0x89, 0x02, 1, ch) == 0 && ch[0] == u'|');

    Require(sceConvertKeycodeGetCharacter(0x1F, 0x02, 2, ch) == 0 && ch[0] == u'"');
    Require(sceConvertKeycodeGetCharacter(0x20, 0x02, 2, ch) == 0 && ch[0] == u'\u00A3');
    Require(sceConvertKeycodeGetCharacter(0x34, 0x02, 2, ch) == 0 && ch[0] == u'@');
    Require(sceConvertKeycodeGetCharacter(0x35, 0, 2, ch) == 0 && ch[0] == u'`');
    Require(sceConvertKeycodeGetCharacter(0x35, 0x02, 2, ch) == 0 && ch[0] == u'\u00AC');
    Require(sceConvertKeycodeGetCharacter(0x64, 0, 2, ch) == 0 && ch[0] == u'\\');
    Require(sceConvertKeycodeGetCharacter(0x64, 0x02, 2, ch) == 0 && ch[0] == u'|');
    Require(sceConvertKeycodeGetCharacter(0x08, 0x40, 2, ch) == 0 && ch[0] == u'\u20AC');
    Require(sceConvertKeycodeGetCharacter(0x21, 0x40, 2, ch) == 0 && ch[0] == u'\u20AC');
    Require(sceConvertKeycodeGetCharacter(0x35, 0x40, 2, ch) == 0 && ch[0] == u'|');

    Require(sceConvertKeycodeGetCharacter(0x1C, 0, 3, ch) == 0 && ch[0] == u'z');
    Require(sceConvertKeycodeGetCharacter(0x1C, 0x02, 3, ch) == 0 && ch[0] == u'Z');
    Require(sceConvertKeycodeGetCharacter(0x16, 0, 3, ch) == 0 && ch[0] == u'y');
    Require(sceConvertKeycodeGetCharacter(0x16, 0x02, 3, ch) == 0 && ch[0] == u'Y');
    Require(sceConvertKeycodeGetCharacter(0x20, 0x02, 3, ch) == 0 && ch[0] == u'\u00A7');
    Require(sceConvertKeycodeGetCharacter(0x2D, 0, 3, ch) == 0 && ch[0] == u'\u00DF');
    Require(sceConvertKeycodeGetCharacter(0x2D, 0x02, 3, ch) == 0 && ch[0] == u'?');
    Require(sceConvertKeycodeGetCharacter(0x2F, 0, 3, ch) == 0 && ch[0] == u'\u00FC');
    Require(sceConvertKeycodeGetCharacter(0x2F, 0x02, 3, ch) == 0 && ch[0] == u'\u00DC');
    Require(sceConvertKeycodeGetCharacter(0x33, 0, 3, ch) == 0 && ch[0] == u'\u00F6');
    Require(sceConvertKeycodeGetCharacter(0x33, 0x02, 3, ch) == 0 && ch[0] == u'\u00D6');
    Require(sceConvertKeycodeGetCharacter(0x34, 0, 3, ch) == 0 && ch[0] == u'\u00E4');
    Require(sceConvertKeycodeGetCharacter(0x34, 0x02, 3, ch) == 0 && ch[0] == u'\u00C4');
    Require(sceConvertKeycodeGetCharacter(0x64, 0, 3, ch) == 0 && ch[0] == u'<');
    Require(sceConvertKeycodeGetCharacter(0x64, 0x02, 3, ch) == 0 && ch[0] == u'>');
    Require(sceConvertKeycodeGetCharacter(0x14, 0x40, 3, ch) == 0 && ch[0] == u'@');
    Require(sceConvertKeycodeGetCharacter(0x08, 0x40, 3, ch) == 0 && ch[0] == u'\u20AC');
    Require(sceConvertKeycodeGetCharacter(0x24, 0x40, 3, ch) == 0 && ch[0] == u'{');
    Require(sceConvertKeycodeGetCharacter(0x25, 0x40, 3, ch) == 0 && ch[0] == u'[');
    Require(sceConvertKeycodeGetCharacter(0x26, 0x40, 3, ch) == 0 && ch[0] == u']');
    Require(sceConvertKeycodeGetCharacter(0x27, 0x40, 3, ch) == 0 && ch[0] == u'}');
    Require(sceConvertKeycodeGetCharacter(0x2D, 0x40, 3, ch) == 0 && ch[0] == u'\\');
    Require(sceConvertKeycodeGetCharacter(0x30, 0x40, 3, ch) == 0 && ch[0] == u'~');
    Require(sceConvertKeycodeGetCharacter(0x64, 0x40, 3, ch) == 0 && ch[0] == u'|');

    Require(sceConvertKeycodeGetCharacter(0x04, 0, 4, ch) == 0 && ch[0] == u'q');
    Require(sceConvertKeycodeGetCharacter(0x04, 0x02, 4, ch) == 0 && ch[0] == u'Q');
    Require(sceConvertKeycodeGetCharacter(0x14, 0, 4, ch) == 0 && ch[0] == u'a');
    Require(sceConvertKeycodeGetCharacter(0x14, 0x02, 4, ch) == 0 && ch[0] == u'A');
    Require(sceConvertKeycodeGetCharacter(0x1A, 0, 4, ch) == 0 && ch[0] == u'z');
    Require(sceConvertKeycodeGetCharacter(0x1A, 0x02, 4, ch) == 0 && ch[0] == u'Z');
    Require(sceConvertKeycodeGetCharacter(0x1D, 0, 4, ch) == 0 && ch[0] == u'w');
    Require(sceConvertKeycodeGetCharacter(0x1D, 0x02, 4, ch) == 0 && ch[0] == u'W');
    Require(sceConvertKeycodeGetCharacter(0x10, 0, 4, ch) == 0 && ch[0] == u',');
    Require(sceConvertKeycodeGetCharacter(0x10, 0x02, 4, ch) == 0 && ch[0] == u'?');
    Require(sceConvertKeycodeGetCharacter(0x33, 0, 4, ch) == 0 && ch[0] == u'm');
    Require(sceConvertKeycodeGetCharacter(0x33, 0x02, 4, ch) == 0 && ch[0] == u'M');
    Require(sceConvertKeycodeGetCharacter(0x1E, 0, 4, ch) == 0 && ch[0] == u'&');
    Require(sceConvertKeycodeGetCharacter(0x1F, 0, 4, ch) == 0 && ch[0] == u'\u00E9');
    Require(sceConvertKeycodeGetCharacter(0x24, 0, 4, ch) == 0 && ch[0] == u'\u00E8');
    Require(sceConvertKeycodeGetCharacter(0x26, 0, 4, ch) == 0 && ch[0] == u'\u00E7');
    Require(sceConvertKeycodeGetCharacter(0x27, 0, 4, ch) == 0 && ch[0] == u'\u00E0');
    Require(sceConvertKeycodeGetCharacter(0x1E, 0x02, 4, ch) == 0 && ch[0] == u'1');
    Require(sceConvertKeycodeGetCharacter(0x27, 0x02, 4, ch) == 0 && ch[0] == u'0');
    Require(sceConvertKeycodeGetCharacter(0x34, 0, 4, ch) == 0 && ch[0] == u'\u00F9');
    Require(sceConvertKeycodeGetCharacter(0x34, 0x02, 4, ch) == 0 && ch[0] == u'%');
    Require(sceConvertKeycodeGetCharacter(0x35, 0, 4, ch) == 0 && ch[0] == u'\u00B2');
    Require(sceConvertKeycodeGetCharacter(0x08, 0x40, 4, ch) == 0 && ch[0] == u'\u20AC');
    Require(sceConvertKeycodeGetCharacter(0x1F, 0x40, 4, ch) == 0 && ch[0] == u'~');
    Require(sceConvertKeycodeGetCharacter(0x20, 0x40, 4, ch) == 0 && ch[0] == u'#');
    Require(sceConvertKeycodeGetCharacter(0x21, 0x40, 4, ch) == 0 && ch[0] == u'{');
    Require(sceConvertKeycodeGetCharacter(0x22, 0x40, 4, ch) == 0 && ch[0] == u'[');
    Require(sceConvertKeycodeGetCharacter(0x23, 0x40, 4, ch) == 0 && ch[0] == u'|');
    Require(sceConvertKeycodeGetCharacter(0x24, 0x40, 4, ch) == 0 && ch[0] == u'`');
    Require(sceConvertKeycodeGetCharacter(0x25, 0x40, 4, ch) == 0 && ch[0] == u'\\');
    Require(sceConvertKeycodeGetCharacter(0x26, 0x40, 4, ch) == 0 && ch[0] == u'^');
    Require(sceConvertKeycodeGetCharacter(0x27, 0x40, 4, ch) == 0 && ch[0] == u'@');

    Require(sceConvertKeycodeGetCharacter(0x20, 0x02, 5, ch) == 0 && ch[0] == u'\u00B7');
    Require(sceConvertKeycodeGetCharacter(0x2E, 0, 5, ch) == 0 && ch[0] == u'\u00A1');
    Require(sceConvertKeycodeGetCharacter(0x2E, 0x02, 5, ch) == 0 && ch[0] == u'\u00BF');
    Require(sceConvertKeycodeGetCharacter(0x31, 0, 5, ch) == 0 && ch[0] == u'\u00E7');
    Require(sceConvertKeycodeGetCharacter(0x31, 0x02, 5, ch) == 0 && ch[0] == u'\u00C7');
    Require(sceConvertKeycodeGetCharacter(0x33, 0, 5, ch) == 0 && ch[0] == u'\u00F1');
    Require(sceConvertKeycodeGetCharacter(0x33, 0x02, 5, ch) == 0 && ch[0] == u'\u00D1');
    Require(sceConvertKeycodeGetCharacter(0x08, 0x40, 5, ch) == 0 && ch[0] == u'\u20AC');
    Require(sceConvertKeycodeGetCharacter(0x1E, 0x40, 5, ch) == 0 && ch[0] == u'|');
    Require(sceConvertKeycodeGetCharacter(0x1F, 0x40, 5, ch) == 0 && ch[0] == u'@');
    Require(sceConvertKeycodeGetCharacter(0x20, 0x40, 5, ch) == 0 && ch[0] == u'#');
    Require(sceConvertKeycodeGetCharacter(0x21, 0x40, 5, ch) == 0 && ch[0] == u'~');

    Require(sceConvertKeycodeGetCharacter(0x20, 0x02, 6, ch) == 0 && ch[0] == u'\u00A3');
    Require(sceConvertKeycodeGetCharacter(0x2E, 0, 6, ch) == 0 && ch[0] == u'\u00EC');
    Require(sceConvertKeycodeGetCharacter(0x2E, 0x02, 6, ch) == 0 && ch[0] == u'^');
    Require(sceConvertKeycodeGetCharacter(0x2F, 0, 6, ch) == 0 && ch[0] == u'\u00E8');
    Require(sceConvertKeycodeGetCharacter(0x2F, 0x02, 6, ch) == 0 && ch[0] == u'\u00E9');
    Require(sceConvertKeycodeGetCharacter(0x31, 0, 6, ch) == 0 && ch[0] == u'\u00F9');
    Require(sceConvertKeycodeGetCharacter(0x31, 0x02, 6, ch) == 0 && ch[0] == u'\u00A7');
    Require(sceConvertKeycodeGetCharacter(0x33, 0, 6, ch) == 0 && ch[0] == u'\u00F2');
    Require(sceConvertKeycodeGetCharacter(0x33, 0x02, 6, ch) == 0 && ch[0] == u'\u00E7');
    Require(sceConvertKeycodeGetCharacter(0x34, 0, 6, ch) == 0 && ch[0] == u'\u00E0');
    Require(sceConvertKeycodeGetCharacter(0x34, 0x02, 6, ch) == 0 && ch[0] == u'\u00B0');
    Require(sceConvertKeycodeGetCharacter(0x08, 0x40, 6, ch) == 0 && ch[0] == u'\u20AC');
    Require(sceConvertKeycodeGetCharacter(0x2F, 0x40, 6, ch) == 0 && ch[0] == u'[');
    Require(sceConvertKeycodeGetCharacter(0x2F, 0x42, 6, ch) == 0 && ch[0] == u'{');
    Require(sceConvertKeycodeGetCharacter(0x30, 0x40, 6, ch) == 0 && ch[0] == u']');
    Require(sceConvertKeycodeGetCharacter(0x30, 0x42, 6, ch) == 0 && ch[0] == u'}');
    Require(sceConvertKeycodeGetCharacter(0x33, 0x40, 6, ch) == 0 && ch[0] == u'@');
    Require(sceConvertKeycodeGetCharacter(0x34, 0x40, 6, ch) == 0 && ch[0] == u'#');

    return 0;
}
