#include "object_render_compiler.hpp"

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
        itemData.object3D.material.baseColor = mesh.material.baseColor.ToArgb();
        outItems.push_back(std::move(itemData));
    }
}
