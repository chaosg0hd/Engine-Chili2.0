#pragma once

#include "../../iprototype.hpp"
#include "../../math/math.hpp"
#include "../../snap/snap_behavior.hpp"
#include "../../../core/engine_context.hpp"
#include "../../../modules/memory/imemory_service.hpp"
#include "../../../modules/memory/memory_types.hpp"
#include "mesh.hpp"

#include <cstdint>
#include <new>
#include <array>
#include <string>
#include <vector>

struct ObjectRuntime
{
    InstanceId instanceId = 0;
    TransformPrototype transform;
    std::vector<MeshPrototype> meshes;
    std::string gameplayRole;
    int scoreValue = 0;
};

class ObjectPrototype final : public IPrototype
{
public:
    ObjectPrototype(
        PrototypeId inId = 0U,
        const char* inDebugName = "ObjectPrototype")
        : m_id(inId),
          m_debugName(inDebugName)
    {
    }

    PrototypeId GetId() const override
    {
        return m_id;
    }

    const char* GetDebugName() const override
    {
        return m_debugName ? m_debugName : "ObjectPrototype";
    }

    void* Construct(EngineContext& context, InstanceId instanceId) const override
    {
        if (!context.Memory)
        {
            return nullptr;
        }

        void* memory = context.Memory->Allocate(
            sizeof(ObjectRuntime),
            MemoryClass::Persistent,
            alignof(ObjectRuntime),
            GetDebugName());
        if (!memory)
        {
            return nullptr;
        }

        auto* runtime = new (memory) ObjectRuntime();
        runtime->instanceId = instanceId;
        runtime->transform = transform;
        runtime->meshes = meshes;
        runtime->gameplayRole = gameplayRole;
        runtime->scoreValue = scoreValue;
        return runtime;
    }

    void Destruct(EngineContext& context, void* runtimeData) const override
    {
        auto* runtime = static_cast<ObjectRuntime*>(runtimeData);
        if (!runtime)
        {
            return;
        }

        runtime->~ObjectRuntime();
        if (context.Memory)
        {
            context.Memory->Free(runtime);
        }
    }

public:
    TransformPrototype transform;
    std::vector<MeshPrototype> meshes = { MeshPrototype{} };
    std::string gameplayRole;
    int scoreValue = 0;

    SnapBehaviorPrototype snapBehavior;

    MeshPrototype& GetPrimaryMesh()
    {
        if (meshes.empty())
        {
            meshes.push_back(MeshPrototype{});
        }

        return meshes.front();
    }

    const MeshPrototype& GetPrimaryMesh() const
    {
        static const MeshPrototype emptyMesh;
        return meshes.empty() ? emptyMesh : meshes.front();
    }

    void SetPrimaryMesh(const MeshPrototype& mesh)
    {
        GetPrimaryMesh() = mesh;
    }

    bool HasMeshes() const
    {
        return !meshes.empty();
    }

    void CollectSnapCandidates(
        std::vector<SnapCandidate>& outCandidates,
        const TransformPrototype& worldTransform,
        std::uint64_t objectId = 0) const
    {
        if (snapBehavior.snapCentroid)
        {
            outCandidates.push_back(MakeObjectOriginCandidate(
                worldTransform.translation,
                objectId,
                "ObjectOrigin"));
        }

        if (snapBehavior.snapVertices)
        {
            const Matrix4 worldMatrix = worldTransform.ToMatrix();
            const MeshPrototype& mesh = GetPrimaryMesh();

            switch (mesh.builtInKind)
            {
            case BuiltInMeshKind::Triangle:
            {
                static constexpr std::array<Vector3, 3> kVertices =
                {{
                    Vector3(0.0f, 0.45f, 0.0f),
                    Vector3(0.45f, -0.35f, 0.0f),
                    Vector3(-0.45f, -0.35f, 0.0f)
                }};
                for (const Vector3& vertex : kVertices)
                {
                    outCandidates.push_back(MakeVertexCandidate(
                        TransformPoint(worldMatrix, vertex),
                        objectId,
                        "TriangleVertex"));
                }
                break;
            }
            case BuiltInMeshKind::Diamond:
            {
                static constexpr std::array<Vector3, 4> kVertices =
                {{
                    Vector3(0.0f, 0.50f, 0.0f),
                    Vector3(0.40f, 0.0f, 0.0f),
                    Vector3(0.0f, -0.50f, 0.0f),
                    Vector3(-0.40f, 0.0f, 0.0f)
                }};
                for (const Vector3& vertex : kVertices)
                {
                    outCandidates.push_back(MakeVertexCandidate(
                        TransformPoint(worldMatrix, vertex),
                        objectId,
                        "DiamondVertex"));
                }
                break;
            }
            case BuiltInMeshKind::Quad:
            {
                static constexpr std::array<Vector3, 4> kVertices =
                {{
                    Vector3(-0.35f, 0.35f, 0.0f),
                    Vector3(0.35f, 0.35f, 0.0f),
                    Vector3(0.35f, -0.35f, 0.0f),
                    Vector3(-0.35f, -0.35f, 0.0f)
                }};
                for (const Vector3& vertex : kVertices)
                {
                    outCandidates.push_back(MakeVertexCandidate(
                        TransformPoint(worldMatrix, vertex),
                        objectId,
                        "QuadVertex"));
                }
                break;
            }
            case BuiltInMeshKind::Octahedron:
            {
                static constexpr std::array<Vector3, 6> kVertices =
                {{
                    Vector3(0.0f, 0.65f, 0.0f),
                    Vector3(0.65f, 0.0f, 0.0f),
                    Vector3(0.0f, 0.0f, 0.65f),
                    Vector3(-0.65f, 0.0f, 0.0f),
                    Vector3(0.0f, 0.0f, -0.65f),
                    Vector3(0.0f, -0.65f, 0.0f)
                }};
                for (const Vector3& vertex : kVertices)
                {
                    outCandidates.push_back(MakeVertexCandidate(
                        TransformPoint(worldMatrix, vertex),
                        objectId,
                        "OctahedronVertex"));
                }
                break;
            }
            case BuiltInMeshKind::Cube:
            default:
            {
                static constexpr std::array<Vector3, 8> kVertices =
                {{
                    Vector3(-0.5f, -0.5f, -0.5f),
                    Vector3(-0.5f, -0.5f, 0.5f),
                    Vector3(-0.5f, 0.5f, -0.5f),
                    Vector3(-0.5f, 0.5f, 0.5f),
                    Vector3(0.5f, -0.5f, -0.5f),
                    Vector3(0.5f, -0.5f, 0.5f),
                    Vector3(0.5f, 0.5f, -0.5f),
                    Vector3(0.5f, 0.5f, 0.5f)
                }};
                for (const Vector3& vertex : kVertices)
                {
                    outCandidates.push_back(MakeVertexCandidate(
                        TransformPoint(worldMatrix, vertex),
                        objectId,
                        "CubeVertex"));
                }
                break;
            }
            }
        }

        if (snapBehavior.snapBasePoint)
        {
            outCandidates.push_back(MakePointCandidate(
                TransformPoint(worldTransform.ToMatrix(), snapBehavior.basePoint),
                objectId,
                "BasePoint"));
        }
    }

private:
    PrototypeId m_id = 0U;
    const char* m_debugName = nullptr;
};
