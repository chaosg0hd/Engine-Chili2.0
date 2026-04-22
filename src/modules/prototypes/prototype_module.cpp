#include "prototype_module.hpp"
#include "prototype_importer.hpp"

#include "../../core/engine_context.hpp"
#include "../../modules/resources/iresource_service.hpp"
#include "../../modules/resources/resource_types.hpp"
#include "../../prototypes/entity/appearance/color.hpp"
#include "../../prototypes/entity/appearance/light.hpp"
#include "../../prototypes/entity/object/object.hpp"

#include <string>
#include <utility>

namespace
{
    MaterialPrototype BuildLightingLabStuccoRoomMaterial()
    {
        MaterialPrototype material;
        material.baseLayer.albedo = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
        material.baseLayer.albedoAssetId = "library/materials/stucco/albedo.png";
        material.baseLayer.normalAssetId = "library/materials/stucco/normal.png";
        material.baseLayer.heightAssetId = "library/materials/stucco/height.png";
        material.baseLayer.roughness = 0.82f;
        material.reflectivity = 0.0f;
        material.reflectionColor = material.baseLayer.albedo;
        material.brdf.ambientStrength = 0.07f;
        material.brdf.diffuseStrength = 0.94f;
        material.brdf.specularStrength = 0.06f;
        material.brdf.specularPower = 10.0f;
        return material;
    }

    MaterialPrototype BuildLightingLabStuccoFloorMaterial()
    {
        MaterialPrototype material = BuildLightingLabStuccoRoomMaterial();
        material.baseLayer.roughness = 0.86f;
        material.brdf.ambientStrength = 0.06f;
        return material;
    }

    MaterialPrototype BuildLightingLabStuccoCubeMaterial()
    {
        MaterialPrototype material = BuildLightingLabStuccoRoomMaterial();
        material.baseLayer.roughness = 0.34f;
        material.brdf.ambientStrength = 0.08f;
        material.brdf.diffuseStrength = 0.90f;
        material.brdf.specularStrength = 0.26f;
        material.brdf.specularPower = 22.0f;
        return material;
    }

    MaterialPrototype BuildLightingLabEmitterMaterial()
    {
        MaterialPrototype material;
        material.baseLayer.albedo = ColorPrototype::FromArgb(0xFFFFF1D0u);
        material.reflectivity = 0.0f;
        material.reflectionColor = material.baseLayer.albedo;
        material.baseLayer.roughness = 0.12f;
        material.brdf.ambientStrength = 0.40f;
        material.brdf.diffuseStrength = 0.45f;
        material.brdf.specularStrength = 0.18f;
        material.brdf.specularPower = 16.0f;
        return material;
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
    m_materialPrototypes.clear();
    m_instances.clear();
    m_materialPrototypes.emplace("lighting_lab/stucco_floor", BuildLightingLabStuccoFloorMaterial());
    m_materialPrototypes.emplace("lighting_lab/stucco_room", BuildLightingLabStuccoRoomMaterial());
    m_materialPrototypes.emplace("lighting_lab/stucco_cube", BuildLightingLabStuccoCubeMaterial());
    m_materialPrototypes.emplace("lighting_lab/emitter", BuildLightingLabEmitterMaterial());
    m_initialized = true;
    return true;
}

void PrototypeModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started)
    {
        return;
    }

    (void)context;
    m_started = true;
}

void PrototypeModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;
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
        m_materialPrototypes.clear();
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

    std::unique_ptr<ImportedPrototypeData> importedData =
        PrototypeImporter::ImportFromJson(m_context->Resources->GetSourceText(resource), assetId);
    if (!importedData)
    {
        return 0U;
    }

    auto prototype = std::make_unique<ObjectPrototype>(std::move(importedData->objectPrototype));
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

const MaterialPrototype* PrototypeModule::GetMaterialPrototype(const std::string& prototypeName) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_materialPrototypes.find(prototypeName);
    return it != m_materialPrototypes.end() ? &it->second : nullptr;
}

bool PrototypeModule::HasMaterialPrototype(const std::string& prototypeName) const
{
    return GetMaterialPrototype(prototypeName) != nullptr;
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
