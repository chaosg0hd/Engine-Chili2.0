#pragma once

#include "pass.hpp"

#include <vector>

struct FramePrototype
{
    std::vector<PassPrototype> passes;

    bool IsEmpty() const { return passes.empty(); }
};
