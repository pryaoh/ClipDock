#include "RecordingOptions.h"
#include <cstdlib>
#include <iostream>
#include <limits>

static void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main() {
    RecordingOptions options;
    require(!options.validate(), "default settings must be valid");
    options.fps = 0;
    require(options.validate().has_value(), "reject zero FPS");
    options.fps = 61;
    require(options.validate().has_value(), "reject excessive FPS");
    options.fps = 25;
    options.bitrateKbps = std::numeric_limits<int>::max();
    require(options.validate().has_value(), "reject overflow before kbps conversion");
    options.bitrateKbps = -1;
    require(options.validate().has_value(), "reject negative bitrate");
    options.bitrateKbps = 5000;
    require(options.fittedSize(1920, 1080) == std::pair{1920, 1080}, "preserve native 1080p");
    require(options.fittedSize(800, 600) == std::pair{800, 600}, "do not upscale");
    require(options.fittedSize(1080, 1920) == std::pair{1080, 1920}, "preserve portrait aspect");
    require(options.fittedSize(0, 1080) == std::pair{0, 0}, "reject missing capture dimensions");
    require(options.fittedSize(std::numeric_limits<double>::quiet_NaN(), 1080) == std::pair{0, 0}, "reject NaN");
    options.maxEdge = 4096;
    const auto [width, height] = options.fittedSize(6016, 3384);
    require(width == 4096 && height % 2 == 0, "cap Retina output and use even sizes");
    require(std::abs(double(width) / height - 6016.0 / 3384.0) < 0.002, "preserve Retina aspect ratio");
    for (int edge : {1280, 1920, 2560, 4096}) {
        options.maxEdge = edge;
        for (const auto size : {std::pair{5120, 1440}, std::pair{3024, 1964}, std::pair{1365, 767}}) {
            const auto fitted = options.fittedSize(size.first, size.second);
            require(fitted.first <= edge && fitted.second <= edge, "fit selected maximum");
            require(fitted.first % 2 == 0 && fitted.second % 2 == 0, "encoder dimensions must be even");
        }
    }
    std::cout << "PASS configuration validation, overflow protection, Retina/portrait/ultrawide sizing\n";
}
