#pragma once

#include "point.hpp"
#include "../../iprototype.hpp"
#include "../../math/math.hpp"
#include "../../../core/engine_context.hpp"
#include "../../../modules/memory/imemory_service.hpp"
#include "../../../modules/memory/memory_types.hpp"

#include <limits>
#include <new>

enum class LineForm : unsigned char
{
    Segment = 0,
    Ray,
    Infinite
};

struct LineRuntime
{
    InstanceId instanceId = 0;
    LineForm form = LineForm::Segment;
    Vector3 origin = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 direction = Vector3(1.0f, 0.0f, 0.0f);
    float minT = 0.0f;
    float maxT = 1.0f;
};

class LinePrototype final : public IPrototype
{
public:
    // TODO: Support nonlinear line paths later without splitting this into a separate curve prototype.
    // The public line API should keep using start/end plus evaluation methods like GetPointAt().
    LinePrototype() = default;

    LinePrototype(
        const PointPrototype& inStart,
        const PointPrototype& inEnd,
        PrototypeId inId = 0U,
        const char* inDebugName = "LinePrototype")
        : start(inStart),
          end(inEnd),
          m_id(inId),
          m_debugName(inDebugName)
    {
        SetSegment(inStart.position, inEnd.position);
    }

    PrototypeId GetId() const override
    {
        return m_id;
    }

    const char* GetDebugName() const override
    {
        return m_debugName ? m_debugName : "LinePrototype";
    }

    void* Construct(EngineContext& context, InstanceId instanceId) const override
    {
        LineRuntime* runtime = AllocateRuntime(context);
        if (!runtime)
        {
            return nullptr;
        }

        runtime->instanceId = instanceId;
        runtime->form = form;
        runtime->origin = origin;
        runtime->direction = direction;
        runtime->minT = minT;
        runtime->maxT = maxT;
        return runtime;
    }

    void Destruct(EngineContext& context, void* runtimeData) const override
    {
        auto* runtime = static_cast<LineRuntime*>(runtimeData);
        if (!runtime)
        {
            return;
        }

        FreeRuntime(context, runtime);
    }

    void SetStart(const PointPrototype& value)
    {
        SetSegment(value.position, end.position);
    }

    const PointPrototype& GetStart() const
    {
        return start;
    }

    void SetEnd(const PointPrototype& value)
    {
        SetSegment(start.position, value.position);
    }

    const PointPrototype& GetEnd() const
    {
        return end;
    }

    void SetSegment(const Vector3& startPosition, const Vector3& endPosition)
    {
        form = LineForm::Segment;
        origin = startPosition;
        start.position = startPosition;
        end.position = endPosition;

        const Vector3 vector = endPosition - startPosition;
        const float length = Length(vector);
        direction = (length > 0.000001f) ? (vector * (1.0f / length)) : Vector3(1.0f, 0.0f, 0.0f);
        minT = 0.0f;
        maxT = length;
    }

    void SetRay(const Vector3& rayOrigin, const Vector3& rayDirection)
    {
        form = LineForm::Ray;
        origin = rayOrigin;
        direction = Normalize(rayDirection);
        if (Length(direction) <= 0.000001f)
        {
            direction = Vector3(1.0f, 0.0f, 0.0f);
        }

        minT = 0.0f;
        maxT = std::numeric_limits<float>::infinity();
        start.position = origin;
        end.position = origin + direction;
    }

    void SetInfiniteLine(const Vector3& lineOrigin, const Vector3& lineDirection)
    {
        form = LineForm::Infinite;
        origin = lineOrigin;
        direction = Normalize(lineDirection);
        if (Length(direction) <= 0.000001f)
        {
            direction = Vector3(1.0f, 0.0f, 0.0f);
        }

        minT = -std::numeric_limits<float>::infinity();
        maxT = std::numeric_limits<float>::infinity();
        start.position = origin - direction;
        end.position = origin + direction;
    }

    bool SetFromEquation2D(float a, float b, float c, float z = 0.0f)
    {
        const float normalLengthSquared = (a * a) + (b * b);
        if (normalLengthSquared <= 0.000001f)
        {
            return false;
        }

        const float inverseNormalLengthSquared = 1.0f / normalLengthSquared;
        const Vector3 pointOnLine(
            -a * c * inverseNormalLengthSquared,
            -b * c * inverseNormalLengthSquared,
            z);
        const Vector3 equationDirection(-b, a, 0.0f);
        SetInfiniteLine(pointOnLine, equationDirection);
        return true;
    }

    LineForm GetForm() const
    {
        return form;
    }

    bool IsSegment() const
    {
        return form == LineForm::Segment;
    }

    bool IsRay() const
    {
        return form == LineForm::Ray;
    }

    bool IsInfinite() const
    {
        return form == LineForm::Infinite;
    }

    Vector3 GetVector() const
    {
        return IsSegment() ? (end.position - start.position) : direction;
    }

    Vector3 GetDirection() const
    {
        return direction;
    }

    float GetLength() const
    {
        return IsSegment() ? (maxT - minT) : std::numeric_limits<float>::infinity();
    }

    float GetLengthSquared() const
    {
        if (!IsSegment())
        {
            return std::numeric_limits<float>::infinity();
        }

        const float length = GetLength();
        return length * length;
    }

    Vector3 GetMidpoint() const
    {
        return IsSegment() ? GetPointAt(0.5f) : origin;
    }

    Vector3 GetPointAt(float t) const
    {
        return IsSegment() ? GetPointAtParameter(t) : GetPointAtDistance(t);
    }

    Vector3 GetPointAtParameter(float t) const
    {
        const float distance = IsSegment() ? (minT + ((maxT - minT) * t)) : t;
        return GetPointAtDistance(distance);
    }

    Vector3 GetPointAtDistance(float distance) const
    {
        return origin + (direction * distance);
    }

    bool ContainsT(float t) const
    {
        return t >= minT && t <= maxT;
    }

    float GetMinT() const
    {
        return minT;
    }

    float GetMaxT() const
    {
        return maxT;
    }

    bool IsPointColinear(const PointPrototype& point, float epsilon = 0.000001f) const
    {
        const Vector3 lineVector = direction;
        const Vector3 pointVector = point.position - origin;
        const Vector3 cross = Cross(lineVector, pointVector);
        return Dot(cross, cross) <= epsilon;
    }

    bool IsColinearWith(const LinePrototype& other, float epsilon = 0.000001f) const
    {
        return IsPointColinear(other.start, epsilon) && IsPointColinear(other.end, epsilon);
    }

    bool IsValid() const
    {
        return Length(direction) > 0.000001f &&
            (!IsSegment() || (start.IsValid() && end.IsValid() && GetLength() > 0.000001f));
    }

public:
    PointPrototype start;
    PointPrototype end = PointPrototype(Vector3(1.0f, 0.0f, 0.0f));
    LineForm form = LineForm::Segment;
    Vector3 origin = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 direction = Vector3(1.0f, 0.0f, 0.0f);
    float minT = 0.0f;
    float maxT = 1.0f;

private:
    LineRuntime* AllocateRuntime(EngineContext& context) const
    {
        if (!context.Memory)
        {
            return nullptr;
        }

        void* memory = context.Memory->Allocate(
            sizeof(LineRuntime),
            MemoryClass::Persistent,
            alignof(LineRuntime),
            GetDebugName());
        if (!memory)
        {
            return nullptr;
        }

        return new (memory) LineRuntime();
    }

    void FreeRuntime(EngineContext& context, LineRuntime* runtime) const
    {
        if (!runtime)
        {
            return;
        }

        runtime->~LineRuntime();

        if (context.Memory)
        {
            context.Memory->Free(runtime);
        }
    }

private:
    PrototypeId m_id = 0U;
    const char* m_debugName = "LinePrototype";
};
