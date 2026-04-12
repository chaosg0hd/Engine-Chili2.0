#include "prototype_module.hpp"

#include "../../core/engine_context.hpp"
#include "../../modules/resources/iresource_service.hpp"
#include "../../modules/resources/resource_types.hpp"
#include "../../prototypes/entity/appearance/light.hpp"
#include "../../prototypes/entity/object/object.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace
{
    bool ExtractStringField(const std::string& text, const std::string& key, std::string& outValue)
    {
        const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
        std::smatch match;
        if (!std::regex_search(text, match, pattern) || match.size() < 2)
        {
            outValue.clear();
            return false;
        }

        outValue = match[1].str();
        return true;
    }

    bool ExtractIntField(const std::string& text, const std::string& key, int& outValue)
    {
        const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+)");
        std::smatch match;
        if (!std::regex_search(text, match, pattern) || match.size() < 2)
        {
            return false;
        }

        outValue = std::stoi(match[1].str());
        return true;
    }

    bool ExtractFloatField(const std::string& text, const std::string& key, float& outValue)
    {
        const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+(\\.[0-9]+)?)");
        std::smatch match;
        if (!std::regex_search(text, match, pattern) || match.size() < 2)
        {
            return false;
        }

        outValue = std::stof(match[1].str());
        return true;
    }

    bool ExtractNumberArrayField(const std::string& text, const std::string& key, std::vector<float>& outValues)
    {
        const std::regex pattern("\"" + key + "\"\\s*:\\s*\\[([^\\]]*)\\]");
        std::smatch match;
        if (!std::regex_search(text, match, pattern) || match.size() < 2)
        {
            outValues.clear();
            return false;
        }

        outValues.clear();
        const std::string body = match[1].str();
        const std::regex numberPattern("-?[0-9]+(\\.[0-9]+)?");
        for (auto it = std::sregex_iterator(body.begin(), body.end(), numberPattern);
             it != std::sregex_iterator();
             ++it)
        {
            outValues.push_back(std::stof(it->str()));
        }

        return !outValues.empty();
    }

    BuiltInMeshKind ParseBuiltInMeshKind(const std::string& value)
    {
        std::string normalized = value;
        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            });

        if (normalized == "triangle")
        {
            return BuiltInMeshKind::Triangle;
        }
        if (normalized == "diamond")
        {
            return BuiltInMeshKind::Diamond;
        }
        if (normalized == "cube")
        {
            return BuiltInMeshKind::Cube;
        }
        if (normalized == "quad")
        {
            return BuiltInMeshKind::Quad;
        }
        if (normalized == "octahedron")
        {
            return BuiltInMeshKind::Octahedron;
        }

        return BuiltInMeshKind::None;
    }

    Vector3 ToVector3(const std::vector<float>& values, const Vector3& fallback)
    {
        if (values.size() < 3U)
        {
            return fallback;
        }

        return Vector3(values[0], values[1], values[2]);
    }
}

const char* PrototypeModule::GetName() const
{
    return "Prototype";
}

bool PrototypeModule::Initialize(EngineContext& context)
{
    if (m_initialized)
    {
        return true;
    }

    m_context = &context;
    m_nextInstanceHandle = 1U;
    m_prototypes.clear();
    m_instances.clear();
    m_initialized = true;
    return true;
}

void PrototypeModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started)
    {
        return;
    }

    m_context = &context;
    m_started = true;
}

void PrototypeModule::Update(EngineContext& context, float deltaTime)
{
    (void)deltaTime;
    m_context = &context;
}

void PrototypeModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [handle, instance] : m_instances)
        {
            (void)handle;
            const auto prototypeIt = m_prototypes.find(instance.prototypeId);
            if (prototypeIt != m_prototypes.end() && instance.runtimeData)
            {
                prototypeIt->second->Destruct(*m_context, instance.runtimeData);
            }
        }

        m_instances.clear();
        m_prototypes.clear();
    }

    m_nextInstanceHandle = 1U;
    m_context = nullptr;
    m_started = false;
    m_initialized = false;
}

bool PrototypeModule::RegisterPrototype(std::unique_ptr<IPrototype> prototype)
{
    if (!m_started || !prototype)
    {
        return false;
    }

    const PrototypeId prototypeId = prototype->GetId();
    if (prototypeId == 0U)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_prototypes.emplace(prototypeId, std::move(prototype)).second;
}

PrototypeId PrototypeModule::LoadPrototypeAsset(const std::string& assetId)
{
    if (!m_started || !m_context || !m_context->Resources)
    {
        return 0U;
    }

    const ResourceHandle resource = m_context->Resources->RequestResource(assetId, ResourceKind::PrototypeJson);
    if (resource == 0U || !m_context->Resources->IsResourceReady(resource))
    {
        return 0U;
    }

    std::unique_ptr<IPrototype> prototype =
        CreatePrototypeFromJson(m_context->Resources->GetSourceText(resource), assetId);
    if (!prototype)
    {
        return 0U;
    }

    const PrototypeId prototypeId = prototype->GetId();
    if (HasPrototype(prototypeId))
    {
        return prototypeId;
    }

    return RegisterPrototype(std::move(prototype)) ? prototypeId : 0U;
}

bool PrototypeModule::UnregisterPrototype(PrototypeId prototypeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [handle, instance] : m_instances)
    {
        (void)handle;
        if (instance.prototypeId == prototypeId)
        {
            return false;
        }
    }

    return m_prototypes.erase(prototypeId) > 0;
}

const IPrototype* PrototypeModule::GetPrototype(PrototypeId prototypeId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_prototypes.find(prototypeId);
    return it != m_prototypes.end() ? it->second.get() : nullptr;
}

bool PrototypeModule::HasPrototype(PrototypeId prototypeId) const
{
    return GetPrototype(prototypeId) != nullptr;
}

PrototypeInstanceHandle PrototypeModule::CreateInstance(PrototypeId prototypeId)
{
    if (!m_started || !m_context)
    {
        return 0U;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    const auto prototypeIt = m_prototypes.find(prototypeId);
    if (prototypeIt == m_prototypes.end())
    {
        return 0U;
    }

    const PrototypeInstanceHandle instanceHandle = m_nextInstanceHandle++;
    void* runtimeData = prototypeIt->second->Construct(*m_context, instanceHandle);
    if (!runtimeData)
    {
        return 0U;
    }

    PrototypeInstanceRecord record;
    record.handle = instanceHandle;
    record.prototypeId = prototypeId;
    record.runtimeData = runtimeData;
    m_instances.emplace(instanceHandle, record);
    return instanceHandle;
}

bool PrototypeModule::DestroyInstance(PrototypeInstanceHandle instanceHandle)
{
    if (!m_started || !m_context)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    const auto instanceIt = m_instances.find(instanceHandle);
    if (instanceIt == m_instances.end())
    {
        return false;
    }

    const auto prototypeIt = m_prototypes.find(instanceIt->second.prototypeId);
    if (prototypeIt != m_prototypes.end())
    {
        prototypeIt->second->Destruct(*m_context, instanceIt->second.runtimeData);
    }

    m_instances.erase(instanceIt);
    return true;
}

void* PrototypeModule::GetInstanceRuntimeData(PrototypeInstanceHandle instanceHandle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_instances.find(instanceHandle);
    return it != m_instances.end() ? it->second.runtimeData : nullptr;
}

PrototypeId PrototypeModule::GetInstancePrototypeId(PrototypeInstanceHandle instanceHandle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_instances.find(instanceHandle);
    return it != m_instances.end() ? it->second.prototypeId : 0U;
}

std::size_t PrototypeModule::GetPrototypeCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_prototypes.size();
}

std::size_t PrototypeModule::GetLiveInstanceCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_instances.size();
}

bool PrototypeModule::IsInitialized() const
{
    return m_initialized;
}

bool PrototypeModule::IsStarted() const
{
    return m_started;
}

std::unique_ptr<IPrototype> PrototypeModule::CreatePrototypeFromJson(
    const std::string& sourceText,
    const std::string& assetId) const
{
    std::string type;
    if (!ExtractStringField(sourceText, "type", type) || type != "object")
    {
        return nullptr;
    }

    std::string idText;
    if (!ExtractStringField(sourceText, "id", idText))
    {
        idText = assetId;
    }

    auto prototype = std::make_unique<ObjectPrototype>(MakePrototypeId(idText));

    std::vector<float> values;
    if (ExtractNumberArrayField(sourceText, "position", values))
    {
        prototype->transform.translation = ToVector3(values, prototype->transform.translation);
    }
    if (ExtractNumberArrayField(sourceText, "rotation", values))
    {
        prototype->transform.rotationRadians = ToVector3(values, prototype->transform.rotationRadians);
    }
    if (ExtractNumberArrayField(sourceText, "scale", values))
    {
        prototype->transform.scale = ToVector3(values, prototype->transform.scale);
    }

    std::string meshKind;
    if (ExtractStringField(sourceText, "kind", meshKind))
    {
        prototype->GetPrimaryMesh().builtInKind = ParseBuiltInMeshKind(meshKind);
    }

    if (ExtractNumberArrayField(sourceText, "baseColor", values) && values.size() >= 4U)
    {
        prototype->GetPrimaryMesh().material.baseColor =
            ColorPrototype(values[0], values[1], values[2], values[3]);
    }
    if (ExtractNumberArrayField(sourceText, "reflectionColor", values) && values.size() >= 4U)
    {
        prototype->GetPrimaryMesh().material.reflectionColor =
            ColorPrototype(values[0], values[1], values[2], values[3]);
    }
    ExtractFloatField(sourceText, "reflectivity", prototype->GetPrimaryMesh().material.reflectivity);
    ExtractFloatField(sourceText, "roughness", prototype->GetPrimaryMesh().material.roughness);
    if (ExtractNumberArrayField(sourceText, "absorptionColor", values) && values.size() >= 4U)
    {
        prototype->GetPrimaryMesh().material.absorptionColor =
            ColorPrototype(values[0], values[1], values[2], values[3]);
    }
    ExtractFloatField(sourceText, "absorption", prototype->GetPrimaryMesh().material.absorption);

    ExtractStringField(sourceText, "role", prototype->gameplayRole);
    ExtractIntField(sourceText, "scoreValue", prototype->scoreValue);
    return prototype;
}

PrototypeId PrototypeModule::MakePrototypeId(const std::string& value)
{
    std::uint32_t hash = 2166136261U;
    for (unsigned char character : value)
    {
        hash ^= character;
        hash *= 16777619U;
    }

    return hash != 0U ? hash : 1U;
}
