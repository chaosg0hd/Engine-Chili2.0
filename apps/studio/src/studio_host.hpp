#pragma once

#include "bridge/engine_bridge.hpp"
#include "commands/command_router.hpp"
#include "runtime/studio_runtime_host.hpp"
#include "studio/file_management_dialog.hpp"
#include "studio/new_project_dialog.hpp"
#include "studio/project_explorer_panel.hpp"
#include "studio/studio_console_panel.hpp"
#include "studio/studio_file_actions.hpp"
#include "transport/http_server.hpp"

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
    bool HandleStudioHttpRequest(const std::string& path, std::string& outContentType, std::string& outBody);
    void TickRuntime();
    void PresentRuntimeViewport();
    std::string BuildRuntimeViewportDisplayText() const;
    std::string BuildRuntimeViewportJson() const;
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

private:
    EngineBridge m_bridge;
    CommandRouter m_commandRouter;
    HttpServer m_httpServer;
    studio::FileManagementDialog m_fileManagementDialog;
    studio::NewProjectDialog m_newProjectDialog;
    studio::ProjectExplorerPanel m_projectExplorerPanel;
    studio::StudioConsolePanel m_consolePanel;
    studio::StudioFileActions m_fileActions;
    studio::StudioProjectSystem m_projectSystem;
    studio_runtime::StudioRuntimeHost m_runtimeHost;
    EngineCore::WebDialogHandle m_topBarDialogHandle = 0U;
    EngineCore::WebDialogHandle m_coreToolsDialogHandle = 0U;
    bool m_initialized = false;
};
