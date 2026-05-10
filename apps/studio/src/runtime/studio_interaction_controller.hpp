#pragma once

#include "runtime/runtime_world.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace studio_runtime
{
    enum class StudioTool
    {
        None,
        Select,
        Move,
        Rotate,
        Scale
    };

    enum class RuntimeMode
    {
        Edit,
        Play,
        Pause,
        Step
    };

    struct StudioInteractionState
    {
        StudioTool activeTool = StudioTool::Select;
        EntityId selectedEntity = 0;
        std::vector<EntityId> selectedEntities;
        EntityId hoveredEntity = 0;
        EntityId lastPickEntity = 0;
        int lastClickX = 0;
        int lastClickY = 0;
        bool lastPickHit = false;
        RuntimeMode mode = RuntimeMode::Edit;
        bool selectionHighlightEnabled = true;
    };

    class StudioInteractionController
    {
    public:
        const StudioInteractionState& GetState() const;
        std::string BuildStateJson() const;
        std::string BuildFeedJson(std::size_t cursor) const;

        void SetActiveTool(StudioTool tool);
        void SelectEntity(EntityId id);
        void ToggleEntitySelection(EntityId id);
        void ClearSelection();
        bool IsEntitySelected(EntityId id) const;
        void SetHoveredEntity(EntityId id);
        void SetRuntimeMode(RuntimeMode mode);
        void EmitViewportClicked(int mouseX, int mouseY, EntityId hitEntity);
        void EmitInspectorDataRequested(EntityId id);

        static StudioTool ToolFromString(const std::string& value);
        static const char* ToolName(StudioTool tool);
        static const char* ModeName(RuntimeMode mode);

    private:
        void PushEvent(const std::string& type, const std::string& body);

    private:
        StudioInteractionState m_state;
        std::vector<std::string> m_events;
    };
}
