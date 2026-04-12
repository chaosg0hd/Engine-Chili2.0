#pragma once

#include "line.hpp"
#include "../../iprototype.hpp"
#include "../../math/math.hpp"
#include "../../../core/engine_context.hpp"
#include "../../../modules/memory/imemory_service.hpp"
#include "../../../modules/memory/memory_types.hpp"

#include <array>
#include <cstddef>
#include <new>

struct FaceRuntime
{
    InstanceId instanceId = 0;
    std::array<PointRuntime, 3> vertices{};
};

class FacePrototype final : public IPrototype
{
public:
    FacePrototype() = default;

    FacePrototype(
        const PointPrototype& a,
        const PointPrototype& b,
        const PointPrototype& c,
        PrototypeId inId = 0U,
        const char* inDebugName = "FacePrototype")
        : edges{{
              LinePrototype(a, b),
              LinePrototype(b, c),
              LinePrototype(c, a)
          }},
          m_id(inId),
          m_debugName(inDebugName)
    {
    }

    FacePrototype(
        const LinePrototype& edgeA,
        const LinePrototype& edgeB,
        const LinePrototype& edgeC,
        PrototypeId inId = 0U,
        const char* inDebugName = "FacePrototype")
        : edges{{ edgeA, edgeB, edgeC }},
          m_id(inId),
          m_debugName(inDebugName)
    {
    }

    PrototypeId GetId() const override
    {
        return m_id;
    }

    const char* GetDebugName() const override
    {
        return m_debugName ? m_debugName : "FacePrototype";
    }

    void* Construct(EngineContext& context, InstanceId instanceId) const override
    {
        FaceRuntime* runtime = AllocateRuntime(context);
        if (!runtime)
        {
            return nullptr;
        }

        const std::array<PointPrototype, 3> vertices = GetVertices();
        runtime->instanceId = instanceId;
        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            runtime->vertices[i].instanceId = instanceId;
            runtime->vertices[i].position = vertices[i].position;
        }

        return runtime;
    }

    void Destruct(EngineContext& context, void* runtimeData) const override
    {
        auto* runtime = static_cast<FaceRuntime*>(runtimeData);
        if (!runtime)
        {
            return;
        }

        FreeRuntime(context, runtime);
    }

    void SetVertices(
        const PointPrototype& a,
        const PointPrototype& b,
        const PointPrototype& c)
    {
        edges[0] = LinePrototype(a, b);
        edges[1] = LinePrototype(b, c);
        edges[2] = LinePrototype(c, a);
    }

    static bool ArePointsEquivalent(const PointPrototype& a, const PointPrototype& b, float epsilon = 0.000001f)
    {
        const Vector3 delta = a.position - b.position;
        return Dot(delta, delta) <= (epsilon * epsilon);
    }

    std::array<PointPrototype, 3> GetVertices() const
    {
        return
        {{
            edges[0].start,
            edges[0].end,
            edges[1].end
        }};
    }

    Vector3 GetNormal() const
    {
        const std::array<PointPrototype, 3> vertices = GetVertices();
        const Vector3 edgeA = vertices[1].position - vertices[0].position;
        const Vector3 edgeB = vertices[2].position - vertices[0].position;
        return Normalize(Cross(edgeA, edgeB));
    }

    float GetArea() const
    {
        const std::array<PointPrototype, 3> vertices = GetVertices();
        const Vector3 edgeA = vertices[1].position - vertices[0].position;
        const Vector3 edgeB = vertices[2].position - vertices[0].position;
        return Length(Cross(edgeA, edgeB)) * 0.5f;
    }

    Vector3 GetCentroid() const
    {
        const std::array<PointPrototype, 3> vertices = GetVertices();
        return (vertices[0].position + vertices[1].position + vertices[2].position) * (1.0f / 3.0f);
    }

    bool IsValid() const
    {
        return edges[0].IsValid() &&
            edges[1].IsValid() &&
            edges[2].IsValid() &&
            ArePointsEquivalent(edges[0].end, edges[1].start) &&
            ArePointsEquivalent(edges[1].end, edges[2].start) &&
            ArePointsEquivalent(edges[2].end, edges[0].start) &&
            GetArea() > 0.000001f;
    }

public:
    std::array<LinePrototype, 3> edges =
    {{
        LinePrototype(PointPrototype(Vector3(0.0f, 0.0f, 0.0f)), PointPrototype(Vector3(1.0f, 0.0f, 0.0f))),
        LinePrototype(PointPrototype(Vector3(1.0f, 0.0f, 0.0f)), PointPrototype(Vector3(0.0f, 1.0f, 0.0f))),
        LinePrototype(PointPrototype(Vector3(0.0f, 1.0f, 0.0f)), PointPrototype(Vector3(0.0f, 0.0f, 0.0f)))
    }};

private:
    FaceRuntime* AllocateRuntime(EngineContext& context) const
    {
        if (!context.Memory)
        {
            return nullptr;
        }

        void* memory = context.Memory->Allocate(
            sizeof(FaceRuntime),
            MemoryClass::Persistent,
            alignof(FaceRuntime),
            GetDebugName());
        if (!memory)
        {
            return nullptr;
        }

        return new (memory) FaceRuntime();
    }

    void FreeRuntime(EngineContext& context, FaceRuntime* runtime) const
    {
        if (!runtime)
        {
            return;
        }

        runtime->~FaceRuntime();

        if (context.Memory)
        {
            context.Memory->Free(runtime);
        }
    }

private:
    PrototypeId m_id = 0U;
    const char* m_debugName = "FacePrototype";
};
