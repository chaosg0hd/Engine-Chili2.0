#pragma once

#include "bridge/engine_bridge.hpp"
#include "commands/command_router.hpp"
#include "runtime/studio_runtime_host.hpp"
#include "studio/file_management_dialog.hpp"
#include "studio/new_project_dialog.hpp"
#include "studio/proxy_library.hpp"
#include "studio/project_explorer_panel.hpp"
#include "studio/studio_layout_state.hpp"
#include "studio/studio_console_panel.hpp"
#include "studio/studio_build_system.hpp"
#include "studio/studio_file_actions.hpp"
#include "transport/http_server.hpp"

#include <string>
#include <vector>

class StudioHost
{
public:
    bool Initialize();
    void Run();
    void Shutdown();

private:
    void LogStudioShellStatus();
    bool InitializeStudioHttpBridge();
    bool InitializeTopBarDialog();
    bool InitializeCoreToolsDialog();
    bool InitializeProjectExplorerPanel();
    bool InitializeConsolePanel();
    bool InitializeFrameGizmoButton();
    bool HandleStudioHttpRequest(const std::string& path, std::string& outContentType, std::string& outBody);
    void TickRuntime();
    void UpdateLayoutState();
    void UpdateFrameGizmoButtonLayout();
    void PresentRuntimeViewport();
    std::string BuildRuntimeViewportDisplayText() const;
    std::string BuildRuntimeViewportJson() const;
    std::string BuildSelectedEntityJson() const;
    std::string BuildInteractionFeedJson(std::size_t cursor) const;
    std::string GetDefaultScenePath() const;
    std::string ExecuteConsoleCommand(const std::string& command);
    bool OpenFileManagementDialog();
    bool OpenNewProjectDialog();
    studio::OpenProjectResult OpenProjectFromFolderPicker();
    std::string ShowProjectContextMenu(int screenX, int screenY);
    std::string ShowProjectExplorerContextMenu(int screenX, int screenY, bool canRename);
    std::string GetCoreToolsRuntimeRootPath() const;
    std::string GetCoreToolsRuntimeContentPath(const std::string& relativePath) const;
    std::string GetFileManagementDialogContentPath() const;
    std::string GetNewProjectDialogContentPath() const;
    std::string GetProjectExplorerContentPath() const;
    std::string GetConsoleContentPath() const;
    std::string GetStudioLogFilePath() const;
    std::string GetCursorDebugLogFilePath() const;
    void LoadPersistedConsoleLog();
    void AppendPersistedConsoleLog(const std::string& line);
    void AppendCursorDebugLogLine(const std::string& line);
    void PushConsoleMessage(const std::string& line);
    std::string BuildConsoleFeedJson(std::size_t cursor) const;
    void RefreshProjectProxyLibrary(const studio::StudioProject& project);
    void RefreshStudioAssetLibrary();
    std::string GetStudioAssetLibraryLink() const;
    bool SetStudioAssetLibraryLink(const std::string& link, std::string& outMessage);
    std::string BuildLibraryEntriesJson() const;
    std::string BuildLibraryEntryJson(const std::string& entryId) const;
    studio_runtime::ProjectRuntimeDesc MakeRuntimeDescWithSceneOverride(const studio::StudioProject& project) const;
    void StartPreviewForSelectedScene();

private:
    EngineBridge m_bridge;
    CommandRouter m_commandRouter;
    HttpServer m_httpServer;
    studio::FileManagementDialog m_fileManagementDialog;
    studio::NewProjectDialog m_newProjectDialog;
    studio::ProjectExplorerPanel m_projectExplorerPanel;
    studio::StudioLayoutState m_layoutState;
    studio::StudioConsolePanel m_consolePanel;
    studio::StudioBuildSystem m_buildSystem;
    studio::ProxyLibrary m_proxyLibrary;
    studio::StudioFileActions m_fileActions;
    studio::StudioProjectSystem m_projectSystem;
    studio_runtime::StudioRuntimeHost m_runtimeHost;
    std::vector<std::string> m_consoleFeed;
    std::string m_proxyFolderPath;
    std::vector<studio::ProxyLibraryEntry> m_proxyEntries;
    std::string m_selectedSceneLogicalPath;
    IAppUi::NativeButtonHandle m_frameGizmoButtonHandle = 0U;
    bool m_previewBackgroundAlt = false;
    bool m_hasLoggedLayout = false;
    int m_lastLayoutWindowWidth = -1;
    int m_lastLayoutWindowHeight = -1;
    ViewportRect m_lastLayoutViewportRect{};
    EngineCore::WebDialogHandle m_topBarDialogHandle = 0U;
    EngineCore::WebDialogHandle m_coreToolsDialogHandle = 0U;
    std::size_t m_cursorDebugFrameCounter = 0U;
    double m_cursorDebugConsoleAccumulator = 0.0;
    std::string m_lastCursorDebugConsoleLine;
    int m_lastCalibratedMouseX = 0;
    int m_lastCalibratedMouseY = 0;
    bool m_hasLastCalibratedMouse = false;
    bool m_forceOsScreenMouseOverride = false;
    bool m_shouldDrawViewportCursor = false;
    int m_viewportCursorX = 0;
    int m_viewportCursorY = 0;
    bool m_initialized = false;
};
