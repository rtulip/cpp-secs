#ifndef ecs_pong_resource_hpp
#define ecs_pong_resource_hpp
#include <chrono>

namespace pong::resource
{
    /**
     * @brief Keyboard Resource
     * 
     * A Resource used for storing keyboard state, to be referenced by different Entities
     * 
     */
    struct KeyboardResource
    {
        // Paddle Control keys
        unsigned char PADDLE_LEFT_UP;
        unsigned char PADDLE_LEFT_DOWN;
        unsigned char PADDLE_RIGHT_UP;
        unsigned char PADDLE_RIGHT_DOWN;

        // Different states of a paddle
        enum PaddleState
        {
            UP,    // Paddle is moving upwards
            STILL, // Paddle isn't moving
            DOWN,  // Paddle is moving downwards.
        };
        // States of the paddle sides.
        PaddleState PADDLE_STATE_LEFT, PADDLE_STATE_RIGHT;

        // Key bor spawning a new ball.
        unsigned char SPAWN_BALL;
        // Flag for spawning a new ball.
        bool SHOULD_SPAWN_BALL;

        KeyboardResource(unsigned char left_up, unsigned char left_down, unsigned char right_up, unsigned char right_down, unsigned char spawn_ball)
        {
            PADDLE_LEFT_UP = left_up;
            PADDLE_LEFT_DOWN = left_down;
            PADDLE_RIGHT_UP = right_up;
            PADDLE_RIGHT_DOWN = right_down;
            SPAWN_BALL = spawn_ball;

            PADDLE_STATE_LEFT = PaddleState::STILL;
            PADDLE_STATE_RIGHT = PaddleState::STILL;
            SHOULD_SPAWN_BALL = false;
        }
    };

    /**
     * @brief Score Resource
     * 
     * A resource for holding a global 'score', which is to be altered by systems. 
     * 
     */
    struct ScoreResource
    {
        size_t SCORE_LEFT;
        size_t SCORE_RIGHT;
    };
} // namespace pong::resource

#endif