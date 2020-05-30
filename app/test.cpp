
#include <ecs/world.hpp>
#include <ecs/system.hpp>
#include <tuple>
#include <iostream>

using ecs::system::System;
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

class MovementSystem : public System<Position, Velocity>
{
private:
    int64_t fx, fy;

public:
    MovementSystem(int64_t, int64_t);
    ~MovementSystem() = default;
    void run(system_data data);
};

MovementSystem::MovementSystem(int64_t x, int64_t y)
{
    fx = x;
    fy = y;
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

class PositionPrinterSystem : public System<const Position>
{
public:
    PositionPrinterSystem(/* args */) = default;
    ~PositionPrinterSystem() = default;
    void run(system_data data);
};

void PositionPrinterSystem::run(PositionPrinterSystem::system_data data)
{
    const Position *pos = std::get<0>(data);
    std::cout << "Pos:\n\tx: " << pos->x << "\n\ty: " << pos->y << std::endl;
}

class VelocityPrinterSystem : public System<const Velocity>
{
public:
    VelocityPrinterSystem(/* args */) = default;
    ~VelocityPrinterSystem() = default;
    void run(system_data data);
};

void VelocityPrinterSystem::run(VelocityPrinterSystem::system_data data)
{
    const Velocity *vel = std::get<0>(data);
    std::cout << "Vel:\n\tx: " << vel->dx << "\n\ty: " << vel->dy << std::endl;
}

int main()
{

    auto world = World::create()
                     .with_component<Position>()
                     .with_component<Velocity>()
                     .build();

    for (int i = 0; i < 20; i++)
    {
        auto b = world.build_entity()
                     .with<Position>({i, i});
        if (i % 2 == 0)
            b.with<Velocity>({i, i});
        b.build();
    }

    MovementSystem sys(2, 1);
    PositionPrinterSystem pos_print;
    VelocityPrinterSystem vel_print;
    world.add_system(&pos_print);
    world.add_system(&vel_print);
    world.add_system(&sys);

    world.dispatch();
    std::cout << "--------------------------------" << std::endl;
    world.dispatch();
}
