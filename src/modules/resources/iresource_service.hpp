#pragma once

#include "../gpu/igpu_service.hpp"
#include "resource_types.hpp"

#include <cstddef>
#include <string>

class IResourceService
{
public:
    virtual ~IResourceService() = default;

    virtual ResourceHandle RequestResource(const std::string& assetId, ResourceKind kind) = 0;
    virtual bool UnloadResource(ResourceHandle handle) = 0;
    virtual ResourceState GetResourceState(ResourceHandle handle) const = 0;
    virtual ResourceKind GetResourceKind(ResourceHandle handle) const = 0;
    virtual std::string GetAssetId(ResourceHandle handle) const = 0;
    virtual std::string GetResolvedPath(ResourceHandle handle) const = 0;
    virtual std::string GetLastError(ResourceHandle handle) const = 0;
    virtual std::size_t GetSourceByteSize(ResourceHandle handle) const = 0;
    virtual GpuResourceHandle GetGpuResourceHandle(ResourceHandle handle) const = 0;
    virtual std::size_t GetUploadedByteSize(ResourceHandle handle) const = 0;
    virtual bool IsResourceReady(ResourceHandle handle) const = 0;
    virtual std::size_t GetResourceCountByState(ResourceState state) const = 0;
    virtual std::size_t GetResourceCount() const = 0;
};
