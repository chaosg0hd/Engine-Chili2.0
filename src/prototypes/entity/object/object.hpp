#pragma once

#include "../../iprototype.hpp"
#include "../../math/math.hpp"
#include "../../../core/engine_context.hpp"
#include "../../../modules/memory/imemory_service.hpp"
#include "../../../modules/memory/memory_types.hpp"
#include "mesh.hpp"

#include <new>
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

private:
    PrototypeId m_id = 0U;
    const char* m_debugName = nullptr;
};
