#pragma once

#include "render_types.hpp"

#include <memory>

class IRenderBackend;

class RenderBackendFactory
{
public:
    static std::unique_ptr<IRenderBackend> Create(RenderBackendType type);
};
