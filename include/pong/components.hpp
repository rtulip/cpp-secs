#ifndef ecs_pong_components_hpp
#define ecs_pong_components_hpp
#include <cstddef>
#include <string>
#include <chrono>

namespace pong::components
{
    /**
     * @brief Position Component
     * 
     */
    struct Position
    {
        float x;
        float y;
    };

    /**
     * @brief 
     * 
     */
    struct Velocity
    {
        float dx;
        float dy;
    };

    /**
     * @brief Rectangle Shape Component
     * 
     */
    struct Rectangle
    {
        float width;
        float height;
    };

    /**
     * @brief RGB Color component
     * 
     */
    struct Color3
    {
        float r;
        float g;
        float b;
    };

    /**
     * @brief Side component.
     * 
     * Used to differentiate between Entities on the Left & Right side of the screen.
     * 
     */
    enum class Side
    {
        LEFT,
        RIGHT
    };

    /**
     * @brief Ball Marker Component
     * 
     * This doesn't have any associated data, but is used as an identifier for 'Ball' type
     * Entities.
     * 
     */
    struct Ball
    {
        float speed;
    };

    /**
     * @brief Ball Spawner Component
     * 
     * Used an an Identifer for a BallSpawner Entity. 
     * 
     */
    struct BallSpawner
    {
    };

    /**
     * @brief Text component
     * 
     * Has a string and a GLUT font pointer
     * 
     */
    struct Text
    {
        std::string str;
        void *font;
    };

    /**
     * @brief FPS Counter Component
     * 
     * Used for seeing how many frames per second are being computed. 
     * 
     */
    struct FPSCounter
    {
        using Clock = std::chrono::steady_clock;
        std::chrono::time_point<Clock> prev_time;
    };

    /**
     * @brief Component for counting the number of entities at a given moment.
     * 
     */
    struct EntityCounter
    {
    };
} // namespace pong::components

#endif
