#pragma once

#include "../geometry/line.hpp"
#include "../../math/math.hpp"
#include "../../snap/snap_behavior.hpp"
#include "../appearance/color.hpp"
#include "../../presentation/item.hpp"

#include <cstdint>
#include <vector>

struct OriginMarkerPrototype
{
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
    float size = 0.07f;
    ColorPrototype albedo = ColorPrototype::FromBytes(215, 225, 235);
    ColorPrototype emissiveColor = ColorPrototype::FromBytes(180, 210, 235);
    float emissiveIntensity = 0.08f;
    float axisHalfLength = 3.5f;
    float axisHeightOffset = 0.01f;
    float axisThickness = 0.03f;
    ColorPrototype xAxisColor = ColorPrototype::FromArgb(0xFFE35D5Bu);
    ColorPrototype yAxisColor = ColorPrototype::FromArgb(0xFF66D37Eu);
    ColorPrototype zAxisColor = ColorPrototype::FromArgb(0xFF62A7FFu);

    SnapBehaviorPrototype snapBehavior = { true, false, false };

    ItemPrototype BuildItem() const
    {
        ItemPrototype item;
        item.kind = ItemKind::Object3D;
        item.object3D.transform.translation = position;
        item.object3D.transform.scale = Vector3(size, size, size);
        MeshPrototype& mesh = item.object3D.GetPrimaryMesh();
        mesh.builtInKind = BuiltInMeshKind::Cube;
        mesh.material.baseLayer.albedo = albedo;
        mesh.material.emissive.enabled = true;
        mesh.material.emissive.color = emissiveColor;
        mesh.material.emissive.intensity = emissiveIntensity;
        return item;
    }

    void AppendItems(std::vector<ItemPrototype>& outItems) const
    {
        outItems.push_back(BuildAxisItem(
            Vector3(-axisHalfLength, axisHeightOffset, 0.0f),
            Vector3(axisHalfLength, axisHeightOffset, 0.0f),
            xAxisColor));
        outItems.push_back(BuildAxisItem(
            Vector3(0.0f, -axisHalfLength, 0.0f),
            Vector3(0.0f, axisHalfLength, 0.0f),
            yAxisColor));
        outItems.push_back(BuildAxisItem(
            Vector3(0.0f, axisHeightOffset, -axisHalfLength),
            Vector3(0.0f, axisHeightOffset, axisHalfLength),
            zAxisColor));
        outItems.push_back(BuildItem());
    }

    void CollectSnapCandidates(
        std::vector<SnapCandidate>& outCandidates,
        std::uint64_t objectId = 0) const
    {
        TransformPrototype t;
        t.translation = position;
        t.scale = Vector3(size, size, size);
        snapBehavior.CollectSnapCandidates(outCandidates, t, objectId);

        const std::vector<LinePrototype> axes = BuildAxisLines();
        for (const LinePrototype& axis : axes)
        {
            axis.CollectSnapCandidates(outCandidates, objectId);
        }
    }

private:
    ItemPrototype BuildAxisItem(
        const Vector3& start,
        const Vector3& end,
        const ColorPrototype& color) const
    {
        ItemPrototype item;
        item.kind = ItemKind::Line;
        item.line.geometry.SetSegment(start + position, end + position);
        item.line.color = color;
        item.line.thickness = axisThickness;
        item.line.fallbackLength = axisHalfLength * 2.0f;
        return item;
    }

    std::vector<LinePrototype> BuildAxisLines() const
    {
        std::vector<LinePrototype> axes;
        axes.reserve(3);

        LinePrototype xAxis;
        xAxis.SetSegment(
            position + Vector3(-axisHalfLength, axisHeightOffset, 0.0f),
            position + Vector3(axisHalfLength, axisHeightOffset, 0.0f));
        axes.push_back(xAxis);

        LinePrototype yAxis;
        yAxis.SetSegment(
            position + Vector3(0.0f, -axisHalfLength, 0.0f),
            position + Vector3(0.0f, axisHalfLength, 0.0f));
        axes.push_back(yAxis);

        LinePrototype zAxis;
        zAxis.SetSegment(
            position + Vector3(0.0f, axisHeightOffset, -axisHalfLength),
            position + Vector3(0.0f, axisHeightOffset, axisHalfLength));
        axes.push_back(zAxis);

        return axes;
    }
};
