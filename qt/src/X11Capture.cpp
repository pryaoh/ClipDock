#include "X11Capture.h"
#include "RegionCapture.h"
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#ifdef Q_OS_LINUX
#include <QtGui/qguiapplication_platform.h>
#include <xcb/xcb.h>
#include <cstdlib>
#include <memory>

namespace {
template<class T> using Reply = std::unique_ptr<T, decltype(&std::free)>;
class ServerGrab {
public:
    explicit ServerGrab(xcb_connection_t *connection) : m_connection(connection) {
        xcb_grab_server(m_connection);
    }
    ~ServerGrab() {
        xcb_ungrab_server(m_connection);
        xcb_flush(m_connection);
    }
private:
    xcb_connection_t *m_connection;
};
}
#endif

bool X11Capture::available() {
#ifdef Q_OS_LINUX
    return QGuiApplication::platformName() == QStringLiteral("xcb")
        && qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
#else
    return false;
#endif
}

X11Capture::Frame X11Capture::grab(QScreen *screen, const QRect &region, const QList<WId> &excluded) {
#ifdef Q_OS_LINUX
    if (!screen || !available()) return {};
    auto *connection = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
    if (!connection || xcb_connection_has_error(connection)) return {};
    QPixmap pixels;
    {
        // Use Qt's connection: grabWindow must remain able to issue XCB requests.
        // No event processing, callbacks, encoding or UI signals inside this scope.
        ServerGrab lock(connection);
        pixels = screen->grabWindow(0);
        if (pixels.isNull()) return {};
        // Use the exact crop endpoints, including fractional-scale rounding.
        const QRect area = region.isEmpty() ? QRect(QPoint(), pixels.size())
            : regionPixelRect(region, screen->size(), pixels.size());
        if (area.isEmpty()) return {};
        const QRect nativeArea = area.translated(screen->geometry().topLeft());
        for (WId id : excluded) {
            Reply<xcb_get_window_attributes_reply_t> attributes(
                xcb_get_window_attributes_reply(connection, xcb_get_window_attributes(connection, id), nullptr), &std::free);
            if (!attributes) return {{}, true}; // Fail closed for a disappearing/recreated native window.
            if (attributes->map_state != XCB_MAP_STATE_VIEWABLE) continue;
            // Include the window manager's frame, not only the Qt client rectangle.
            xcb_window_t frame = id;
            xcb_window_t root = XCB_WINDOW_NONE;
            for (int depth = 0; depth < 64; ++depth) {
                Reply<xcb_query_tree_reply_t> tree(
                    xcb_query_tree_reply(connection, xcb_query_tree(connection, frame), nullptr), &std::free);
                if (!tree) return {{}, true};
                root = tree->root;
                if (tree->parent == root) break;
                if (!tree->parent || tree->parent == frame || depth == 63) return {{}, true};
                frame = tree->parent;
            }
            Reply<xcb_get_geometry_reply_t> geometry(
                xcb_get_geometry_reply(connection, xcb_get_geometry(connection, frame), nullptr), &std::free);
            Reply<xcb_translate_coordinates_reply_t> position(
                xcb_translate_coordinates_reply(connection,
                    xcb_translate_coordinates(connection, frame, root, 0, 0), nullptr), &std::free);
            if (!geometry || !position) return {{}, true};
            const int border = geometry->border_width;
            const QRect bounds(position->dst_x - border, position->dst_y - border,
                               geometry->width + 2 * border, geometry->height + 2 * border);
            if (nativeArea.intersects(bounds)) return {{}, true};
        }
    }
    return {pixels.toImage(), false};
#else
    Q_UNUSED(screen);
    Q_UNUSED(region);
    Q_UNUSED(excluded);
    return {};
#endif
}
