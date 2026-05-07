#include "runtime/studio_interaction_controller.hpp"

#include <algorithm>
#include <cctype>

namespace studio_runtime
{
    namespace
    {
        std::string JsonBool(bool value)
        {
            return value ? "true" : "false";
        }
    }

    const StudioInteractionState& StudioInteractionController::GetState() const
    {
        return m_state;
    }

    std::string StudioInteractionController::BuildStateJson() const
    {
        std::string json = "{\"ok\":true";
        json += ",\"activeTool\":\"" + std::string(ToolName(m_state.activeTool)) + "\"";
        json += ",\"selectedEntity\":" + std::to_string(m_state.selectedEntity);
        json += ",\"hoveredEntity\":" + std::to_string(m_state.hoveredEntity);
        json += ",\"lastPickEntity\":" + std::to_string(m_state.lastPickEntity);
        json += ",\"lastClickX\":" + std::to_string(m_state.lastClickX);
        json += ",\"lastClickY\":" + std::to_string(m_state.lastClickY);
        json += ",\"lastPickHit\":" + JsonBool(m_state.lastPickHit);
        json += ",\"mode\":\"" + std::string(ModeName(m_state.mode)) + "\"";
        json += ",\"selectionHighlightEnabled\":" + JsonBool(m_state.selectionHighlightEnabled);
        json += "}";
        return json;
    }

    std::string StudioInteractionController::BuildFeedJson(std::size_t cursor) const
    {
        if (cursor > m_events.size())
        {
            cursor = m_events.size();
        }

        std::string json = "{\"ok\":true,\"cursor\":" + std::to_string(m_events.size()) + ",\"events\":[";
        for (std::size_t i = cursor; i < m_events.size(); ++i)
        {
            if (i > cursor)
            {
                json += ",";
            }
            json += m_events[i];
        }
        json += "]}";
        return json;
    }

    void StudioInteractionController::SetActiveTool(StudioTool tool)
    {
        if (m_state.activeTool == tool)
        {
            return;
        }

        m_state.activeTool = tool;
        m_state.selectionHighlightEnabled = tool == StudioTool::Select && m_state.mode == RuntimeMode::Edit;
        PushEvent("ToolChanged", "\"activeTool\":\"" + std::string(ToolName(tool)) + "\"");
    }

    void StudioInteractionController::SelectEntity(EntityId id)
    {
        if (id == 0)
        {
            ClearSelection();
            return;
        }

        if (m_state.selectedEntity == id)
        {
            return;
        }

        m_state.selectedEntity = id;
        m_state.selectionHighlightEnabled = m_state.activeTool == StudioTool::Select && m_state.mode == RuntimeMode::Edit;
        PushEvent("EntitySelected", "\"selectedEntity\":" + std::to_string(id));
    }

    void StudioInteractionController::ClearSelection()
    {
        if (m_state.selectedEntity == 0)
        {
            return;
        }

        m_state.selectedEntity = 0;
        PushEvent("EntityDeselected", "\"selectedEntity\":0");
    }

    void StudioInteractionController::SetHoveredEntity(EntityId id)
    {
        if (m_state.hoveredEntity == id)
        {
            return;
        }

        m_state.hoveredEntity = id;
        PushEvent("ViewportHovered", "\"hoveredEntity\":" + std::to_string(id));
    }

    void StudioInteractionController::SetRuntimeMode(RuntimeMode mode)
    {
        if (m_state.mode == mode)
        {
            return;
        }

        m_state.mode = mode;
        m_state.selectionHighlightEnabled = m_state.activeTool == StudioTool::Select && mode == RuntimeMode::Edit;
        PushEvent("RuntimeModeChanged", "\"mode\":\"" + std::string(ModeName(mode)) + "\"");
    }

    void StudioInteractionController::EmitViewportClicked(int mouseX, int mouseY, EntityId hitEntity)
    {
        m_state.lastClickX = mouseX;
        m_state.lastClickY = mouseY;
        m_state.lastPickEntity = hitEntity;
        m_state.lastPickHit = hitEntity != 0;
        PushEvent(
            "ViewportClicked",
            "\"mouseX\":" + std::to_string(mouseX) +
            ",\"mouseY\":" + std::to_string(mouseY) +
            ",\"hitEntity\":" + std::to_string(hitEntity));
    }

    void StudioInteractionController::EmitInspectorDataRequested(EntityId id)
    {
        PushEvent("InspectorDataRequested", "\"selectedEntity\":" + std::to_string(id));
    }

    StudioTool StudioInteractionController::ToolFromString(const std::string& value)
    {
        std::string lower = value;
        std::transform(
            lower.begin(),
            lower.end(),
            lower.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

        if (lower == "select") return StudioTool::Select;
        if (lower == "move") return StudioTool::Move;
        if (lower == "rotate") return StudioTool::Rotate;
        if (lower == "scale") return StudioTool::Scale;
        return StudioTool::None;
    }

    const char* StudioInteractionController::ToolName(StudioTool tool)
    {
        switch (tool)
        {
        case StudioTool::Select: return "Select";
        case StudioTool::Move: return "Move";
        case StudioTool::Rotate: return "Rotate";
        case StudioTool::Scale: return "Scale";
        case StudioTool::None:
        default:
            return "None";
        }
    }

    const char* StudioInteractionController::ModeName(RuntimeMode mode)
    {
        switch (mode)
        {
        case RuntimeMode::Play: return "Play";
        case RuntimeMode::Pause: return "Pause";
        case RuntimeMode::Step: return "Step";
        case RuntimeMode::Edit:
        default:
            return "Edit";
        }
    }

    void StudioInteractionController::PushEvent(const std::string& type, const std::string& body)
    {
        std::string event = "{\"type\":\"" + type + "\"";
        if (!body.empty())
        {
            event += "," + body;
        }
        event += "}";
        m_events.push_back(event);
    }
}
