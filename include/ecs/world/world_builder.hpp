#ifndef ecs_world_builder_hpp
#define ecs_world_builder_hpp

#include <ecs/world.hpp>
#include <ecs/entity.hpp>
#include <vector>

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
        std::vector<size_t> resource_idxs;

    public:
        WorldBuilder() = default;
        ~WorldBuilder() = default;
        WorldBuilder(WorldBuilder &&) = default;

        template <class T>
        WorldBuilder &with_component();
        template <class T>
        WorldBuilder &add_resource(T &&t);
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
     * @brief Adds a resource of type T to the World being built.
     * 
     * This consumes the T.
     * 
     * @tparam T - The component to be registered
     * @return World::WorldBuilder& - This WorldBuilder
     */
    template <class T>
    World::WorldBuilder &World::WorldBuilder::add_resource(T &&t)
    {
        world.add_resource<T>(std::move(t));
        return *this;
    }

    /**
     * @brief Finishes building the world.
     * 
     * @return World - The built world.
     */
    World World::WorldBuilder::build()
    {
        world.component_mask = ecs::entity::bitset(world.count_components());
        int i = 0;
        for (auto node : world.nodes)
        {
            if (!node.is_resource())
                world.component_mask.set(i);
            i++;
        }
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