#include "WindowCaptureBootstrap.h"
#include <QThread>

#ifdef Q_OS_WIN
#include <QDebug>
#include <QElapsedTimer>
#include <QtMultimedia/private/qcapturablewindow_p.h>
#include <unknwn.h>
#include <winrt/base.h>
// Windows SDK forward declaration workaround, also used by Qt Multimedia.
namespace winrt::impl {
template <typename Async>
auto wait_for(Async const &, Windows::Foundation::TimeSpan const &);
}
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <d3d11.h>
#include <dxgi1_2.h>

namespace {
using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

struct Apartment {
    Apartment() { init_apartment(); }
    ~Apartment() { uninit_apartment(); }
};

com_ptr<IDXGIAdapter1> captureAdapter(HWND window) {
    com_ptr<IDXGIFactory1> factory;
    check_hresult(CreateDXGIFactory1(guid_of<IDXGIFactory1>(), factory.put_void()));
    const HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
    com_ptr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, adapter.put()) == S_OK; adapter = nullptr, ++i) {
        com_ptr<IDXGIOutput> output;
        for (UINT j = 0; adapter->EnumOutputs(j, output.put()) == S_OK; output = nullptr, ++j) {
            DXGI_OUTPUT_DESC description{};
            if (SUCCEEDED(output->GetDesc(&description)) && description.Monitor == monitor) return adapter;
        }
    }
    return {};
}

QImage firstWindowImage(HWND window) {
    if (!IsWindow(window)) return {};
    Apartment apartment;
    const auto interop = get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    GraphicsCaptureItem item{nullptr};
    check_hresult(interop->CreateForWindow(window, guid_of<GraphicsCaptureItem>(), put_abi(item)));
    if (QThread::currentThread()->isInterruptionRequested()) return {};
    auto adapter = captureAdapter(window);
    com_ptr<ID3D11Device> device;
    com_ptr<ID3D11DeviceContext> context;
    check_hresult(D3D11CreateDevice(adapter.get(), adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
        nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, context.put()));
    com_ptr<IInspectable> inspectable;
    check_hresult(CreateDirect3D11DeviceFromDXGIDevice(device.as<IDXGIDevice>().get(), inspectable.put()));
    const auto captureDevice = inspectable.as<IDirect3DDevice>();
    // The pool must use the actual capture item's size, not GetWindowRect's
    // invisible resize margins. Preserve its first frame even when those differ.
    auto pool = Direct3D11CaptureFramePool::CreateFreeThreaded(captureDevice,
        DirectXPixelFormat::B8G8R8A8UIntNormalized, 1, item.Size());
    auto session = pool.CreateCaptureSession(item);
    if (const auto session3 = session.try_as<IGraphicsCaptureSession3>()) session3.IsBorderRequired(false);
    session.StartCapture();
    QElapsedTimer elapsed;
    elapsed.start();
    QImage image;
    while (!QThread::currentThread()->isInterruptionRequested() && elapsed.elapsed() < 15000 && IsWindow(window)) {
        auto frame = pool.TryGetNextFrame();
        if (!frame) { QThread::msleep(10); continue; }
        const auto size = frame.ContentSize();
        if (size.Width <= 0 || size.Height <= 0) continue;
        com_ptr<ID3D11Texture2D> texture;
        check_hresult(frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()
            ->GetInterface(guid_of<ID3D11Texture2D>(), texture.put_void()));
        D3D11_TEXTURE2D_DESC description{};
        texture->GetDesc(&description);
        if (UINT(size.Width) > description.Width || UINT(size.Height) > description.Height) {
            // A real resize can outgrow the pool. Only this case needs a new
            // texture; a smaller content rectangle can be copied immediately.
            frame.Close();
            pool.Recreate(captureDevice, DirectXPixelFormat::B8G8R8A8UIntNormalized, 1, size);
            continue;
        }
        description.Usage = D3D11_USAGE_STAGING;
        description.BindFlags = 0;
        description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        description.MiscFlags = 0;
        com_ptr<ID3D11Texture2D> staging;
        check_hresult(device->CreateTexture2D(&description, nullptr, staging.put()));
        context->CopyResource(staging.get(), texture.get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        check_hresult(context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped));
        image = QImage(static_cast<const uchar *>(mapped.pData), size.Width, size.Height,
            mapped.RowPitch, QImage::Format_ARGB32).copy();
        context->Unmap(staging.get(), 0);
        break;
    }
    session.Close(); pool.Close();
    return image;
}
}
#endif

WindowCaptureBootstrap::WindowCaptureBootstrap(QObject *parent) : QObject(parent) {}
WindowCaptureBootstrap::~WindowCaptureBootstrap() { stop(); }

void WindowCaptureBootstrap::start(const QCapturableWindow &window) {
    stop();
#ifdef Q_OS_WIN
    const auto *handle = QCapturableWindowPrivate::handle(window);
    if (!handle || !handle->id) return;
    const auto id = quintptr(handle->id);
    const auto generation = m_generation;
    m_worker = QThread::create([this, id, generation] {
        QImage image;
        try { image = firstWindowImage(reinterpret_cast<HWND>(id)); }
        catch (const winrt::hresult_error &error) {
            // Qt's normal capture remains active and owns user-facing errors.
            qWarning() << "Initial Windows window frame unavailable:" << Qt::hex << quint32(error.code().value);
        }
        if (image.isNull() || QThread::currentThread()->isInterruptionRequested()) return;
        QMetaObject::invokeMethod(this, [this, generation, image] {
            if (generation != m_generation) return; // Stopped/replaced capture.
            stop(); // Close the extra session before passing ownership to Qt.
            emit frameReady(image);
        }, Qt::QueuedConnection);
    });
    m_worker->start();
#else
    Q_UNUSED(window);
#endif
}

void WindowCaptureBootstrap::stop() {
    ++m_generation;
    if (!m_worker) return;
    m_worker->requestInterruption();
    m_worker->wait();
    delete m_worker;
    m_worker = nullptr;
}
