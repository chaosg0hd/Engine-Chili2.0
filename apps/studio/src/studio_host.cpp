#include "studio_host.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shobjidl.h>

#include "native_ui/native_ui_builder.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr int kStudioTopBarHeight = 76;
    constexpr int kCoreToolsDockWidth = 128;
    constexpr int kProjectExplorerDockWidth = 300;
    constexpr unsigned short kStudioHttpPort = 37620;
    constexpr UINT_PTR kProjectExplorerMenuOpen = 1001U;
    constexpr UINT_PTR kProjectExplorerMenuRename = 1002U;
    constexpr UINT_PTR kProjectExplorerMenuDuplicate = 1003U;
    constexpr UINT_PTR kProjectExplorerMenuDelete = 1004U;
    constexpr UINT_PTR kProjectMenuNewProject = 2001U;
    constexpr UINT_PTR kProjectMenuOpenProject = 2002U;
    constexpr UINT_PTR kProjectMenuSaveProject = 2003U;

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
            "\"src/" + EscapeJson(result.projectId) + ".hpp\"," +
            "\"src/" + EscapeJson(result.projectId) + ".cpp\"," +
            "\"scenes/main.scene\"," +
            "\"config/game.config\"" +
            "]}";
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
        desc.runtimeName = project.runtimeName;
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
}

bool StudioHost::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

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

    m_commandRouter.Bind(&m_bridge);
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

    m_projectExplorerPanel.Close(m_bridge.GetCapabilities());
    m_consolePanel.Close(m_bridge.GetCapabilities());
    m_newProjectDialog.Close(m_bridge.GetCapabilities());
    m_fileManagementDialog.Close(m_bridge.GetCapabilities());
    m_httpServer.Stop(m_bridge);

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
    config.indexFilePath = "topbar/studio-top.html";

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
    dialogDesc.dockSize = kStudioTopBarHeight;
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
    dialogDesc.dockSize = kCoreToolsDockWidth;
    dialogDesc.dockInsetTop = kStudioTopBarHeight;
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
        kStudioTopBarHeight,
        kProjectExplorerDockWidth);
}

bool StudioHost::InitializeConsolePanel()
{
    return m_consolePanel.Open(
        m_bridge.GetCapabilities(),
        GetConsoleContentPath(),
        kCoreToolsDockWidth,
        kProjectExplorerDockWidth);
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

    if (route == "/studio/console/execute")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto command = query.find("command");
        const std::string commandText = command == query.end() ? std::string() : command->second;
        outBody = commandText == "clear" ?
            BuildConsoleJsonResponse(true, "Console cleared.", true) :
            BuildConsoleJsonResponse(true, ExecuteConsoleCommand(commandText));
        return true;
    }

    if (route == "/studio/runtime/play")
    {
        std::string error;
        const bool started = m_runtimeHost.Play(MakeRuntimeDesc(m_projectSystem.GetCurrentProject()), error);
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

    if (route == "/studio/runtime/viewport")
    {
        outBody = BuildRuntimeViewportJson();
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
    }

    return result;
}

void StudioHost::TickRuntime()
{
    AppCapabilities& capabilities = m_bridge.GetCapabilities();

    studio_runtime::RuntimeInput input;
    if (capabilities.input)
    {
        input.escapePressed = capabilities.input->WasKeyPressed(static_cast<AppKey>(VK_ESCAPE));
        input.servePressed = capabilities.input->WasKeyPressed(AppKey::Space);
        input.resetPressed = capabilities.input->WasKeyPressed(AppKey::R);
        input.leftUpDown = capabilities.input->IsKeyDown(AppKey::W);
        input.leftDownDown = capabilities.input->IsKeyDown(AppKey::S);
        input.rightUpDown = capabilities.input->IsKeyDown(AppKey::Up);
        input.rightDownDown = capabilities.input->IsKeyDown(AppKey::Down);
    }

    const float deltaSeconds = capabilities.time ?
        static_cast<float>(capabilities.time->GetDeltaSeconds()) :
        (1.0F / 60.0F);

    m_runtimeHost.Tick(deltaSeconds, input);
    if (m_runtimeHost.GetState() == studio_runtime::StudioRuntimePlayState::Stopped)
    {
        m_bridge.SetEscapeShutdownEnabled(true);
    }

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

    NativeUiBuilder ui;
    ui.WindowTitle(L"Engine Studio")
        .OverlayEnabled(false)
        .ClearColor(playing || paused ? 0xFF17110Fu : 0xFF100C0Bu);

    if (m_runtimeHost.HasRenderFrame())
    {
        ui.ContentFrame(m_runtimeHost.GetRenderFrame());
    }

    ui.Panel("Runtime")
        .Anchor(NativeUiAnchor::BottomCenter, 0, 190, 420, 112)
        .Colors(0xFFFFF2E4u, 0xDD241A18u)
        .Row("Project", project.isOpen ? project.projectName : "none")
        .Row("Runtime", project.isOpen ? project.runtimeName : "none")
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
        const bool started = m_runtimeHost.Play(MakeRuntimeDesc(m_projectSystem.GetCurrentProject()), error);
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

std::string StudioHost::GetCoreToolsRuntimeRootPath() const
{
    std::string coreToolsEntry = m_bridge.GetCoreToolsContentPath();
    std::replace(coreToolsEntry.begin(), coreToolsEntry.end(), '\\', '/');

    constexpr const char* kShellEntry = "/shell/index.html";
    if (EndsWithPath(coreToolsEntry, kShellEntry))
    {
        coreToolsEntry.resize(coreToolsEntry.size() - std::string(kShellEntry).size());
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
    return GetCoreToolsRuntimeContentPath("file/file-management.html");
}

std::string StudioHost::GetNewProjectDialogContentPath() const
{
    return GetCoreToolsRuntimeContentPath("file/new-project.html");
}

std::string StudioHost::GetProjectExplorerContentPath() const
{
    return GetCoreToolsRuntimeContentPath("panels/project-explorer/project-explorer.html");
}

std::string StudioHost::GetConsoleContentPath() const
{
    return GetCoreToolsRuntimeContentPath("bottom/console.html");
}
