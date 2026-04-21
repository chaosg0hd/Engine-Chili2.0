#pragma once

#include <cstdint>

class MovingCubeSampleScenePrototype
{
public:
    void SetTheoreticalWorkIterations(unsigned int iterations);
    unsigned int GetTheoreticalWorkIterations() const;
    std::uint32_t Sample(float screenX, float screenY, double time) const;

private:
    unsigned int m_theoreticalWorkIterations = 0U;
};
