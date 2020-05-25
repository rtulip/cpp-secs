#ifndef ecs_world_hpp
#define ecs_world_hpp
#include <iostream>
#include <memory>
#include <ecs/registry.hpp>
#include <ecs/entity.hpp>

using ecs::entity::Entity;
using ecs::registry::RegistryNode;

namespace ecs::world
{

    class World
    {
    private:
        size_t next_eid;
        std::vector<RegistryNode> nodes;
        std::unordered_map<size_t, size_t> node_index_lookup;

        template <class T>
        void register_component();
        template <class T>
        bool has_component();

    public:
        World(/* args */) = default;
        ~World() = default;
        World(const World &world) = default;
        World(World &&world) = default;

        void add_entity();

        template <class T>
        RegistryNode *find();

        class WorldBuilder;
        static WorldBuilder create();
    };

    class World::WorldBuilder
    {
    private:
        World world;

    public:
        WorldBuilder();
        ~WorldBuilder();

        template <class T>
        WorldBuilder &with_component();
        World build();
    };

    void World::add_entity()
    {
        auto node = this->find<Entity>();
        node->push<Entity>(Entity(this->next_eid++, this->nodes.size()));
    }

    World::WorldBuilder::WorldBuilder() {}
    World::WorldBuilder::~WorldBuilder() {}

    template <class T>
    World::WorldBuilder &World::WorldBuilder::with_component()
    {
        world.register_component<T>();
        return *this;
    }

    World World::WorldBuilder::build()
    {
        world.register_component<Entity>();
        return std::move(world);
    }

    World::WorldBuilder World::create()
    {
        World::WorldBuilder wb;
        return wb;
    }

    template <class T>
    bool World::has_component()
    {
        return this->node_index_lookup.find(typeid(T).hash_code()) != this->node_index_lookup.end();
    }

    template <class T>
    void World::register_component()
    {
        if (this->has_component<T>())
            throw std::runtime_error("Component is already registered");
        this->node_index_lookup.emplace(typeid(T).hash_code(), this->nodes.size());
        this->nodes.push_back(RegistryNode::create<T>());
    }

    template <class T>
    RegistryNode *World::find()
    {
        if (!this->has_component<T>())
            throw std::runtime_error("Component is not registered");
        size_t idx = this->node_index_lookup[typeid(T).hash_code()];
        return &this->nodes[idx];
    }

} // namespace ecs::world
#endif