#include "ShortcutBackend.h"
#include <QGuiApplication>
#include <QTimer>
#if defined(Q_OS_WIN)
#include <qt_windows.h>
#elif defined(Q_OS_MACOS)
#include <Carbon/Carbon.h>
#include "MacShortcutKeys.h"
#elif defined(CLIPDOCK_HAS_X11)
#include <QSocketNotifier>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <vector>
#endif

#ifdef CLIPDOCK_HAS_PORTAL
#include "PortalGlobalShortcuts.h"
#endif

#ifdef Q_OS_WIN
class WindowsShortcuts final : public ShortcutBackend {
public:
    ~WindowsShortcuts() override { stop(); }
    void start() override {
        stop();
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = &WindowsShortcuts::receive;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"ClipDock.GlobalShortcuts";
        RegisterClassW(&windowClass);
        m_window = CreateWindowExW(0, windowClass.lpszClassName, L"", 0, 0, 0, 0, 0,
                                  HWND_MESSAGE, nullptr, windowClass.hInstance, this);
        for (auto action : shortcutActions) {
            const int i = int(action);
            const bool ok = m_window && RegisterHotKey(m_window, i + 1,
                MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, keys[i]);
            const auto error = ok ? 0 : GetLastError();
            m_registered[i] = ok;
            emit registrationChanged(action, ok, ok ? QStringLiteral("다른 앱에서도 사용 가능")
                : error == ERROR_HOTKEY_ALREADY_REGISTERED ? QStringLiteral("다른 앱이 이 단축키를 사용 중입니다.")
                : QStringLiteral("Windows 전역 등록 실패 (오류 %1)").arg(error));
        }
    }
    void stop() override {
        if (m_window) {
            for (int i = 0; i < 3; ++i) if (m_registered[i]) UnregisterHotKey(m_window, i + 1);
            DestroyWindow(m_window);
        }
        m_window = nullptr;
        m_registered.fill(false);
    }
private:
    static LRESULT CALLBACK receive(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        if (message == WM_NCCREATE) {
            auto *creation = reinterpret_cast<CREATESTRUCTW *>(lparam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(creation->lpCreateParams));
        }
        auto *self = reinterpret_cast<WindowsShortcuts *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_HOTKEY && self && wparam >= 1 && wparam <= 3) {
            const int i = int(wparam) - 1;
            if (self->m_registered[i] && HIWORD(lparam) == keys[i]
                && LOWORD(lparam) == (MOD_CONTROL | MOD_SHIFT)) {
                // MOD_NOREPEAT makes WM_HOTKEY a single pulse per physical press.
                emit self->keyStateChanged(shortcutActions[i], true);
                emit self->keyStateChanged(shortcutActions[i], false);
            }
            return 0;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }
    inline static constexpr UINT keys[] = {'R', 'P', 'S'};
    HWND m_window = nullptr;
    std::array<bool, 3> m_registered{};
};
#elif defined(Q_OS_MACOS)
class MacShortcuts final : public ShortcutBackend {
public:
    MacShortcuts() {
        CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), this,
            &MacShortcuts::layoutChanged, kTISNotifySelectedKeyboardInputSourceChanged, nullptr,
            CFNotificationSuspensionBehaviorDeliverImmediately);
    }
    ~MacShortcuts() override {
        CFNotificationCenterRemoveEveryObserver(CFNotificationCenterGetDistributedCenter(), this);
        stop();
    }
    void start() override {
        stop();
        m_started = true;
        const EventTypeSpec events[] = {{kEventClassKeyboard, kEventHotKeyPressed},
                                        {kEventClassKeyboard, kEventHotKeyReleased}};
        const auto handlerResult = InstallApplicationEventHandler(&MacShortcuts::receive, 2, events, this, &m_handler);
        auto source = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
        const auto keys = macShortcutKeys(source);
        if (source) CFRelease(source);
        for (auto action : shortcutActions) {
            const auto i = int(action);
            EventHotKeyID id{signature, UInt32(i + 1)};
            const auto result = keys[i] < 0 ? paramErr : handlerResult == noErr
                ? RegisterEventHotKey(keys[i], cmdKey | shiftKey, id, GetApplicationEventTarget(), 0, &m_keys[i])
                : handlerResult;
            emit registrationChanged(action, result == noErr, result == noErr
                ? QStringLiteral("다른 앱에서도 사용 가능")
                : QStringLiteral("다른 앱과 충돌하거나 macOS 등록이 거부되었습니다. (오류 %1)").arg(result));
        }
    }
    void stop() override {
        m_started = false;
        for (auto &key : m_keys) { if (key) UnregisterEventHotKey(key); key = nullptr; }
        if (m_handler) RemoveEventHandler(m_handler);
        m_handler = nullptr;
    }
private:
    static void layoutChanged(CFNotificationCenterRef, void *observer, CFStringRef, const void *, CFDictionaryRef) {
        auto *self = static_cast<MacShortcuts *>(observer);
        QTimer::singleShot(0, self, [self] { if (self->m_started) self->start(); });
    }
    static OSStatus receive(EventHandlerCallRef, EventRef event, void *context) {
        EventHotKeyID id{};
        if (GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr,
                              sizeof(id), nullptr, &id) != noErr || id.signature != signature || id.id < 1 || id.id > 3)
            return eventNotHandledErr;
        auto *self = static_cast<MacShortcuts *>(context);
        if (!self->m_keys[id.id - 1]) return eventNotHandledErr;
        emit self->keyStateChanged(shortcutActions[id.id - 1], GetEventKind(event) == kEventHotKeyPressed);
        return noErr;
    }
    static constexpr OSType signature = 0x4344484b; // CDHK
    std::array<EventHotKeyRef, 3> m_keys{};
    EventHandlerRef m_handler = nullptr;
    bool m_started = false;
};
#elif defined(CLIPDOCK_HAS_X11)
namespace {
Display *grabbingDisplay = nullptr;
int grabError = 0;
XErrorHandler previousErrorHandler = nullptr;
int recordGrabError(Display *display, XErrorEvent *event) {
    if (display == grabbingDisplay) { grabError = event->error_code; return 0; }
    return previousErrorHandler ? previousErrorHandler(display, event) : 0;
}
}
class X11Shortcuts final : public ShortcutBackend {
public:
    ~X11Shortcuts() override { stop(); }
    void start() override {
        stop();
        m_display = XOpenDisplay(nullptr);
        if (!m_display) {
            for (auto action : shortcutActions)
                emit registrationChanged(action, false, QStringLiteral("X11 디스플레이에 연결할 수 없습니다."));
            return;
        }
        Bool detectable = False;
        XkbSetDetectableAutoRepeat(m_display, True, &detectable);
        m_detectable = detectable;
        refreshBindings();
        m_notifier = std::make_unique<QSocketNotifier>(ConnectionNumber(m_display), QSocketNotifier::Read);
        connect(m_notifier.get(), &QSocketNotifier::activated, this, [this] { receive(); });
        // XSync during registration can already have buffered events in Xlib.
        QTimer::singleShot(0, this, [this] { if (m_display) receive(); });
    }
    void stop() override {
        m_notifier.reset();
        if (m_display) {
            for (int i = 0; i < 3; ++i) if (m_registered[i]) ungrab(i);
            XCloseDisplay(m_display);
        }
        m_display = nullptr;
        m_registered.fill(false);
        m_codes.fill(0);
        m_activeKey = 0;
    }
private:
    void refreshBindings() {
        unsigned locks = LockMask;
        auto *modifiers = XGetModifierMapping(m_display);
        if (modifiers) {
            const auto num = XKeysymToKeycode(m_display, XK_Num_Lock);
            const auto scroll = XKeysymToKeycode(m_display, XK_Scroll_Lock);
            for (int mod = 0; mod < 8; ++mod)
                for (int key = 0; key < modifiers->max_keypermod; ++key) {
                    const auto code = modifiers->modifiermap[mod * modifiers->max_keypermod + key];
                    if (code && (code == num || code == scroll)) locks |= 1u << mod;
                }
            XFreeModifiermap(modifiers);
        }
        std::vector<unsigned> modifiersToGrab{0};
        for (unsigned bit = 1; bit <= Mod5Mask; bit <<= 1) {
            if (!(locks & bit)) continue;
            const auto count = modifiersToGrab.size();
            for (size_t i = 0; i < count; ++i) modifiersToGrab.push_back(modifiersToGrab[i] | bit);
        }
        const KeySym keys[] = {XK_r, XK_p, XK_s};
        std::array<KeyCode, 3> codes{};
        for (int i = 0; i < 3; ++i) codes[i] = XKeysymToKeycode(m_display, keys[i]);
        if (codes == m_codes && modifiersToGrab == m_modifiers) return;
        for (int i = 0; i < 3; ++i) if (m_registered[i]) {
            ungrab(i);
            emit keyStateChanged(shortcutActions[i], false);
        }
        m_activeKey = 0;
        m_lockMask = locks;
        m_codes = codes;
        m_modifiers = std::move(modifiersToGrab);
        for (auto action : shortcutActions) {
            const auto i = int(action);
            XSync(m_display, False);
            grabbingDisplay = m_display;
            grabError = 0;
            previousErrorHandler = XSetErrorHandler(recordGrabError);
            if (m_codes[i]) for (int screen = 0; screen < ScreenCount(m_display); ++screen)
                for (auto modifier : m_modifiers)
                    XGrabKey(m_display, m_codes[i], ControlMask | ShiftMask | modifier,
                             RootWindow(m_display, screen), False, GrabModeAsync, GrabModeAsync);
            XSync(m_display, False);
            XSetErrorHandler(previousErrorHandler);
            grabbingDisplay = nullptr;
            m_registered[i] = m_codes[i] && !grabError;
            if (!m_registered[i]) ungrab(i);
            emit registrationChanged(action, m_registered[i], m_registered[i]
                ? QStringLiteral("다른 앱에서도 사용 가능")
                : QStringLiteral("다른 앱과 충돌하거나 X11 키 등록이 거부되었습니다."));
        }
        XFlush(m_display);
    }
    void ungrab(int i) {
        if (!m_codes[i]) return;
        for (int screen = 0; screen < ScreenCount(m_display); ++screen)
            for (auto modifier : m_modifiers)
                XUngrabKey(m_display, m_codes[i], ControlMask | ShiftMask | modifier, RootWindow(m_display, screen));
    }
    void receive() {
        while (XPending(m_display)) {
            XEvent event{};
            XNextEvent(m_display, &event);
            if (event.type == MappingNotify) {
                XRefreshKeyboardMapping(&event.xmapping);
                if (event.xmapping.request != MappingPointer) refreshBindings();
                continue;
            }
            if (event.type != KeyPress && event.type != KeyRelease) continue;
            if (event.type == KeyRelease && !m_detectable && XPending(m_display)) {
                XEvent next{};
                XPeekEvent(m_display, &next);
                if (next.type == KeyPress && next.xkey.keycode == event.xkey.keycode
                    && next.xkey.time == event.xkey.time) { XNextEvent(m_display, &next); continue; }
            }
            for (int i = 0; i < 3; ++i) {
                if (!m_registered[i] || event.xkey.keycode != m_codes[i]) continue;
                if (event.type == KeyRelease) {
                    if (m_activeKey == m_codes[i]) {
                        emit keyStateChanged(shortcutActions[i], false);
                        m_activeKey = 0;
                    }
                } else {
                    // A passive grab receives unrelated keys until its trigger
                    // releases. Do not treat those keys as additional shortcuts.
                    if (m_activeKey && m_activeKey != m_codes[i]) continue;
                    const auto modifiers = event.xkey.state & 0xffu & ~m_lockMask;
                    if (modifiers != (ControlMask | ShiftMask)) continue;
                    m_activeKey = m_codes[i];
                    emit keyStateChanged(shortcutActions[i], true);
                }
            }
        }
    }
    Display *m_display = nullptr;
    std::unique_ptr<QSocketNotifier> m_notifier;
    std::array<KeyCode, 3> m_codes{};
    std::array<bool, 3> m_registered{};
    std::vector<unsigned> m_modifiers;
    bool m_detectable = false;
    unsigned m_lockMask = LockMask;
    KeyCode m_activeKey = 0;
};
#endif

class UnavailableShortcuts final : public ShortcutBackend {
public:
    void start() override {
        for (auto action : shortcutActions)
            emit registrationChanged(action, false, QStringLiteral("이 환경에서는 전역 단축키를 사용할 수 없습니다."));
    }
    void stop() override {}
};

std::unique_ptr<ShortcutBackend> createShortcutBackend() {
    const auto platform = QGuiApplication::platformName();
#if defined(Q_OS_WIN)
    if (platform == QStringLiteral("windows")) return std::make_unique<WindowsShortcuts>();
#elif defined(Q_OS_MACOS)
    if (platform == QStringLiteral("cocoa")) return std::make_unique<MacShortcuts>();
#elif defined(CLIPDOCK_HAS_X11)
    const bool wayland = platform.startsWith(QStringLiteral("wayland"))
        || (platform == QStringLiteral("xcb") && (!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
            || qEnvironmentVariable("XDG_SESSION_TYPE") == QStringLiteral("wayland")));
#ifdef CLIPDOCK_HAS_PORTAL
    if (wayland) return std::make_unique<PortalGlobalShortcuts>();
#endif
    if (platform == QStringLiteral("xcb") && !wayland) return std::make_unique<X11Shortcuts>();
#endif
    return std::make_unique<UnavailableShortcuts>();
}
