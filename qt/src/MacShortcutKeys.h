#pragma once
#include <Carbon/Carbon.h>
#include <array>

// Resolve letters using the Command+Shift layer, including layouts such as
// Dvorak-QWERTY Command. A missing character keeps that command app-local.
inline std::array<int, 3> macShortcutKeys(TISInputSourceRef source) {
    std::array<int, 3> keys{-1, -1, -1};
    if (!source) return keys;
    auto data = static_cast<CFDataRef>(TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData));
    if (!data) return keys;
    auto layout = reinterpret_cast<const UCKeyboardLayout *>(CFDataGetBytePtr(data));
    constexpr UniChar letters[] = {'R', 'P', 'S'};
    for (UInt16 code = 0; code < 128; ++code) {
        UInt32 dead = 0;
        UniChar text[4]{};
        UniCharCount count = 0;
        if (UCKeyTranslate(layout, code, kUCKeyActionDown, (cmdKey | shiftKey) >> 8,
                           LMGetKbdType(), kUCKeyTranslateNoDeadKeysMask, &dead, 4, &count, text) != noErr || count != 1)
            continue;
        for (int i = 0; i < 3; ++i)
            if (keys[i] < 0 && (text[0] == letters[i] || text[0] == letters[i] + ('a' - 'A'))) keys[i] = code;
    }
    return keys;
}
