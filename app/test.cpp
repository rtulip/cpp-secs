#include <iostream>
#include <ecs/world.hpp>
#include <tuple>

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

class MovementSystem
{
private:
    int64_t fx, fy;

public:
    typedef std::tuple<Position *, Velocity *> system_data;
    MovementSystem(int64_t, int64_t);
    ~MovementSystem();
    void run(system_data data);
};

MovementSystem::MovementSystem(int64_t x, int64_t y)
{
    fx = x;
    fy = y;
}

MovementSystem::~MovementSystem()
{
}

void MovementSystem::run(MovementSystem::system_data data)
{
    Position *pos = std::get<0>(data);
    Velocity *vel = std::get<1>(data);

    pos->x += vel->dx;
    pos->y += vel->dy;

    if (vel->dx >= this->fx)
        vel->dx -= this->fx;
    else if (vel->dx <= -this->fx)
        vel->dx += this->fx;
    else
        vel->dx = 0;

    if (vel->dy >= this->fy)
        vel->dy -= this->fy;
    else if (vel->dy <= -this->fy)
        vel->dy += this->fy;
    else
        vel->dy = 0;
}

int main()
{

    auto w = World::create()
                 .with_component<Position>()
                 .with_component<Velocity>()
                 .build();

    MovementSystem sys(2, 1);

    w.build_entity()
        .with<Velocity>({2, 2})
        .build();

    w.build_entity()
        .with<Position>({1, 1})
        .build();

    w.build_entity()
        .with<Position>({4, 4})
        .with<Velocity>({2, 2})
        .build();

    w.build_entity()
        .with<Position>({5, 5})
        .with<Velocity>({2, 2})
        .build();

    w.build_entity()
        .with<Position>({6, 6})
        .with<Velocity>({2, 2})
        .build();

    w.build_entity()
        .with<Position>({7, 7})
        .with<Velocity>({2, 2})
        .build();

    auto node = w.find<Entity>();

    size_t count = 1;
    size_t pos_id = w.get_cid<Position>();
    size_t vel_id = w.get_cid<Velocity>();
    for (auto e : *(node->iter<Entity>()))
    {

        if (e.has_component(pos_id) && e.has_component(vel_id))
        {
            MovementSystem::system_data data = std::make_tuple(
                w.find<Position>()->get<Position>(e.get_component(pos_id)),
                w.find<Velocity>()->get<Velocity>(e.get_component(vel_id)));

            std::cout << "Before\nEntity " << count
                      << "\n\tPos: "
                      << "{ " << std::get<0>(data)->x << ", " << std::get<0>(data)->y << "}"
                      << "\n\tVel: "
                      << "{ " << std::get<1>(data)->dx << ", " << std::get<1>(data)->dy << "}"
                      << std::endl;

            sys.run(data);

            data = std::make_tuple(
                w.find<Position>()->get<Position>(e.get_component(pos_id)),
                w.find<Velocity>()->get<Velocity>(e.get_component(vel_id)));

            std::cout << "After\nEntity " << count
                      << "\n\tPos: "
                      << "{ " << std::get<0>(data)->x << ", " << std::get<0>(data)->y << "}"
                      << "\n\tVel: "
                      << "{ " << std::get<1>(data)->dx << ", " << std::get<1>(data)->dy << "}"
                      << std::endl;
        }
        else
        {
            std::cout << "Entity " << count << " does not meet requirements" << std::endl;
        }
        count++;
    }
}
