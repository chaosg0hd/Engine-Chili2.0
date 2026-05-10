#pragma once

#include "runtime/runtime_world.hpp"

#include <string>
#include <unordered_map>

namespace studio_runtime
{
    struct ResolvedPrototype
    {
        std::string id;
        std::string type;
        std::string meshAsset;
        std::string materialAsset;
        std::string lightAsset;
        std::string objectKind = "Object";
        bool visible = true;
        bool selectable = true;
        bool hasDefaultTransform = false;
        TransformPrototype defaultTransform;
    };

    class ProxyPrototypeResolver
    {
    public:
        bool LoadFromProxyFolder(const std::string& proxyFolderPath);
        void Clear();
        bool TryResolve(const std::string& prototypeId, ResolvedPrototype& outPrototype) const;

    private:
        std::unordered_map<std::string, ResolvedPrototype> m_prototypes;
    };
}
