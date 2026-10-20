import AppKit
import AVFoundation
import CoreImage
import ScreenCaptureKit
import ScreenEncoderCore

enum AudioSource: String, CaseIterable, Identifiable {
    case none = "녹음 안 함", system = "시스템 소리", microphone = "마이크 (macOS 15 이상)"
    var id: String { rawValue }
}

/// Stream callbacks and all writer access share a single serial queue.
final class CaptureOutput: NSObject, SCStreamOutput, SCStreamDelegate, @unchecked Sendable {
    let queue = DispatchQueue(label: "ScreenEncoder.samples", qos: .userInitiated)
    var movie: MovieWriter?
    var onPreview: ((CGImage) -> Void)?
    var onFailure: ((Error) -> Void)?
    private let context = CIContext(options: [.cacheIntermediates: false])
    private var lastPreview = CFAbsoluteTimeGetCurrent()
    private var failed = false

    func stream(_ stream: SCStream, didOutputSampleBuffer sample: CMSampleBuffer, of type: SCStreamOutputType) {
        guard !failed, sample.isValid else { return }
        let isVideo = type == .screen
        if isVideo {
            guard let attachments = CMSampleBufferGetSampleAttachmentsArray(sample, createIfNecessary: false) as? [[SCStreamFrameInfo: Any]],
                  let raw = attachments.first?[.status] as? Int,
                  SCFrameStatus(rawValue: raw) == .complete,
                  let pixel = sample.imageBuffer else { return }
            let now = CFAbsoluteTimeGetCurrent()
            if now - lastPreview >= 0.2 {
                lastPreview = now
                let source = CIImage(cvPixelBuffer: pixel)
                let scale = min(1, 800 / source.extent.width)
                let small = source.transformed(by: CGAffineTransform(scaleX: scale, y: scale))
                if let image = context.createCGImage(small, from: small.extent) {
                    onPreview?(image)
                }
            }
        }
        do { try movie?.append(sample, isVideo: isVideo) }
        catch { report(error) }
    }

    func stream(_ stream: SCStream, didStopWithError error: Error) {
        queue.async { self.report(error) }
    }

    private func report(_ error: Error) {
        guard !failed else { return }
        failed = true
        onFailure?(error)
    }
}

@MainActor
final class CaptureEngine {
    private var stream: SCStream?
    private var output: CaptureOutput?
    // SCStreamConfiguration.backgroundColor is an assign (non-retaining) property.
    private let backgroundColor = CGColor(gray: 0, alpha: 1)

    func start(filter: SCContentFilter, settings: RecordingSettings, audio: AudioSource,
               cursor: Bool, url: URL?, preview: @escaping (CGImage) -> Void,
               failure: @escaping (Error) -> Void) async throws {
        try settings.validate()
        let config = SCStreamConfiguration()
        config.width = settings.width
        config.height = settings.height
        config.minimumFrameInterval = CMTime(value: 1, timescale: Int32(settings.fps))
        config.pixelFormat = kCVPixelFormatType_32BGRA
        config.queueDepth = 5
        config.showsCursor = cursor
        config.scalesToFit = true
        config.backgroundColor = backgroundColor
        config.capturesAudio = audio == .system
        config.sampleRate = 48000
        config.channelCount = 2
        config.excludesCurrentProcessAudio = true
        if #available(macOS 14, *) {
            config.preservesAspectRatio = true
            config.ignoreShadowsSingleWindow = true
        }
        if audio == .microphone {
            guard #available(macOS 15, *) else {
                throw RecordingError.message("마이크 녹음에는 macOS 15 이상이 필요합니다.")
            }
            guard await AVCaptureDevice.requestAccess(for: .audio) else {
                throw RecordingError.message("시스템 설정 > 개인정보 보호 및 보안 > 마이크에서 ScreenEncoder를 허용하세요.")
            }
            config.captureMicrophone = true
        }
        let output = CaptureOutput()
        output.onPreview = { image in DispatchQueue.main.async { preview(image) } }
        output.onFailure = { error in DispatchQueue.main.async { failure(error) } }
        if let url { output.movie = try MovieWriter(url: url, settings: settings) }
        let stream = SCStream(filter: filter, configuration: config, delegate: output)
        do {
            try stream.addStreamOutput(output, type: .screen, sampleHandlerQueue: output.queue)
            if audio == .system { try stream.addStreamOutput(output, type: .audio, sampleHandlerQueue: output.queue) }
            if #available(macOS 15, *), audio == .microphone {
                try stream.addStreamOutput(output, type: .microphone, sampleHandlerQueue: output.queue)
            }
            self.output = output
            self.stream = stream
            try await stream.startCapture()
        } catch {
            output.queue.sync { output.movie?.cancel() }
            self.output = nil
            self.stream = nil
            throw error
        }
    }

    func pause(_ paused: Bool) {
        guard let output else { return }
        let time = CMClockGetTime(CMClockGetHostTimeClock())
        output.queue.async { output.movie?.setPaused(paused, at: time) }
    }

    func stop() async throws -> URL? {
        guard let stream, let output else { return nil }
        self.stream = nil
        self.output = nil
        let time = CMClockGetTime(CMClockGetHostTimeClock())
        // A stream may already have stopped due to its target closing. Still finalize the file.
        try? await stream.stopCapture()
        return try await withCheckedThrowingContinuation { continuation in
            output.queue.async {
                output.onPreview = nil
                output.onFailure = nil
                guard let movie = output.movie else { continuation.resume(returning: nil); return }
                movie.finish(at: time) { result in continuation.resume(with: result.map { Optional($0) }) }
                output.movie = nil
            }
        }
    }
}
