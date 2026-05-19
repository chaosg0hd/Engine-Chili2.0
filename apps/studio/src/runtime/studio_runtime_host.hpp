#pragma once

#include "runtime/runtime_game_api.hpp"
#include "runtime/proxy_prototype_resolver.hpp"
#include "runtime/runtime_registry.hpp"
#include "runtime/studio_connector.hpp"
#include "runtime/studio_interaction_controller.hpp"
#include "runtime/studio_picking_service.hpp"
#include "modules/render/render_types.hpp"
#include "input/input_system.h"
#include "prototypes/entity/scene/origin_marker.hpp"
#include "prototypes/snap/snap_debug.hpp"
#include "prototypes/snap/snap_marker.hpp"
#include "prototypes/snap/snap_types.hpp"
#include "engine/script/script_context.hpp"
#include "engine/script/script_instance.hpp"
#include "engine/script/script_module.hpp"
#include "app/app_capabilities.hpp"

#include <memory>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace studio_runtime
{
    enum class StudioRuntimePlayState
    {
        Edit,
        Playing,
        Paused,
        Step
    };

    class StudioRuntimeHost
    {
    public:
        StudioRuntimeHost();
        ~StudioRuntimeHost();

        bool Initialize(const std::string& scenePath, std::string& outError);
        bool SaveScene(const std::string& scenePath, std::string& outError) const;
        bool Play(const ProjectRuntimeDesc& project, std::string& outError);
        void Stop();
        void TogglePause();
        void StepOnce();
        void Tick(float deltaSeconds, const RuntimeInput& input, const ViewportRect& viewportRect);

        StudioRuntimePlayState GetState() const;
        std::string GetStateName() const;
        const std::string& GetViewportText() const;
        std::string GetFpsText() const;
        const FramePrototype& GetRenderFrame() const;
        bool HasRenderFrame() const;
        const ProjectRuntimeDesc& GetActiveProject() const;
        StudioConnector& GetConnector();
        const StudioConnector& GetConnector() const;
        StudioInteractionController& GetInteraction();
        const StudioInteractionController& GetInteraction() const;
        const RuntimeWorld& GetWorld() const;
        void SetActiveTool(StudioTool tool);
        void ConfigurePrototypeLibrary(const std::string& proxyFolderPath);
        bool CreateInstanceFromPrototype(const std::string& prototypeId, EntityId& outEntityId, std::string& outError);
        bool DestroyEntity(EntityId entityId, std::string& outError);
        bool RenameEntity(EntityId entityId, const std::string& name, std::string& outError);
        bool DuplicateEntity(EntityId entityId, EntityId& outNewId, std::string& outError);
        bool ActivateCameraEntity(EntityId entityId, std::string& outCameraName, std::string& outError);
        bool SwapToNextCamera(std::string& outCameraName, std::string& outError);
        std::string GetActiveCameraName() const;
        void SetGridVisible(bool visible);
        bool IsGridVisible() const;
        void SetSnapDebugVisible(bool visible);
        bool IsSnapDebugVisible() const;
        void SetCameraGizmoVisible(bool visible);
        bool IsCameraGizmoVisible() const;

        void SetSceneRenderSettings(const SceneRenderSettings& settings);
        const SceneRenderSettings& GetSceneRenderSettings() const;
        void ApplyRenderConfigurationPreset(RenderConfigurationPreset preset);
        void ApplyLowRenderConfiguration();
        void SetViewRenderMode(ViewRenderMode mode);
        ViewRenderMode GetViewRenderMode() const;
        void SetScriptLogSink(std::function<void(const std::string&)> sink);

        // Scene transition signals routed from script events ("goto_scene:N", "exit").
        // Game hosts call these each frame to drive scene switching.
        void SetInputInterface(IAppInput* input);
        int  ConsumeSceneTransition(); // returns -1 if none pending
        bool ConsumeExitRequest();

    private:
        void ConfigureInputContexts();
        void RegisterStudioBehaviors();
        std::string CreateFallbackScene();
        void RebuildScriptInstances();
        void DestroyAllScriptInstances();
        void DestroyScriptInstance(EntityId entityId);
        void UpdateScripts(float deltaSeconds);
        engine::script::ScriptContext BuildScriptContext(EntityId entityId);
        void RefreshSceneCameraBinding();
        void ApplyCameraEntityToView(EntityId entityId);
        void SyncActiveCameraEntityFromView();
        void HandleEditInput(const RuntimeInput& input, const ViewportRect& viewportRect);
        void StartMoveManipulation(const RuntimeInput& input, const ViewportRect& viewportRect);
        void UpdateMoveManipulation(const RuntimeInput& input, const ViewportRect& viewportRect);
        void EndMoveManipulation();
        void UpdateSnapDebug(const RuntimeInput& input, const ViewportRect& viewportRect);
        FramePrototype BuildWorldFrame() const;

    private:
        struct MoveManipulationState
        {
            bool active = false;
            EntityId entityId = 0;
            Vector3 startEntityTranslation = Vector3(0.0f, 0.0f, 0.0f);
            Vector3 grabSnapPosition = Vector3(0.0f, 0.0f, 0.0f);
            Vector3 planeNormal = Vector3(0.0f, 0.0f, 1.0f);
            Vector3 startPlaneIntersection = Vector3(0.0f, 0.0f, 0.0f);
        };

    private:
        int  m_pendingSceneTransition = -1;
        bool m_pendingExit            = false;

        RuntimeRegistry m_registry;
        std::unique_ptr<IGameRuntime> m_runtime;
        RuntimeWorld m_world;
        RuntimeWorld m_editSnapshot;
        StudioCamera m_studioCamera;
        StudioConnector m_connector;
        StudioPickingService m_pickingService;
        StudioInteractionController m_interaction;
        ProxyPrototypeResolver m_prototypeResolver;
        InputSystem m_inputSystem;
        engine::script::ScriptModule m_scriptModule;
        std::unordered_map<EntityId, engine::script::ScriptInstance> m_scriptInstances;
        std::function<void(const std::string&)> m_scriptLogSink;
        ProjectRuntimeDesc m_activeProject;
        FramePrototype m_renderFrame;
        OriginMarkerPrototype m_originMarker;
        SnapMarkerPrototype m_snapMarker;
        SnapDebugReport m_snapDebugReport;
        SnapResult m_snapDebugResult;
        MoveManipulationState m_moveManipulation;
        StudioRuntimePlayState m_state = StudioRuntimePlayState::Edit;
        std::string m_viewportText = "Edit Mode.";
        double m_fpsAccumulatedSeconds = 0.0;
        std::uint32_t m_fpsAccumulatedFrames = 0U;
        float m_currentFramesPerSecond = 0.0f;
        bool m_gridVisible = true;
        bool m_snapDebugVisible = true;
        bool m_cameraGizmoVisible = true;

        bool m_hasRenderFrame = true;
        ViewRenderMode m_viewRenderMode = ViewRenderMode::Shaded;
        EntityId m_activeCameraEntityId = 0;
    };
}
