#ifndef ecs_world_builder_hpp
#define ecs_world_builder_hpp

#include <ecs/world.hpp>

namespace ecs::world
{

    /**
     * @brief WorldBuilder is a builder class which constructs a World with components
     * 
     * In order to more easily register components to the world, this builder class
     * is used. Components can easily be registered by using .with_component<T>()
     * Upon calling .build() the World has been defined and is returned.
     * 
     */
    class World::WorldBuilder
    {
    private:
        World world;

    public:
        WorldBuilder() = default;
        ~WorldBuilder() = default;
        WorldBuilder(WorldBuilder &&) = default;

        template <class T>
        WorldBuilder &with_component();
        World build();
    };

    /**
     * @brief Registers a component T to the World being built.
     * 
     * @tparam T - The component to be registered
     * @return World::WorldBuilder& - This WorldBuilder
     */
    template <class T>
    World::WorldBuilder &World::WorldBuilder::with_component()
    {
        world.register_component<T>();
        return *this;
    }

    /**
     * @brief Finishes building the world.
     * 
     * @return World - The built world.
     */
    World World::WorldBuilder::build()
    {
        world.register_component<Entity>();
        return std::move(world);
    }

    /**
     * @brief The public world constructor. 
     * 
     * @return World::WorldBuilder
     */
    World::WorldBuilder World::create()
    {
        World::WorldBuilder wb;
        return wb;
    }

} // namespace ecs::world

#endif