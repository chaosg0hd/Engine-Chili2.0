#include "runtime/studio_runtime_host.hpp"

#include "pong_ball_script.hpp"
#include "pong_paddle_script.hpp"
#include "scene_manager_script.hpp"
#include "runtime/scene_serializer.hpp"
#include "runtime/studio_default_scene_template.hpp"

#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"
#include "prototypes/presentation/view.hpp"
#include "prototypes/entity/appearance/color.hpp"
#include "prototypes/snap/snap_resolver.hpp"
#include "prototypes/snap/snap_settings.hpp"
#include "input/input_binding.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace studio_runtime
{
    namespace
    {
        bool HasAuthoredVisibilityIntent(const LightPrototype& light)
        {
            return light.visibility.kind != LightVisibilityKind::None || light.visibility.policy.enabled;
        }

        void ApplyLightVisibilityPreset(LightPrototype& light, RenderConfigurationPreset preset)
        {
            const bool hadVisibilityIntent = HasAuthoredVisibilityIntent(light);
            if (preset == RenderConfigurationPreset::Low)
            {
                light.visibility.kind = LightVisibilityKind::None;
                light.visibility.policy.enabled = false;
                light.visibility.policy.method = LightVisibilityMethodPrototype::None;
                light.visibility.policy.resolution = 128U;
                return;
            }

            if (!hadVisibilityIntent)
            {
                return;
            }

            light.visibility.kind = LightVisibilityKind::RasterShadowCubemap;
            light.visibility.policy.enabled = true;
            light.visibility.policy.method = LightVisibilityMethodPrototype::RasterCubemap;
            light.visibility.policy.resolution =
                preset == RenderConfigurationPreset::High ? 512U : 256U;
        }

        BuiltInMeshKind MeshKindFromAssetId(const std::string& assetId)
        {
            if (assetId == "builtin:cube") return BuiltInMeshKind::Cube;
            if (assetId == "builtin:quad") return BuiltInMeshKind::Quad;
            if (assetId == "builtin:octahedron") return BuiltInMeshKind::Octahedron;
            if (assetId == "builtin:diamond") return BuiltInMeshKind::Diamond;
            if (assetId == "builtin:triangle") return BuiltInMeshKind::Triangle;
            return BuiltInMeshKind::None;
        }

        bool TryIntersectRayWithPlane(
            const Ray& ray,
            const Vector3& pointOnPlane,
            const Vector3& planeNormal,
            Vector3& outPoint)
        {
            const Vector3 normal = Normalize(planeNormal);
            const float denominator = Dot(ray.direction, normal);
            if (std::abs(denominator) <= 0.000001f)
            {
                return false;
            }

            const float distance = Dot(pointOnPlane - ray.origin, normal) / denominator;
            if (distance < 0.0f)
            {
                return false;
            }

            outPoint = ray.origin + (ray.direction * distance);
            return true;
        }
    }

    StudioRuntimeHost::StudioRuntimeHost()
        : m_connector(m_world, m_studioCamera)
        , m_pickingService(m_world, m_studioCamera)
    {
        ConfigureInputContexts();
        RegisterStudioBehaviors();
    }

    StudioRuntimeHost::~StudioRuntimeHost()
    {
        DestroyAllScriptInstances();
    }

    void StudioRuntimeHost::ConfigureInputContexts()
    {
        InputContext& studio = m_inputSystem.RegisterContext("Studio", 100);
        studio.BindAction("Select", BindMouseButton(InputMouseButton::Left, false, false, false, true));
        studio.BindAction("MultiSelect", BindMouseButton(InputMouseButton::Left, false, true, false, true));
        studio.BindAction("OrbitCamera", BindMouseButton(InputMouseButton::Middle, false, false, false, true));
        studio.BindAction("PanCamera", BindMouseButton(InputMouseButton::Middle, false, true, false, true));
        studio.BindAction("ZoomCamera", BindMouseWheel(false, false, false, true));
        studio.BindAction("FlyCamera", BindMouseButton(InputMouseButton::Right, false, false, false, true));
        studio.BindAction("FlyForward", BindKey(InputKey::W));
        studio.BindAction("FlyBackward", BindKey(InputKey::S));
        studio.BindAction("FlyLeft", BindKey(InputKey::A));
        studio.BindAction("FlyRight", BindKey(InputKey::D));
        studio.BindAction("FlyUp", BindKey(InputKey::E));
        studio.BindAction("FlyDown", BindKey(InputKey::Q));
        studio.BindAction("FlyBoost", BindKey(InputKey::Shift));
        studio.BindAction("FocusSelection", BindKey(InputKey::F));

        InputContext& gameSample = m_inputSystem.RegisterContext("GameSample", 10);
        gameSample.BindAction("MoveForward", BindKey(InputKey::W));
        gameSample.BindAction("MoveBackward", BindKey(InputKey::S));
        gameSample.BindAction("MoveLeft", BindKey(InputKey::A));
        gameSample.BindAction("MoveRight", BindKey(InputKey::D));
        gameSample.BindAction("Jump", BindKey(InputKey::Space));
        gameSample.BindAction("Fire", BindMouseButton(InputMouseButton::Left));
        gameSample.BindAction("Aim", BindMouseButton(InputMouseButton::Right));
    }

    bool StudioRuntimeHost::Initialize(const std::string& scenePath, std::string& outError)
    {
        DestroyAllScriptInstances();
        m_world.Clear();

        if (!SceneSerializer::LoadFromFile(scenePath, m_world, outError))
        {
            const std::string templateReport = CreateFallbackScene();
            if (!templateReport.empty())
            {
                if (!outError.empty())
                {
                    outError += " | ";
                }
                outError += templateReport;
            }
        }

        RefreshSceneCameraBinding();

        m_renderFrame = BuildWorldFrame();
        m_hasRenderFrame = true;
        m_state = StudioRuntimePlayState::Edit;
        m_interaction.SetRuntimeMode(RuntimeMode::Edit);
        m_viewportText = "Edit Mode.";
        return true;
    }

    bool StudioRuntimeHost::SaveScene(const std::string& scenePath, std::string& outError) const
    {
        return SceneSerializer::SaveToFile(scenePath, m_world, outError);
    }

    bool StudioRuntimeHost::Play(const ProjectRuntimeDesc& project, std::string& outError)
    {
        if (m_state == StudioRuntimePlayState::Playing || m_state == StudioRuntimePlayState::Paused)
        {
            return true;
        }

        if (project.codeEntryKind == ProjectCodeEntryKind::NativeArtifact ||
            !project.exportedArtifactPath.empty())
        {
            // TEMPORARY: artifact-based preview (launcher→engine.dll→app DLL) is not yet implemented.
            // Long-term owner: project build system; artifact path set by the build pipeline.
            // Deferral reason: DLL hot-load stack not yet wired up.
            outError = "Artifact-based preview launch is not yet implemented. exportedArtifactPath='" + project.exportedArtifactPath + "'";
            return false;
        }

        if (project.codeEntryKind == ProjectCodeEntryKind::LuaScript)
        {
            outError = "Lua script project entry is not yet implemented. scriptEntryPath='" + project.scriptEntryPath + "'";
            return false;
        }

        if (project.codeEntryKind == ProjectCodeEntryKind::ExternalAdapter)
        {
            outError = "External adapter project entry is not yet implemented. adapterExecutablePath='" + project.adapterExecutablePath + "'";
            return false;
        }

        if (project.previewRuntimeName.empty())
        {
            outError = "Project has no preview runtime configured. Set previewRuntimeName (in-process) or exportedArtifactPath (artifact).";
            return false;
        }

        m_editSnapshot = m_world;
        DestroyAllScriptInstances();
        RebuildScriptInstances();
        m_activeProject = project;

        // TEMPORARY: in-process runtime lookup. See RuntimeRegistry for ownership contract.
        m_runtime = m_registry.Create(project.previewRuntimeName);
        if (!m_runtime)
        {
            outError = "Runtime '" + project.previewRuntimeName + "' is not registered. Preview cannot start.";
            m_activeProject = ProjectRuntimeDesc{};
            return false;
        }
        m_runtime->BeginPlay(project);

        m_state = StudioRuntimePlayState::Playing;
        m_interaction.SetRuntimeMode(RuntimeMode::Play);
        m_viewportText = "Play Mode.";
        m_renderFrame = BuildWorldFrame();
        m_hasRenderFrame = true;
        return true;
    }

    void StudioRuntimeHost::Stop()
    {
        DestroyAllScriptInstances();

        if (m_runtime)
        {
            m_runtime->EndPlay();
        }

        m_runtime.reset();
        if (m_state == StudioRuntimePlayState::Playing || m_state == StudioRuntimePlayState::Paused || m_state == StudioRuntimePlayState::Step)
        {
            m_world = m_editSnapshot;
            RefreshSceneCameraBinding();
        }
        m_state = StudioRuntimePlayState::Edit;
        m_interaction.SetRuntimeMode(RuntimeMode::Edit);
        m_viewportText = "Edit Mode.";
        m_renderFrame = BuildWorldFrame();
        m_hasRenderFrame = true;
        m_activeProject = ProjectRuntimeDesc{};
    }

    void StudioRuntimeHost::TogglePause()
    {
        if (m_state == StudioRuntimePlayState::Playing)
        {
            m_state = StudioRuntimePlayState::Paused;
            m_interaction.SetRuntimeMode(RuntimeMode::Pause);
            m_viewportText = "Runtime paused.";
            return;
        }

        if (m_state == StudioRuntimePlayState::Paused)
        {
            m_state = StudioRuntimePlayState::Playing;
            m_interaction.SetRuntimeMode(RuntimeMode::Play);
            m_viewportText = "Play Mode.";
        }
    }

    void StudioRuntimeHost::StepOnce()
    {
        if (m_state == StudioRuntimePlayState::Paused)
        {
            m_state = StudioRuntimePlayState::Step;
            m_interaction.SetRuntimeMode(RuntimeMode::Step);
        }
    }

    void StudioRuntimeHost::Tick(
        float deltaSeconds,
        const RuntimeInput& input,
        const ViewportRect& viewportRect)
    {
        if (deltaSeconds > 0.0f)
        {
            m_fpsAccumulatedSeconds += static_cast<double>(deltaSeconds);
            ++m_fpsAccumulatedFrames;
            if (m_fpsAccumulatedSeconds >= 0.25)
            {
                m_currentFramesPerSecond = static_cast<float>(
                    static_cast<double>(m_fpsAccumulatedFrames) / m_fpsAccumulatedSeconds);
                m_fpsAccumulatedSeconds = 0.0;
                m_fpsAccumulatedFrames = 0U;
            }
        }

        m_studioCamera.GetCamera().projection.aspectRatioOverride = viewportRect.Aspect();

        if (m_state == StudioRuntimePlayState::Edit)
        {
            HandleEditInput(input, viewportRect);
            SyncActiveCameraEntityFromView();
            m_renderFrame = BuildWorldFrame();
            m_hasRenderFrame = true;
            return;
        }

        if (m_state == StudioRuntimePlayState::Paused)
        {
            UpdateSnapDebug(input, viewportRect);
            SyncActiveCameraEntityFromView();
            m_renderFrame = BuildWorldFrame();
            m_hasRenderFrame = true;
            return;
        }

        if (m_runtime)
        {
            RuntimeFrame frame;
            m_runtime->Tick(deltaSeconds, input, frame);
            m_viewportText = frame.textOutput.empty() ? "Play Mode." : frame.textOutput;

            if (frame.exitRequested)
            {
                Stop();
                return;
            }
        }

        UpdateScripts(deltaSeconds);
        UpdateSnapDebug(input, viewportRect);
        SyncActiveCameraEntityFromView();

        m_renderFrame = BuildWorldFrame();
        m_hasRenderFrame = true;
        if (m_state == StudioRuntimePlayState::Step)
        {
            m_state = StudioRuntimePlayState::Paused;
            m_interaction.SetRuntimeMode(RuntimeMode::Pause);
            m_viewportText = "Runtime paused.";
        }
    }

    StudioRuntimePlayState StudioRuntimeHost::GetState() const
    {
        return m_state;
    }

    std::string StudioRuntimeHost::GetStateName() const
    {
        switch (m_state)
        {
        case StudioRuntimePlayState::Playing:
            return "Play";
        case StudioRuntimePlayState::Paused:
            return "Pause";
        case StudioRuntimePlayState::Step:
            return "Step";
        case StudioRuntimePlayState::Edit:
        default:
            return "Edit";
        }
    }

    const std::string& StudioRuntimeHost::GetViewportText() const
    {
        return m_viewportText;
    }

    std::string StudioRuntimeHost::GetFpsText() const
    {
        std::ostringstream text;
        text << std::fixed << std::setprecision(1) << m_currentFramesPerSecond;
        return text.str();
    }

    const FramePrototype& StudioRuntimeHost::GetRenderFrame() const
    {
        return m_renderFrame;
    }

    bool StudioRuntimeHost::HasRenderFrame() const
    {
        return m_hasRenderFrame;
    }

    const ProjectRuntimeDesc& StudioRuntimeHost::GetActiveProject() const
    {
        return m_activeProject;
    }

    StudioConnector& StudioRuntimeHost::GetConnector()
    {
        return m_connector;
    }

    const StudioConnector& StudioRuntimeHost::GetConnector() const
    {
        return m_connector;
    }

    const RuntimeWorld& StudioRuntimeHost::GetWorld() const
    {
        return m_world;
    }

    StudioInteractionController& StudioRuntimeHost::GetInteraction()
    {
        return m_interaction;
    }

    const StudioInteractionController& StudioRuntimeHost::GetInteraction() const
    {
        return m_interaction;
    }

    void StudioRuntimeHost::SetActiveTool(StudioTool tool)
    {
        m_interaction.SetActiveTool(tool);
    }

    void StudioRuntimeHost::RegisterStudioBehaviors()
    {
        engine::script::ScriptPrototype ball;
        ball.SetName("script.pong_ball").SetFactory(&pong::CreatePongBallScript);
        m_scriptModule.Registry().Register(ball);

        engine::script::ScriptPrototype paddle;
        paddle.SetName("script.pong_paddle").SetFactory(&pong::CreatePongPaddleScript);
        m_scriptModule.Registry().Register(paddle);

        engine::script::ScriptPrototype sceneManager;
        sceneManager.SetName("script.scene_manager").SetFactory(&pong::CreateSceneManagerScript);
        m_scriptModule.Registry().Register(sceneManager);
    }

    void StudioRuntimeHost::SetInputInterface(IAppInput* input)
    {
        pong::SetSceneManagerInput(input);
    }

    int StudioRuntimeHost::ConsumeSceneTransition()
    {
        const int t = m_pendingSceneTransition;
        m_pendingSceneTransition = -1;
        return t;
    }

    bool StudioRuntimeHost::ConsumeExitRequest()
    {
        const bool e = m_pendingExit;
        m_pendingExit = false;
        return e;
    }

    void StudioRuntimeHost::ConfigurePrototypeLibrary(const std::string& proxyFolderPath)
    {
        m_scriptModule.LoadFromProxyFolder(proxyFolderPath);
        RegisterStudioBehaviors();

        if (proxyFolderPath.empty())
        {
            m_prototypeResolver.Clear();
            return;
        }

        if (!m_prototypeResolver.LoadFromProxyFolder(proxyFolderPath))
        {
            // Keep runtime behavior stable even when a project library is missing or incomplete.
            // Scene/runtime fallback behavior remains owned by the runtime host and serializer.
            m_prototypeResolver.Clear();
        }
    }

    bool StudioRuntimeHost::CreateInstanceFromPrototype(
        const std::string& prototypeId,
        EntityId& outEntityId,
        std::string& outError)
    {
        outEntityId = 0;
        outError.clear();

        if (prototypeId.empty())
        {
            outError = "Prototype id is empty.";
            return false;
        }

        ResolvedPrototype prototype;
        if (!m_prototypeResolver.TryResolve(prototypeId, prototype))
        {
            outError = "Prototype '" + prototypeId + "' is not available in the loaded proxy library.";
            return false;
        }

        const std::string entityName = !prototype.id.empty() ? prototype.id : "PrototypeInstance";
        const EntityId entityId = m_connector.Commands().CreateEntity(entityName);
        if (entityId == 0)
        {
            outError = "Failed to create runtime entity.";
            return false;
        }

        TransformPrototype transform;
        if (prototype.hasDefaultTransform)
        {
            transform = prototype.defaultTransform;
        }
        m_connector.Commands().SetTransform(entityId, transform);

        ObjectComponent object;
        object.kind = prototype.objectKind;
        object.prototypeId = prototype.id;
        object.selectable = prototype.selectable;
        m_world.SetObject(entityId, object);

        const BuiltInMeshKind meshKind = MeshKindFromAssetId(prototype.meshAsset);
        if (meshKind != BuiltInMeshKind::None)
        {
            RenderableComponent renderable;
            renderable.mesh = meshKind;
            renderable.visible = prototype.visible;
            m_world.SetRenderable(entityId, renderable);
        }

        if (!prototype.lightAsset.empty())
        {
            LightComponent light;
            light.light.enabled = prototype.visible;
            light.light.emitter.kind = LightEmitterKind::Point;
            light.light.emitter.position = transform.translation;
            light.light.emitter.color = ColorPrototype::FromBytes(255, 243, 214);
            light.light.emitter.intensity = 3.6f;
            light.light.emitter.range = 22.0f;
            m_world.SetLight(entityId, light);
        }

        m_interaction.SelectEntity(entityId);
        outEntityId = entityId;
        return true;
    }

    bool StudioRuntimeHost::DestroyEntity(EntityId entityId, std::string& outError)
    {
        outError.clear();

        if (entityId == 0)
        {
            outError = "Entity id is invalid.";
            return false;
        }

        if (!m_world.Contains(entityId))
        {
            outError = "Entity not found.";
            return false;
        }

        if (m_moveManipulation.active && m_moveManipulation.entityId == entityId)
        {
            EndMoveManipulation();
        }

        DestroyScriptInstance(entityId);

        if (!m_connector.Commands().DeleteEntity(entityId))
        {
            outError = "Runtime entity delete failed.";
            return false;
        }

        if (m_interaction.GetState().selectedEntity == entityId ||
            m_interaction.IsEntitySelected(entityId))
        {
            m_interaction.ClearSelection();
        }

        if (m_interaction.GetState().hoveredEntity == entityId)
        {
            m_interaction.SetHoveredEntity(0);
        }

        if (m_activeCameraEntityId == entityId)
        {
            RefreshSceneCameraBinding();
        }

        return true;
    }

    bool StudioRuntimeHost::RenameEntity(EntityId entityId, const std::string& name, std::string& outError)
    {
        if (!m_world.Contains(entityId))
        {
            outError = "Entity not found.";
            return false;
        }
        m_world.SetName(entityId, name);
        return true;
    }

    bool StudioRuntimeHost::DuplicateEntity(EntityId entityId, EntityId& outNewId, std::string& outError)
    {
        EntityInfo info;
        if (!m_world.GetEntityInfo(entityId, info))
        {
            outError = "Entity not found.";
            return false;
        }

        const EntityId newId = m_world.CreateEntity(info.name + "_copy");
        if (newId == 0)
        {
            outError = "Failed to allocate entity.";
            return false;
        }

        TransformPrototype t = info.transform;
        t.translation.x += 0.5f;
        t.translation.z += 0.5f;
        m_world.SetTransform(newId, t);

        if (info.hasObject)   { m_world.SetObject(newId, info.object); }
        if (info.hasRenderable) { m_world.SetRenderable(newId, info.renderable); }
        if (info.hasLight)    { m_world.SetLight(newId, info.light); }
        if (info.hasCamera)   { m_world.SetCamera(newId, info.camera); }

        outNewId = newId;
        return true;
    }

    bool StudioRuntimeHost::SwapToNextCamera(std::string& outCameraName, std::string& outError)
    {
        outCameraName.clear();
        outError.clear();

        const std::vector<EntityId> entities = m_world.GetEntityList();
        std::vector<EntityId> cameraIds;
        for (const EntityId entityId : entities)
        {
            if (m_world.GetCamera(entityId) != nullptr)
            {
                cameraIds.push_back(entityId);
            }
        }

        if (cameraIds.empty())
        {
            outError = "No scene cameras are available.";
            return false;
        }

        SyncActiveCameraEntityFromView();
        std::sort(cameraIds.begin(), cameraIds.end());

        std::size_t nextIndex = 0U;
        for (std::size_t i = 0; i < cameraIds.size(); ++i)
        {
            if (cameraIds[i] == m_activeCameraEntityId)
            {
                nextIndex = (i + 1U) % cameraIds.size();
                break;
            }
        }

        ApplyCameraEntityToView(cameraIds[nextIndex]);
        outCameraName = GetActiveCameraName();
        return true;
    }

    bool StudioRuntimeHost::ActivateCameraEntity(EntityId entityId, std::string& outCameraName, std::string& outError)
    {
        outCameraName.clear();
        outError.clear();

        if (m_world.GetCamera(entityId) == nullptr)
        {
            outError = "Entity is not a camera.";
            return false;
        }

        SyncActiveCameraEntityFromView();
        ApplyCameraEntityToView(entityId);
        outCameraName = GetActiveCameraName();
        return true;
    }

    std::string StudioRuntimeHost::GetActiveCameraName() const
    {
        if (m_activeCameraEntityId == 0)
        {
            return "Viewport";
        }

        const NameComponent* name = m_world.GetName(m_activeCameraEntityId);
        if (name == nullptr || name->name.empty())
        {
            return "Camera " + std::to_string(m_activeCameraEntityId);
        }

        return name->name;
    }

    void StudioRuntimeHost::SetGridVisible(bool visible)
    {
        m_gridVisible = visible;
    }

    bool StudioRuntimeHost::IsGridVisible() const
    {
        return m_gridVisible;
    }

    void StudioRuntimeHost::SetSnapDebugVisible(bool visible)
    {
        m_snapDebugVisible = visible;
        if (!visible)
        {
            m_snapDebugReport = SnapDebugReport{};
            m_snapDebugResult = SnapResult{};
        }
    }

    bool StudioRuntimeHost::IsSnapDebugVisible() const
    {
        return m_snapDebugVisible;
    }

    void StudioRuntimeHost::SetCameraGizmoVisible(bool visible)
    {
        m_cameraGizmoVisible = visible;
    }

    bool StudioRuntimeHost::IsCameraGizmoVisible() const
    {
        return m_cameraGizmoVisible;
    }


    void StudioRuntimeHost::SetSceneRenderSettings(const SceneRenderSettings& settings)
    {
        m_world.SetSceneRenderSettings(settings);
        if (m_state == StudioRuntimePlayState::Playing ||
            m_state == StudioRuntimePlayState::Paused ||
            m_state == StudioRuntimePlayState::Step)
        {
            m_editSnapshot.SetSceneRenderSettings(settings);
        }
    }

    const SceneRenderSettings& StudioRuntimeHost::GetSceneRenderSettings() const
    {
        return m_world.GetSceneRenderSettings();
    }

    void StudioRuntimeHost::ApplyRenderConfigurationPreset(RenderConfigurationPreset preset)
    {
        SetSceneRenderSettings(MakeSceneRenderSettings(preset));

        const std::vector<EntityId> entityIds = m_world.GetEntityList();
        for (const EntityId entityId : entityIds)
        {
            LightComponent* light = m_world.GetLight(entityId);
            if (!light)
            {
                continue;
            }

            ApplyLightVisibilityPreset(light->light, preset);
        }

        if (m_state == StudioRuntimePlayState::Playing ||
            m_state == StudioRuntimePlayState::Paused ||
            m_state == StudioRuntimePlayState::Step)
        {
            m_editSnapshot = m_world;
        }
    }

    void StudioRuntimeHost::ApplyLowRenderConfiguration()
    {
        ApplyRenderConfigurationPreset(RenderConfigurationPreset::Low);
    }

    void StudioRuntimeHost::SetViewRenderMode(ViewRenderMode mode)
    {
        m_viewRenderMode = mode;
    }

    ViewRenderMode StudioRuntimeHost::GetViewRenderMode() const
    {
        return m_viewRenderMode;
    }

    void StudioRuntimeHost::SetScriptLogSink(std::function<void(const std::string&)> sink)
    {
        m_scriptLogSink = std::move(sink);
    }

    void StudioRuntimeHost::RefreshSceneCameraBinding()
    {
        const std::vector<EntityId> entities = m_world.GetEntityList();
        EntityId previewCameraId = 0;
        EntityId fallbackCameraId = 0;
        EntityId namedEditorCameraId = 0;

        for (const EntityId entityId : entities)
        {
            const CameraComponent* camera = m_world.GetCamera(entityId);
            if (camera == nullptr)
            {
                continue;
            }

            if (fallbackCameraId == 0)
            {
                fallbackCameraId = entityId;
            }

            const NameComponent* name = m_world.GetName(entityId);
            if (name != nullptr && name->name == "EditorCamera")
            {
                namedEditorCameraId = entityId;
            }

            if (camera->camera.purpose == CameraPurposePrototype::Preview)
            {
                previewCameraId = entityId;
            }
        }

        const EntityId resolvedCameraId =
            namedEditorCameraId != 0 ? namedEditorCameraId :
            (previewCameraId != 0 ? previewCameraId : fallbackCameraId);
        if (resolvedCameraId != 0)
        {
            ApplyCameraEntityToView(resolvedCameraId);
        }
        else
        {
            m_activeCameraEntityId = 0;
        }
    }

    void StudioRuntimeHost::ApplyCameraEntityToView(EntityId entityId)
    {
        const CameraComponent* camera = m_world.GetCamera(entityId);
        if (camera == nullptr)
        {
            return;
        }

        m_studioCamera.GetCamera() = camera->camera;
        if (camera->camera.pose.useTarget)
        {
            m_studioCamera.SetOrbitTarget(camera->camera.pose.target);
        }
        m_activeCameraEntityId = entityId;
    }

    void StudioRuntimeHost::SyncActiveCameraEntityFromView()
    {
        if (m_activeCameraEntityId == 0)
        {
            return;
        }

        CameraComponent* camera = m_world.GetCamera(m_activeCameraEntityId);
        if (camera == nullptr)
        {
            return;
        }

        camera->camera = m_studioCamera.GetCamera();
        if (TransformComponent* transform = m_world.GetTransform(m_activeCameraEntityId))
        {
            transform->transform.translation = camera->camera.pose.position;
        }
    }

    std::string StudioRuntimeHost::CreateFallbackScene()
    {
        ApplyDefaultSceneTemplate(m_world, m_studioCamera);
        const DefaultSceneTemplatePresence presence = ValidateDefaultSceneTemplate(m_world, m_studioCamera);
        return BuildDefaultSceneTemplateReport(presence);
    }

    void StudioRuntimeHost::RebuildScriptInstances()
    {
        DestroyAllScriptInstances();

        for (const EntityId entityId : m_world.GetEntityList())
        {
            const ObjectComponent* object = m_world.GetObject(entityId);
            if (object == nullptr || object->behaviorPrototypeId.empty())
            {
                continue;
            }

            const engine::script::ScriptPrototype* prototype =
                m_scriptModule.Registry().Find(object->behaviorPrototypeId);
            if (prototype == nullptr)
            {
                if (m_scriptLogSink)
                {
                    m_scriptLogSink("Behavior prototype not found: " + object->behaviorPrototypeId);
                }
                continue;
            }

            engine::script::ScriptInstance instance(prototype->CreateInstanceBehavior());
            if (!instance.HasBehavior())
            {
                if (m_scriptLogSink)
                {
                    m_scriptLogSink("Behavior prototype factory failed: " + object->behaviorPrototypeId);
                }
                continue;
            }

            engine::script::ScriptContext context = BuildScriptContext(entityId);
            instance.Construct(context);
            instance.Start(context);
            m_scriptInstances.emplace(entityId, std::move(instance));
        }
    }

    void StudioRuntimeHost::DestroyAllScriptInstances()
    {
        for (auto& entry : m_scriptInstances)
        {
            if (!m_world.Contains(entry.first))
            {
                continue;
            }

            engine::script::ScriptContext context = BuildScriptContext(entry.first);
            entry.second.Destruct(context);
        }

        m_scriptInstances.clear();
    }

    void StudioRuntimeHost::DestroyScriptInstance(EntityId entityId)
    {
        const auto found = m_scriptInstances.find(entityId);
        if (found == m_scriptInstances.end())
        {
            return;
        }

        if (m_world.Contains(entityId))
        {
            engine::script::ScriptContext context = BuildScriptContext(entityId);
            found->second.Destruct(context);
        }

        m_scriptInstances.erase(found);
    }

    void StudioRuntimeHost::UpdateScripts(float deltaSeconds)
    {
        if (deltaSeconds <= 0.0f)
        {
            return;
        }

        for (auto& entry : m_scriptInstances)
        {
            if (!m_world.Contains(entry.first))
            {
                continue;
            }

            engine::script::ScriptContext context = BuildScriptContext(entry.first);
            entry.second.Update(context, deltaSeconds);
        }
    }

    engine::script::ScriptContext StudioRuntimeHost::BuildScriptContext(EntityId entityId)
    {
        static TransformPrototype s_fallbackTransform;
        TransformComponent* transform = m_world.GetTransform(entityId);
        TransformPrototype& resolved = transform ? transform->transform : s_fallbackTransform;

        engine::script::ScriptContext context(
            entityId,
            resolved,
            [this](std::string_view message)
            {
                if (m_scriptLogSink)
                    m_scriptLogSink(std::string(message));
            },
            [this](const engine::script::ScriptEvent& event)
            {
                if (event.type == "exit")
                {
                    m_pendingExit = true;
                    return;
                }
                constexpr std::string_view kPrefix = "goto_scene:";
                if (event.type.size() > kPrefix.size() &&
                    event.type.compare(0, kPrefix.size(), kPrefix) == 0)
                {
                    try
                    {
                        m_pendingSceneTransition =
                            std::stoi(event.type.substr(kPrefix.size()));
                    }
                    catch (...) {}
                }
            });

        context.SetWorldQuery([this](std::string_view name) -> const TransformPrototype*
        {
            for (const EntityId eid : m_world.GetEntityList())
            {
                const NameComponent* nc = m_world.GetName(eid);
                if (nc && nc->name == name)
                {
                    const TransformComponent* tc = m_world.GetTransform(eid);
                    return tc ? &tc->transform : nullptr;
                }
            }
            return nullptr;
        });

        return context;
    }

    void StudioRuntimeHost::HandleEditInput(
        const RuntimeInput& input,
        const ViewportRect& viewportRect)
    {
        m_inputSystem.Evaluate(input.rawInput);
        UpdateSnapDebug(input, viewportRect);

        if (m_interaction.GetState().activeTool != StudioTool::Move && m_moveManipulation.active)
        {
            EndMoveManipulation();
        }

        if (m_inputSystem.Down("Studio", "PanCamera"))
        {
            const InputVec2 delta = m_inputSystem.Delta("Studio", "PanCamera");
            m_studioCamera.Pan(-delta.x * 0.0125f, delta.y * 0.0125f);
        }
        else if (m_inputSystem.Down("Studio", "OrbitCamera"))
        {
            if (m_inputSystem.Pressed("Studio", "OrbitCamera") && m_snapDebugResult.didSnap)
            {
                m_studioCamera.SetOrbitTarget(m_snapDebugResult.snapPosition);
            }
            const InputVec2 delta = m_inputSystem.Delta("Studio", "OrbitCamera");
            m_studioCamera.Orbit(delta.x * 0.0065f, delta.y * 0.0065f);
        }
        else if (input.rawInput.mouseDeltaX != 0 || input.rawInput.mouseDeltaY != 0)
        {
            const StudioPickRequest hoverRequest{ input.mouseX, input.mouseY, viewportRect };
            const StudioPickResult hoverResult = m_pickingService.Pick(hoverRequest);
            m_interaction.SetHoveredEntity(hoverResult.entity);
        }

        const float zoomValue = m_inputSystem.Value("Studio", "ZoomCamera");
        if (zoomValue != 0.0f)
        {
            Ray scrollRay;
            if (m_studioCamera.TryScreenPointToRay(input.mouseX, input.mouseY, viewportRect, scrollRay))
            {
                const float scrollAmount = zoomValue / 120.0f * 0.8f;
                m_studioCamera.DollyToward(scrollRay.direction, scrollAmount);
            }
        }

        if (m_inputSystem.Down("Studio", "FlyCamera"))
        {
            const float speed = m_inputSystem.Down("Studio", "FlyBoost") ? 0.22f : 0.12f;
            if (m_inputSystem.Down("Studio", "FlyForward")) m_studioCamera.Dolly(speed);
            if (m_inputSystem.Down("Studio", "FlyBackward")) m_studioCamera.Dolly(-speed);
            if (m_inputSystem.Down("Studio", "FlyRight")) m_studioCamera.Pan(speed * 0.8f, 0.0f);
            if (m_inputSystem.Down("Studio", "FlyLeft")) m_studioCamera.Pan(-speed * 0.8f, 0.0f);
            if (m_inputSystem.Down("Studio", "FlyUp")) m_studioCamera.Pan(0.0f, speed * 0.8f);
            if (m_inputSystem.Down("Studio", "FlyDown")) m_studioCamera.Pan(0.0f, -speed * 0.8f);
        }

        if (m_inputSystem.Pressed("Studio", "FocusSelection"))
        {
            EntityInfo selected{};
            if (m_world.GetEntityInfo(m_interaction.GetState().selectedEntity, selected))
            {
                const Vector3 toTarget = selected.transform.translation - m_studioCamera.GetCamera().GetWorldPosition();
                m_studioCamera.DollyToward(toTarget, 0.8f);
            }
        }

        if (m_interaction.GetState().activeTool == StudioTool::Move)
        {
            if (!m_moveManipulation.active && m_inputSystem.Pressed("Studio", "Select"))
            {
                StartMoveManipulation(input, viewportRect);
            }
            else if (m_moveManipulation.active && m_inputSystem.Down("Studio", "Select"))
            {
                UpdateMoveManipulation(input, viewportRect);
            }

            if (m_moveManipulation.active && m_inputSystem.Released("Studio", "Select"))
            {
                EndMoveManipulation();
            }
        }

        const bool orbiting = m_inputSystem.Down("Studio", "OrbitCamera");
        const bool selectPressed = m_inputSystem.Pressed("Studio", "Select") && !orbiting;
        const bool multiSelectPressed = m_inputSystem.Pressed("Studio", "MultiSelect") && !orbiting;
        if (selectPressed || multiSelectPressed)
        {
            EntityId hitEntity = 0;
            if (m_interaction.GetState().activeTool == StudioTool::Select)
            {
                const StudioPickRequest pickRequest{ input.mouseX, input.mouseY, viewportRect };
                const StudioPickResult pickResult = m_pickingService.Pick(pickRequest);
                hitEntity = pickResult.entity;

                if (pickResult.hit)
                {
                    m_interaction.SelectEntity(pickResult.entity); // TODO: Multi-select set.
                }
                else
                {
                    m_interaction.ClearSelection();
                }
            }
            m_interaction.EmitViewportClicked(input.mouseX, input.mouseY, hitEntity);
        }

        std::ostringstream debug;
        debug << "Ctx:";
        for (const std::string& contextName : m_inputSystem.GetActiveContextsByPriority())
        {
            debug << " " << contextName;
        }
        debug << " | Mouse(" << input.rawInput.mouseX << "," << input.rawInput.mouseY << ")";
        debug << " d(" << input.rawInput.mouseDeltaX << "," << input.rawInput.mouseDeltaY << ")";
        debug << " wheel=" << input.rawInput.mouseWheelDelta;
        if (m_inputSystem.Down("Studio", "OrbitCamera")) debug << " | Orbit";
        if (m_inputSystem.Down("Studio", "PanCamera")) debug << " | Pan";
        if (m_inputSystem.Down("Studio", "FlyCamera")) debug << " | Fly";
        if (m_moveManipulation.active) debug << " | MoveDrag";
        if (selectPressed) debug << " | SelectPressed";
        if (multiSelectPressed) debug << " | MultiSelectPressed";
        m_viewportText = debug.str();
    }

    void StudioRuntimeHost::StartMoveManipulation(
        const RuntimeInput& input,
        const ViewportRect& viewportRect)
    {
        const EntityId selectedEntity = m_interaction.GetState().selectedEntity;
        if (selectedEntity == 0 ||
            !m_snapDebugResult.didSnap ||
            !m_snapDebugResult.hasSourceObjectId ||
            m_snapDebugResult.sourceObjectId != static_cast<std::uint64_t>(selectedEntity))
        {
            return;
        }

        EntityInfo selectedInfo;
        if (!m_world.GetEntityInfo(selectedEntity, selectedInfo))
        {
            return;
        }

        Ray ray;
        if (!m_studioCamera.TryScreenPointToRay(input.mouseX, input.mouseY, viewportRect, ray))
        {
            return;
        }

        Vector3 planeIntersection = m_snapDebugResult.snapPosition;
        const Vector3 planeNormal = Normalize(m_studioCamera.GetCamera().GetForward());
        TryIntersectRayWithPlane(ray, m_snapDebugResult.snapPosition, planeNormal, planeIntersection);

        m_moveManipulation.active = true;
        m_moveManipulation.entityId = selectedEntity;
        m_moveManipulation.startEntityTranslation = selectedInfo.transform.translation;
        m_moveManipulation.grabSnapPosition = m_snapDebugResult.snapPosition;
        m_moveManipulation.planeNormal = planeNormal;
        m_moveManipulation.startPlaneIntersection = planeIntersection;
    }

    void StudioRuntimeHost::UpdateMoveManipulation(
        const RuntimeInput& input,
        const ViewportRect& viewportRect)
    {
        if (!m_moveManipulation.active)
        {
            return;
        }

        EntityInfo selectedInfo;
        if (!m_world.GetEntityInfo(m_moveManipulation.entityId, selectedInfo))
        {
            EndMoveManipulation();
            return;
        }

        Ray ray;
        if (!m_studioCamera.TryScreenPointToRay(input.mouseX, input.mouseY, viewportRect, ray))
        {
            return;
        }

        Vector3 planeIntersection;
        if (!TryIntersectRayWithPlane(
                ray,
                m_moveManipulation.grabSnapPosition,
                m_moveManipulation.planeNormal,
                planeIntersection))
        {
            return;
        }

        const Vector3 delta = planeIntersection - m_moveManipulation.startPlaneIntersection;
        const Vector3 draggedGrabPoint = m_moveManipulation.grabSnapPosition + delta;
        Vector3 resolvedGrabPoint = draggedGrabPoint;

        std::vector<SnapCandidate> candidates;
        for (const EntityId id : m_world.GetEntityList())
        {
            if (id == m_moveManipulation.entityId)
            {
                continue;
            }

            EntityInfo info;
            if (!m_world.GetEntityInfo(id, info) || !info.hasRenderable || !info.renderable.visible)
            {
                continue;
            }

            ObjectPrototype snapObject;
            snapObject.GetPrimaryMesh().builtInKind = info.renderable.mesh;
            snapObject.CollectSnapCandidates(
                candidates,
                info.transform,
                static_cast<std::uint64_t>(id));
        }
        m_originMarker.CollectSnapCandidates(candidates);

        SnapContext context;
        context.activeTool = SnapToolKind::Translate;
        context.probe.hasPoint = true;
        context.probe.point = draggedGrabPoint;

        SnapSettings settings = CreateDefaultSnapSettings();
        settings.maxSnapDistance = 1.5f;
        settings.enabledTypes = { SnapCandidateType::Point, SnapCandidateType::ObjectOrigin };

        SnapResolver resolver;
        const SnapResult moveSnap = resolver.Resolve(context, settings, candidates);
        if (moveSnap.didSnap)
        {
            resolvedGrabPoint = moveSnap.snapPosition;
        }

        TransformPrototype nextTransform = selectedInfo.transform;
        nextTransform.translation =
            m_moveManipulation.startEntityTranslation +
            (resolvedGrabPoint - m_moveManipulation.grabSnapPosition);
        m_connector.Commands().SetTransform(m_moveManipulation.entityId, nextTransform);
    }

    void StudioRuntimeHost::EndMoveManipulation()
    {
        m_moveManipulation = MoveManipulationState{};
    }

    void StudioRuntimeHost::UpdateSnapDebug(
        const RuntimeInput& input,
        const ViewportRect& viewportRect)
    {
        Ray ray;
        if (!m_studioCamera.TryScreenPointToRay(input.mouseX, input.mouseY, viewportRect, ray))
        {
            m_snapDebugReport = SnapDebugReport{};
            m_snapDebugResult = SnapResult{};
            if (m_snapDebugVisible)
            {
                m_snapDebugReport.summary = "Snap debug: cursor outside viewport.";
            }
            return;
        }

        std::vector<SnapCandidate> candidates;
        for (const EntityId id : m_world.GetEntityList())
        {
            EntityInfo info;
            if (!m_world.GetEntityInfo(id, info) || !info.hasRenderable || !info.renderable.visible)
            {
                continue;
            }

            ObjectPrototype snapObject;
            snapObject.GetPrimaryMesh().builtInKind = info.renderable.mesh;
            snapObject.CollectSnapCandidates(
                candidates,
                info.transform,
                static_cast<std::uint64_t>(id));
        }

        m_originMarker.CollectSnapCandidates(candidates);

        SnapContext context;
        context.activeTool = SnapToolKind::Translate;
        context.probe.hasRay = true;
        context.probe.ray.origin = ray.origin;
        context.probe.ray.direction = ray.direction;

        SnapSettings settings = CreateDefaultSnapSettings();
        settings.maxSnapDistance = 1.5f;
        settings.enabledTypes = { SnapCandidateType::Point, SnapCandidateType::ObjectOrigin };

        m_snapDebugReport = SnapDebugReport{};
        SnapResolver resolver;
        m_snapDebugResult = resolver.Resolve(context, settings, candidates, nullptr, &m_snapDebugReport);
    }

    FramePrototype StudioRuntimeHost::BuildWorldFrame() const
    {
        ViewPrototype view;
        view.kind = ViewKind::Scene3D;
        view.renderMode = m_viewRenderMode;
        view.camera = m_studioCamera.GetCamera();

        auto appendCameraGizmo =
            [&view, this](const EntityInfo& info)
            {
                if (!info.hasCamera)
                {
                    return;
                }

                const bool isActiveCamera = (info.id == m_activeCameraEntityId);
                const ColorPrototype bodyColor = isActiveCamera
                    ? ColorPrototype::FromBytes(255, 220, 120)
                    : ColorPrototype::FromBytes(120, 205, 255);
                const ColorPrototype lineColor = isActiveCamera
                    ? ColorPrototype::FromBytes(255, 238, 180)
                    : ColorPrototype::FromBytes(150, 220, 255);

                const Vector3 position = info.camera.camera.pose.position;
                const Vector3 target = info.camera.camera.pose.target;
                const Vector3 forward = Normalize(target - position);
                const Vector3 right = Normalize(Cross(info.camera.camera.pose.worldUp, forward));
                const Vector3 up = Normalize(Cross(forward, right));

                // Shift gizmo behind the camera so it doesn't sit inside the frustum
                // or clip through the camera's own near plane.
                const Vector3 gizmoOrigin = position - (forward * 0.35f);

                ItemPrototype bodyItem;
                bodyItem.kind = ItemKind::Object3D;
                bodyItem.object3D.transform.translation = gizmoOrigin;
                bodyItem.object3D.transform.scale = Vector3(0.18f, 0.18f, 0.28f);
                MeshPrototype& bodyMesh = bodyItem.object3D.GetPrimaryMesh();
                bodyMesh.builtInKind = BuiltInMeshKind::Octahedron;
                bodyMesh.material.baseLayer.albedo = bodyColor;
                bodyMesh.material.emissive.enabled = true;
                bodyMesh.material.emissive.color = bodyColor;
                bodyMesh.material.emissive.intensity = isActiveCamera ? 0.35f : 0.18f;
                view.items.push_back(bodyItem);

                const Vector3 tip = gizmoOrigin + (forward * 0.42f);
                const Vector3 wingRight = gizmoOrigin + (right * 0.18f) + (up * 0.06f);
                const Vector3 wingLeft = gizmoOrigin - (right * 0.18f) + (up * 0.06f);
                const Vector3 top = gizmoOrigin + (up * 0.18f);

                auto pushLine =
                    [&view, &lineColor](const Vector3& start, const Vector3& end, float thickness)
                    {
                        ItemPrototype lineItem;
                        lineItem.kind = ItemKind::Line;
                        lineItem.line.geometry.start = start;
                        lineItem.line.geometry.end = end;
                        lineItem.line.color = lineColor;
                        lineItem.line.thickness = thickness;
                        view.items.push_back(lineItem);
                    };

                pushLine(gizmoOrigin, tip, 0.022f);
                pushLine(gizmoOrigin, wingRight, 0.016f);
                pushLine(gizmoOrigin, wingLeft, 0.016f);
                pushLine(gizmoOrigin, top, 0.016f);
                pushLine(wingLeft, tip, 0.014f);
                pushLine(wingRight, tip, 0.014f);
            };

        if (m_gridVisible)
        {
            ItemPrototype gridItem;
            gridItem.kind = ItemKind::Grid;
            gridItem.grid.origin = Vector3(0.0f, 0.0f, 0.0f);
            gridItem.grid.extent = 30.0f;
            gridItem.grid.cellSize = 1.0f;
            gridItem.grid.majorLineEvery = 4;
            gridItem.grid.baseColor = ColorPrototype::FromArgb(0xFF111820u);
            gridItem.grid.majorLineColor = ColorPrototype::FromArgb(0xFF3A4A60u);
            gridItem.grid.minorLineColor = ColorPrototype::FromArgb(0xFF263446u);
            gridItem.grid.lineThickness = 0.012f;
            view.items.push_back(gridItem);
        }

        m_originMarker.AppendItems(view.items);

        if (m_snapDebugVisible)
        {
            for (const SnapDecisionEntry& entry : m_snapDebugReport.entries)
            {
                const SnapMarkerState state = entry.accepted
                    ? SnapMarkerState::Accepted
                    : SnapMarkerState::Rejected;
                view.items.push_back(m_snapMarker.BuildItem(entry.resolvedPosition, state));
            }

            if (m_snapDebugResult.didSnap)
            {
                view.items.push_back(m_snapMarker.BuildItem(m_snapDebugResult.snapPosition, SnapMarkerState::Selected));
            }
        }


        for (const EntityId id : m_world.GetEntityList())
        {
            EntityInfo info;
            if (!m_world.GetEntityInfo(id, info))
            {
                continue;
            }

            if (m_cameraGizmoVisible && info.hasCamera)
            {
                appendCameraGizmo(info);
            }

            if (info.hasLight && info.light.light.enabled)
            {
                view.directLights.push_back(info.light.light);
            }

            if (!info.hasRenderable || !info.renderable.visible)
            {
                continue;
            }

            ItemPrototype item;
            item.kind = ItemKind::Object3D;
            item.object3D.transform = info.transform;
            MeshPrototype& mesh = item.object3D.GetPrimaryMesh();
            mesh.builtInKind = info.renderable.mesh;
            mesh.material = info.renderable.material;
            const EntityId selectedEntity = m_interaction.GetState().selectedEntity;
            const EntityId hoveredEntity = m_interaction.GetState().hoveredEntity;
            if (id == selectedEntity && m_interaction.GetState().selectionHighlightEnabled)
            {
                mesh.material.emissive.enabled = true;
                mesh.material.emissive.color = ColorPrototype::FromBytes(255, 220, 120);
                mesh.material.emissive.intensity = 0.2f;
            }
            else if (id == hoveredEntity && hoveredEntity != 0)
            {
                mesh.material.emissive.enabled = true;
                mesh.material.emissive.color = ColorPrototype::FromBytes(160, 200, 255);
                mesh.material.emissive.intensity = 0.12f;
            }
            view.items.push_back(item);
        }

        if (view.directLights.empty())
        {
            LightPrototype light;
            light.emitter.kind = LightEmitterKind::Directional;
            light.emitter.direction = Vector3(0.35f, -1.0f, 0.25f);
            light.emitter.color = ColorPrototype::FromBytes(255, 248, 225);
            light.emitter.intensity = 2.8f;
            view.directLights.push_back(light);
        }

        PassPrototype pass;
        pass.kind = PassKind::Scene;
        pass.views.push_back(view);

        FramePrototype frame;
        frame.passes.push_back(pass);
        return frame;
    }
}
