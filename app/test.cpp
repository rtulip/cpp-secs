
#include <ecs/world.hpp>
#include <ecs/system.hpp>
#include <ecs/entity.hpp>
#include <tuple>
#include <iostream>
#include <chrono>

using ecs::entity::Entity;
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

class PositionPrinterSystem : public System<Position>
{
public:
    PositionPrinterSystem(/* args */) = default;
    ~PositionPrinterSystem() = default;
    void run(system_data data)
    {
        const Position *pos = std::get<0>(data);
        std::cout << "Pos:\n\tx: " << pos->x << "\n\ty: " << pos->y << std::endl;
    }
};

class VelocityPrinterSystem : public System<Velocity>
{
public:
    VelocityPrinterSystem(/* args */) = default;
    ~VelocityPrinterSystem() = default;
    void run(system_data data)
    {
        const Velocity *vel = std::get<0>(data);
        std::cout << "Vel:\n\tx: " << vel->dx << "\n\ty: " << vel->dy << std::endl;
    }
};

class Exe : public System<Entity>
{
private:
    std::string s;

public:
    Exe(std::string str) { this->s = str; }
    ~Exe() = default;
    void run(system_data data)
    {
        const Entity *e = std::get<0>(data);
        std::cout << this->s << " " << e->eid() << std::endl;
    }
};

int main()
{

    {
        auto world = World::create()
                         .with_component<Position>()
                         .with_component<Velocity>()
                         .build();

        for (int i = 0; i < 10; i++)
        {
            auto b = world.build_entity()
                         .with<Position>({i, i});
            if (i % 2 == 0)
                b.with<Velocity>({i, i});
            b.build();
        }

        std::cout << "Finished building Entities" << std::endl;

        MovementSystem move_sys(2, 1);
        PositionPrinterSystem pos_print;
        VelocityPrinterSystem vel_print;
        world.add_systems()
            .add_system(&move_sys, "MovementSys", {})
            .add_system(&pos_print, "PositionPrinter", {"MovementSys"})
            // .add_system(&vel_print, "VelocityPrinter", {"MovementSys"}) // This will run in parallel with Position Printer
            // .add_system(&vel_print, "VelocityPrinter", {"MovementSys", "PositionPrinter"}) // This will run in series after position printer
            .done();

        std::cout << "Finished adding Systems" << std::endl;

        auto t1 = std::chrono::high_resolution_clock::now();
        world.dispatch();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

        std::cout << "dispatch time: " << duration << std::endl;
        std::cout << "--------------------------------" << std::endl;
        t1 = std::chrono::high_resolution_clock::now();
        world.dispatch();
        t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        std::cout << "dispatch time: " << duration << std::endl;
    }

    {
        auto world = World::create().build();
        for (int i = 0; i < 30; i++)
            world.build_entity().build();

        Exe A("A");
        Exe B("B");
        Exe C("C");
        Exe D("D");
        Exe E("E");
        Exe F("F");
        Exe G("G");
        Exe H("H");
        Exe I("I");
        Exe J("J");
        Exe K("K");
        Exe L("L");
        Exe M("M");

        world.add_systems()
            // First logical group
            .add_system(&C, "Sys C", {})
            .add_system(&E, "Sys E", {})
            // Second logical group
            .add_system(&A, "Sys A", {"Sys C", "Sys E"})
            .add_system(&B, "Sys B", {"Sys C"})
            // Third logical group
            .add_system(&D, "Sys D", {"Sys B", "Sys A", "Sys E"})
            .add_system(&F, "Sys F", {"Sys E"})
            // Fourth logical group
            .add_system(&G, "Sys G", {"Sys D"})
            .add_system(&H, "Sys H", {"Sys D"})
            .add_system(&K, "Sys K", {"Sys F"})
            // Fifth locial group
            .add_system(&I, "Sys I", {"Sys H", "Sys G"})
            .add_system(&J, "Sys J", {"Sys H", "Sys F", "Sys K"})
            // Sixth logical group
            .add_system(&L, "Sys L", {"Sys J", "Sys I"})
            .add_system(&M, "Sys M", {"Sys J"})
            .done();

        std::cout << "Dispatching! " << std::endl;

        world.dispatch();
    }
}
