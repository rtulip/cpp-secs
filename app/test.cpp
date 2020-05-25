#include <iostream>
#include <ecs/registry.hpp>
#include <ecs/world.hpp>

using ecs::world::World;
using ecs::registry::Registry;

struct Position {
    int64_t x;
    int64_t y;
};

struct Velocity {
    int64_t dx;
    int64_t dy;
};

int main() {

    { // Testing world stuffs

        auto w = World::create()
            .with_component<Position>()
            .with_component<Velocity>()
            .build();
        std::cout << w.count_components() << std::endl;

        auto iter = w.registry.iter_to<Velocity>();
        if ((*iter).check_type<Velocity>()) std::cout << "Holy shit it worked!" << std::endl;
        
        w.add_entity();
        w.add_entity();
        w.add_entity();

        iter = w.registry.iter_to<Entity>();
        if ((*iter).get_array<Entity>()->size() == 3) std::cout << "Found 3 entities!" << std::endl;

    } // End testing world stuffs


}
