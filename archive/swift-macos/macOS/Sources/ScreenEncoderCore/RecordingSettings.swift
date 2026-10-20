import Foundation

public enum RecordingError: LocalizedError {
    case message(String)
    public var errorDescription: String? {
        switch self { case .message(let text): return text }
    }
}

public struct RecordingSettings {
    public var width: Int
    public var height: Int
    public var fps: Int
    public var bitrate: Int
    public var keyFrameInterval: Int
    public var audio: Bool

    public init(width: Int, height: Int, fps: Int = 25, bitrate: Int = 5_000_000,
                keyFrameInterval: Int = 50, audio: Bool = false) {
        self.width = width
        self.height = height
        self.fps = fps
        self.bitrate = bitrate
        self.keyFrameInterval = keyFrameInterval
        self.audio = audio
    }

    public func validate() throws {
        guard (16...4096).contains(width), (16...4096).contains(height),
              width % 2 == 0, height % 2 == 0 else {
            throw RecordingError.message("가로·세로는 16~4096 범위의 짝수여야 합니다.")
        }
        guard (1...60).contains(fps), (100_000...100_000_000).contains(bitrate),
              (1...600).contains(keyFrameInterval) else {
            throw RecordingError.message("FPS는 1~60, 비트레이트는 100~100000 kbps, 키 프레임 간격은 1~600이어야 합니다.")
        }
    }

    /// Preserve aspect ratio and fit Retina / ultrawide sources within H.264 limits.
    public static func fittedSize(width: Double, height: Double, maxEdge: Int) -> (Int, Int) {
        guard width.isFinite, height.isFinite, width > 0, height > 0 else { return (1920, 1080) }
        let scale = min(1, Double(min(4096, max(16, maxEdge))) / max(width, height))
        return (max(16, Int(width * scale) / 2 * 2), max(16, Int(height * scale) / 2 * 2))
    }
}
