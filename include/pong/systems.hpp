#ifndef ecs_pong_systems_hpp
#define ecs_pong_systems_hpp

#include <pong/components.hpp>
#include <pong/resources.hpp>
#include <ecs/system.hpp>
#include <ecs/world.hpp>
#include <GL/glut.h>
#include <math.h>
#include <random>

namespace pc = pong::components;
namespace pr = pong::resource;

namespace pong::systems
{
    /**
     * @brief Movement System
     * 
     * Components:
     *      - Position 
     *      - Velocity
     * 
     * Updates Every Position based on it's velocity.
     * 
     */
    class MovementSystem : public ecs::system::System<pc::Position, pc::Velocity>
    {
    public:
        MovementSystem() = default;
        ~MovementSystem() = default;
        void run(system_data data)
        {
            auto pos = std::get<0>(data);
            auto vel = std::get<1>(data);
            pos->x += vel->dx;
            pos->y += vel->dy;
        }
    };

    /**
     * @brief System for checking Ball-Wall collisions
     * 
     * Components:
     *      - Position
     *      - Rectangle
     *      - Velocity
     *      - Ball
     *      - Entity
     *      - Score Resource
     *      - World Resource
     * 
     * Checks if a 'Ball' Entity has collided with the edges of the screen.
     * 
     * Within this context a the Ball component is being used to differentiate the 
     * 'Balls' from other Entities.
     * 
     * The Position and Rectangle components are required to check if the Entity is 
     * properly within the range [-width, width] in the X direction, and [-height, height]
     * in the Y direction.
     * 
     * If the Entity is colliding with the wall, than it's velocity in the y direction 
     * needs to change in order to 'Bounce' off the top or bottom of the screen
     * 
     * If the Entity is colliding with the Left or Right sides of the screen, then the
     * Score needs to increase, the ball needs to staged for removal. This requires the
     * ScoreResource for increasing the score, and the WorldResource and Entity component
     * for removing the 'Ball' Entity.
     * 
     */
    class BallWallCollisionSystem : public ecs::system::System<
                                        pc::Position,
                                        pc::Rectangle,
                                        pc::Velocity,
                                        pc::Ball,
                                        pr::ScoreResource,
                                        ecs::world::WorldResource,
                                        ecs::entity::Entity>
    {

    private:
        float width;  // Width of the screen
        float height; // Height of the screen

    public:
        BallWallCollisionSystem(float w, float h) : width(w), height(h) {}
        ~BallWallCollisionSystem() = default;
        void run(system_data data)
        {
            auto pos = std::get<0>(data);
            auto rect = std::get<1>(data);
            auto vel = std::get<2>(data);
            auto score = std::get<4>(data);
            auto world_res = std::get<5>(data);
            auto entity = std::get<6>(data);

            // Check if colliding with the Left & Right sides of the screen.
            if (pos->x < -width)
            {
                score->SCORE_RIGHT++;
                world_res->remove_entity(entity);
            }
            else if (pos->x + rect->width > width)
            {
                score->SCORE_LEFT++;
                world_res->remove_entity(entity);
            }

            // Bounce of the top or bottom of the screen.
            if (pos->y - rect->height < -this->height)
            {
                pos->y = -this->height + rect->height;
                vel->dy = -vel->dy;
            }
            else if (pos->y > this->height)
            {
                pos->y = this->height;
                vel->dy = -vel->dy;
            }
        }
    };

    /**
     * @brief System for checking Ball-Paddle collisions.
     * 
     * Components:
     *      - Position
     *      - Rectangle 
     *      - Side
     *      - World Resource
     * 
     * Checks if a 'Ball' is colliding with a paddle, and performs a 'Bounce' appropriately.
     * 
     */
    class BallPaddleCollisionSystem : public ecs::system::System<pc::Position, pc::Rectangle, pc::Side, ecs::world::WorldResource>
    {
    private:
        float MAX_BOUNCE_ANGLE;

    public:
        BallPaddleCollisionSystem(float max_bounce_angle) : MAX_BOUNCE_ANGLE(max_bounce_angle) {}
        ~BallPaddleCollisionSystem() = default;
        /**
         * @brief Checks if two rectangles are colliding.
         * 
         * @param pos1 
         * @param rect1 
         * @param pos2 
         * @param rect2 
         * @return true 
         * @return false 
         */
        bool overlap(pc::Position *pos1, pc::Rectangle *rect1, pc::Position *pos2, pc::Rectangle *rect2)
        {
            pc::Position l1 = *pos1;
            pc::Position r1 = {pos1->x + rect1->width,
                               pos1->y - rect1->height};

            pc::Position l2 = *pos2;
            pc::Position r2 = {pos2->x + rect2->width,
                               pos2->y - rect2->height};

            if (l1.x >= r2.x || l2.x >= r1.x)
                return false;

            if (l1.y <= r2.y || l2.y <= r1.y)
                return false;

            return true;
        }

        bool point_on_line(float x, float line_start, float line_end)
        {
            return line_start <= x && x <= line_end;
        }

        void run(system_data data)
        {
            auto paddle_pos = std::get<0>(data);
            auto paddle_rect = std::get<1>(data);
            auto paddle_side = std::get<2>(data);
            auto world = std::get<3>(data);

            // Do a fetch for the 'Balls' in the run function.
            auto balls = world->world()->safe_fetch<pc::Position, pc::Rectangle, pc::Velocity, pc::Ball>();
            for (auto ball : balls)
            {

                auto ball_pos = std::get<0>(ball);
                auto ball_rect = std::get<1>(ball);
                auto ball_vel = std::get<2>(ball);
                auto ball_speed = std::get<3>(ball);

                if (overlap(paddle_pos, paddle_rect, ball_pos, ball_rect))
                {
                    float paddle_top = paddle_pos->y;
                    float paddle_left = paddle_pos->x;
                    float paddle_right = paddle_pos->x + paddle_rect->width;
                    float paddle_bot = paddle_pos->y - paddle_rect->height;
                    float ball_top = ball_pos->y;
                    float ball_bot = ball_pos->y - ball_rect->height;
                    float ball_left = ball_pos->x;
                    float ball_right = ball_pos->x + ball_rect->width;

                    float ball_center = ball_pos->y - (ball_rect->height / 2.0);
                    float paddle_y_center = paddle_pos->y - (paddle_rect->height / 2.0);
                    float ball_y_relative_to_paddle = ball_center - paddle_y_center;
                    float ball_y_relative_to_paddle_normalized = ball_y_relative_to_paddle / (paddle_rect->height / 2.0);

                    float ball_new_vel_dx_sign = (ball_vel->dx > 0 ? -1.0 : 1.0);

                    // Correct case where ball center is above paddle
                    if (std::abs(ball_y_relative_to_paddle_normalized) > 1.0)
                        ball_y_relative_to_paddle_normalized = (ball_y_relative_to_paddle_normalized < 0 ? -1.0 : 1.0);

                    // Fix the collision
                    while (overlap(paddle_pos, paddle_rect, ball_pos, ball_rect))
                    {
                        ball_pos->x -= ball_vel->dx;
                        ball_pos->y -= ball_vel->dy;
                    }

                    // Set the new velocity
                    float new_x_vel = ball_speed->speed * cos(MAX_BOUNCE_ANGLE * ball_y_relative_to_paddle_normalized) * ball_new_vel_dx_sign;
                    float new_y_vel = ball_speed->speed * sin(MAX_BOUNCE_ANGLE * ball_y_relative_to_paddle_normalized);
                    *ball_vel = {new_x_vel, new_y_vel};
                }
            }
        }
    };

    /**
     * @brief System to check 'Paddle-Wall' Collisions.
     * 
     * Components:
     *      - Position
     *      - Rectangle 
     *      - Side
     * 
     */
    class PaddleWallCollisionSystem : public ecs::system::System<pc::Position, pc::Rectangle, pc::Side>
    {
    private:
        float width;
        float height;

    public:
        PaddleWallCollisionSystem(float w, float h) : width(w), height(h) {}
        ~PaddleWallCollisionSystem() = default;
        void run(system_data data)
        {
            auto pos = std::get<0>(data);
            auto rect = std::get<1>(data);

            // Correct the position if the Paddle is off the edge of the screen.
            if (pos->y - rect->height < -this->height)
            {
                pos->y = -this->height + rect->height;
            }
            else if (pos->y > this->height)
            {
                pos->y = this->height;
            }
        }
    };

    /**
     * @brief System for drawing shapes to the screen
     * 
     * Components:
     *      - Position
     *      - Rectangle 
     *      - Color3
     * Note:
     *      Must be executed in the main thread in order for the Entities to be drawn properly.
     */
    class DrawSystem : public ecs::system::System<pc::Position, pc::Rectangle, pc::Color3>
    {
    private:
        float WINDOW_SIZE;

    public:
        DrawSystem(float window_size) : WINDOW_SIZE(window_size) {}
        ~DrawSystem() = default;
        float convert_from_pixel(float pixel) { return pixel / WINDOW_SIZE; }
        void run(system_data data)
        {
            auto pos = std::get<0>(data);
            auto rect = std::get<1>(data);
            auto color = std::get<2>(data);
            glColor3f(color->r, color->g, color->b);
            glRectf(
                convert_from_pixel(pos->x),
                convert_from_pixel(pos->y),
                convert_from_pixel(pos->x + rect->width),
                convert_from_pixel(pos->y - rect->height));
        }
    };

    /**
     * @brief System for drawing the Score to the screen. 
     * 
     * Components:
     *      - Position
     *      - Color 3 
     *      - Side
     *      - Score Resource
     *      - FontWrapper
     * 
     * Note:
     *      This must be run in the main thread in order for the text to be drawn properly.
     * 
     */
    class DrawTextSystem : public ecs::system::System<pc::Position, pc::Color3, pc::Text>
    {
    private:
        float WINDOW_SIZE;

    public:
        DrawTextSystem(float window_size) : WINDOW_SIZE(window_size) {}
        ~DrawTextSystem() = default;
        float convert_from_pixel(float pixel) { return pixel / WINDOW_SIZE; }
        void run(system_data data)
        {
            auto pos = std::get<0>(data);
            auto color = std::get<1>(data);
            auto text = std::get<2>(data);

            glColor3f(color->r, color->g, color->b);
            glRasterPos2f(convert_from_pixel(pos->x), convert_from_pixel(pos->y));
            for (auto &c : text->str)
                glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }
    };

    /**
     * @brief System for updating the Score Text.
     * 
     */
    class UpdateScoreTextSystem : public ecs::system::System<pc::Side, pc::Text, pr::ScoreResource>
    {
    public:
        UpdateScoreTextSystem() = default;
        ~UpdateScoreTextSystem() = default;
        void run(system_data data)
        {
            auto side = std::get<0>(data);
            auto text = std::get<1>(data);
            auto score_res = std::get<2>(data);

            if (*side == pc::Side::LEFT)
                text->str = std::to_string(score_res->SCORE_LEFT);
            else if (*side == pc::Side::RIGHT)
                text->str = std::to_string(score_res->SCORE_RIGHT);
        }
    };

    /**
     * @brief System for Updating 'Paddle' Control withthe Keyboard
     * 
     * Components:
     *      - Side
     *      - Velocity
     *      - Keyboard Resource
     */
    class KeyboardSystem : public ecs::system::System<pc::Side, pc::Velocity, pr::KeyboardResource>
    {
    private:
        float paddle_velocity;

    public:
        KeyboardSystem(float velocity) : paddle_velocity(velocity){};
        ~KeyboardSystem() = default;
        void run(system_data data)
        {
            auto *side = std::get<0>(data);
            auto *vel = std::get<1>(data);
            auto *res = std::get<2>(data);

            switch (*side)
            {
            case pc::Side::LEFT:
                switch (res->PADDLE_STATE_LEFT)
                {
                case pr::KeyboardResource::PaddleState::DOWN:
                    vel->dy = -this->paddle_velocity;
                    break;
                case pr::KeyboardResource::PaddleState::UP:
                    vel->dy = this->paddle_velocity;
                    break;
                case pr::KeyboardResource::PaddleState::STILL:
                    vel->dy = 0;
                    break;
                }
                break;
            case pc::Side::RIGHT:
                switch (res->PADDLE_STATE_RIGHT)
                {
                case pr::KeyboardResource::PaddleState::DOWN:
                    vel->dy = -this->paddle_velocity;
                    break;
                case pr::KeyboardResource::PaddleState::UP:
                    vel->dy = this->paddle_velocity;
                    break;
                case pr::KeyboardResource::PaddleState::STILL:
                    vel->dy = 0;
                    break;
                }
                break;
            }
        }
    };

    /**
     * @brief System for Spawning a ball with the Keyboard.
     * 
     * Components:
     *      - Keyboard Resource
     *      - World Resource
     *      - BallSpawner
     */
    class SpawnBallSystem : public ecs::system::System<pr::KeyboardResource, ecs::world::WorldResource, pc::BallSpawner>
    {
    private:
        size_t MAX_BALLS;
        float BALL_SPEED;
        float MAX_BALL_ANGLE;

    public:
        SpawnBallSystem(int max_balls, float ball_speed, float angle) : MAX_BALLS(max_balls), BALL_SPEED(ball_speed), MAX_BALL_ANGLE(angle) {}
        ~SpawnBallSystem() = default;
        float generate_random_angle()
        {
            return static_cast<float>(rand() / static_cast<float>(RAND_MAX / this->MAX_BALL_ANGLE));
        }
        void run(system_data data)
        {
            auto keyboard_res = std::get<0>(data);
            auto world_res = std::get<1>(data);

            if (keyboard_res->SHOULD_SPAWN_BALL)
            {
                ecs::registry::RegistryNode *ball_node;
                ball_node = world_res->world()->find<pc::Ball>();
                if (ball_node->size<pc::Ball>() < MAX_BALLS)
                {
                    float angle = generate_random_angle();
                    int x_sign = rand() % 2;
                    int y_sign = rand() % 2;
                    float x_vel, y_vel;

                    if (x_sign < 1)
                    {
                        x_vel = this->BALL_SPEED * cos(angle);
                    }
                    else
                    {
                        x_vel = -this->BALL_SPEED * cos(angle);
                    }

                    if (y_sign < 1)
                    {
                        y_vel = this->BALL_SPEED * sin(angle);
                    }
                    else
                    {
                        y_vel = -this->BALL_SPEED * sin(angle);
                    }
                    world_res->world()
                        ->build_entity()
                        .with<pc::Position>({0, 0})
                        .with<pc::Velocity>({x_vel, y_vel})
                        .with<pc::Rectangle>({25.0, 25.0})
                        .with<pc::Color3>({0.0, 0.0, 0.0})
                        .with<pc::Ball>({BALL_SPEED})
                        .build();
                }
                keyboard_res->SHOULD_SPAWN_BALL = false;
            }
        }
    };

    struct FPSSystem : public ecs::system::System<pc::FPSCounter, pc::Text>
    {
        void run(system_data data)
        {
            using namespace std::literals::chrono_literals;
            auto fps_count = std::get<0>(data);
            auto text = std::get<1>(data);
            auto this_frame = pc::FPSCounter::Clock::now();
            std::chrono::nanoseconds delta_t = std::chrono::duration_cast<std::chrono::nanoseconds>(this_frame - fps_count->prev_time);
            fps_count->prev_time = this_frame;

            text->str = "FPS: " + std::to_string((int)(1 / (delta_t.count() * 1.0e-9)));
        }
    };

    struct EntityCountSystem : public ecs::system::System<pc::EntityCounter, pc::Text, ecs::world::WorldResource>
    {
        void run(system_data data)
        {
            auto text = std::get<1>(data);
            auto world_res = std::get<2>(data);

            size_t entity_count = world_res->world()->find<ecs::entity::Entity>()->size<ecs::entity::Entity>();
            text->str = std::to_string(entity_count) + " Entities";
        }
    };
} // namespace pong::systems

#endif