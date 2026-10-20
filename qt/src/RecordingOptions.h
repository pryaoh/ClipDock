#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <utility>

struct RecordingOptions {
    int maxEdge = 1920;
    int fps = 25;
    int bitrateKbps = 5000;
    bool microphone = false;

    std::optional<std::string> validate() const {
        if (maxEdge < 16 || maxEdge > 4096)
            return "최대 해상도는 16~4096 픽셀이어야 합니다.";
        if (fps < 1 || fps > 60)
            return "FPS는 1~60이어야 합니다.";
        if (bitrateKbps < 100 || bitrateKbps > 100000)
            return "비트레이트는 100~100000 kbps이어야 합니다.";
        return std::nullopt;
    }

    // Fit Retina and ultrawide sources without upscaling; encoders need even dimensions.
    std::pair<int, int> fittedSize(double width, double height) const {
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0)
            return {0, 0};
        const auto edge = std::clamp(maxEdge, 16, 4096);
        const auto scale = std::min(1.0, edge / std::max(width, height));
        return {std::max(16, static_cast<int>(width * scale) / 2 * 2),
                std::max(16, static_cast<int>(height * scale) / 2 * 2)};
    }
};
