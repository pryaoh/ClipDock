#include "MacShortcutKeys.h"
#include <cstdlib>
#include <iostream>

static void check(bool value, const char *message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
static std::array<int, 3> keysFor(CFStringRef id) {
    const void *key = kTISPropertyInputSourceID;
    const void *value = id;
    auto filter = CFDictionaryCreate(nullptr, &key, &value, 1,
                                    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    auto sources = TISCreateInputSourceList(filter, true);
    CFRelease(filter);
    check(sources && CFArrayGetCount(sources) > 0, "built-in keyboard layout is available");
    auto source = static_cast<TISInputSourceRef>(const_cast<void *>(CFArrayGetValueAtIndex(sources, 0)));
    const auto keys = macShortcutKeys(source);
    CFRelease(sources);
    return keys;
}
int main() {
    check(macShortcutKeys(nullptr) == std::array<int, 3>{-1, -1, -1}, "missing layout falls back safely");
    check(keysFor(CFSTR("com.apple.keylayout.US")) == std::array<int, 3>{kVK_ANSI_R, kVK_ANSI_P, kVK_ANSI_S},
          "US shortcut letters resolve to their expected keys");
    check(keysFor(CFSTR("com.apple.keylayout.Dvorak")) == std::array<int, 3>{kVK_ANSI_O, kVK_ANSI_R, kVK_ANSI_Semicolon},
          "Dvorak shortcut letters resolve independently of US positions");
    std::cout << "PASS: US, Dvorak and unavailable shortcut keyboard layouts\n";
}
