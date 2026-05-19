#pragma once

#include "../entity/appearance/color.hpp"
#include "../presentation/item.hpp"
#include "snap_behavior.hpp"

enum class SnapMarkerState : unsigned char
{
    Rejected = 0,
    Accepted,
    Selected
};

struct SnapMarkerPrototype
{
    SnapBehaviorPrototype behavior;

    float rejectedSize = 0.045f;
    ColorPrototype rejectedAlbedo = ColorPrototype::FromBytes(180, 150, 50);
    ColorPrototype rejectedEmissive = ColorPrototype::FromBytes(140, 115, 40);
    float rejectedEmissiveIntensity = 0.06f;

    float acceptedSize = 0.075f;
    ColorPrototype acceptedAlbedo = ColorPrototype::FromBytes(255, 210, 96);
    ColorPrototype acceptedEmissive = ColorPrototype::FromBytes(255, 225, 130);
    float acceptedEmissiveIntensity = 0.22f;

    float selectedSize = 0.12f;
    ColorPrototype selectedAlbedo = ColorPrototype::FromBytes(98, 240, 170);
    ColorPrototype selectedEmissive = ColorPrototype::FromBytes(98, 240, 170);
    float selectedEmissiveIntensity = 0.35f;

    ItemPrototype BuildItem(const Vector3& position, SnapMarkerState state) const
    {
        ItemPrototype item;
        item.kind = ItemKind::Object3D;
        item.object3D.transform.translation = position;

        float size = rejectedSize;
        ColorPrototype albedo = rejectedAlbedo;
        ColorPrototype emissive = rejectedEmissive;
        float emissiveIntensity = rejectedEmissiveIntensity;

        if (state == SnapMarkerState::Accepted)
        {
            size = acceptedSize;
            albedo = acceptedAlbedo;
            emissive = acceptedEmissive;
            emissiveIntensity = acceptedEmissiveIntensity;
        }
        else if (state == SnapMarkerState::Selected)
        {
            size = selectedSize;
            albedo = selectedAlbedo;
            emissive = selectedEmissive;
            emissiveIntensity = selectedEmissiveIntensity;
        }

        item.object3D.transform.scale = Vector3(size, size, size);
        MeshPrototype& mesh = item.object3D.GetPrimaryMesh();
        mesh.builtInKind = BuiltInMeshKind::Cube;
        mesh.material.baseLayer.albedo = albedo;
        mesh.material.emissive.enabled = true;
        mesh.material.emissive.color = emissive;
        mesh.material.emissive.intensity = emissiveIntensity;
        return item;
    }
};
