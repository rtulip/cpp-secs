#include <iostream>
#include <ecs/world.hpp>

using ecs::world::World;

struct Position
{
    int64_t x;
    int64_t y;
};

struct Velocity
{
    int64_t dx;
    int64_t dy;
};

int main()
{

    auto w = World::create()
                 .with_component<Position>()
                 .with_component<Velocity>()
                 .build();

    w.build_entity()
        .with<Position>({1, 1})
        .with<Velocity>({2, 2})
        .build();

    auto node = w.find<Entity>();
    auto e = node->get<Entity>(0);
    if (e.has_component(w.get_cid<Position>()))
        std::cout << "Yay!" << std::endl;

    node = w.find<Position>();
    Position pos = node->get<Position>(e.get_component(w.get_cid<Position>()));
    std::cout << "Position: \n\tx: " << pos.x << "\n\ty: " << pos.y << std::endl;

    node = w.find<Velocity>();
    Velocity vel = node->get<Velocity>(e.get_component(w.get_cid<Velocity>()));
    std::cout << "Velocity: \n\tx: " << vel.dx << "\n\ty: " << vel.dy << std::endl;
}
