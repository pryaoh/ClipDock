import AVFoundation
import CoreMedia

/// All methods are called on CaptureEngine's serial sample queue.
public final class MovieWriter {
    public let url: URL
    private let writer: AVAssetWriter
    private let video: AVAssetWriterInput
    private let audio: AVAssetWriterInput?
    private let frameDuration: CMTime
    private var firstTime: CMTime?
    private var lastVideoTime: CMTime?
    private var lastAudioTime: CMTime?
    private var lastVideo: CMSampleBuffer?
    private var pausedAt: CMTime?
    private var pauseOffset = CMTime.zero
    private var finishing = false
    public private(set) var frameCount = 0

    public init(url: URL, settings: RecordingSettings) throws {
        try settings.validate()
        guard !FileManager.default.fileExists(atPath: url.path) else {
            throw RecordingError.message("같은 이름의 파일이 이미 존재합니다. 다른 이름을 선택하세요.")
        }
        self.url = url
        frameDuration = CMTime(value: 1, timescale: Int32(settings.fps))
        writer = try AVAssetWriter(outputURL: url, fileType: .mp4)
        video = AVAssetWriterInput(mediaType: .video, outputSettings: [
            AVVideoCodecKey: AVVideoCodecType.h264,
            AVVideoWidthKey: settings.width, AVVideoHeightKey: settings.height,
            AVVideoCompressionPropertiesKey: [
                AVVideoAverageBitRateKey: settings.bitrate,
                AVVideoExpectedSourceFrameRateKey: settings.fps,
                AVVideoMaxKeyFrameIntervalKey: settings.keyFrameInterval,
                AVVideoProfileLevelKey: AVVideoProfileLevelH264HighAutoLevel,
                AVVideoAllowFrameReorderingKey: false
            ]
        ])
        video.expectsMediaDataInRealTime = true
        guard writer.canAdd(video) else { throw RecordingError.message("H.264 인코더를 초기화할 수 없습니다.") }
        writer.add(video)
        if settings.audio {
            let input = AVAssetWriterInput(mediaType: .audio, outputSettings: [
                AVFormatIDKey: kAudioFormatMPEG4AAC, AVSampleRateKey: 48000,
                AVNumberOfChannelsKey: 2, AVEncoderBitRateKey: 192_000
            ])
            input.expectsMediaDataInRealTime = true
            guard writer.canAdd(input) else { throw RecordingError.message("AAC 인코더를 초기화할 수 없습니다.") }
            writer.add(input)
            audio = input
        } else { audio = nil }
        guard writer.startWriting() else { throw writer.error ?? RecordingError.message("파일을 생성할 수 없습니다.") }
    }

    public func setPaused(_ paused: Bool, at time: CMTime = CMClockGetTime(CMClockGetHostTimeClock())) {
        if paused, pausedAt == nil { pausedAt = time }
        if !paused, let start = pausedAt {
            pauseOffset = pauseOffset + max(.zero, time - start)
            pausedAt = nil
        }
    }

    public func append(_ sample: CMSampleBuffer, isVideo: Bool) throws {
        guard !finishing, pausedAt == nil, sample.isValid, CMSampleBufferDataIsReady(sample) else { return }
        if writer.status == .failed { throw writer.error ?? RecordingError.message("인코딩 중 오류가 발생했습니다.") }
        let time = sample.presentationTimeStamp - pauseOffset
        guard time.isNumeric else { return }
        if firstTime == nil {
            guard isVideo else { return } // Audio must never start the video session.
            writer.startSession(atSourceTime: time)
            firstTime = time
        }
        guard let firstTime, time >= firstTime else { return }
        let input = isVideo ? video : audio
        guard let input, input.isReadyForMoreMediaData else { return }
        if let previous = isVideo ? lastVideoTime : lastAudioTime, time <= previous { return }
        let adjusted = try Self.retimed(sample, subtracting: pauseOffset)
        guard input.append(adjusted) else { throw writer.error ?? RecordingError.message("미디어 프레임을 저장하지 못했습니다.") }
        if isVideo {
            lastVideoTime = time
            lastVideo = adjusted
            frameCount += 1
        } else { lastAudioTime = time }
    }

    public func finish(at stopTime: CMTime = CMClockGetTime(CMClockGetHostTimeClock()),
                       completion: @escaping (Result<URL, Error>) -> Void) {
        guard !finishing else { return }
        finishing = true
        guard firstTime != nil, let lastVideoTime else {
            cancel()
            completion(.failure(RecordingError.message("녹화된 화면이 없습니다. 화면 권한과 녹화 대상을 확인하세요.")))
            return
        }
        guard writer.status == .writing else {
            let error = writer.error ?? RecordingError.message("녹화 파일을 완성하지 못했습니다.")
            cancel()
            completion(.failure(error))
            return
        }
        // ScreenCaptureKit does not produce new complete frames for an unchanged screen.
        // Repeat the final frame so a static tail (or stop while paused) has the right duration.
        let end = max(lastVideoTime + frameDuration, (pausedAt ?? stopTime) - pauseOffset)
        if let lastVideo, end - frameDuration > lastVideoTime, video.isReadyForMoreMediaData {
            do {
                let tail = try Self.retimed(lastVideo, subtracting: lastVideoTime - (end - frameDuration))
                guard video.append(tail) else { throw writer.error ?? RecordingError.message("마지막 프레임 저장 실패") }
            } catch {
                cancel()
                completion(.failure(error))
                return
            }
        }
        writer.endSession(atSourceTime: end)
        video.markAsFinished()
        audio?.markAsFinished()
        lastVideo = nil
        writer.finishWriting { [self] in
            if writer.status == .completed { completion(.success(url)) }
            else {
                let error = writer.error ?? RecordingError.message("녹화 파일 저장에 실패했습니다.")
                try? FileManager.default.removeItem(at: url)
                completion(.failure(error))
            }
        }
    }

    public func cancel() {
        finishing = true
        lastVideo = nil
        writer.cancelWriting()
        try? FileManager.default.removeItem(at: url)
    }

    private static func retimed(_ sample: CMSampleBuffer, subtracting offset: CMTime) throws -> CMSampleBuffer {
        var count = 0
        var status = CMSampleBufferGetSampleTimingInfoArray(sample, entryCount: 0, arrayToFill: nil, entriesNeededOut: &count)
        guard status == noErr else { throw RecordingError.message("프레임 시간 정보를 읽을 수 없습니다: \(status)") }
        var timing = Array(repeating: CMSampleTimingInfo(), count: count)
        status = CMSampleBufferGetSampleTimingInfoArray(sample, entryCount: count, arrayToFill: &timing, entriesNeededOut: &count)
        guard status == noErr else { throw RecordingError.message("프레임 시간 정보 오류: \(status)") }
        for i in timing.indices {
            timing[i].presentationTimeStamp = timing[i].presentationTimeStamp - offset
            if timing[i].decodeTimeStamp.isNumeric { timing[i].decodeTimeStamp = timing[i].decodeTimeStamp - offset }
        }
        var copy: CMSampleBuffer?
        status = CMSampleBufferCreateCopyWithNewTiming(allocator: kCFAllocatorDefault, sampleBuffer: sample,
                                                     sampleTimingEntryCount: count, sampleTimingArray: &timing,
                                                     sampleBufferOut: &copy)
        guard status == noErr, let copy else { throw RecordingError.message("프레임 시간 변환 실패: \(status)") }
        return copy
    }
}
