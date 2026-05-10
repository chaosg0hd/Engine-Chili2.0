#include "runtime/cursor_space_calibration.h"

#include <algorithm>
#include <cmath>

namespace studio_runtime
{
    CursorSpaceCalibrationResult ComputeCursorSpaceCalibration(const CursorSpaceCalibrationInput& input)
    {
        CursorSpaceCalibrationResult result;

        result.windowWidth = std::max(0, input.windowScreenRight - input.windowScreenLeft);
        result.windowHeight = std::max(0, input.windowScreenBottom - input.windowScreenTop);
        result.clientWidth = std::max(0, input.clientWidth);
        result.clientHeight = std::max(0, input.clientHeight);

        result.borderLeft = input.clientOriginScreenX - input.windowScreenLeft;
        result.borderTop = input.clientOriginScreenY - input.windowScreenTop;
        result.borderRight = input.windowScreenRight - (input.clientOriginScreenX + result.clientWidth);
        result.borderBottom = input.windowScreenBottom - (input.clientOriginScreenY + result.clientHeight);

        result.viewportInsetLeft = input.viewportX;
        result.viewportInsetTop = input.viewportY;
        result.viewportInsetRight = std::max(0, result.clientWidth - (input.viewportX + input.viewportWidth));
        result.viewportInsetBottom = std::max(0, result.clientHeight - (input.viewportY + input.viewportHeight));

        result.osClientX = input.osScreenX - input.clientOriginScreenX;
        result.osClientY = input.osScreenY - input.clientOriginScreenY;

        result.osViewportLocalX = result.osClientX - input.viewportX;
        result.osViewportLocalY = result.osClientY - input.viewportY;

        const int frameWidth = std::max(1, input.viewportWidth);
        const int frameHeight = std::max(1, input.viewportHeight);
        const int virtualWidth = std::max(1, input.virtualWidth > 0 ? input.virtualWidth : input.viewportWidth);
        const int virtualHeight = std::max(1, input.virtualHeight > 0 ? input.virtualHeight : input.viewportHeight);
        result.normalizedU = std::clamp(static_cast<float>(result.osViewportLocalX) / static_cast<float>(frameWidth), 0.0f, 1.0f);
        result.normalizedV = std::clamp(static_cast<float>(result.osViewportLocalY) / static_cast<float>(frameHeight), 0.0f, 1.0f);

        result.renderX = static_cast<int>(std::lround(result.normalizedU * static_cast<float>(virtualWidth)));
        result.renderY = static_cast<int>(std::lround(result.normalizedV * static_cast<float>(virtualHeight)));
        result.customCursorLocalX = result.renderX;
        result.customCursorLocalY = result.renderY;

        const int hotspotX = static_cast<int>(std::lround(input.hotspotX));
        const int hotspotY = static_cast<int>(std::lround(input.hotspotY));
        result.finalDrawClientX = input.viewportX + result.customCursorLocalX - hotspotX;
        result.finalDrawClientY = input.viewportY + result.customCursorLocalY - hotspotY;

        result.valid = (result.clientWidth > 0 && result.clientHeight > 0 && input.viewportWidth > 0 && input.viewportHeight > 0);
        return result;
    }
}
