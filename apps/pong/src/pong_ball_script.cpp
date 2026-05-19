#include "pong_ball_script.hpp"
#include "pong_rules.hpp"
#include "engine/script/script_context.hpp"

#include <cmath>
#include <memory>

namespace pong
{
    void PongBallScript::OnStart(engine::script::ScriptContext&)
    {
        m_vx = kBallSpeed;
        m_vz = kBallSpeed * 0.6f;
    }

    void PongBallScript::OnUpdate(engine::script::ScriptContext& ctx, float dt)
    {
        auto& t = ctx.Transform();
        t.translation.x += m_vx * dt;
        t.translation.z += m_vz * dt;

        // Wall bounce (top / bottom)
        if (t.translation.z > kWallHalfZ)
        {
            t.translation.z = kWallHalfZ;
            m_vz = -std::abs(m_vz);
        }
        else if (t.translation.z < -kWallHalfZ)
        {
            t.translation.z = -kWallHalfZ;
            m_vz = std::abs(m_vz);
        }

        // Left paddle collision
        const TransformPrototype* left = ctx.FindEntity("Pong_LeftPaddle");
        if (m_vx < 0.0f && left)
        {
            const float paddleRight = left->translation.x + kPaddleHalfX;
            if (t.translation.x - kBallHalfSize <= paddleRight &&
                t.translation.x > left->translation.x - 0.3f &&
                std::abs(t.translation.z - left->translation.z) <= kPaddleHalfZ + kBallHalfSize)
            {
                t.translation.x = paddleRight + kBallHalfSize;
                m_vx = std::abs(m_vx);
            }
        }

        // Right paddle collision
        const TransformPrototype* right = ctx.FindEntity("Pong_RightPaddle");
        if (m_vx > 0.0f && right)
        {
            const float paddleLeft = right->translation.x - kPaddleHalfX;
            if (t.translation.x + kBallHalfSize >= paddleLeft &&
                t.translation.x < right->translation.x + 0.3f &&
                std::abs(t.translation.z - right->translation.z) <= kPaddleHalfZ + kBallHalfSize)
            {
                t.translation.x = paddleLeft - kBallHalfSize;
                m_vx = -std::abs(m_vx);
            }
        }

        // Reset when ball passes a paddle
        if (t.translation.x < -(kPaddleX + 1.0f))
        {
            t.translation.x = 0.0f;
            t.translation.z = 0.0f;
            m_vx = -kBallSpeed;
            m_vz = kBallSpeed * 0.6f;
        }
        else if (t.translation.x > (kPaddleX + 1.0f))
        {
            t.translation.x = 0.0f;
            t.translation.z = 0.0f;
            m_vx = kBallSpeed;
            m_vz = kBallSpeed * 0.6f;
        }
    }

    std::unique_ptr<engine::script::IScriptBehavior> CreatePongBallScript()
    {
        return std::make_unique<PongBallScript>();
    }
}
