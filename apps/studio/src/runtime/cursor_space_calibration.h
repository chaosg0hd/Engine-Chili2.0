#pragma once

namespace studio_runtime
{
    struct CursorSpaceCalibrationInput
    {
        int windowScreenLeft = 0;
        int windowScreenTop = 0;
        int windowScreenRight = 0;
        int windowScreenBottom = 0;

        int clientWidth = 0;
        int clientHeight = 0;
        int clientOriginScreenX = 0;
        int clientOriginScreenY = 0;

        int viewportX = 0;
        int viewportY = 0;
        int viewportWidth = 0;
        int viewportHeight = 0;
        int virtualWidth = 0;
        int virtualHeight = 0;

        int osScreenX = 0;
        int osScreenY = 0;

        float hotspotX = 0.0f;
        float hotspotY = 0.0f;
    };

    struct CursorSpaceCalibrationResult
    {
        bool valid = false;

        int windowWidth = 0;
        int windowHeight = 0;
        int clientWidth = 0;
        int clientHeight = 0;

        int borderLeft = 0;
        int borderTop = 0;
        int borderRight = 0;
        int borderBottom = 0;

        int viewportInsetLeft = 0;
        int viewportInsetTop = 0;
        int viewportInsetRight = 0;
        int viewportInsetBottom = 0;

        int osClientX = 0;
        int osClientY = 0;
        int osViewportLocalX = 0;
        int osViewportLocalY = 0;
        float normalizedU = 0.0f;
        float normalizedV = 0.0f;

        int customCursorLocalX = 0;
        int customCursorLocalY = 0;
        int renderX = 0;
        int renderY = 0;
        int finalDrawClientX = 0;
        int finalDrawClientY = 0;
    };

    CursorSpaceCalibrationResult ComputeCursorSpaceCalibration(const CursorSpaceCalibrationInput& input);
}
