#include "object_render_compiler.hpp"

#include "../math/reflectivity_math.hpp"

#include <algorithm>
#include <utility>

void ObjectRenderCompiler::Append(const ObjectPrototype& object, std::vector<RenderItemData>& outItems)
{
    for (const MeshPrototype& mesh : object.meshes)
    {
        RenderItemData itemData;
        itemData.kind = RenderItemDataKind::Object3D;
        itemData.object3D.transform.translation = RenderVector3(
            object.transform.translation.x,
            object.transform.translation.y,
            object.transform.translation.z);
        itemData.object3D.transform.rotationRadians = RenderVector3(
            object.transform.rotationRadians.x,
            object.transform.rotationRadians.y,
            object.transform.rotationRadians.z);
        itemData.object3D.transform.scale = RenderVector3(
            object.transform.scale.x,
            object.transform.scale.y,
            object.transform.scale.z);

        itemData.object3D.mesh.handle = mesh.handle;
        switch (mesh.builtInKind)
        {
        case BuiltInMeshKind::Triangle:
            itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Triangle;
            break;
        case BuiltInMeshKind::Diamond:
            itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Diamond;
            break;
        case BuiltInMeshKind::Cube:
            itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Cube;
            break;
        case BuiltInMeshKind::Quad:
            itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Quad;
            break;
        case BuiltInMeshKind::Octahedron:
            itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Octahedron;
            break;
        default:
            itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::None;
            break;
        }

        itemData.object3D.material.handle = mesh.material.handle;
        itemData.object3D.material.baseColor = mesh.material.ResolveSurfaceAlbedo().ToArgb();
        itemData.object3D.material.albedoAssetId = mesh.material.baseLayer.albedoAssetId;
        itemData.object3D.material.albedoBlend = std::clamp(mesh.material.baseLayer.albedoBlend, 0.0f, 1.0f);
        itemData.object3D.material.reflectionColor = mesh.material.reflectionColor.ToArgb();
        itemData.object3D.material.reflectivity = ClampReflectivity(mesh.material.reflectivity);
        itemData.object3D.material.roughness = ClampRoughness(mesh.material.baseLayer.roughness);
        itemData.object3D.material.emissiveEnabled = mesh.material.emissive.enabled;
        itemData.object3D.material.emissiveColor = mesh.material.emissive.color.ToArgb();
        itemData.object3D.material.emissiveIntensity = std::max(mesh.material.emissive.intensity, 0.0f);
        itemData.object3D.material.ambientStrength = mesh.material.brdf.ambientStrength;
        itemData.object3D.material.directBrdf.diffuseStrength = std::max(
            mesh.material.brdf.diffuseStrength,
            0.0f);
        itemData.object3D.material.directBrdf.diffuseStrength =
            std::min(
                itemData.object3D.material.directBrdf.diffuseStrength,
                ComputeDiffuseStrengthFromReflectivity(
                    itemData.object3D.material.reflectivity,
                    itemData.object3D.material.roughness));
        itemData.object3D.material.directBrdf.specularStrength = std::max(
            mesh.material.brdf.specularStrength,
            ComputeSpecularStrengthFromReflectivity(itemData.object3D.material.reflectivity));
        itemData.object3D.material.directBrdf.specularPower = std::max(
            mesh.material.brdf.specularPower,
            ComputeSpecularPowerFromRoughness(itemData.object3D.material.roughness));
        itemData.object3D.shadowParticipation.casts = mesh.shadowParticipation.casts;
        itemData.object3D.shadowParticipation.receives = mesh.shadowParticipation.receives;
        outItems.push_back(std::move(itemData));
    }
}
