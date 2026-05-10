#include "studio_host.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shobjidl.h>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#include "native_ui/native_ui_builder.hpp"
#include "input/input_key.h"
#include "input/input_mouse.h"
#include "runtime/cursor_space_calibration.h"
#include "runtime/scene_serializer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr int kStudioTopToolbarHeight = 82;
    constexpr int kStudioLeftSidebarWidth = 96;
    constexpr int kStudioRightInspectorWidth = 300;
    constexpr int kStudioBottomConsoleHeight = 148;
    constexpr unsigned short kStudioHttpPort = 37620;
    constexpr UINT_PTR kProjectExplorerMenuOpen = 1001U;
    constexpr UINT_PTR kProjectExplorerMenuRename = 1002U;
    constexpr UINT_PTR kProjectExplorerMenuDuplicate = 1003U;
    constexpr UINT_PTR kProjectExplorerMenuDelete = 1004U;
    constexpr UINT_PTR kProjectMenuNewProject = 2001U;
    constexpr UINT_PTR kProjectMenuOpenProject = 2002U;
    constexpr UINT_PTR kProjectMenuSaveProject = 2003U;
    constexpr UINT_PTR kProjectMenuBuildApp = 2004U;
    constexpr UINT_PTR kProjectMenuPreviewApp = 2005U;
    constexpr UINT_PTR kBuildMenuBuildApp = 3001U;
    constexpr UINT_PTR kBuildMenuPreviewApp = 3002U;

    std::string EscapeJson(const std::string& value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (const char ch : value)
        {
            if (ch == '\\' || ch == '"')
            {
                escaped.push_back('\\');
            }

            escaped.push_back(ch);
        }

        return escaped;
    }

    std::string BuildJsonResponse(bool ok, const std::string& message, const std::string& projectId = std::string(), const std::string& logicalPath = std::string())
    {
        return std::string("{\"ok\":") +
            (ok ? "true" : "false") +
            ",\"message\":\"" +
            EscapeJson(message) +
            "\",\"projectId\":\"" +
            EscapeJson(projectId) +
            "\",\"logicalPath\":\"" +
            EscapeJson(logicalPath) +
            "\"}";
    }

    std::string BuildRenameJsonResponse(
        bool ok,
        const std::string& message,
        const std::string& oldLogicalPath,
        const std::string& newLogicalPath)
    {
        return std::string("{\"ok\":") +
            (ok ? "true" : "false") +
            ",\"message\":\"" +
            EscapeJson(message) +
            "\",\"oldLogicalPath\":\"" +
            EscapeJson(oldLogicalPath) +
            "\",\"newLogicalPath\":\"" +
            EscapeJson(newLogicalPath) +
            "\",\"logicalPath\":\"" +
            EscapeJson(newLogicalPath) +
            "\"}";
    }

    std::string BuildMenuJsonResponse(const std::string& action)
    {
        return std::string("{\"ok\":true,\"action\":\"") + EscapeJson(action) + "\"}";
    }

    std::string BuildCreateProjectJsonResponse(const studio::CreateProjectResult& result)
    {
        const std::string message = result.success ? result.message : result.error;
        return std::string("{\"ok\":") +
            (result.success ? "true" : "false") +
            ",\"message\":\"" +
            EscapeJson(message) +
            "\",\"projectId\":\"" +
            EscapeJson(result.projectId) +
            "\",\"logicalPath\":\"" +
            EscapeJson(result.logicalProjectPath) +
            "\",\"generatedFiles\":[" +
            "\"project.enginegame\"," +
            "\"project.chili.json\"," +
            "\"src/" + EscapeJson(result.projectId) + ".hpp\"," +
            "\"src/" + EscapeJson(result.projectId) + ".cpp\"," +
            "\"scenes/main.scene\"," +
            "\"config/game.config\"" +
            "]}";
    }

    std::string BuildCreateSceneTemplate(const std::string& sceneName)
    {
        std::string text;
        text += "name = " + sceneName + "\n";
        text += "viewport_message = Scene " + sceneName + " ready.\n";
        text += "line_color = #FFF2E4\n";
        text += "left_paddle_color = #F4B183\n";
        text += "right_paddle_color = #D27D48\n";
        text += "ball_color = #FFE3C8\n";
        text += "serve_color = #FFC780\n";
        return text;
    }

    std::string NormalizeSceneName(const std::string& value)
    {
        std::string out;
        out.reserve(value.size());
        for (const char ch : value)
        {
            const unsigned char byte = static_cast<unsigned char>(ch);
            if ((byte >= 'a' && byte <= 'z') ||
                (byte >= 'A' && byte <= 'Z') ||
                (byte >= '0' && byte <= '9') ||
                ch == '_' ||
                ch == '-')
            {
                out.push_back(ch);
            }
            else if (ch == ' ')
            {
                out.push_back('_');
            }
        }

        while (!out.empty() && out.back() == '.')
        {
            out.pop_back();
        }

        return out;
    }

    std::string BuildConsoleJsonResponse(bool ok, const std::string& output, bool clear = false)
    {
        return std::string("{\"ok\":") +
            (ok ? "true" : "false") +
            ",\"output\":\"" +
            EscapeJson(output) +
            "\",\"clear\":" +
            (clear ? "true" : "false") +
            "}";
    }

    std::string BuildRuntimeActionJsonResponse(
        bool ok,
        const std::string& message,
        const std::string& state,
        const std::string& viewportText)
    {
        return std::string("{\"ok\":") +
            (ok ? "true" : "false") +
            ",\"message\":\"" +
            EscapeJson(message) +
            "\",\"state\":\"" +
            EscapeJson(state) +
            "\",\"viewportText\":\"" +
            EscapeJson(viewportText) +
            "\"}";
    }

    std::string ExtractJsonStringField(const std::string& text, const std::string& fieldName)
    {
        const std::string key = "\"" + fieldName + "\"";
        const std::size_t keyPos = text.find(key);
        if (keyPos == std::string::npos)
        {
            return std::string();
        }

        const std::size_t colonPos = text.find(':', keyPos + key.size());
        if (colonPos == std::string::npos)
        {
            return std::string();
        }

        const std::size_t firstQuote = text.find('"', colonPos + 1U);
        if (firstQuote == std::string::npos)
        {
            return std::string();
        }

        const std::size_t secondQuote = text.find('"', firstQuote + 1U);
        if (secondQuote == std::string::npos || secondQuote <= firstQuote)
        {
            return std::string();
        }

        return text.substr(firstQuote + 1U, secondQuote - firstQuote - 1U);
    }

    bool IsTruthyQueryValue(const std::unordered_map<std::string, std::string>& query, const std::string& key)
    {
        const auto value = query.find(key);
        if (value == query.end())
        {
            return false;
        }

        return value->second == "true" || value->second == "1" || value->second == "yes";
    }

    bool HasProjectManifest(const studio::FileProxy& files, const std::string& projectId)
    {
        return !projectId.empty() && files.Exists(studio::StudioProjectSystem::GetProjectSourcePath(projectId) + "/project.enginegame");
    }

    std::wstring ToWideString(const std::string& value)
    {
        return std::wstring(value.begin(), value.end());
    }

    std::string ToNarrowString(const std::wstring& value)
    {
        return std::string(value.begin(), value.end());
    }

    studio_runtime::ProjectRuntimeDesc MakeRuntimeDesc(const studio::StudioProject& project)
    {
        studio_runtime::ProjectRuntimeDesc desc;
        desc.projectId = project.projectId;
        desc.projectName = project.projectName;
        desc.projectRootPath = project.projectRootPath;
        desc.previewRuntimeName = project.previewRuntimeName;
        desc.defaultScenePath = project.defaultScenePath;
        desc.sourceEntryPath = project.sourceEntryPath;
        return desc;
    }

    std::string NormalizeNativePath(std::filesystem::path path)
    {
        std::error_code error;
        path = std::filesystem::absolute(path, error);
        if (error)
        {
            return std::string();
        }

        path = path.lexically_normal();
        std::string normalized = path.string();
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        return normalized;
    }

    bool TryExtractProjectIdFromFolder(const std::string& selectedFolder, std::string& outProjectId, std::string& outError)
    {
        studio::FileProxy files;
        const std::string userRoot = NormalizeNativePath(files.Resolve(std::string()));
        const std::string selected = NormalizeNativePath(selectedFolder);
        if (userRoot.empty() || selected.empty())
        {
            outError = "Could not resolve the selected project folder.";
            return false;
        }

        if (selected != userRoot &&
            (selected.size() <= userRoot.size() || selected.find(userRoot + "/") != 0U))
        {
            outError = "Selected folder is outside the User workspace.";
            return false;
        }

        std::string logicalPath = selected == userRoot ? std::string() : selected.substr(userRoot.size() + 1U);
        logicalPath = files.NormalizeVirtualPath(logicalPath);
        if (logicalPath.empty())
        {
            outError = "Select a project folder inside User.";
            return false;
        }

        const std::string projectSuffix = "/Project";
        if (logicalPath.size() > projectSuffix.size() &&
            logicalPath.compare(logicalPath.size() - projectSuffix.size(), projectSuffix.size(), projectSuffix) == 0)
        {
            logicalPath.resize(logicalPath.size() - projectSuffix.size());
        }

        if (logicalPath.find('/') != std::string::npos || logicalPath.find('\\') != std::string::npos)
        {
            outError = "Select User/<project_id> or User/<project_id>/Project.";
            return false;
        }

        if (!HasProjectManifest(files, logicalPath))
        {
            outError = "Selected folder does not contain a Studio-readable project manifest.";
            return false;
        }

        outProjectId = logicalPath;
        return true;
    }

    int HexValue(char ch)
    {
        if (ch >= '0' && ch <= '9')
        {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f')
        {
            return 10 + (ch - 'a');
        }
        if (ch >= 'A' && ch <= 'F')
        {
            return 10 + (ch - 'A');
        }

        return -1;
    }

    std::string UrlDecode(const std::string& value)
    {
        std::string decoded;
        decoded.reserve(value.size());

        for (std::size_t index = 0; index < value.size(); ++index)
        {
            if (value[index] == '+' )
            {
                decoded.push_back(' ');
                continue;
            }

            if (value[index] == '%' && index + 2U < value.size())
            {
                const int high = HexValue(value[index + 1U]);
                const int low = HexValue(value[index + 2U]);
                if (high >= 0 && low >= 0)
                {
                    decoded.push_back(static_cast<char>((high << 4) | low));
                    index += 2U;
                    continue;
                }
            }

            decoded.push_back(value[index]);
        }

        return decoded;
    }

    std::unordered_map<std::string, std::string> ParseQuery(const std::string& path)
    {
        std::unordered_map<std::string, std::string> values;
        const std::size_t queryStart = path.find('?');
        if (queryStart == std::string::npos)
        {
            return values;
        }

        std::size_t cursor = queryStart + 1U;
        while (cursor < path.size())
        {
            const std::size_t separator = path.find('&', cursor);
            const std::string pair = path.substr(cursor, separator == std::string::npos ? std::string::npos : separator - cursor);
            const std::size_t equals = pair.find('=');
            if (equals != std::string::npos)
            {
                values[UrlDecode(pair.substr(0, equals))] = UrlDecode(pair.substr(equals + 1U));
            }

            if (separator == std::string::npos)
            {
                break;
            }

            cursor = separator + 1U;
        }

        return values;
    }

    std::string StripQuery(const std::string& path)
    {
        const std::size_t queryStart = path.find('?');
        return queryStart == std::string::npos ? path : path.substr(0, queryStart);
    }

    bool EndsWithPath(const std::string& value, const std::string& suffix)
    {
        return value.size() >= suffix.size() &&
            value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool EndsWithInsensitive(std::string value, std::string suffix)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        std::transform(
            suffix.begin(),
            suffix.end(),
            suffix.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return value.size() >= suffix.size() &&
            value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    const char* MeshName(BuiltInMeshKind mesh)
    {
        switch (mesh)
        {
        case BuiltInMeshKind::Cube: return "builtin:cube";
        case BuiltInMeshKind::Quad: return "builtin:quad";
        case BuiltInMeshKind::Octahedron: return "builtin:octahedron";
        case BuiltInMeshKind::Diamond: return "builtin:diamond";
        case BuiltInMeshKind::Triangle: return "builtin:triangle";
        case BuiltInMeshKind::None:
        default:
            return "builtin:none";
        }
    }

    std::string BuildWorldSignature(const studio_runtime::RuntimeWorld& world)
    {
        std::ostringstream signature;
        const std::vector<studio_runtime::EntityId> ids = world.GetEntityList();
        signature << "count=" << ids.size();
        for (const studio_runtime::EntityId id : ids)
        {
            studio_runtime::EntityInfo info;
            if (!world.GetEntityInfo(id, info))
            {
                continue;
            }

            signature
                << "|id=" << info.id
                << ",name=" << info.name
                << ",proto=" << (info.hasObject ? info.object.prototypeId : "")
                << ",r=" << (info.hasRenderable ? 1 : 0)
                << ",l=" << (info.hasLight ? 1 : 0)
                << ",px=" << info.transform.translation.x
                << ",py=" << info.transform.translation.y
                << ",pz=" << info.transform.translation.z;
        }
        return signature.str();
    }

}

std::string ShowBuildContextMenu(HWND owner, int screenX, int screenY);

bool StudioHost::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    m_layoutState.SetChromeSizes(
        kStudioTopToolbarHeight,
        kStudioLeftSidebarWidth,
        kStudioRightInspectorWidth,
        kStudioBottomConsoleHeight);

    if (!m_bridge.Initialize())
    {
        return false;
    }

    const HWND windowHandle = m_bridge.GetNativeWindowHandle();
    if (!windowHandle)
    {
        m_bridge.LogError("Studio: native host window is not available.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeStudioHttpBridge())
    {
        m_bridge.LogWarn("Studio: HTTP bridge is unavailable; HTML Studio actions will be display-only.");
    }

    if (!InitializeTopBarDialog())
    {
        m_bridge.LogError("Studio: failed to initialize the engine-owned Studio top bar.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeCoreToolsDialog())
    {
        m_bridge.LogError("Studio: failed to initialize the engine-owned CoreTools dialog.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeProjectExplorerPanel())
    {
        m_bridge.LogError("Studio: failed to initialize the project explorer panel.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeConsolePanel())
    {
        m_bridge.LogError("Studio: failed to initialize the console panel.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeFrameGizmoButton())
    {
        m_bridge.LogError("Studio: failed to initialize frame gizmo button.");
        m_bridge.Shutdown();
        return false;
    }

    m_commandRouter.Bind(&m_bridge);
    m_consoleFeed.clear();
    LoadPersistedConsoleLog();
    PushConsoleMessage("Studio console feed online.");
    std::string sceneError;
    m_runtimeHost.Initialize(GetDefaultScenePath(), sceneError);
    RefreshStudioAssetLibrary();
    if (!sceneError.empty())
    {
        PushConsoleMessage("Default scene load warning: " + sceneError);
    }
    LogStudioShellStatus();
    m_initialized = true;
    return true;
}

void StudioHost::Run()
{
    if (!m_initialized)
    {
        return;
    }

    m_bridge.LogInfo("Studio: native host entering main loop.");
    m_bridge.LogInfo("Studio: close the native window or press Escape to stop the studio host.");

    while (!m_bridge.ShouldExit())
    {
        if (!m_bridge.Tick())
        {
            break;
        }

        TickRuntime();
        m_httpServer.Tick(m_bridge);
    }
}

void StudioHost::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    if (m_topBarDialogHandle != 0U)
    {
        m_bridge.GetCapabilities().ui->DestroyWebDialog(m_topBarDialogHandle);
        m_topBarDialogHandle = 0U;
    }

    if (m_coreToolsDialogHandle != 0U)
    {
        m_bridge.GetCapabilities().ui->DestroyWebDialog(m_coreToolsDialogHandle);
        m_coreToolsDialogHandle = 0U;
    }

    if (m_frameGizmoButtonHandle != 0U)
    {
        m_bridge.GetCapabilities().ui->DestroyNativeButton(m_frameGizmoButtonHandle);
        m_frameGizmoButtonHandle = 0U;
    }

    m_projectExplorerPanel.Close(m_bridge.GetCapabilities());
    m_consolePanel.Close(m_bridge.GetCapabilities());
    m_newProjectDialog.Close(m_bridge.GetCapabilities());
    m_fileManagementDialog.Close(m_bridge.GetCapabilities());
    m_httpServer.Stop(m_bridge);

    if (m_bridge.GetCapabilities().window)
    {
        m_bridge.GetCapabilities().window->SetCursorVisible(true);
    }

    m_bridge.Shutdown();
    m_initialized = false;
}

void StudioHost::LogStudioShellStatus()
{
    m_bridge.LogInfo("Studio: shell boot complete.");
    m_bridge.LogInfo("Studio: native outer window is active.");
    m_bridge.LogInfo(
        std::string("Studio: CoreTools entry = ") +
        m_bridge.GetCoreToolsContentPath());
    m_bridge.LogInfo(
        std::string("Studio: top bar entry = ") +
        m_bridge.GetStudioTopBarContentPath());
    m_bridge.LogInfo("Studio: shell chrome = docked-top and docked-left via engine web dialog API.");
    m_bridge.LogInfo("Studio: File actions are driven by HTML dialogs through the Studio HTTP bridge.");
}

bool StudioHost::InitializeStudioHttpBridge()
{
    HttpServerConfig config;
    config.host = "127.0.0.1";
    config.port = kStudioHttpPort;
    config.webRootPath = GetCoreToolsRuntimeRootPath();
    config.indexFilePath = "top-bar/top-bar.html";

    m_httpServer.SetRequestHandler(
        [this](const std::string& path, std::string& outContentType, std::string& outBody)
        {
            return HandleStudioHttpRequest(path, outContentType, outBody);
        });

    return m_httpServer.Start(config, m_bridge);
}

bool StudioHost::InitializeTopBarDialog()
{
    WebDialogDesc dialogDesc;
    dialogDesc.name = "StudioTopBar";
    dialogDesc.title = L"Studio Top Bar";
    dialogDesc.contentPath = m_bridge.GetStudioTopBarContentPath();
    dialogDesc.dockMode = WebDialogDockMode::Top;
    dialogDesc.dockSize = m_layoutState.GetTopToolbarHeight();
    dialogDesc.visible = true;
    dialogDesc.resizable = false;

    m_topBarDialogHandle = m_bridge.GetCapabilities().ui->CreateWebDialog(dialogDesc);
    return m_topBarDialogHandle != 0U;
}

bool StudioHost::InitializeCoreToolsDialog()
{
    WebDialogDesc dialogDesc;
    dialogDesc.name = "CoreTools";
    dialogDesc.title = L"CoreTools";
    dialogDesc.contentPath = m_bridge.GetCoreToolsContentPath();
    dialogDesc.dockMode = WebDialogDockMode::Left;
    dialogDesc.dockSize = m_layoutState.GetLeftSidebarWidth();
    dialogDesc.dockInsetTop = m_layoutState.GetTopToolbarHeight();
    dialogDesc.visible = true;
    dialogDesc.resizable = false;

    m_coreToolsDialogHandle = m_bridge.GetCapabilities().ui->CreateWebDialog(dialogDesc);
    return m_coreToolsDialogHandle != 0U;
}

bool StudioHost::InitializeProjectExplorerPanel()
{
    return m_projectExplorerPanel.Open(
        m_bridge.GetCapabilities(),
        GetProjectExplorerContentPath(),
        m_layoutState.GetTopToolbarHeight(),
        m_layoutState.GetRightInspectorWidth());
}

bool StudioHost::InitializeConsolePanel()
{
    return m_consolePanel.Open(
        m_bridge.GetCapabilities(),
        GetConsoleContentPath(),
        m_layoutState.GetLeftSidebarWidth(),
        m_layoutState.GetRightInspectorWidth(),
        m_layoutState.GetBottomConsoleHeight());
}

bool StudioHost::InitializeFrameGizmoButton()
{
    AppCapabilities& capabilities = m_bridge.GetCapabilities();
    if (!capabilities.ui)
    {
        return false;
    }

    NativeButtonDesc buttonDesc;
    buttonDesc.name = "PreviewBgToggle";
    buttonDesc.text = L"Gizmo: BG";
    buttonDesc.rect = NativeControlRect{ 0, 0, 108, 32 };
    buttonDesc.visible = true;
    buttonDesc.enabled = true;

    m_frameGizmoButtonHandle = capabilities.ui->CreateNativeButton(buttonDesc);
    if (m_frameGizmoButtonHandle == 0U)
    {
        return false;
    }

    UpdateFrameGizmoButtonLayout();
    return true;
}

void StudioHost::UpdateFrameGizmoButtonLayout()
{
    AppCapabilities& capabilities = m_bridge.GetCapabilities();
    if (!capabilities.ui || !capabilities.window || m_frameGizmoButtonHandle == 0U)
    {
        return;
    }

    const int windowWidth = std::max(0, capabilities.window->GetWindowWidth());
    const int topInset = m_layoutState.GetTopToolbarHeight();
    const int leftInset = m_layoutState.GetLeftSidebarWidth();
    const int rightInset = m_projectExplorerPanel.IsVisible() ? m_layoutState.GetRightInspectorWidth() : 0;

    const int x = std::max(leftInset + 8, windowWidth - rightInset - 118);
    const int y = topInset + 10;
    capabilities.ui->SetNativeButtonBounds(
        m_frameGizmoButtonHandle,
        NativeControlRect{ x, y, 108, 32 });
}

void StudioHost::UpdateLayoutState()
{
    AppCapabilities& capabilities = m_bridge.GetCapabilities();
    if (!capabilities.window)
    {
        return;
    }

    m_layoutState.Recalculate(
        capabilities.window->GetWindowWidth(),
        capabilities.window->GetWindowHeight(),
        m_projectExplorerPanel.IsVisible(),
        m_consolePanel.IsVisible());

    const ViewportRect& viewport = m_layoutState.GetViewportRect();
    const bool windowChanged =
        m_layoutState.GetWindowWidth() != m_lastLayoutWindowWidth ||
        m_layoutState.GetWindowHeight() != m_lastLayoutWindowHeight;
    const bool viewportChanged =
        viewport.x != m_lastLayoutViewportRect.x ||
        viewport.y != m_lastLayoutViewportRect.y ||
        viewport.width != m_lastLayoutViewportRect.width ||
        viewport.height != m_lastLayoutViewportRect.height;

    if (!m_hasLoggedLayout || windowChanged || viewportChanged)
    {
        std::ostringstream stream;
        stream << "Window: "
               << m_layoutState.GetWindowWidth() << " x " << m_layoutState.GetWindowHeight()
               << " | Viewport: "
               << viewport.x << ", " << viewport.y << ", " << viewport.width << ", " << viewport.height
               << " | Aspect: " << std::fixed << std::setprecision(4) << viewport.Aspect();
        m_bridge.LogInfo(stream.str());

        m_hasLoggedLayout = true;
        m_lastLayoutWindowWidth = m_layoutState.GetWindowWidth();
        m_lastLayoutWindowHeight = m_layoutState.GetWindowHeight();
        m_lastLayoutViewportRect = viewport;
    }

    if (capabilities.rendering)
    {
        capabilities.rendering->SetViewportRect(viewport);
    }
}

bool StudioHost::HandleStudioHttpRequest(const std::string& path, std::string& outContentType, std::string& outBody)
{
    const std::string route = StripQuery(path);
    outContentType = "application/json; charset=utf-8";

    if (route == "/studio/open-file-management")
    {
        const bool opened = OpenFileManagementDialog();
        outBody = BuildJsonResponse(opened, opened ? "File dialog opened." : "Failed to open File dialog.");
        return true;
    }

    if (route == "/studio/workspace/save-layout")
    {
        const studio::StudioProject project = m_projectSystem.GetCurrentProject();
        if (project.isOpen)
        {
            studio::SaveProjectRequest request;
            request.projectId = project.projectId;
            request.editorState = std::string("workspace=open\nexplorer_visible=") +
                (m_projectExplorerPanel.IsVisible() ? "true" : "false") +
                "\nconsole_visible=" +
                (m_consolePanel.IsVisible() ? "true" : "false") +
                "\nlast_project=" +
                project.projectId +
                "\n";
            const studio::SaveProjectResult result = m_projectSystem.SaveProject(request);
            outBody = BuildJsonResponse(
                result.success,
                result.success ? "Layout saved." : result.error,
                result.projectId,
                result.logicalSavePath);
            return true;
        }

        outBody = BuildJsonResponse(false, "No project open.");
        return true;
    }

    if (route == "/studio/workspace/toggle-explorer")
    {
        const bool nextVisible = !m_projectExplorerPanel.IsVisible();
        const bool updated = m_projectExplorerPanel.SetVisible(m_bridge.GetCapabilities(), nextVisible);
        outBody = BuildJsonResponse(updated, updated ? (nextVisible ? "Explorer shown." : "Explorer hidden.") : "Explorer panel unavailable.");
        return true;
    }

    if (route == "/studio/workspace/toggle-console")
    {
        const bool nextVisible = !m_consolePanel.IsVisible();
        const bool updated = m_consolePanel.SetVisible(m_bridge.GetCapabilities(), nextVisible);
        outBody = BuildJsonResponse(updated, updated ? (nextVisible ? "Console shown." : "Console hidden.") : "Console panel unavailable.");
        return true;
    }

    if (route == "/studio/open-new-project")
    {
        const bool opened = OpenNewProjectDialog();
        outBody = BuildJsonResponse(opened, opened ? "New Project dialog opened." : "Failed to open New Project dialog.");
        return true;
    }

    if (route == "/studio/close-new-project")
    {
        m_newProjectDialog.Close(m_bridge.GetCapabilities());
        outBody = BuildJsonResponse(true, "New Project dialog closed.");
        return true;
    }

    if (route == "/studio/project/context-menu" || route == "/studio/file/context-menu")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto screenX = query.find("screenX");
        const auto screenY = query.find("screenY");
        const int x = screenX == query.end() ? 0 : std::atoi(screenX->second.c_str());
        const int y = screenY == query.end() ? 0 : std::atoi(screenY->second.c_str());

        outBody = BuildMenuJsonResponse(ShowProjectContextMenu(x, y));
        return true;
    }

    if (route == "/studio/build/context-menu")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto screenX = query.find("screenX");
        const auto screenY = query.find("screenY");
        const int x = screenX == query.end() ? 0 : std::atoi(screenX->second.c_str());
        const int y = screenY == query.end() ? 0 : std::atoi(screenY->second.c_str());
        outBody = BuildMenuJsonResponse(ShowBuildContextMenu(m_bridge.GetNativeWindowHandle(), x, y));
        return true;
    }

    if (route == "/studio/console/execute")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto command = query.find("command");
        const std::string commandText = command == query.end() ? std::string() : command->second;
        if (commandText == "clear")
        {
            m_consoleFeed.clear();
            PushConsoleMessage("Console cleared.");
            outBody = BuildConsoleJsonResponse(true, "Console cleared.", true);
            return true;
        }

        PushConsoleMessage("> " + commandText);
        const std::string output = ExecuteConsoleCommand(commandText);
        PushConsoleMessage(output);
        outBody = BuildConsoleJsonResponse(true, output);
        return true;
    }

    if (route == "/studio/console/feed")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto cursorValue = query.find("cursor");
        std::size_t cursor = 0U;
        if (cursorValue != query.end() && !cursorValue->second.empty())
        {
            cursor = static_cast<std::size_t>(std::strtoull(cursorValue->second.c_str(), nullptr, 10));
        }

        outBody = BuildConsoleFeedJson(cursor);
        return true;
    }

    if (route == "/studio/runtime/play")
    {
        std::string error;
        const bool started = m_runtimeHost.Play(MakeRuntimeDescWithSceneOverride(m_projectSystem.GetCurrentProject()), error);
        m_bridge.SetEscapeShutdownEnabled(!started);
        outBody = BuildRuntimeActionJsonResponse(
            started,
            started ? "Runtime started." : error,
            m_runtimeHost.GetStateName(),
            m_runtimeHost.GetViewportText());
        return true;
    }

    if (route == "/studio/runtime/stop")
    {
        m_runtimeHost.Stop();
        m_bridge.SetEscapeShutdownEnabled(true);
        outBody = BuildRuntimeActionJsonResponse(
            true,
            "Runtime stopped.",
            m_runtimeHost.GetStateName(),
            m_runtimeHost.GetViewportText());
        return true;
    }

    if (route == "/studio/runtime/pause")
    {
        m_runtimeHost.TogglePause();
        outBody = BuildRuntimeActionJsonResponse(
            true,
            "Runtime pause toggled.",
            m_runtimeHost.GetStateName(),
            m_runtimeHost.GetViewportText());
        return true;
    }

    if (route == "/studio/runtime/step")
    {
        m_runtimeHost.StepOnce();
        outBody = BuildRuntimeActionJsonResponse(
            true,
            "Runtime stepped.",
            m_runtimeHost.GetStateName(),
            m_runtimeHost.GetViewportText());
        return true;
    }

    if (route == "/studio/runtime/viewport")
    {
        outBody = BuildRuntimeViewportJson();
        return true;
    }

    if (route == "/studio/world/entities")
    {
        const std::vector<studio_runtime::EntityId> ids = m_runtimeHost.GetConnector().Queries().GetEntityList();
        std::string json = "{\"ok\":true,\"entities\":[";
        bool first = true;
        for (const studio_runtime::EntityId id : ids)
        {
            studio_runtime::EntityInfo info;
            if (!m_runtimeHost.GetConnector().Queries().GetEntityInfo(id, info))
            {
                continue;
            }
            if (!first) { json += ","; }
            first = false;
            json += "{\"id\":" + std::to_string(id);
            json += ",\"name\":\"" + EscapeJson(info.name) + "\"";
            json += ",\"selected\":" + std::string(m_runtimeHost.GetInteraction().IsEntitySelected(id) ? "true" : "false");
            json += ",\"hasRenderable\":" + std::string(info.hasRenderable ? "true" : "false");
            json += ",\"hasLight\":" + std::string(info.hasLight ? "true" : "false");
            json += "}";
        }
        json += "]}";
        outBody = json;
        return true;
    }

    if (route == "/studio/selection/set")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto entityIt = query.find("entity");
        if (entityIt != query.end() && !entityIt->second.empty())
        {
            const studio_runtime::EntityId id = static_cast<studio_runtime::EntityId>(
                std::strtoull(entityIt->second.c_str(), nullptr, 10));
            m_runtimeHost.GetInteraction().SelectEntity(id);
            outBody = "{\"ok\":true,\"selectedEntity\":" + std::to_string(id) + "}";
        }
        else
        {
            m_runtimeHost.GetInteraction().ClearSelection();
            outBody = "{\"ok\":true,\"selectedEntity\":0}";
        }
        return true;
    }

    if (route == "/studio/tool/set")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto nameIt = query.find("name");
        const auto activeIt = query.find("active");
        if (nameIt != query.end())
        {
            bool active = true;
            if (activeIt != query.end())
            {
                active = activeIt->second == "1" || activeIt->second == "true";
            }

            const studio_runtime::StudioTool tool = active
                ? studio_runtime::StudioInteractionController::ToolFromString(nameIt->second)
                : studio_runtime::StudioTool::None;
            m_runtimeHost.SetActiveTool(tool);
            outBody = m_runtimeHost.GetInteraction().BuildStateJson();
        }
        else
        {
            outBody = "{\"ok\":false,\"message\":\"unknown tool\"}";
        }
        return true;
    }

    if (route == "/studio/interaction/state")
    {
        outBody = m_runtimeHost.GetInteraction().BuildStateJson();
        return true;
    }

    if (route == "/studio/interaction/feed")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto cursorValue = query.find("cursor");
        std::size_t cursor = 0U;
        if (cursorValue != query.end() && !cursorValue->second.empty())
        {
            cursor = static_cast<std::size_t>(std::strtoull(cursorValue->second.c_str(), nullptr, 10));
        }

        outBody = BuildInteractionFeedJson(cursor);
        return true;
    }

    if (route == "/studio/selection/info")
    {
        m_runtimeHost.GetInteraction().EmitInspectorDataRequested(
            m_runtimeHost.GetInteraction().GetState().selectedEntity);
        outBody = BuildSelectedEntityJson();
        return true;
    }

    if (route == "/studio/library/entries")
    {
        outBody = BuildLibraryEntriesJson();
        return true;
    }

    if (route == "/studio/library/config")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto linkIt = query.find("link");
        if (linkIt == query.end())
        {
            outBody = std::string("{\"ok\":true,\"link\":\"") + EscapeJson(GetStudioAssetLibraryLink()) + "\"}";
            return true;
        }

        std::string message;
        const bool updated = SetStudioAssetLibraryLink(linkIt->second, message);
        outBody = std::string("{\"ok\":") + (updated ? "true" : "false") +
            ",\"message\":\"" + EscapeJson(message) +
            "\",\"link\":\"" + EscapeJson(GetStudioAssetLibraryLink()) + "\"}";
        return true;
    }

    if (route == "/studio/library/entry")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto id = query.find("id");
        outBody = BuildLibraryEntryJson(id == query.end() ? std::string() : id->second);
        return true;
    }

    if (route == "/studio/selection/feed")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto cursorValue = query.find("cursor");
        std::size_t cursor = 0U;
        if (cursorValue != query.end() && !cursorValue->second.empty())
        {
            cursor = static_cast<std::size_t>(std::strtoull(cursorValue->second.c_str(), nullptr, 10));
        }

        outBody = BuildInteractionFeedJson(cursor);
        return true;
    }

    if (route == "/studio/scene/save")
    {
        std::string error;
        const bool saved = m_runtimeHost.SaveScene(GetDefaultScenePath(), error);
        outBody = BuildJsonResponse(saved, saved ? "Scene saved." : error);
        return true;
    }

    if (route == "/studio/scene/load")
    {
        std::string error;
        const bool loaded = m_runtimeHost.Initialize(GetDefaultScenePath(), error);
        outBody = BuildJsonResponse(loaded, error.empty() ? "Scene loaded." : ("Scene loaded with fallback: " + error));
        return true;
    }

    if (route == "/studio/scene/roundtrip-check")
    {
        const studio_runtime::RuntimeWorld& beforeWorld = m_runtimeHost.GetWorld();
        const std::string beforeSignature = BuildWorldSignature(beforeWorld);
        const std::string serialized = studio_runtime::SceneSerializer::SaveToText(beforeWorld);
        studio_runtime::RuntimeWorld reloaded;
        std::string error;
        const bool loaded = studio_runtime::SceneSerializer::LoadFromText(serialized, reloaded, error);
        const std::string afterSignature = loaded ? BuildWorldSignature(reloaded) : std::string();
        const bool same = loaded && beforeSignature == afterSignature;

        outBody = std::string("{\"ok\":") + (same ? "true" : "false") +
            ",\"loaded\":" + (loaded ? std::string("true") : std::string("false")) +
            ",\"sameSignature\":" + (same ? std::string("true") : std::string("false")) +
            ",\"before\":\"" + EscapeJson(beforeSignature) + "\"" +
            ",\"after\":\"" + EscapeJson(afterSignature) + "\"" +
            ",\"error\":\"" + EscapeJson(error) + "\"}";
        return true;
    }

    if (route == "/studio/build/project")
    {
        const studio::StudioProject project = m_projectSystem.GetCurrentProject();
        if (!project.isOpen)
        {
            PushConsoleMessage("Build request rejected: no project open.");
            outBody = BuildJsonResponse(false, "No project open.", std::string(), std::string());
            return true;
        }

        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        studio::BuildAndRunProjectRequest request;
        request.projectId = project.projectId;
        request.runAfterBuild = IsTruthyQueryValue(query, "runAfterBuild");
        request.exportAfterBuild = IsTruthyQueryValue(query, "exportAfterBuild");
        PushConsoleMessage("Build requested for project '" + request.projectId + "'.");
        const studio::BuildAndRunProjectResult result = m_buildSystem.BuildAndRunProject(request);
        if (!result.success)
        {
            PushConsoleMessage(
                "Build failed for '" + request.projectId +
                "' (configureExit=" + std::to_string(result.configureExitCode) +
                ", buildExit=" + std::to_string(result.buildExitCode) +
                "): " + (result.error.empty() ? "Unknown error." : result.error));
        }
        else
        {
            PushConsoleMessage(result.message);
            if (!result.logicalExportPath.empty())
            {
                PushConsoleMessage("Export output: " + result.logicalExportPath);
            }
            if (!result.executablePath.empty() && request.runAfterBuild)
            {
                PushConsoleMessage("Preview executable: " + result.executablePath);
            }
        }
        outBody = BuildJsonResponse(
            result.success,
            result.success ? result.message : result.error,
            result.projectId,
            result.logicalExportPath.empty() ? result.logicalBuildPath : result.logicalExportPath);
        return true;
    }

    if (route == "/studio/open-project")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        studio::OpenProjectRequest request;
        const auto projectId = query.find("projectId");
        request.projectId = projectId == query.end() ? std::string() : projectId->second;
        if (request.projectId.empty())
        {
            const studio::OpenProjectResult pickerResult = OpenProjectFromFolderPicker();
            outBody = BuildJsonResponse(
                pickerResult.success,
                pickerResult.success ? pickerResult.message : pickerResult.error,
                pickerResult.projectId,
                pickerResult.logicalProjectPath);
            return true;
        }

        const studio::OpenProjectResult result = m_projectSystem.OpenProject(request);
        if (result.success)
        {
            m_projectExplorerPanel.SetSelectedLogicalPath(m_projectSystem.GetCurrentProjectRoot());
            const studio::StudioProject project = m_projectSystem.GetCurrentProject();
            m_selectedSceneLogicalPath = project.projectRootPath + "/" + project.defaultScenePath;
            RefreshProjectProxyLibrary(project);
            StartPreviewForSelectedScene();
        }

        outBody = BuildJsonResponse(
            result.success,
            result.success ? result.message : result.error,
            result.projectId,
            result.logicalProjectPath);
        return true;
    }

    if (route == "/studio/save-project")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        studio::SaveProjectRequest request;
        const auto projectId = query.find("projectId");
        request.projectId = projectId == query.end() ? std::string("pong") : projectId->second;
        request.editorState = "workspace=open\nlast_project=" + request.projectId + "\n";

        const studio::SaveProjectResult result = m_projectSystem.SaveProject(request);
        outBody = BuildJsonResponse(
            result.success,
            result.success ? result.message : result.error,
            result.projectId,
            result.logicalSavePath);
        return true;
    }

    if (route == "/studio/project/create" || route == "/studio/create-project")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        studio::CreateProjectRequest request;
        const auto projectName = query.find("projectName");
        const auto templateName = query.find("templateName");
        request.projectName = projectName == query.end() ? std::string() : projectName->second;
        request.templateName = templateName == query.end() ? std::string("Arcade2D") : templateName->second;
        request.overwrite = IsTruthyQueryValue(query, "overwrite");

        const studio::CreateProjectResult result = m_projectSystem.CreateProject(request);
        if (result.success)
        {
            m_projectExplorerPanel.SetSelectedLogicalPath(m_projectSystem.GetCurrentProjectRoot());
            const studio::StudioProject project = m_projectSystem.GetCurrentProject();
            m_selectedSceneLogicalPath = project.projectRootPath + "/" + project.defaultScenePath;
            RefreshProjectProxyLibrary(project);
            StartPreviewForSelectedScene();
        }

        outBody = BuildCreateProjectJsonResponse(result);
        return true;
    }

    if (route == "/studio/project-explorer/tree")
    {
        outBody = m_projectExplorerPanel.BuildTreeJson(m_projectSystem);
        return true;
    }

    if (route == "/studio/project-explorer/select")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto selectedPath = query.find("path");
        std::string message;
        const bool selected = m_projectExplorerPanel.SelectFile(
            m_projectSystem,
            selectedPath == query.end() ? std::string() : selectedPath->second,
            message);
        outBody = BuildJsonResponse(
            selected,
            message,
            m_projectSystem.GetCurrentProject().projectId,
            m_projectExplorerPanel.GetSelectedFileLogicalPath());

        if (selected)
        {
            const std::string logicalPath = m_projectExplorerPanel.GetSelectedFileLogicalPath();
            if (EndsWithInsensitive(logicalPath, ".scene"))
            {
                m_selectedSceneLogicalPath = logicalPath;
                StartPreviewForSelectedScene();
            }
        }
        return true;
    }

    if (route == "/studio/project-explorer/context-menu")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto screenX = query.find("screenX");
        const auto screenY = query.find("screenY");
        const auto isRoot = query.find("isRoot");
        const int x = screenX == query.end() ? 0 : std::atoi(screenX->second.c_str());
        const int y = screenY == query.end() ? 0 : std::atoi(screenY->second.c_str());
        const bool canRename = isRoot == query.end() || isRoot->second != "true";

        outBody = BuildMenuJsonResponse(ShowProjectExplorerContextMenu(x, y, canRename));
        return true;
    }

    if (route == "/studio/project-explorer/rename")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto sourcePath = query.find("path");
        const auto newName = query.find("newName");

        studio::RenameFileActionRequest request;
        request.logicalPath = sourcePath == query.end() ? std::string() : sourcePath->second;
        request.newName = newName == query.end() ? std::string() : newName->second;

        const studio::RenameFileActionResult result = m_fileActions.Rename(m_projectSystem, request);
        if (result.success)
        {
            m_projectExplorerPanel.SetSelectedLogicalPath(result.newLogicalPath);
        }

        outBody = BuildRenameJsonResponse(
            result.success,
            result.success ? result.message : result.error,
            result.oldLogicalPath,
            result.newLogicalPath);
        return true;
    }

    if (route == "/studio/project-explorer/create-scene")
    {
        const studio::StudioProject project = m_projectSystem.GetCurrentProject();
        if (!project.isOpen || project.projectRootPath.empty())
        {
            outBody = BuildJsonResponse(false, "Open a project first.");
            return true;
        }

        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        std::string baseName = "scene";
        const auto requestedName = query.find("name");
        if (requestedName != query.end())
        {
            const std::string normalized = NormalizeSceneName(requestedName->second);
            if (!normalized.empty())
            {
                baseName = normalized;
            }
        }

        studio::FileProxy files;
        const std::string scenesFolder = project.projectRootPath + "/scenes";
        std::string ensureError;
        if (!files.Exists(scenesFolder) && !files.CreateDirectory(scenesFolder, ensureError))
        {
            outBody = BuildJsonResponse(false, ensureError);
            return true;
        }

        std::string fileName;
        std::string logicalPath;
        for (int index = 1; index <= 999; ++index)
        {
            std::ostringstream suffix;
            suffix << std::setfill('0') << std::setw(3) << index;
            fileName = baseName + "_" + suffix.str() + ".scene";
            logicalPath = scenesFolder + "/" + fileName;
            if (!files.Exists(logicalPath))
            {
                break;
            }

            fileName.clear();
            logicalPath.clear();
        }

        if (logicalPath.empty())
        {
            outBody = BuildJsonResponse(false, "Could not allocate a new scene file name.");
            return true;
        }

        const std::string sceneDisplayName = fileName.substr(0U, fileName.size() - std::string(".scene").size());
        std::string error;
        if (!files.WriteText(logicalPath, BuildCreateSceneTemplate(sceneDisplayName), error))
        {
            outBody = BuildJsonResponse(false, error);
            return true;
        }

        m_projectExplorerPanel.SetSelectedLogicalPath(logicalPath);
        outBody = BuildJsonResponse(
            true,
            "Created scene " + fileName + ".",
            project.projectId,
            logicalPath);
        return true;
    }

    return false;
}

bool StudioHost::OpenFileManagementDialog()
{
    if (!m_fileManagementDialog.Open(m_bridge.GetCapabilities(), GetFileManagementDialogContentPath()))
    {
        m_bridge.LogError("Studio: failed to open File dialog.");
        return false;
    }

    m_bridge.LogInfo("Studio: File dialog opened.");
    return true;
}

bool StudioHost::OpenNewProjectDialog()
{
    if (!m_newProjectDialog.Open(m_bridge.GetCapabilities(), GetNewProjectDialogContentPath()))
    {
        m_bridge.LogError("Studio: failed to open New Project dialog.");
        return false;
    }

    m_bridge.LogInfo("Studio: New Project dialog opened.");
    return true;
}

studio::OpenProjectResult StudioHost::OpenProjectFromFolderPicker()
{
    studio::OpenProjectResult result;

    studio::FileProxy files;
    std::string workspaceError;
    files.EnsureWorkspaceFolders(workspaceError);

    IFileOpenDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || !dialog)
    {
        result.error = "Could not open the native folder picker.";
        return result;
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Open Studio Project");

    const std::wstring userFolder = ToWideString(NormalizeNativePath(files.Resolve(std::string())));
    IShellItem* userFolderItem = nullptr;
    if (!userFolder.empty() &&
        SUCCEEDED(SHCreateItemFromParsingName(userFolder.c_str(), nullptr, IID_PPV_ARGS(&userFolderItem))) &&
        userFolderItem)
    {
        dialog->SetFolder(userFolderItem);
        userFolderItem->Release();
    }

    hr = dialog->Show(m_bridge.GetNativeWindowHandle());
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        dialog->Release();
        result.error = "Open Project was cancelled.";
        return result;
    }

    if (FAILED(hr))
    {
        dialog->Release();
        result.error = "Native folder picker failed.";
        return result;
    }

    IShellItem* selectedItem = nullptr;
    hr = dialog->GetResult(&selectedItem);
    dialog->Release();
    if (FAILED(hr) || !selectedItem)
    {
        result.error = "No project folder was selected.";
        return result;
    }

    PWSTR selectedPath = nullptr;
    hr = selectedItem->GetDisplayName(SIGDN_FILESYSPATH, &selectedPath);
    selectedItem->Release();
    if (FAILED(hr) || !selectedPath)
    {
        result.error = "Could not read the selected folder path.";
        return result;
    }

    std::string projectId;
    std::string error;
    const std::string selectedFolder = ToNarrowString(selectedPath);
    CoTaskMemFree(selectedPath);

    if (!TryExtractProjectIdFromFolder(selectedFolder, projectId, error))
    {
        result.error = error;
        return result;
    }

    studio::OpenProjectRequest request;
    request.projectId = projectId;
    result = m_projectSystem.OpenProject(request);
    if (result.success)
    {
        m_projectExplorerPanel.SetSelectedLogicalPath(m_projectSystem.GetCurrentProjectRoot());
        const studio::StudioProject project = m_projectSystem.GetCurrentProject();
        m_selectedSceneLogicalPath = project.projectRootPath + "/" + project.defaultScenePath;
        StartPreviewForSelectedScene();
    }

    return result;
}

void StudioHost::TickRuntime()
{
    AppCapabilities& capabilities = m_bridge.GetCapabilities();
    UpdateLayoutState();
    bool hideOsCursorForViewport = false;
    UpdateFrameGizmoButtonLayout();
    if (capabilities.ui &&
        m_frameGizmoButtonHandle != 0U &&
        capabilities.ui->ConsumeNativeButtonPressed(m_frameGizmoButtonHandle))
    {
        m_previewBackgroundAlt = !m_previewBackgroundAlt;
        PushConsoleMessage(std::string("Gizmo: preview background ") + (m_previewBackgroundAlt ? "alt" : "default") + ".");
    }

    studio_runtime::RuntimeInput input;
    if (capabilities.input)
    {
        auto setKeyState =
            [&input, &capabilities](InputKey key, AppKey appKey)
            {
                RawButtonState& state = input.rawInput.keys[static_cast<std::size_t>(key)];
                state.pressed = capabilities.input->WasKeyPressed(appKey);
                state.down = capabilities.input->IsKeyDown(appKey);
                state.released = capabilities.input->WasKeyReleased(appKey);
            };
        auto setMouseState =
            [&input, &capabilities](InputMouseButton button, AppMouseButton appButton)
            {
                RawButtonState& state = input.rawInput.mouseButtons[static_cast<std::size_t>(button)];
                state.pressed = capabilities.input->WasMouseButtonPressed(appButton);
                state.down = capabilities.input->IsMouseButtonDown(appButton);
                state.released = capabilities.input->WasMouseButtonReleased(appButton);
            };

        input.escapePressed = capabilities.input->WasKeyPressed(static_cast<AppKey>(VK_ESCAPE));
        input.servePressed = capabilities.input->WasKeyPressed(AppKey::Space);
        input.resetPressed = capabilities.input->WasKeyPressed(AppKey::R);
        input.leftUpDown = capabilities.input->IsKeyDown(AppKey::W);
        input.leftDownDown = capabilities.input->IsKeyDown(AppKey::S);
        input.rightUpDown = capabilities.input->IsKeyDown(AppKey::Up);
        input.rightDownDown = capabilities.input->IsKeyDown(AppKey::Down);
        input.leftMousePressed = capabilities.input->WasMouseButtonPressed(AppMouseButton::Left);
        input.middleMouseDown = capabilities.input->IsMouseButtonDown(AppMouseButton::Middle);
        input.rightMouseDown = capabilities.input->IsMouseButtonDown(AppMouseButton::Right);
        input.mouseX = capabilities.input->GetMouseX();
        input.mouseY = capabilities.input->GetMouseY();
        input.mouseDeltaX = capabilities.input->GetMouseDeltaX();
        input.mouseDeltaY = capabilities.input->GetMouseDeltaY();
        input.mouseScrollDelta = capabilities.input->GetMouseScrollDelta();

        input.rawInput.mouseX = input.mouseX;
        input.rawInput.mouseY = input.mouseY;
        input.rawInput.mouseDeltaX = input.mouseDeltaX;
        input.rawInput.mouseDeltaY = input.mouseDeltaY;
        input.rawInput.mouseWheelDelta = input.mouseScrollDelta;

        if (capabilities.window)
        {
            const HWND hwnd = capabilities.window->GetWindowHandle();
            POINT osPoint{};
            if (hwnd != nullptr && GetCursorPos(&osPoint) != 0)
            {
                POINT clientPoint = osPoint;
                if (ScreenToClient(hwnd, &clientPoint) != 0)
                {
                    const ViewportRect viewport = m_layoutState.GetViewportRect();
                    const bool insideViewport =
                        clientPoint.x >= viewport.x &&
                        clientPoint.y >= viewport.y &&
                        clientPoint.x < (viewport.x + viewport.width) &&
                        clientPoint.y < (viewport.y + viewport.height);
                    hideOsCursorForViewport = insideViewport;
                    input.cursorDebug.valid = true;
                    input.cursorDebug.osScreenX = osPoint.x;
                    input.cursorDebug.osScreenY = osPoint.y;
                    input.cursorDebug.clientX = clientPoint.x;
                    input.cursorDebug.clientY = clientPoint.y;
                    input.cursorDebug.viewportLocalX = clientPoint.x - viewport.x;
                    input.cursorDebug.viewportLocalY = clientPoint.y - viewport.y;
                    input.cursorDebug.drawX = viewport.x + input.cursorDebug.viewportLocalX;
                    input.cursorDebug.drawY = viewport.y + input.cursorDebug.viewportLocalY;

                    const int windowWidth = std::max(1, capabilities.window->GetWindowWidth());
                    const int windowHeight = std::max(1, capabilities.window->GetWindowHeight());
                    RECT clientRect{};
                    if (GetClientRect(hwnd, &clientRect) != 0)
                    {
                        const int clientWidth = std::max(1, static_cast<int>(clientRect.right - clientRect.left));
                        const int clientHeight = std::max(1, static_cast<int>(clientRect.bottom - clientRect.top));
                        input.cursorDebug.dpiScaleX = static_cast<float>(windowWidth) / static_cast<float>(clientWidth);
                        input.cursorDebug.dpiScaleY = static_cast<float>(windowHeight) / static_cast<float>(clientHeight);
                    }

                    const float safeScaleX = std::max(0.0001f, input.cursorDebug.dpiScaleX);
                    const float safeScaleY = std::max(0.0001f, input.cursorDebug.dpiScaleY);
                    input.cursorDebug.clientDpiAdjustedX = static_cast<int>(std::lround(static_cast<float>(clientPoint.x) / safeScaleX));
                    input.cursorDebug.clientDpiAdjustedY = static_cast<int>(std::lround(static_cast<float>(clientPoint.y) / safeScaleY));
                    input.cursorDebug.viewportLocalDpiAdjustedX = input.cursorDebug.clientDpiAdjustedX - viewport.x;
                    input.cursorDebug.viewportLocalDpiAdjustedY = input.cursorDebug.clientDpiAdjustedY - viewport.y;
                    input.cursorDebug.drawDpiAdjustedX = viewport.x + input.cursorDebug.viewportLocalDpiAdjustedX;
                    input.cursorDebug.drawDpiAdjustedY = viewport.y + input.cursorDebug.viewportLocalDpiAdjustedY;

                    RECT windowRect{};
                    if (GetWindowRect(hwnd, &windowRect) != 0)
                    {
                        input.cursorDebug.windowScreenLeft = static_cast<int>(windowRect.left);
                        input.cursorDebug.windowScreenTop = static_cast<int>(windowRect.top);
                        input.cursorDebug.windowScreenRight = static_cast<int>(windowRect.right);
                        input.cursorDebug.windowScreenBottom = static_cast<int>(windowRect.bottom);

                        studio_runtime::CursorSpaceCalibrationInput calibrationInput;
                        calibrationInput.windowScreenLeft = input.cursorDebug.windowScreenLeft;
                        calibrationInput.windowScreenTop = input.cursorDebug.windowScreenTop;
                        calibrationInput.windowScreenRight = input.cursorDebug.windowScreenRight;
                        calibrationInput.windowScreenBottom = input.cursorDebug.windowScreenBottom;
                        calibrationInput.clientWidth = std::max(1, static_cast<int>(clientRect.right - clientRect.left));
                        calibrationInput.clientHeight = std::max(1, static_cast<int>(clientRect.bottom - clientRect.top));
                        POINT clientOriginScreen{ 0, 0 };
                        if (ClientToScreen(hwnd, &clientOriginScreen) != 0)
                        {
                            calibrationInput.clientOriginScreenX = clientOriginScreen.x;
                            calibrationInput.clientOriginScreenY = clientOriginScreen.y;
                        }
                        else
                        {
                            calibrationInput.clientOriginScreenX = osPoint.x - clientPoint.x;
                            calibrationInput.clientOriginScreenY = osPoint.y - clientPoint.y;
                        }
                        calibrationInput.viewportX = viewport.x;
                        calibrationInput.viewportY = viewport.y;
                        calibrationInput.viewportWidth = viewport.width;
                        calibrationInput.viewportHeight = viewport.height;
                        calibrationInput.virtualWidth = viewport.width;
                        calibrationInput.virtualHeight = viewport.height;
                        calibrationInput.osScreenX = osPoint.x;
                        calibrationInput.osScreenY = osPoint.y;
                        calibrationInput.hotspotX = 0.0f;
                        calibrationInput.hotspotY = 0.0f;

                        const studio_runtime::CursorSpaceCalibrationResult calibration = studio_runtime::ComputeCursorSpaceCalibration(calibrationInput);
                        input.cursorDebug.windowWidth = calibration.windowWidth;
                        input.cursorDebug.windowHeight = calibration.windowHeight;
                        input.cursorDebug.clientWidth = calibration.clientWidth;
                        input.cursorDebug.clientHeight = calibration.clientHeight;
                        input.cursorDebug.borderLeft = calibration.borderLeft;
                        input.cursorDebug.borderTop = calibration.borderTop;
                        input.cursorDebug.borderRight = calibration.borderRight;
                        input.cursorDebug.borderBottom = calibration.borderBottom;
                        input.cursorDebug.viewportInsetLeft = calibration.viewportInsetLeft;
                        input.cursorDebug.viewportInsetTop = calibration.viewportInsetTop;
                        input.cursorDebug.viewportInsetRight = calibration.viewportInsetRight;
                        input.cursorDebug.viewportInsetBottom = calibration.viewportInsetBottom;
                        input.cursorDebug.viewportU = calibration.normalizedU;
                        input.cursorDebug.viewportV = calibration.normalizedV;
                        input.cursorDebug.renderX = calibration.renderX;
                        input.cursorDebug.renderY = calibration.renderY;
                        input.cursorDebug.calibrationDrawX = calibration.finalDrawClientX;
                        input.cursorDebug.calibrationDrawY = calibration.finalDrawClientY;
                    }

                    // Default path maps OS screen -> client -> frame-local -> normalized -> render space.
                    // The screen override is kept as a narrow diagnostic escape hatch.
                    const int calibratedX = m_forceOsScreenMouseOverride
                        ? input.cursorDebug.osScreenX
                        : input.cursorDebug.calibrationDrawX;
                    const int calibratedY = m_forceOsScreenMouseOverride
                        ? input.cursorDebug.osScreenY
                        : input.cursorDebug.calibrationDrawY;
                    if (m_hasLastCalibratedMouse)
                    {
                        input.mouseDeltaX = calibratedX - m_lastCalibratedMouseX;
                        input.mouseDeltaY = calibratedY - m_lastCalibratedMouseY;
                    }
                    else
                    {
                        input.mouseDeltaX = 0;
                        input.mouseDeltaY = 0;
                        m_hasLastCalibratedMouse = true;
                    }

                    m_lastCalibratedMouseX = calibratedX;
                    m_lastCalibratedMouseY = calibratedY;
                    input.mouseX = calibratedX;
                    input.mouseY = calibratedY;
                    input.rawInput.mouseX = calibratedX;
                    input.rawInput.mouseY = calibratedY;
                    input.rawInput.mouseDeltaX = input.mouseDeltaX;
                    input.rawInput.mouseDeltaY = input.mouseDeltaY;
                }
            }
        }

        setMouseState(InputMouseButton::Left, AppMouseButton::Left);
        setMouseState(InputMouseButton::Right, AppMouseButton::Right);
        setMouseState(InputMouseButton::Middle, AppMouseButton::Middle);

        setKeyState(InputKey::Escape, static_cast<AppKey>(VK_ESCAPE));
        setKeyState(InputKey::Space, AppKey::Space);
        setKeyState(InputKey::W, AppKey::W);
        setKeyState(InputKey::A, AppKey::A);
        setKeyState(InputKey::S, AppKey::S);
        setKeyState(InputKey::D, static_cast<AppKey>('D'));
        setKeyState(InputKey::Q, static_cast<AppKey>('Q'));
        setKeyState(InputKey::E, static_cast<AppKey>('E'));
        setKeyState(InputKey::F, static_cast<AppKey>('F'));
        setKeyState(InputKey::R, AppKey::R);
        setKeyState(InputKey::Left, AppKey::Left);
        setKeyState(InputKey::Right, AppKey::Right);
        setKeyState(InputKey::Up, AppKey::Up);
        setKeyState(InputKey::Down, AppKey::Down);
        setKeyState(InputKey::Shift, static_cast<AppKey>(VK_SHIFT));
        setKeyState(InputKey::Ctrl, static_cast<AppKey>(VK_CONTROL));
        setKeyState(InputKey::Alt, static_cast<AppKey>(VK_MENU));
    }

    if (capabilities.window)
    {
        const bool shouldShowOsCursor = !hideOsCursorForViewport;
        if (capabilities.window->IsCursorVisible() != shouldShowOsCursor)
        {
            capabilities.window->SetCursorVisible(shouldShowOsCursor);
        }
    }

    const float deltaSeconds = capabilities.time ?
        static_cast<float>(capabilities.time->GetDeltaSeconds()) :
        (1.0F / 60.0F);

    if (input.cursorDebug.valid)
    {
        ++m_cursorDebugFrameCounter;
        if ((m_cursorDebugFrameCounter % 6U) == 0U)
        {
            std::ostringstream line;
            line << "OS(" << input.cursorDebug.osScreenX << "," << input.cursorDebug.osScreenY << ") "
                 << "Client(" << input.cursorDebug.clientX << "," << input.cursorDebug.clientY << ") "
                 << "ViewportLocal(" << input.cursorDebug.viewportLocalX << "," << input.cursorDebug.viewportLocalY << ") "
                 << "UV(" << std::fixed << std::setprecision(3) << input.cursorDebug.viewportU << "," << input.cursorDebug.viewportV << ") "
                 << std::defaultfloat << std::setprecision(6)
                 << "Render(" << input.cursorDebug.renderX << "," << input.cursorDebug.renderY << ") "
                 << "DrawCal(" << input.cursorDebug.calibrationDrawX << "," << input.cursorDebug.calibrationDrawY << ") "
                 << "VP(" << m_layoutState.GetViewportRect().x << "," << m_layoutState.GetViewportRect().y << ","
                 << m_layoutState.GetViewportRect().width << "," << m_layoutState.GetViewportRect().height << ") "
                 << "DPI(" << input.cursorDebug.dpiScaleX << "," << input.cursorDebug.dpiScaleY << ") "
                 << "Borders(" << input.cursorDebug.borderLeft << "," << input.cursorDebug.borderTop << ","
                 << input.cursorDebug.borderRight << "," << input.cursorDebug.borderBottom << ")";
            AppendCursorDebugLogLine(line.str());
        }
    }

    m_runtimeHost.Tick(deltaSeconds, input, m_layoutState.GetViewportRect());

    m_cursorDebugConsoleAccumulator += static_cast<double>(deltaSeconds);
    if (m_cursorDebugConsoleAccumulator >= 1.0)
    {
        m_cursorDebugConsoleAccumulator = 0.0;
        std::ostringstream debugLine;
        if (input.cursorDebug.valid)
        {
            debugLine
                << "[CursorCalib] src=OS"
                << " os=(" << input.cursorDebug.osScreenX << "," << input.cursorDebug.osScreenY << ")"
                << " client=(" << input.cursorDebug.clientX << "," << input.cursorDebug.clientY << ")"
                << " local=(" << input.cursorDebug.viewportLocalX << "," << input.cursorDebug.viewportLocalY << ")"
                << " uv=(" << std::fixed << std::setprecision(3) << input.cursorDebug.viewportU << "," << input.cursorDebug.viewportV << ")"
                << std::defaultfloat << std::setprecision(6)
                << " render=(" << input.cursorDebug.renderX << "," << input.cursorDebug.renderY << ")"
                << " vp=(" << m_layoutState.GetViewportRect().x << "," << m_layoutState.GetViewportRect().y
                << "," << m_layoutState.GetViewportRect().width << "," << m_layoutState.GetViewportRect().height << ")"
                << " drawCal=(" << input.cursorDebug.calibrationDrawX << "," << input.cursorDebug.calibrationDrawY << ")"
                << " dpi=(" << input.cursorDebug.dpiScaleX << "," << input.cursorDebug.dpiScaleY << ")";
        }
        else
        {
            debugLine << "[CursorCalib] src=FallbackInput reason=cursorDebug.invalid";
        }

        const std::string line = debugLine.str();
        if (line != m_lastCursorDebugConsoleLine)
        {
            m_lastCursorDebugConsoleLine = line;
            PushConsoleMessage(line);
        }
    }

    m_bridge.SetEscapeShutdownEnabled(m_runtimeHost.GetState() == studio_runtime::StudioRuntimePlayState::Edit);

    PresentRuntimeViewport();
}

void StudioHost::PresentRuntimeViewport()
{
    AppCapabilities& capabilities = m_bridge.GetCapabilities();
    if (!capabilities.nativeUi)
    {
        return;
    }

    const bool playing = m_runtimeHost.GetState() == studio_runtime::StudioRuntimePlayState::Playing;
    const bool paused = m_runtimeHost.GetState() == studio_runtime::StudioRuntimePlayState::Paused;
    const studio::StudioProject project = m_projectSystem.GetCurrentProject();
    std::uint32_t clearColor = (playing || paused) ? 0xFF17110Fu : 0xFF100C0Bu;
    if (m_previewBackgroundAlt)
    {
        clearColor = (playing || paused) ? 0xFF102033u : 0xFF0A1422u;
    }

    NativeUiBuilder ui;
    ui.WindowTitle(L"Engine Studio")
        .OverlayEnabled(false)
        .ClearColor(clearColor);

    if (m_runtimeHost.HasRenderFrame())
    {
        ui.ContentFrame(m_runtimeHost.GetRenderFrame());
    }

    int panelWidth = 420;
    int panelHeight = 112;
    int panelBottomOffset = 24;
    if (capabilities.window)
    {
        const int windowWidth = std::max(0, capabilities.window->GetWindowWidth());
        const int windowHeight = std::max(0, capabilities.window->GetWindowHeight());
        if (windowWidth > 0)
        {
            panelWidth = std::clamp(windowWidth / 3, 340, 560);
        }
        if (windowHeight > 0)
        {
            panelHeight = std::clamp(windowHeight / 7, 96, 168);
            panelBottomOffset = std::clamp(windowHeight / 36, 14, 40);
        }
    }

    ui.Panel("Runtime")
        .Anchor(NativeUiAnchor::BottomCenter, 0, panelBottomOffset, panelWidth, panelHeight)
        .Colors(0xFFFFF2E4u, 0xDD241A18u)
        .Row("Project", project.isOpen ? project.projectName : "none")
        .Row("Runtime", project.isOpen ? project.previewRuntimeName : "none")
        .Row("State", m_runtimeHost.GetStateName())
        .Row("Output", m_runtimeHost.GetViewportText());

    capabilities.nativeUi->Submit(ui.Build());
}

std::string StudioHost::BuildRuntimeViewportDisplayText() const
{
    const studio::StudioProject project = m_projectSystem.GetCurrentProject();
    std::string text = "Studio Runtime Viewport\n";
    text += "State: " + m_runtimeHost.GetStateName() + "\n";
    text += "Project: ";
    text += project.isOpen ? (project.projectName + " (" + project.projectId + ")") : "No project open";
    text += "\n\n";
    text += m_runtimeHost.GetViewportText();
    text += "\n\nPlay starts the runtime. ESC stops it while playing.";
    return text;
}

std::string StudioHost::BuildRuntimeViewportJson() const
{
    return BuildRuntimeActionJsonResponse(
        true,
        "Runtime viewport updated.",
        m_runtimeHost.GetStateName(),
        BuildRuntimeViewportDisplayText());
}

std::string StudioHost::BuildSelectedEntityJson() const
{
    const studio_runtime::EntityId selectedEntity = m_runtimeHost.GetInteraction().GetState().selectedEntity;
    if (selectedEntity == 0)
    {
        return "{\"ok\":true,\"hasSelection\":false}";
    }

    studio_runtime::EntityInfo info;
    if (!m_runtimeHost.GetConnector().Queries().GetEntityInfo(selectedEntity, info))
    {
        return "{\"ok\":true,\"hasSelection\":false}";
    }

    const TransformPrototype& transform = info.transform;
    std::string json = "{\"ok\":true,\"hasSelection\":true";
    json += ",\"id\":" + std::to_string(info.id);
    json += ",\"selectionCount\":" + std::to_string(m_runtimeHost.GetInteraction().GetState().selectedEntities.size());
    json += ",\"name\":\"" + EscapeJson(info.name) + "\"";
    json += ",\"hasObject\":" + std::string(info.hasObject ? "true" : "false");
    if (info.hasObject)
    {
        json += ",\"object\":{";
        json += "\"kind\":\"" + EscapeJson(info.object.kind) + "\"";
        json += ",\"prototype\":\"" + EscapeJson(info.object.prototypeId) + "\"";
        json += ",\"selectable\":" + std::string(info.object.selectable ? "true" : "false");
        json += "}";
    }
    json += ",\"transform\":{";
    json += "\"position\":{\"x\":" + std::to_string(transform.translation.x) + ",\"y\":" + std::to_string(transform.translation.y) + ",\"z\":" + std::to_string(transform.translation.z) + "}";
    json += ",\"rotation\":{\"x\":" + std::to_string(transform.rotationRadians.x) + ",\"y\":" + std::to_string(transform.rotationRadians.y) + ",\"z\":" + std::to_string(transform.rotationRadians.z) + "}";
    json += ",\"scale\":{\"x\":" + std::to_string(transform.scale.x) + ",\"y\":" + std::to_string(transform.scale.y) + ",\"z\":" + std::to_string(transform.scale.z) + "}";
    json += "}";
    json += ",\"hasRenderable\":" + std::string(info.hasRenderable ? "true" : "false");
    if (info.hasRenderable)
    {
        json += ",\"renderable\":{";
        json += "\"mesh\":\"" + std::string(MeshName(info.renderable.mesh)) + "\"";
        json += ",\"material\":\"default\"";
        json += ",\"visible\":" + std::string(info.renderable.visible ? "true" : "false");
        json += "}";
    }
    json += ",\"hasLight\":" + std::string(info.hasLight ? "true" : "false");
    json += "}";
    return json;
}

void StudioHost::RefreshProjectProxyLibrary(const studio::StudioProject& project)
{
    if (!project.isOpen || project.projectRootPath.empty())
    {
        RefreshStudioAssetLibrary();
        return;
    }

    const studio::ProxyLibraryScanResult scanResult = m_proxyLibrary.ScanProjectLibrary(
        project.projectRootPath,
        project.assetProxyFolder);
    if (!scanResult.success)
    {
        PushConsoleMessage("Proxy library scan failed: " + scanResult.message);
        return;
    }

    m_proxyFolderPath = scanResult.proxyFolderPath;
    m_proxyEntries = scanResult.entries;
    m_runtimeHost.ConfigurePrototypeLibrary(m_proxyFolderPath);
    std::string registryError;
    if (!m_proxyLibrary.WriteProjectRegistry(project.projectRootPath, m_proxyEntries, registryError))
    {
        PushConsoleMessage("Proxy registry write warning: " + registryError);
    }
}

void StudioHost::RefreshStudioAssetLibrary()
{
    studio::FileProxy files;
    const std::string studioRoot = "studio";
    std::string error;
    files.CreateDirectory(studioRoot, error);

    const std::string configPath = studioRoot + "/studio.chili.json";
    std::string link = "../StudioAssetLibrary";
    std::string text;
    if (files.Exists(configPath) && files.ReadText(configPath, text, error))
    {
        const std::string saved = ExtractJsonStringField(text, "assetProxyFolder");
        if (!saved.empty())
        {
            link = saved;
        }
    }
    else
    {
        const std::string defaultConfig = "{\n  \"assetProxyFolder\": \"../StudioAssetLibrary\"\n}\n";
        files.WriteText(configPath, defaultConfig, error);
    }

    const studio::ProxyLibraryScanResult scanResult = m_proxyLibrary.ScanProjectLibrary(studioRoot, link);
    if (!scanResult.success)
    {
        PushConsoleMessage("Studio asset library scan failed: " + scanResult.message);
        return;
    }

    m_proxyFolderPath = scanResult.proxyFolderPath;
    m_proxyEntries = scanResult.entries;
    m_runtimeHost.ConfigurePrototypeLibrary(m_proxyFolderPath);
    if (!m_proxyLibrary.WriteProjectRegistry(studioRoot, m_proxyEntries, error))
    {
        PushConsoleMessage("Studio asset registry write warning: " + error);
    }
}

std::string StudioHost::GetStudioAssetLibraryLink() const
{
    studio::FileProxy files;
    std::string text;
    std::string error;
    const std::string configPath = "studio/studio.chili.json";
    if (files.Exists(configPath) && files.ReadText(configPath, text, error))
    {
        const std::string link = ExtractJsonStringField(text, "assetProxyFolder");
        if (!link.empty())
        {
            return link;
        }
    }
    return "../StudioAssetLibrary";
}

bool StudioHost::SetStudioAssetLibraryLink(const std::string& link, std::string& outMessage)
{
    if (link.empty())
    {
        outMessage = "Library link cannot be empty.";
        return false;
    }

    studio::FileProxy files;
    std::string error;
    if (!files.CreateDirectory("studio", error))
    {
        outMessage = error;
        return false;
    }

    const std::string config = std::string("{\n  \"assetProxyFolder\": \"") + EscapeJson(link) + "\"\n}\n";
    if (!files.WriteText("studio/studio.chili.json", config, error))
    {
        outMessage = error;
        return false;
    }

    RefreshStudioAssetLibrary();
    outMessage = "Studio asset library link updated.";
    return true;
}

std::string StudioHost::BuildLibraryEntriesJson() const
{
    std::string json = "{\"ok\":true,\"proxyFolderPath\":\"" + EscapeJson(m_proxyFolderPath) + "\",\"entries\":[";
    for (std::size_t i = 0; i < m_proxyEntries.size(); ++i)
    {
        const studio::ProxyLibraryEntry& entry = m_proxyEntries[i];
        if (i > 0U)
        {
            json += ",";
        }
        json += "{";
        json += "\"id\":\"" + EscapeJson(entry.id) + "\"";
        json += ",\"type\":\"" + EscapeJson(entry.type) + "\"";
        json += ",\"name\":\"" + EscapeJson(entry.name) + "\"";
        json += ",\"sourcePath\":\"" + EscapeJson(entry.sourcePath) + "\"";
        json += ",\"libraryRelativePath\":\"" + EscapeJson(entry.libraryRelativePath) + "\"";
        json += "}";
    }
    json += "]}";
    return json;
}

std::string StudioHost::BuildLibraryEntryJson(const std::string& entryId) const
{
    if (entryId.empty())
    {
        return "{\"ok\":false,\"message\":\"missing id\"}";
    }

    auto found = std::find_if(
        m_proxyEntries.begin(),
        m_proxyEntries.end(),
        [&entryId](const studio::ProxyLibraryEntry& entry) { return entry.id == entryId; });
    if (found == m_proxyEntries.end())
    {
        return "{\"ok\":false,\"message\":\"entry not found\"}";
    }

    studio::ProxyPrototypeInfo proto;
    const bool hasPrototypeInfo =
        found->libraryRelativePath.find("prototypes/") == 0U &&
        m_proxyLibrary.GetPrototypeInfoById(m_proxyFolderPath, entryId, proto);

    std::string json = "{\"ok\":true";
    json += ",\"id\":\"" + EscapeJson(found->id) + "\"";
    json += ",\"type\":\"" + EscapeJson(found->type) + "\"";
    json += ",\"name\":\"" + EscapeJson(found->name) + "\"";
    json += ",\"sourcePath\":\"" + EscapeJson(found->sourcePath) + "\"";
    json += ",\"libraryRelativePath\":\"" + EscapeJson(found->libraryRelativePath) + "\"";
    if (hasPrototypeInfo && proto.found)
    {
        json += ",\"prototype\":{";
        json += "\"id\":\"" + EscapeJson(proto.id) + "\"";
        json += ",\"name\":\"" + EscapeJson(proto.name) + "\"";
        json += ",\"kind\":\"" + EscapeJson(proto.type) + "\"";
        json += ",\"mesh\":\"" + EscapeJson(proto.meshAsset) + "\"";
        json += ",\"material\":\"" + EscapeJson(proto.materialAsset) + "\"";
        json += ",\"light\":\"" + EscapeJson(proto.lightAsset) + "\"";
        json += ",\"components\":[";
        for (std::size_t i = 0; i < proto.components.size(); ++i)
        {
            if (i > 0U) json += ",";
            json += "\"" + EscapeJson(proto.components[i]) + "\"";
        }
        json += "]";
        json += "}";
    }
    json += "}";
    return json;
}

std::string StudioHost::GetDefaultScenePath() const
{
    if (m_bridge.IsInitialized() && m_bridge.GetCapabilities().resources)
    {
        return m_bridge.GetCapabilities().resources->GetAbsolutePath("assets/scenes/default.scene.json");
    }

    return "assets/scenes/default.scene.json";
}

std::string StudioHost::BuildInteractionFeedJson(std::size_t cursor) const
{
    return m_runtimeHost.GetInteraction().BuildFeedJson(cursor);
}

std::string StudioHost::ExecuteConsoleCommand(const std::string& command)
{
    if (command.empty())
    {
        return "Enter a Studio command. Type 'help' for available commands.";
    }

    if (command == "help")
    {
        return "Commands: help, status, project, runtime, play, stop, pause, new, open <project_id>, save, clear";
    }

    if (command == "status")
    {
        const studio::StudioProject project = m_projectSystem.GetCurrentProject();
        return std::string("Studio online. Current project: ") +
            (project.isOpen ? project.projectId : "none") +
            ".";
    }

    if (command == "project")
    {
        const studio::StudioProject project = m_projectSystem.GetCurrentProject();
        if (!project.isOpen)
        {
            return "No project open.";
        }

        return "Project " + project.projectName + " (" + project.projectId + ") at " + project.logicalProjectPath + ".";
    }

    if (command == "runtime")
    {
        return m_runtimeHost.GetStateName() + ": " + m_runtimeHost.GetViewportText();
    }

    if (command == "play")
    {
        std::string error;
        const bool started = m_runtimeHost.Play(MakeRuntimeDescWithSceneOverride(m_projectSystem.GetCurrentProject()), error);
        m_bridge.SetEscapeShutdownEnabled(!started);
        return started ? "Runtime started." : error;
    }

    if (command == "stop")
    {
        m_runtimeHost.Stop();
        m_bridge.SetEscapeShutdownEnabled(true);
        return "Runtime stopped.";
    }

    if (command == "pause")
    {
        m_runtimeHost.TogglePause();
        return "Runtime pause toggled. State = " + m_runtimeHost.GetStateName() + ".";
    }

    if (command == "new")
    {
        return OpenNewProjectDialog() ? "Opened New Project dialog." : "Failed to open New Project dialog.";
    }

    if (command == "save")
    {
        const studio::StudioProject project = m_projectSystem.GetCurrentProject();
        if (!project.isOpen)
        {
            return "No project open to save.";
        }

        studio::SaveProjectRequest request;
        request.projectId = project.projectId;
        request.editorState = "workspace=open\nlast_project=" + project.projectId + "\n";
        const studio::SaveProjectResult result = m_projectSystem.SaveProject(request);
        return result.success ? result.message : result.error;
    }

    const std::string openPrefix = "open ";
    if (command.find(openPrefix) == 0U)
    {
        studio::OpenProjectRequest request;
        request.projectId = command.substr(openPrefix.size());
        const studio::OpenProjectResult result = m_projectSystem.OpenProject(request);
        if (result.success)
        {
            const studio::StudioProject project = m_projectSystem.GetCurrentProject();
            RefreshProjectProxyLibrary(project);
        }
        return result.success ? result.message : result.error;
    }

    return "Unknown command: " + command + ". Type 'help' for available commands.";
}

std::string StudioHost::ShowProjectContextMenu(int screenX, int screenY)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
    {
        return std::string();
    }

    AppendMenuW(menu, MF_STRING, kProjectMenuNewProject, L"New Project");
    AppendMenuW(menu, MF_STRING, kProjectMenuOpenProject, L"Open Project");
    AppendMenuW(menu, MF_STRING, kProjectMenuSaveProject, L"Save Project");
    AppendMenuW(menu, MF_SEPARATOR, 0U, nullptr);
    AppendMenuW(menu, MF_STRING, kProjectMenuBuildApp, L"Build App");
    AppendMenuW(menu, MF_STRING, kProjectMenuPreviewApp, L"Preview App");

    HWND owner = m_bridge.GetNativeWindowHandle();
    if (owner)
    {
        SetForegroundWindow(owner);
    }

    const UINT command = TrackPopupMenu(
        menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        screenX,
        screenY,
        0,
        owner,
        nullptr);
    DestroyMenu(menu);

    switch (command)
    {
    case kProjectMenuNewProject:
        return "new-project";
    case kProjectMenuOpenProject:
        return "open-project";
    case kProjectMenuSaveProject:
        return "save-project";
    case kProjectMenuBuildApp:
        return "build-app";
    case kProjectMenuPreviewApp:
        return "preview-app";
    default:
        return std::string();
    }
}

std::string StudioHost::ShowProjectExplorerContextMenu(int screenX, int screenY, bool canRename)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
    {
        return std::string();
    }

    AppendMenuW(menu, MF_STRING, kProjectExplorerMenuOpen, L"Open");
    AppendMenuW(menu, canRename ? MF_STRING : (MF_STRING | MF_DISABLED | MF_GRAYED), kProjectExplorerMenuRename, L"Rename");
    AppendMenuW(menu, MF_STRING | MF_DISABLED | MF_GRAYED, kProjectExplorerMenuDuplicate, L"Duplicate");
    AppendMenuW(menu, MF_STRING | MF_DISABLED | MF_GRAYED, kProjectExplorerMenuDelete, L"Delete");

    HWND owner = m_bridge.GetNativeWindowHandle();
    if (owner)
    {
        SetForegroundWindow(owner);
    }

    const UINT command = TrackPopupMenu(
        menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        screenX,
        screenY,
        0,
        owner,
        nullptr);
    DestroyMenu(menu);

    switch (command)
    {
    case kProjectExplorerMenuOpen:
        return "open";
    case kProjectExplorerMenuRename:
        return "rename";
    case kProjectExplorerMenuDuplicate:
        return "duplicate";
    case kProjectExplorerMenuDelete:
        return "delete";
    default:
        return std::string();
    }
}

std::string ShowBuildContextMenu(HWND owner, int screenX, int screenY)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
    {
        return std::string();
    }

    AppendMenuW(menu, MF_STRING, kBuildMenuBuildApp, L"Build App");
    AppendMenuW(menu, MF_STRING, kBuildMenuPreviewApp, L"Preview App");

    if (owner)
    {
        SetForegroundWindow(owner);
    }

    const UINT command = TrackPopupMenu(
        menu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        screenX,
        screenY,
        0,
        owner,
        nullptr);
    DestroyMenu(menu);

    switch (command)
    {
    case kBuildMenuBuildApp:
        return "build-app";
    case kBuildMenuPreviewApp:
        return "preview-app";
    default:
        return std::string();
    }
}

studio_runtime::ProjectRuntimeDesc StudioHost::MakeRuntimeDescWithSceneOverride(const studio::StudioProject& project) const
{
    studio_runtime::ProjectRuntimeDesc desc = MakeRuntimeDesc(project);
    if (!m_selectedSceneLogicalPath.empty())
    {
        const std::string projectRoot = project.projectRootPath;
        const std::string prefix = projectRoot.empty() ? std::string() : (projectRoot + "/");
        if (!prefix.empty() && m_selectedSceneLogicalPath.find(prefix) == 0U)
        {
            desc.defaultScenePath = m_selectedSceneLogicalPath.substr(prefix.size());
        }
    }

    return desc;
}

void StudioHost::StartPreviewForSelectedScene()
{
    PushConsoleMessage("Scene selected for edit viewport: " + m_selectedSceneLogicalPath);
}

std::string StudioHost::GetCoreToolsRuntimeRootPath() const
{
    std::string coreToolsEntry = m_bridge.GetCoreToolsContentPath();
    std::replace(coreToolsEntry.begin(), coreToolsEntry.end(), '\\', '/');

    constexpr const char* kLeftBarEntry = "/left-bar/left-bar.html";
    if (EndsWithPath(coreToolsEntry, kLeftBarEntry))
    {
        coreToolsEntry.resize(coreToolsEntry.size() - std::string(kLeftBarEntry).size());
        return coreToolsEntry;
    }

    if (!m_bridge.IsInitialized())
    {
        return "apps/studio/coretools/runtime";
    }

    return m_bridge.GetCapabilities().resources->GetAbsolutePath("apps/studio/coretools/runtime");
}

std::string StudioHost::GetCoreToolsRuntimeContentPath(const std::string& relativePath) const
{
    std::string root = GetCoreToolsRuntimeRootPath();
    std::replace(root.begin(), root.end(), '\\', '/');

    if (root.empty())
    {
        return relativePath;
    }

    if (!relativePath.empty() && relativePath.front() == '/')
    {
        return root + relativePath;
    }

    return root + "/" + relativePath;
}

std::string StudioHost::GetFileManagementDialogContentPath() const
{
    return GetCoreToolsRuntimeContentPath("dialogs/file-dialog.html");
}

std::string StudioHost::GetNewProjectDialogContentPath() const
{
    return GetCoreToolsRuntimeContentPath("dialogs/new-project-dialog.html");
}

std::string StudioHost::GetProjectExplorerContentPath() const
{
    return GetCoreToolsRuntimeContentPath("right-bar/right-bar.html");
}

std::string StudioHost::GetConsoleContentPath() const
{
    return GetCoreToolsRuntimeContentPath("bottom-bar/bottom-bar.html");
}

std::string StudioHost::GetStudioLogFilePath() const
{
    if (!m_bridge.IsInitialized())
    {
        return "User/studio/logs/studio.log";
    }

    return m_bridge.GetCapabilities().resources->GetAbsolutePath("User/studio/logs/studio.log");
}

std::string StudioHost::GetCursorDebugLogFilePath() const
{
    if (!m_bridge.IsInitialized())
    {
        return "User/studio/logs/cursor-debug.log";
    }

    return m_bridge.GetCapabilities().resources->GetAbsolutePath("User/studio/logs/cursor-debug.log");
}

void StudioHost::LoadPersistedConsoleLog()
{
    const std::string logPath = GetStudioLogFilePath();
    std::ifstream stream(logPath);
    if (!stream.is_open())
    {
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty())
        {
            lines.push_back(line);
        }
    }

    constexpr std::size_t kMaxLinesToRestore = 500U;
    const std::size_t start = lines.size() > kMaxLinesToRestore ? (lines.size() - kMaxLinesToRestore) : 0U;
    for (std::size_t index = start; index < lines.size(); ++index)
    {
        m_consoleFeed.push_back(lines[index]);
    }
}

void StudioHost::AppendPersistedConsoleLog(const std::string& line)
{
    const std::string logPath = GetStudioLogFilePath();
    std::error_code error;
    const std::filesystem::path logFilePath(logPath);
    const std::filesystem::path logDirectory = logFilePath.parent_path();
    if (!logDirectory.empty())
    {
        std::filesystem::create_directories(logDirectory, error);
        if (error)
        {
            return;
        }
    }

    std::ofstream stream(logPath, std::ios::app);
    if (!stream.is_open())
    {
        return;
    }

    stream << line << '\n';
}

void StudioHost::AppendCursorDebugLogLine(const std::string& line)
{
    const std::string logPath = GetCursorDebugLogFilePath();
    std::error_code error;
    const std::filesystem::path logFilePath(logPath);
    const std::filesystem::path logDirectory = logFilePath.parent_path();
    if (!logDirectory.empty())
    {
        std::filesystem::create_directories(logDirectory, error);
        if (error)
        {
            return;
        }
    }

    std::ofstream stream(logPath, std::ios::app);
    if (!stream.is_open())
    {
        return;
    }

    stream << line << '\n';
}

void StudioHost::PushConsoleMessage(const std::string& line)
{
    if (!line.empty())
    {
        m_consoleFeed.push_back(line);
        AppendPersistedConsoleLog(line);
    }
}

std::string StudioHost::BuildConsoleFeedJson(std::size_t cursor) const
{
    if (cursor > m_consoleFeed.size())
    {
        cursor = m_consoleFeed.size();
    }

    std::string json = "{\"ok\":true,\"cursor\":" + std::to_string(m_consoleFeed.size()) + ",\"lines\":[";
    for (std::size_t i = cursor; i < m_consoleFeed.size(); ++i)
    {
        if (i > cursor)
        {
            json += ",";
        }

        json += "\"" + EscapeJson(m_consoleFeed[i]) + "\"";
    }

    json += "]}";
    return json;
}
