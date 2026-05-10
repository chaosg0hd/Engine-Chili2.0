#include "runtime/studio_runtime_host.hpp"

#include "runtime/scene_serializer.hpp"
#include "runtime/studio_default_scene_template.hpp"

#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"
#include "prototypes/presentation/view.hpp"
#include "input/input_binding.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace studio_runtime
{
    StudioRuntimeHost::StudioRuntimeHost()
        : m_connector(m_world, m_studioCamera)
        , m_pickingService(m_world, m_studioCamera)
    {
        ConfigureInputContexts();
    }

    void StudioRuntimeHost::ConfigureInputContexts()
    {
        InputContext& studio = m_inputSystem.RegisterContext("Studio", 100);
        studio.BindAction("Select", BindMouseButton(InputMouseButton::Left, false, false, false, true));
        studio.BindAction("MultiSelect", BindMouseButton(InputMouseButton::Left, false, true, false, true));
        studio.BindAction("OrbitCamera", BindMouseButton(InputMouseButton::Left, false, false, true, true));
        studio.BindAction("PanCamera", BindMouseButton(InputMouseButton::Middle, false, false, false, true));
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

        if (!project.exportedArtifactPath.empty())
        {
            // TEMPORARY: artifact-based preview (launcher→engine.dll→app DLL) is not yet implemented.
            // Long-term owner: project build system; artifact path set by the build pipeline.
            // Deferral reason: DLL hot-load stack not yet wired up.
            outError = "Artifact-based preview launch is not yet implemented. exportedArtifactPath='" + project.exportedArtifactPath + "'";
            return false;
        }

        if (project.previewRuntimeName.empty())
        {
            outError = "Project has no preview runtime configured. Set previewRuntimeName (in-process) or exportedArtifactPath (artifact).";
            return false;
        }

        m_editSnapshot = m_world;
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
        if (m_runtime)
        {
            m_runtime->EndPlay();
        }

        m_runtime.reset();
        if (m_state == StudioRuntimePlayState::Playing || m_state == StudioRuntimePlayState::Paused || m_state == StudioRuntimePlayState::Step)
        {
            m_world = m_editSnapshot;
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
        m_studioCamera.GetCamera().projection.aspectRatioOverride = viewportRect.Aspect();

        if (m_state == StudioRuntimePlayState::Edit)
        {
            HandleEditInput(input, viewportRect);
            m_renderFrame = BuildWorldFrame();
            m_hasRenderFrame = true;
            return;
        }

        if (m_state == StudioRuntimePlayState::Paused)
        {
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

    void StudioRuntimeHost::ConfigurePrototypeLibrary(const std::string& proxyFolderPath)
    {
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

    std::string StudioRuntimeHost::CreateFallbackScene()
    {
        ApplyDefaultSceneTemplate(m_world, m_studioCamera);
        const DefaultSceneTemplatePresence presence = ValidateDefaultSceneTemplate(m_world, m_studioCamera);
        return BuildDefaultSceneTemplateReport(presence);
    }

    void StudioRuntimeHost::HandleEditInput(
        const RuntimeInput& input,
        const ViewportRect& viewportRect)
    {
        m_inputSystem.Evaluate(input.rawInput);

        if (m_inputSystem.Down("Studio", "PanCamera"))
        {
            const InputVec2 delta = m_inputSystem.Delta("Studio", "PanCamera");
            m_studioCamera.Pan(-delta.x * 0.0125f, delta.y * 0.0125f);
        }
        else if (m_inputSystem.Down("Studio", "OrbitCamera"))
        {
            const InputVec2 delta = m_inputSystem.Delta("Studio", "OrbitCamera");
            m_studioCamera.Orbit(delta.x * 0.0065f, -delta.y * 0.0065f);
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
        if (selectPressed) debug << " | SelectPressed";
        if (multiSelectPressed) debug << " | MultiSelectPressed";
        m_viewportText = debug.str();
    }

    FramePrototype StudioRuntimeHost::BuildWorldFrame() const
    {
        ViewPrototype view;
        view.kind = ViewKind::Scene3D;
        view.camera = m_studioCamera.GetCamera();

        ItemPrototype gridItem;
        gridItem.kind = ItemKind::Grid;
        gridItem.grid.origin = Vector3(0.0f, -1.25f, 0.0f);
        gridItem.grid.extent = 30.0f;
        gridItem.grid.cellSize = 1.0f;
        gridItem.grid.majorLineEvery = 4;
        gridItem.grid.baseColor = ColorPrototype::FromArgb(0xFF111820u);
        gridItem.grid.majorLineColor = ColorPrototype::FromArgb(0xFF3A4A60u);
        gridItem.grid.minorLineColor = ColorPrototype::FromArgb(0xFF263446u);
        gridItem.grid.lineThickness = 0.012f;
        view.items.push_back(gridItem);

        // Sky light plane — always present as an editor constant regardless of scene contents,
        // same as the grid. Provides a visible overhead reference for directional lighting.
        ItemPrototype skyItem;
        skyItem.kind = ItemKind::Object3D;
        // Quad mesh is XY-plane (z=0). Rotate 90° around X to lay it horizontal,
        // then scale X/Y to get an 18x18 overhead plane. Z scale has no effect on Quad.
        skyItem.object3D.transform.translation = Vector3(0.0f, 9.0f, 0.0f);
        skyItem.object3D.transform.rotationRadians = Vector3(1.5708f, 0.0f, 0.0f);
        skyItem.object3D.transform.scale = Vector3(18.0f, 18.0f, 1.0f);
        MeshPrototype& skyMesh = skyItem.object3D.GetPrimaryMesh();
        skyMesh.builtInKind = BuiltInMeshKind::Quad;
        skyMesh.material.baseLayer.albedo = ColorPrototype::FromBytes(18, 22, 28);
        skyMesh.material.emissive.enabled = true;
        skyMesh.material.emissive.color = ColorPrototype::FromBytes(255, 248, 225);
        skyMesh.material.emissive.intensity = 0.85f;
        view.items.push_back(skyItem);

        for (const EntityId id : m_world.GetEntityList())
        {
            EntityInfo info;
            if (!m_world.GetEntityInfo(id, info))
            {
                continue;
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
