#pragma once

#include "../core/runtime_types.hpp"
#include "../modules/sound/isound_service.hpp"
#include "../modules/resources/resource_types.hpp"
#include "../modules/render/render_types.hpp"
#include "../native_ui/native_ui_frame.hpp"
#include "../prototypes/entity/appearance/color.hpp"
#include "../prototypes/presentation/frame.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <windef.h>

enum class AppKey : unsigned char
{
    Space = 0x20,
    A = 'A',
    L = 'L',
    R = 'R',
    S = 'S',
    W = 'W',
    Left = 0x25,
    Up = 0x26,
    Right = 0x27,
    Down = 0x28
};

enum class WebDialogDockMode
{
    Floating,
    Left,
    Right,
    Top,
    Bottom,
    Fill,
    ManualChild
};

struct WebDialogRect
{
    int x = 80;
    int y = 80;
    int width = 640;
    int height = 480;
};

struct WebDialogDesc
{
    std::string name;
    std::wstring title;
    std::string contentPath;
    WebDialogDockMode dockMode = WebDialogDockMode::Floating;
    WebDialogRect rect{};
    int dockSize = 360;
    int dockInsetTop = 0;
    int dockInsetLeft = 0;
    int dockInsetRight = 0;
    bool visible = true;
    bool resizable = true;
    bool alwaysOnTop = false;
};

struct NativeControlRect
{
    int x = 0;
    int y = 0;
    int width = 96;
    int height = 36;
};

struct NativeButtonDesc
{
    std::string name;
    std::wstring text;
    NativeControlRect rect{};
    bool visible = true;
    bool enabled = true;
};

struct NativeLabelDesc
{
    std::string name;
    std::wstring text;
    NativeControlRect rect{};
    bool visible = true;
    std::uint32_t textColor = 0xFFFFFFFFu;
    std::uint32_t backgroundColor = 0x00000000u;
    bool transparentBackground = true;
    bool multiline = false;
    bool alignLeft = false;
};

class IAppLogging
{
public:
    virtual ~IAppLogging() = default;

    virtual void Info(const std::string& message) = 0;
    virtual void Warn(const std::string& message) = 0;
    virtual void Error(const std::string& message) = 0;
    virtual std::string GetLogFilePath() const = 0;
    virtual bool IsFileLoggingAvailable() const = 0;
};

class IAppResources
{
public:
    virtual ~IAppResources() = default;

    virtual bool FileExists(const std::string& path) const = 0;
    virtual bool DirectoryExists(const std::string& path) const = 0;
    virtual bool CreateDirectory(const std::string& path) = 0;
    virtual bool ReadTextFile(const std::string& path, std::string& outContent) const = 0;
    virtual bool ReadBinaryFile(const std::string& path, std::vector<std::byte>& outContent) const = 0;
    virtual std::string GetAbsolutePath(const std::string& path) const = 0;
    virtual ResourceHandle RequestResource(const std::string& assetId, ResourceKind kind) = 0;
    virtual ResourceState GetResourceState(ResourceHandle handle) const = 0;
    virtual std::size_t GetResourceUploadedByteSize(ResourceHandle handle) const = 0;
};

class IAppRendering
{
public:
    virtual ~IAppRendering() = default;

    virtual void ClearFrame(std::uint32_t color) = 0;
    void ClearFrame(const ColorPrototype& color) { ClearFrame(color.ToArgb()); }
    virtual void SubmitFrame(const FramePrototype& frame) = 0;
    virtual int GetFrameWidth() const = 0;
    virtual int GetFrameHeight() const = 0;
    virtual double GetFrameAspectRatio() const = 0;
    virtual std::size_t GetSubmittedItemCount() const = 0;
    virtual void SetDerivedBounceFillSettings(const DerivedBounceFillSettings& settings) = 0;
    virtual DerivedBounceFillSettings GetDerivedBounceFillSettings() const = 0;
    virtual void SetTracedIndirectSettings(const TracedIndirectSettings& settings) = 0;
    virtual TracedIndirectSettings GetTracedIndirectSettings() const = 0;
};

class IAppJobs
{
public:
    virtual ~IAppJobs() = default;

    virtual bool Submit(std::function<void()> job) = 0;
    virtual void WaitForAll() = 0;
    virtual unsigned int GetWorkerCount() const = 0;
    virtual unsigned int GetIdleWorkerCount() const = 0;
};

class IAppInput
{
public:
    virtual ~IAppInput() = default;

    virtual bool IsKeyDown(AppKey key) const = 0;
    virtual bool WasKeyPressed(AppKey key) const = 0;
    virtual bool WasKeyReleased(AppKey key) const = 0;
};

class IAppTime
{
public:
    virtual ~IAppTime() = default;

    virtual double GetDeltaSeconds() const = 0;
    virtual double GetTotalSeconds() const = 0;
};

class IAppWindow
{
public:
    virtual ~IAppWindow() = default;

    virtual std::wstring BuildDebugViewText() const = 0;
    virtual void SetOverlayText(const std::wstring& text) = 0;
    virtual void SetOverlayEnabled(bool enabled) = 0;
    virtual void SetWindowTitle(const std::wstring& title) = 0;
    virtual HWND GetWindowHandle() const = 0;
    virtual int GetWindowWidth() const = 0;
    virtual int GetWindowHeight() const = 0;
    virtual double GetDeltaTime() const = 0;
    virtual double GetTotalTime() const = 0;
    virtual bool IsKeyDown(unsigned char key) const = 0;
    virtual bool WasKeyPressed(unsigned char key) const = 0;
    virtual bool WasKeyReleased(unsigned char key) const = 0;
};

class IAppUi
{
public:
    using WebDialogHandle = std::uint32_t;
    using NativeButtonHandle = std::uint32_t;
    using NativeLabelHandle = std::uint32_t;

public:
    virtual ~IAppUi() = default;

    virtual WebDialogHandle CreateWebDialog(const WebDialogDesc& desc) = 0;
    virtual bool DestroyWebDialog(WebDialogHandle handle) = 0;
    virtual bool SetWebDialogBounds(WebDialogHandle handle, const WebDialogRect& rect) = 0;
    virtual bool SetWebDialogVisible(WebDialogHandle handle, bool visible) = 0;
    virtual NativeButtonHandle CreateNativeButton(const NativeButtonDesc& desc) = 0;
    virtual bool DestroyNativeButton(NativeButtonHandle handle) = 0;
    virtual bool SetNativeButtonBounds(NativeButtonHandle handle, const NativeControlRect& rect) = 0;
    virtual bool ConsumeNativeButtonPressed(NativeButtonHandle handle) = 0;
    virtual NativeLabelHandle CreateNativeLabel(const NativeLabelDesc& desc) = 0;
    virtual bool DestroyNativeLabel(NativeLabelHandle handle) = 0;
    virtual bool SetNativeLabelText(NativeLabelHandle handle, const std::wstring& text) = 0;
    virtual bool SetNativeLabelBounds(NativeLabelHandle handle, const NativeControlRect& rect) = 0;
    virtual bool SetNativeLabelVisible(NativeLabelHandle handle, bool visible) = 0;
};

class IAppNativeUi
{
public:
    virtual ~IAppNativeUi() = default;

    virtual void Submit(const NativeUiFrame& frame) = 0;
};

class IAppSound
{
public:
    virtual ~IAppSound() = default;

    virtual bool IsAvailable() const = 0;
    virtual bool RegisterSound(const SoundClipDesc& desc) = 0;
    virtual void SetMasterVolume(float volume) = 0;
    virtual float GetMasterVolume() const = 0;
    virtual void SetMuted(bool muted) = 0;
    virtual bool IsMuted() const = 0;
    virtual void StopAll() = 0;
    virtual bool PlayOneShot(const std::string& soundId) = 0;
    virtual bool PlayLoop(const std::string& soundId) = 0;
    virtual void StopSound(const std::string& soundId) = 0;
};

class IAppRuntime
{
public:
    virtual ~IAppRuntime() = default;

    virtual PressureState GetPressureState() const = 0;
    virtual PressureState GetLogicPressureState() const = 0;
    virtual PressureState GetPresentationPressureState() const = 0;
    virtual bool IsDegradedMode() const = 0;
    virtual bool IsEmergencyMode() const = 0;
    virtual int GetLogicThrottleLevel() const = 0;
    virtual int GetPresentationThrottleLevel() const = 0;
    virtual std::uint64_t GetPublishedPresentationSnapshotVersion() const = 0;
};

struct AppCapabilities
{
    IAppLogging* logging = nullptr;
    IAppResources* resources = nullptr;
    IAppRendering* rendering = nullptr;
    IAppInput* input = nullptr;
    IAppTime* time = nullptr;
    IAppJobs* jobs = nullptr;
    IAppWindow* window = nullptr;
    IAppUi* ui = nullptr;
    IAppNativeUi* nativeUi = nullptr;
    IAppSound* sound = nullptr;
    IAppRuntime* runtime = nullptr;
};
