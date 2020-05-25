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
        size_t get_eid();

        size_t count_components();

    public:
        World(/* args */) = default;
        ~World() = default;
        World(const World &world) = default;
        World(World &&world) = default;

        void add_entity();
        void add_entity(Entity &&entity);

        template <class T>
        RegistryNode *find();
        template <class T>
        size_t get_cid();

        class WorldBuilder;
        static WorldBuilder create();

        class EntityBuilder;
        friend class EntityBuilder;
        EntityBuilder build_entity();
    };

    class World::WorldBuilder
    {
    private:
        World world;

    public:
        WorldBuilder() = default;
        ~WorldBuilder() = default;

        template <class T>
        WorldBuilder &with_component();
        World build();
    };

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

    class World::EntityBuilder
    {
    private:
        Entity entity;
        World *world_ptr;

    public:
        EntityBuilder(World *ptr);
        ~EntityBuilder(){};

        template <class T>
        EntityBuilder &with();
        void build();
    };

    World::EntityBuilder::EntityBuilder(World *ptr) : entity(ptr->get_eid(), ptr->count_components())
    {
        this->world_ptr = ptr;
    }

    template <class T>
    World::EntityBuilder &World::EntityBuilder::with()
    {
        entity.add_component(world_ptr->get_cid<T>());
        return *this;
    }

    void World::EntityBuilder::build()
    {
        world_ptr->add_entity(std::move(entity));
    }

    World::EntityBuilder World::build_entity()
    {
        World::EntityBuilder entity_builder(this);
        return entity_builder;
    }

    size_t World::get_eid()
    {
        return this->next_eid++;
    }

    template <class T>
    size_t World::get_cid()
    {
        if (!this->has_component<T>())
            throw std::runtime_error("Component is not registered");
        return this->node_index_lookup[typeid(T).hash_code()];
    }

    size_t World::count_components()
    {
        return this->nodes.size();
    }

    void World::add_entity()
    {
        auto node = this->find<Entity>();
        node->push<Entity>(Entity(this->next_eid++, this->nodes.size()));
    }

    void World::add_entity(Entity &&entity)
    {
        auto node = this->find<Entity>();
        node->push<Entity>(std::move(entity));
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