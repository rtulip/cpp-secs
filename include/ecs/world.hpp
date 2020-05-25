#ifndef ecs_world_hpp
#define ecs_world_hpp
#include <iostream>
#include <memory>
#include <ecs/registry.hpp>
#include <ecs/entity.hpp>

using ecs::registry::Registry;
using ecs::entity::Entity;

namespace ecs::world {

class World
{
private:
    
    size_t num_components;
public:
    Registry registry;
    World(/* args */);
    ~World();
    World(const World& world);
    World(World&& world);

    size_t count_components(){ return num_components; }
    void add_entity();

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

    template<class T>
    WorldBuilder& with_component();
    World build();
};

World::World(/* args */){ num_components = 0; }
World::~World(){}
World::World(World&& w){
    registry = w.registry;
    num_components = w.num_components;
}

void World::add_entity(){
    auto iter = registry.iter_to<Entity>();
    size_t eid = (*iter).get_array<Entity>()->size();
    (*iter).push(Entity(eid, num_components));
}

World::WorldBuilder::WorldBuilder(){}
World::WorldBuilder::~WorldBuilder(){}

template<class T>
World::WorldBuilder& World::WorldBuilder::with_component(){
    world.registry.register_component<T>();
    world.num_components++;
    return *this;
}

World World::WorldBuilder::build(){
    world.registry.register_entities();
    return std::move(world); 
}

World::WorldBuilder World::create(){
    World::WorldBuilder wb;
    return wb;
}

}
#endif