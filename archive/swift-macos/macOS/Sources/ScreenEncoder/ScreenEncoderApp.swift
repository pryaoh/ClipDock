import AppKit
import SwiftUI

@main
struct ScreenEncoderApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) private var delegate
    @StateObject private var model = RecorderModel()

    var body: some Scene {
        WindowGroup("ScreenEncoder") {
            RecorderView(model: model)
                .onAppear {
                    delegate.model = model
                    NSApp.setActivationPolicy(.regular)
                    NSApp.activate(ignoringOtherApps: true)
                }
        }
        .defaultSize(width: 780, height: 710)
        .windowResizability(.contentSize)
        .commands {
            CommandGroup(replacing: .newItem) { }
            CommandMenu("녹화") {
                Button("녹화 시작") { Task { await model.start(recording: true) } }
                    .keyboardShortcut("r", modifiers: [.command, .shift])
                    .disabled(!model.canConfigure || model.selected == nil)
                Button(model.state == .paused ? "녹화 재개" : "일시정지") { model.togglePause() }
                    .keyboardShortcut("p", modifiers: [.command, .shift])
                    .disabled(!model.isRecording)
                Button("중지 및 저장") { Task { await model.stop() } }
                    .keyboardShortcut("s", modifiers: [.command, .shift])
                    .disabled(model.state == .idle || model.isBusy)
            }
        }
    }
}

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    weak var model: RecorderModel?
    private var terminating = false

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }

    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        guard let model, model.state != .idle else { return .terminateNow }
        // Starting includes a permission prompt/save panel; don't exit halfway through startup.
        guard model.state != .starting else { return .terminateCancel }
        guard !terminating else { return .terminateLater }
        terminating = true
        Task {
            if model.state == .stopping {
                while model.state == .stopping { try? await Task.sleep(nanoseconds: 100_000_000) }
            } else { await model.stop() }
            if let message = model.errorMessage {
                let alert = NSAlert()
                alert.messageText = "녹화 종료 결과"
                alert.informativeText = message
                alert.runModal()
            }
            sender.reply(toApplicationShouldTerminate: true)
        }
        return .terminateLater
    }
}

struct RecorderView: View {
    @ObservedObject var model: RecorderModel

    var body: some View {
        VStack(alignment: .leading, spacing: 18) {
            HStack {
                Image(systemName: "record.circle").font(.system(size: 32)).foregroundStyle(.red)
                VStack(alignment: .leading, spacing: 3) {
                    Text("ScreenEncoder").font(.title2.bold())
                    Text("화면과 소리를 MP4로 기록하세요.").foregroundStyle(.secondary)
                }
                Spacer()
                Text(timeText).font(.system(.title2, design: .monospaced))
                    .foregroundStyle(model.isRecording ? Color.red : Color.secondary)
            }
            GroupBox {
                VStack(alignment: .leading, spacing: 12) {
                    HStack {
                        Picker("녹화 대상", selection: $model.selectedID) {
                            if model.targets.isEmpty { Text("화면 목록을 불러오세요").tag("") }
                            ForEach(model.targets) { Text($0.title).tag($0.id) }
                        }
                        .disabled(!model.canConfigure)
                        Button(model.loading ? "불러오는 중…" : "화면 목록 새로고침") {
                            Task { await model.refresh() }
                        }.disabled(!model.canConfigure)
                    }
                    HStack(spacing: 18) {
                        Picker("최대 해상도", selection: $model.maxEdge) {
                            Text("1280 px").tag(1280)
                            Text("1920 px").tag(1920)
                            Text("2560 px").tag(2560)
                            Text("원본 (최대 4096 px)").tag(4096)
                        }
                        Picker("FPS", selection: $model.fps) {
                            ForEach([10, 15, 25, 30, 60], id: \.self) { Text("\($0)").tag($0) }
                        }.frame(width: 145)
                    }.disabled(!model.canConfigure)
                    HStack {
                        Text("비트레이트 (kbps)")
                        TextField("5000", value: $model.bitrateKbps, format: .number.grouping(.never)).frame(width: 90)
                        Spacer()
                        Text("키 프레임 간격")
                        TextField("50", value: $model.keyFrameInterval, format: .number.grouping(.never)).frame(width: 70)
                        Text("프레임").foregroundStyle(.secondary)
                    }.disabled(!model.canConfigure)
                    HStack {
                        Picker("소리", selection: $model.audio) {
                            Text(AudioSource.none.rawValue).tag(AudioSource.none)
                            Text(AudioSource.system.rawValue).tag(AudioSource.system)
                            if #available(macOS 15, *) { Text(AudioSource.microphone.rawValue).tag(AudioSource.microphone) }
                        }
                        Toggle("마우스 커서 포함", isOn: $model.cursor)
                    }.disabled(!model.canConfigure)
                    Text("출력 \(model.dimensions.0) × \(model.dimensions.1) · H.264 / AAC · 화면 비율 유지")
                        .font(.caption).foregroundStyle(.secondary)
                }.padding(8)
            }
            ZStack {
                RoundedRectangle(cornerRadius: 10).fill(Color.black.opacity(0.92))
                if let image = model.preview {
                    Image(decorative: image, scale: 1).resizable().scaledToFit().padding(6)
                } else {
                    VStack(spacing: 10) {
                        Image(systemName: "display").font(.system(size: 38))
                        Text("미리보기 또는 녹화를 시작하면 화면이 표시됩니다.").font(.callout)
                    }.foregroundStyle(.white.opacity(0.65))
                }
                if model.state == .paused {
                    Label("일시정지", systemImage: "pause.fill")
                        .padding(10).background(.regularMaterial, in: Capsule())
                }
            }.frame(height: 270)
            HStack(spacing: 10) {
                Button { Task { await model.start(recording: true) } } label: {
                    Label("녹화 시작", systemImage: "record.circle")
                }.buttonStyle(.borderedProminent).tint(.red)
                    .disabled(!model.canConfigure || model.selected == nil)
                Button("미리보기") { Task { await model.start(recording: false) } }
                    .disabled(!model.canConfigure || model.selected == nil)
                Button(model.state == .paused ? "재개" : "일시정지") { model.togglePause() }
                    .disabled(!model.isRecording)
                Button(model.state == .previewing ? "미리보기 종료" : "중지 및 저장") { Task { await model.stop() } }
                    .disabled(model.state == .idle || model.isBusy)
                Spacer()
                if model.isBusy { ProgressView().controlSize(.small) }
            }
            HStack {
                Text(model.status).font(.callout).lineLimit(2).textSelection(.enabled)
                Spacer()
                if let url = model.lastFile {
                    Button("파일 보기") { NSWorkspace.shared.activateFileViewerSelecting([url]) }
                }
            }
            HStack {
                Text("최초 사용 시 macOS의 화면 기록 권한이 필요합니다.")
                Spacer()
                Button("권한 설정 열기") {
                    if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture") {
                        NSWorkspace.shared.open(url)
                    }
                }
            }.font(.caption).foregroundStyle(.secondary)
        }
        .padding(24)
        .frame(width: 750)
        .alert("ScreenEncoder", isPresented: Binding(get: { model.errorMessage != nil }, set: { if !$0 { model.errorMessage = nil } })) {
            Button("확인", role: .cancel) { model.errorMessage = nil }
        } message: { Text(model.errorMessage ?? "") }
    }

    private var timeText: String {
        let total = Int(model.seconds)
        return String(format: "%02d:%02d:%02d", total / 3600, total / 60 % 60, total % 60)
    }
}
