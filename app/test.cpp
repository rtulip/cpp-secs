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

    auto node = w.find<Position>();
    if (node->check_type<Position>())
        std::cout << "Yup!" << std::endl;

    node = w.find<Velocity>();
    if (node->check_type<Velocity>())
        std::cout << "Yup!" << std::endl;

    w.build_entity()
        .with<Position>()
        .build();

    node = w.find<Entity>();
    auto e = node->get<Entity>(0);
    if (e.has_component(w.get_cid<Position>()))
        std::cout << "Yay!" << std::endl;
}
