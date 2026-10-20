import AVFoundation
import ScreenEncoderCore

final class MovieWriterChecks {
    private var cleanup: [() -> Void] = []
    deinit { cleanup.forEach { $0() } }
    private func addTeardownBlock(_ action: @escaping () -> Void) { cleanup.append(action) }
    private func time(_ seconds: Double) -> CMTime { CMTime(seconds: seconds, preferredTimescale: 48000) }

    private func video(at seconds: Double) throws -> CMSampleBuffer {
        var pixel: CVPixelBuffer?
        expectEqual(CVPixelBufferCreate(kCFAllocatorDefault, 320, 180, kCVPixelFormatType_32BGRA,
                                         [kCVPixelBufferIOSurfacePropertiesKey: [:]] as CFDictionary, &pixel), kCVReturnSuccess)
        let buffer = try unwrap(pixel)
        CVPixelBufferLockBaseAddress(buffer, [])
        let bytes = CVPixelBufferGetBaseAddress(buffer)!.assumingMemoryBound(to: UInt8.self)
        let stride = CVPixelBufferGetBytesPerRow(buffer)
        for y in 0..<180 {
            for x in 0..<320 {
                let offset = y * stride + x * 4
                bytes[offset] = UInt8(x % 256)
                bytes[offset + 1] = UInt8(y % 256)
                bytes[offset + 2] = 200
                bytes[offset + 3] = 255
            }
        }
        CVPixelBufferUnlockBaseAddress(buffer, [])
        var format: CMVideoFormatDescription?
        expectEqual(CMVideoFormatDescriptionCreateForImageBuffer(allocator: kCFAllocatorDefault, imageBuffer: buffer,
                                                                  formatDescriptionOut: &format), noErr)
        var timing = CMSampleTimingInfo(duration: time(0.04), presentationTimeStamp: time(seconds), decodeTimeStamp: .invalid)
        var sample: CMSampleBuffer?
        expectEqual(CMSampleBufferCreateReadyWithImageBuffer(allocator: kCFAllocatorDefault, imageBuffer: buffer,
                                                              formatDescription: try unwrap(format), sampleTiming: &timing,
                                                              sampleBufferOut: &sample), noErr)
        return try unwrap(sample)
    }

    private func audio(at seconds: Double) throws -> CMSampleBuffer {
        let count = 1920 // 40 ms, stereo signed 16-bit PCM
        var description = AudioStreamBasicDescription(mSampleRate: 48000, mFormatID: kAudioFormatLinearPCM,
            mFormatFlags: kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked,
            mBytesPerPacket: 4, mFramesPerPacket: 1, mBytesPerFrame: 4, mChannelsPerFrame: 2,
            mBitsPerChannel: 16, mReserved: 0)
        var format: CMAudioFormatDescription?
        expectEqual(CMAudioFormatDescriptionCreate(allocator: kCFAllocatorDefault, asbd: &description,
            layoutSize: 0, layout: nil, magicCookieSize: 0, magicCookie: nil, extensions: nil,
            formatDescriptionOut: &format), noErr)
        var block: CMBlockBuffer?
        expectEqual(CMBlockBufferCreateWithMemoryBlock(allocator: kCFAllocatorDefault, memoryBlock: nil,
            blockLength: count * 4, blockAllocator: kCFAllocatorDefault, customBlockSource: nil,
            offsetToData: 0, dataLength: count * 4, flags: 0, blockBufferOut: &block), noErr)
        let data = try unwrap(block)
        expectEqual(CMBlockBufferFillDataBytes(with: 0, blockBuffer: data, offsetIntoDestination: 0,
                                                dataLength: count * 4), noErr)
        var timing = CMSampleTimingInfo(duration: CMTime(value: 1, timescale: 48000),
                                       presentationTimeStamp: time(seconds), decodeTimeStamp: .invalid)
        var size = 4
        var sample: CMSampleBuffer?
        expectEqual(CMSampleBufferCreateReady(allocator: kCFAllocatorDefault, dataBuffer: data,
            formatDescription: try unwrap(format), sampleCount: count, sampleTimingEntryCount: 1,
            sampleTimingArray: &timing, sampleSizeEntryCount: 1, sampleSizeArray: &size,
            sampleBufferOut: &sample), noErr)
        return try unwrap(sample)
    }

    private func destination() throws -> URL {
        let dir = FileManager.default.temporaryDirectory.appendingPathComponent("ScreenEncoder-test-\(UUID())")
        try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        addTeardownBlock { try? FileManager.default.removeItem(at: dir) }
        return dir.appendingPathComponent("recording.mp4")
    }

    private func appendVideo(_ writer: MovieWriter, at seconds: Double) throws {
        let sample = try video(at: seconds)
        let before = writer.frameCount
        // Allow the asynchronous hardware encoder to become ready without dropping test frames.
        for _ in 0..<200 {
            try writer.append(sample, isVideo: true)
            if writer.frameCount > before { return }
            Thread.sleep(forTimeInterval: 0.005)
        }
        fail("Encoder did not accept a video frame")
    }

    private func finish(_ writer: MovieWriter, at seconds: Double) async throws -> URL {
        try await withCheckedThrowingContinuation { continuation in
            writer.finish(at: time(seconds)) { continuation.resume(with: $0) }
        }
    }

    func testEncodeDecodeWithAudioAndPause() async throws {
        let url = try destination()
        let writer = try MovieWriter(url: url, settings: RecordingSettings(width: 320, height: 180, audio: true))
        // Audio before the first video must not create a leading gap or start a session.
        try writer.append(audio(at: 9), isVideo: false)
        for index in 0..<10 {
            let timestamp = 10 + Double(index) * 0.04
            try appendVideo(writer, at: timestamp)
            try writer.append(audio(at: timestamp), isVideo: false)
        }
        writer.setPaused(true, at: time(10.4))
        try writer.append(video(at: 11), isVideo: true)
        try writer.append(audio(at: 11), isVideo: false)
        expectEqual(writer.frameCount, 10)
        writer.setPaused(false, at: time(13.4))
        for index in 0..<15 {
            let timestamp = 13.4 + Double(index) * 0.04
            try appendVideo(writer, at: timestamp)
            try writer.append(audio(at: timestamp), isVideo: false)
        }
        _ = try await finish(writer, at: 14)
        let asset = AVURLAsset(url: url)
        let duration = try await asset.load(.duration)
        expectEqual(duration.seconds, 1, accuracy: 0.08)
        let videoTracks = try await asset.loadTracks(withMediaType: .video)
        let audioTracks = try await asset.loadTracks(withMediaType: .audio)
        expectEqual(videoTracks.count, 1)
        expectEqual(audioTracks.count, 1)
        let track = try unwrap(videoTracks.first)
        let dimensions = try await track.load(.naturalSize)
        expectEqual(dimensions, CGSize(width: 320, height: 180))
        let descriptions = try await track.load(.formatDescriptions)
        expectEqual(CMFormatDescriptionGetMediaSubType(try unwrap(descriptions.first)), kCMVideoCodecType_H264)
        let reader = try AVAssetReader(asset: asset)
        let output = AVAssetReaderTrackOutput(track: track, outputSettings: [kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA])
        reader.add(output)
        expectTrue(reader.startReading())
        var timestamps: [Double] = []
        while let sample = output.copyNextSampleBuffer() {
            expectNotNil(sample.imageBuffer)
            timestamps.append(sample.presentationTimeStamp.seconds)
        }
        expectEqual(reader.status, .completed)
        expectAtLeast(timestamps.count, 25)
        for (a, b) in zip(timestamps, timestamps.dropFirst()) {
            expectGreater(b, a)
            expectLess(b - a, 0.09, "Pause must not leave a gap in the video")
        }
        let audioReader = try AVAssetReader(asset: asset)
        let audioOutput = AVAssetReaderTrackOutput(track: try unwrap(audioTracks.first),
                                                 outputSettings: [AVFormatIDKey: kAudioFormatLinearPCM])
        audioReader.add(audioOutput)
        expectTrue(audioReader.startReading())
        var audioSamples = 0
        while let sample = audioOutput.copyNextSampleBuffer() { audioSamples += sample.numSamples }
        expectEqual(audioReader.status, .completed)
        expectGreater(audioSamples, 30_000)
    }

    func testStaticScreenAndStopWhilePaused() async throws {
        let writer = try MovieWriter(url: destination(), settings: RecordingSettings(width: 320, height: 180))
        try appendVideo(writer, at: 10)
        writer.setPaused(true, at: time(12))
        let url = try await finish(writer, at: 18)
        let asset = AVURLAsset(url: url)
        let duration = try await asset.load(.duration)
        expectEqual(duration.seconds, 2, accuracy: 0.05)
        let tracks = try await asset.loadTracks(withMediaType: .audio)
        expectTrue(tracks.isEmpty)
    }

    func testEmptyRecordingIsRemoved() async throws {
        let url = try destination()
        let writer = try MovieWriter(url: url, settings: RecordingSettings(width: 320, height: 180))
        do { _ = try await finish(writer, at: 12); fail("Empty recording should fail") }
        catch { expectFalse(FileManager.default.fileExists(atPath: url.path)) }
    }

    func testExistingFileIsPreserved() throws {
        let url = try destination()
        let original = Data("existing recording".utf8)
        try original.write(to: url)
        expectThrows(try MovieWriter(url: url, settings: RecordingSettings(width: 320, height: 180)))
        expectEqual(try Data(contentsOf: url), original)
    }

    func testSettingsAndRetinaSizing() throws {
        expectThrows(try RecordingSettings(width: 321, height: 180).validate())
        expectThrows(try RecordingSettings(width: 320, height: 180, fps: 0).validate())
        expectThrows(try RecordingSettings(width: 320, height: 180, bitrate: -1).validate())
        let size = RecordingSettings.fittedSize(width: 6016, height: 3384, maxEdge: 4096)
        expectEqual(size.0, 4096)
        expectEqual(size.1 % 2, 0)
        expectEqual(Double(size.0) / Double(size.1), 6016 / 3384, accuracy: 0.002)
    }
}


private func fail(_ message: String, file: StaticString = #filePath, line: UInt = #line) -> Never {
    fatalError(message, file: file, line: line)
}
private func expectEqual<T: Equatable>(_ actual: T, _ expected: T) {
    if actual != expected { fail("Expected \(expected), got \(actual)") }
}
private func expectEqual(_ actual: Double, _ expected: Double, accuracy: Double) {
    if abs(actual - expected) > accuracy { fail("Expected \(expected) ± \(accuracy), got \(actual)") }
}
private func expectTrue(_ value: Bool) { if !value { fail("Expected true") } }
private func expectFalse(_ value: Bool) { expectTrue(!value) }
private func expectNotNil<T>(_ value: T?) { expectTrue(value != nil) }
private func expectAtLeast<T: Comparable>(_ actual: T, _ limit: T) { expectTrue(actual >= limit) }
private func expectGreater<T: Comparable>(_ actual: T, _ limit: T) { expectTrue(actual > limit) }
private func expectLess<T: Comparable>(_ actual: T, _ limit: T, _ message: String) {
    if actual >= limit { fail(message) }
}
private func expectThrows<T>(_ action: @autoclosure () throws -> T) {
    do { _ = try action(); fail("Expected an error") } catch { }
}
private func unwrap<T>(_ value: T?) throws -> T {
    guard let value else { throw RecordingError.message("Unexpected nil in media check") }
    return value
}

@main
struct CheckRunner {
    static func main() async throws {
        let checks = MovieWriterChecks()
        try checks.testSettingsAndRetinaSizing()
        print("PASS settings validation and Retina sizing")
        try checks.testExistingFileIsPreserved()
        print("PASS existing file preservation")
        try await checks.testEmptyRecordingIsRemoved()
        print("PASS empty recording cleanup")
        try await checks.testStaticScreenAndStopWhilePaused()
        print("PASS static screen duration and stop while paused")
        try await checks.testEncodeDecodeWithAudioAndPause()
        print("PASS H.264/AAC encoding, decoding, A/V tracks and pause timeline")
        print("All 5 checks passed.")
    }
}
