import AppKit
import Combine
import ScreenCaptureKit
import ScreenEncoderCore
import UniformTypeIdentifiers

@MainActor
final class RecorderModel: ObservableObject {
    enum State { case idle, starting, previewing, recording, paused, stopping }
    struct Target: Identifiable {
        let id: String
        let title: String
        let display: SCDisplay?
        let window: SCWindow?
        var size: CGSize {
            if let display { return CGSize(width: display.width, height: display.height) }
            return window?.frame.size ?? CGSize(width: 1920, height: 1080)
        }
    }

    @Published var state: State = .idle
    @Published var targets: [Target] = []
    @Published var selectedID = ""
    @Published var maxEdge = 1920
    @Published var fps = 25
    @Published var bitrateKbps = 5000
    @Published var keyFrameInterval = 50
    @Published var audio = AudioSource.none
    @Published var cursor = true
    @Published var preview: CGImage?
    @Published var status = "화면 목록을 불러와 녹화 대상을 선택하세요."
    @Published var errorMessage: String?
    @Published var lastFile: URL?
    @Published var loading = false
    @Published var seconds: TimeInterval = 0
    private let engine = CaptureEngine()
    private var content: SCShareableContent?
    private var timer: Timer?
    private var started: Date?
    private var pauseStart: Date?
    private var pausedSeconds: TimeInterval = 0

    var isRecording: Bool { state == .recording || state == .paused }
    var isBusy: Bool { state == .starting || state == .stopping }
    var canConfigure: Bool { state == .idle && !loading }
    var selected: Target? { targets.first { $0.id == selectedID } }
    var dimensions: (Int, Int) {
        let size = selected?.size ?? CGSize(width: 1920, height: 1080)
        return RecordingSettings.fittedSize(width: size.width, height: size.height, maxEdge: maxEdge)
    }

    func refresh() async {
        guard state == .idle, !loading else { return }
        loading = true
        defer { loading = false }
        do {
            let content = try await SCShareableContent.excludingDesktopWindows(true, onScreenWindowsOnly: true)
            self.content = content
            let displays = content.displays.enumerated().map { index, display in
                Target(id: "display-\(display.displayID)", title: "디스플레이 \(index + 1) · \(display.width) × \(display.height)",
                       display: display, window: nil)
            }
            let windows = content.windows.filter {
                $0.owningApplication?.processID != ProcessInfo.processInfo.processIdentifier &&
                $0.windowLayer == 0 && $0.frame.width > 16 && $0.frame.height > 16 && !($0.title ?? "").isEmpty
            }.sorted { ($0.owningApplication?.applicationName ?? "") < ($1.owningApplication?.applicationName ?? "") }
                .map { Target(id: "window-\($0.windowID)", title: "\($0.owningApplication?.applicationName ?? "앱") — \($0.title ?? "창")", display: nil, window: $0) }
            targets = displays + windows
            if !targets.contains(where: { $0.id == selectedID }) { selectedID = targets.first?.id ?? "" }
            status = targets.isEmpty ? "녹화할 화면이 없습니다." : "녹화 준비 완료"
        } catch {
            targets = []
            selectedID = ""
            status = "화면 목록을 가져오지 못했습니다. 화면 기록 권한을 확인하세요."
            errorMessage = "\(error.localizedDescription)\n\n시스템 설정 > 개인정보 보호 및 보안 > 화면 및 시스템 오디오 기록에서 ScreenEncoder를 허용한 뒤 앱을 다시 실행하세요."
        }
    }

    func start(recording: Bool) async {
        guard state == .idle, let target = selected else { return }
        guard (100...100_000).contains(bitrateKbps), (1...600).contains(keyFrameInterval) else {
            errorMessage = "비트레이트는 100~100000 kbps, 키 프레임 간격은 1~600이어야 합니다."
            return
        }
        state = .starting
        var url: URL?
        if recording {
            let panel = NSSavePanel()
            panel.allowedContentTypes = [.mpeg4Movie]
            panel.canCreateDirectories = true
            let formatter = DateFormatter()
            formatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
            panel.nameFieldStringValue = "ScreenEncoder_\(formatter.string(from: Date())).mp4"
            panel.directoryURL = FileManager.default.urls(for: .moviesDirectory, in: .userDomainMask).first
            guard panel.runModal() == .OK, let chosen = panel.url else { state = .idle; return }
            // Do not truncate an existing recording, including when NSSavePanel offers Replace.
            guard !FileManager.default.fileExists(atPath: chosen.path) else {
                state = .idle
                errorMessage = "기존 녹화 파일을 보호하기 위해 다른 파일 이름을 사용하세요."
                return
            }
            url = chosen
        }
        let filter: SCContentFilter
        if let display = target.display {
            let ownApp = content?.applications.filter { $0.processID == ProcessInfo.processInfo.processIdentifier } ?? []
            filter = SCContentFilter(display: display, excludingApplications: ownApp, exceptingWindows: [])
        } else if let window = target.window {
            filter = SCContentFilter(desktopIndependentWindow: window)
        } else { state = .idle; return }
        let (width, height) = dimensions
        let audioSource = recording ? audio : .none
        let settings = RecordingSettings(width: width, height: height, fps: fps, bitrate: bitrateKbps * 1000,
                                         keyFrameInterval: keyFrameInterval, audio: audioSource != .none)
        preview = nil
        do {
            try await engine.start(filter: filter, settings: settings, audio: audioSource, cursor: cursor, url: url,
                                   preview: { [weak self] image in self?.preview = image },
                                   failure: { [weak self] error in
                Task { @MainActor in await self?.handleFailure(error) }
            })
            guard state == .starting else { return }
            state = recording ? .recording : .previewing
            status = recording ? "녹화 중 · \(width) × \(height) · \(fps) FPS" : "미리보기 중 · 파일은 저장되지 않습니다."
            seconds = 0
            pausedSeconds = 0
            pauseStart = nil
            started = Date()
            if recording {
                timer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: true) { [weak self] _ in
                    Task { @MainActor in self?.updateTime() }
                }
            }
        } catch {
            state = .idle
            status = "시작 실패"
            errorMessage = error.localizedDescription
        }
    }

    func togglePause() {
        guard isRecording else { return }
        let paused = state == .recording
        engine.pause(paused)
        if paused { pauseStart = Date(); status = "일시정지 중" }
        else {
            if let pauseStart { pausedSeconds += Date().timeIntervalSince(pauseStart) }
            pauseStart = nil
            status = "녹화 중"
        }
        state = paused ? .paused : .recording
    }

    func stop() async {
        guard state != .idle, state != .stopping else { return }
        state = .stopping
        timer?.invalidate()
        timer = nil
        status = "녹화를 마무리하는 중…"
        do {
            if let url = try await engine.stop() { lastFile = url; status = "저장 완료 · \(url.lastPathComponent)" }
            else { status = "미리보기 종료" }
        } catch { errorMessage = error.localizedDescription; status = "저장 실패" }
        state = .idle
    }

    private func handleFailure(_ error: Error) async {
        guard state != .idle, state != .stopping else { return }
        await stop()
        errorMessage = "캡처가 중단되었습니다: \(error.localizedDescription)\n\(status)"
    }

    private func updateTime() {
        guard let started else { return }
        seconds = max(0, (pauseStart ?? Date()).timeIntervalSince(started) - pausedSeconds)
    }
}
