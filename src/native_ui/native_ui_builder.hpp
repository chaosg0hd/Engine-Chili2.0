#pragma once

#include "native_ui_frame.hpp"
#include "prototypes/entity/appearance/color.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <utility>

class NativeUiBuilder
{
public:
    class OverlayBuilder
    {
    public:
        explicit OverlayBuilder(NativeUiBuilder& owner);

        OverlayBuilder& Line(const std::wstring& text);
        OverlayBuilder& Line(const wchar_t* text);

    private:
        NativeUiBuilder& m_owner;
    };

    class PanelBuilder
    {
    public:
        PanelBuilder(NativeUiBuilder& owner, std::size_t panelIndex);

        PanelBuilder& Visible(bool visible);
        PanelBuilder& Bounds(int x, int y, int width, int height);
        PanelBuilder& Anchor(NativeUiAnchor anchor, int offsetX, int offsetY, int width, int height);
        PanelBuilder& Colors(std::uint32_t textColor, std::uint32_t backgroundColor);
        PanelBuilder& Colors(const ColorPrototype& textColor, const ColorPrototype& backgroundColor);
        PanelBuilder& Row(const std::string& label, const std::string& value);
        PanelBuilder& Row(const std::string& label, const char* value);

        template<typename T>
        PanelBuilder& Row(const std::string& label, const T& value)
        {
            std::ostringstream stream;
            stream << value;
            return Row(label, stream.str());
        }

    private:
        NativeUiBuilder& m_owner;
        std::size_t m_panelIndex = 0U;
    };

public:
    NativeUiBuilder& WindowTitle(const std::wstring& title);
    NativeUiBuilder& WindowTitle(const wchar_t* title);
    NativeUiBuilder& OverlayEnabled(bool enabled);
    NativeUiBuilder& ClearColor(std::uint32_t color);
    NativeUiBuilder& ClearColor(const ColorPrototype& color);
    NativeUiBuilder& ContentFrame(const FramePrototype& frame);
    NativeUiBuilder& ContentFrame(FramePrototype&& frame);
    NativeUiBuilder& Text(const std::wstring& text, float x, float y, float scale = 1.0f, std::uint32_t color = 0xFFFFFFFFu);
    NativeUiBuilder& Text(const std::wstring& text, float x, float y, float scale, const ColorPrototype& color);
    NativeUiBuilder& Label(const std::string& name, const std::wstring& text, int x, int y, int width, int height, std::uint32_t color = 0xFFFFFFFFu);
    NativeUiBuilder& Label(const std::string& name, const std::wstring& text, int x, int y, int width, int height, const ColorPrototype& color);
    NativeUiBuilder& AnchoredLabel(const std::string& name, const std::wstring& text, NativeUiAnchor anchor, int offsetX, int offsetY, int width, int height, std::uint32_t color = 0xFFFFFFFFu);
    NativeUiBuilder& AnchoredLabel(const std::string& name, const std::wstring& text, NativeUiAnchor anchor, int offsetX, int offsetY, int width, int height, const ColorPrototype& color);
    NativeUiBuilder& Scoreboard(int leftScore, int rightScore, float centerX, float centerY, float scale = 1.0f, std::uint32_t color = 0xFFFFFFFFu);
    NativeUiBuilder& Scoreboard(int leftScore, int rightScore, float centerX, float centerY, float scale, const ColorPrototype& color);
    NativeUiBuilder& Rect(float centerX, float centerY, float halfWidth, float halfHeight, std::uint32_t color);
    NativeUiBuilder& Rect(float centerX, float centerY, float halfWidth, float halfHeight, const ColorPrototype& color);
    NativeUiBuilder& Hex(float centerX, float centerY, float halfWidth, float halfHeight, std::uint32_t color);
    NativeUiBuilder& Hex(float centerX, float centerY, float halfWidth, float halfHeight, const ColorPrototype& color);
    NativeUiBuilder& Line(float startX, float startY, float endX, float endY, float thickness, std::uint32_t color);
    NativeUiBuilder& Line(float startX, float startY, float endX, float endY, float thickness, const ColorPrototype& color);

    OverlayBuilder Overlay();
    PanelBuilder Panel(const std::string& title);

    NativeUiFrame Build() const;

private:
    NativeUiFrame m_frame;
};
