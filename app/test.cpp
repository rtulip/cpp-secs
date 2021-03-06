
#include <ecs/world.hpp>
#include <ecs/system.hpp>
#include <ecs/entity.hpp>
#include <tuple>
#include <iostream>
#include <chrono>
#include <string>

using ecs::entity::Entity;
using ecs::system::System;
using ecs::world::World;
using ecs::world::WorldResource;

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

class ResourceUser : public System<std::string>
{
public:
    ResourceUser() = default;
    ~ResourceUser() = default;
    void run(system_data data)
    {
        const std::string *str = std::get<0>(data);
        std::cout << "String Resource: " << *str << std::endl;
    }
};

class EntityAdder : public System<Entity, WorldResource>
{
public:
    EntityAdder() = default;
    ~EntityAdder() = default;

    void run(system_data data)
    {
        WorldResource *world_res = std::get<1>(data);
        std::cout << "Entity Is making another entity!" << std::endl;
        world_res->world()->build_entity().build();
    }
};

class EntityAccessor : public System<Entity>
{
public:
    EntityAccessor() = default;
    ~EntityAccessor() = default;

    void run(system_data data)
    {
        Entity *e = std::get<0>(data);
        std::cout << "Looking at entity: " << e->eid() << std::endl;
    }
};

struct ToRemove
{
};

class EntityComponentRemover : public System<Entity, WorldResource, ToRemove>
{
public:
    EntityComponentRemover() = default;
    ~EntityComponentRemover() = default;
    void run(system_data data)
    {
        Entity *e = std::get<0>(data);
        WorldResource *world_res = std::get<1>(data);
        world_res->remove_entity_component<ToRemove>(e);
    }
};

class EntityRemover : public System<Entity, WorldResource, ToRemove>
{
public:
    EntityRemover() = default;
    ~EntityRemover() = default;
    void run(system_data data)
    {
        Entity *e = std::get<0>(data);
        WorldResource *world_res = std::get<1>(data);
        world_res->remove_entity(e);
    }
};

class EntityPrinter : public System<Entity>
{
public:
    EntityPrinter() = default;
    ~EntityPrinter() = default;
    void run(system_data data)
    {
        Entity *e = std::get<0>(data);
        std::cout << "Entity " << e->eid() << " is in the world!" << std::endl;
    }
};

template <class T>
class ComponentAdder : public System<Entity, WorldResource>
{
public:
    ComponentAdder() = default;
    ~ComponentAdder() = default;
    void run(system_data data)
    {
        Entity *e = std::get<0>(data);
        WorldResource *world_res = std::get<1>(data);
        size_t cid = world_res->world()->get_cid<T>();
        T t;
        if (!e->has_component(cid))
            world_res->add_component_to_entity(e, std::move(t));
    }
};

class ToRemovePrinter : public System<Entity, ToRemove>
{
public:
    ToRemovePrinter() = default;
    ~ToRemovePrinter() = default;
    void run(system_data data)
    {
        Entity *e = std::get<0>(data);
        std::cout << "Entity " << e->eid() << " Has a ToRemove!" << std::endl;
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
        auto world = World::create()
                         .build();
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

    {

        auto world = World::create()
                         .add_resource(std::string("This is a global string!"))
                         .build();

        for (int i = 0; i < 10; i++)
            world.build_entity().build();

        ResourceUser resource_user;
        world.add_systems()
            .add_system(&resource_user, "Test Resource User", {})
            .done();

        world.dispatch();
    }

    {
        auto world = World::create().build();

        for (int i = 0; i < 10; i++)
            world.build_entity().build();

        EntityAdder adder;
        EntityAccessor accessor;
        world.add_systems()
            .add_system(&adder, "Entity Adder", {})
            .add_system(&accessor, "Entity Accessor", {"Entity Adder"})
            .done();

        std::cout << "Starting Adder dispatch!" << std::endl;

        world.dispatch();
        std::cout << "---------------------" << std::endl;
        world.dispatch();
    }

    {
        auto world = World::create()
                         .with_component<ToRemove>()
                         .build();

        for (int i = 0; i < 10; i++)
        {
            auto builder = world.build_entity();
            if (i % 2 == 0)
                builder.with<ToRemove>({});
            builder.build();
        }

        EntityComponentRemover remover;
        ToRemovePrinter before_printer;
        ToRemovePrinter after_printer;

        world.add_systems()
            .add_system(&before_printer, "Before Printer", {})
            .add_system(&remover, "Remover", {"Before Printer"})
            .add_system(&after_printer, "After Printer", {"Remover"})
            .done();

        world.dispatch();
    }

    {
        std::cout << " ---------- Test remove Entity ----------" << std::endl;

        auto world = World::create()
                         .with_component<ToRemove>()
                         .build();

        for (int i = 0; i < 10; i++)
        {
            auto builder = world.build_entity();
            if (i % 2 == 0)
                builder.with<ToRemove>({});
            builder.build();
        }

        EntityRemover remover;
        EntityPrinter entity_printer;
        ToRemovePrinter to_remove_printer;

        world.add_systems()
            .add_system(&entity_printer, "Entity Printer", {})
            .add_system(&to_remove_printer, "To Remove Printer", {"Entity Printer"})
            .add_system(&remover, "Entity Remover", {"To Remove Printer"})
            .add_system(&to_remove_printer, "After To Remove Printer", {"Entity Remover"})
            .done();

        for (int i = 0; i < 5; i++)
        {
            world.dispatch();
            std::cout << "----- End of Dispatch -----" << std::endl;
        }
    }

    {
        std::cout << "--- Test Add & remove! ---" << std::endl;
        auto world = World::create()
                         .with_component<ToRemove>()
                         .build();

        for (int i = 0; i < 10; i++)
        {
            auto builder = world.build_entity();
            if (i % 2 == 0)
                builder.with<ToRemove>({});
            builder.build();
        }

        EntityComponentRemover remover;
        ComponentAdder<ToRemove> adder;
        ToRemovePrinter to_remove_printer;

        world.add_systems()
            .add_system(&to_remove_printer, "Before Printer", {})
            .add_system(&adder, "Adder", {"Before Printer"})
            .add_system(&remover, "Remover", {"Before Printer"})
            // .add_system(&to_remove_printer, "After Printer", {"Remover", "Adder"})
            .done();

        for (int i = 0; i < 5; i++)
        {
            world.dispatch();
            std::cout << "----- End of Dispatch -----" << std::endl;
        }
    }

    {
        std::cout << "------------ Entity Removal Order ----------" << std::endl;
        struct C1
        {
        };
        struct C4
        {
        };

        struct SysC1 : public System<C1>
        {
            void run(system_data) {}
        } sys_c1;
        struct SysC4 : public System<C4>
        {
            void run(system_data) {}
        } sys_c4;

        auto world = World::create()
                         .with_component<C1>()
                         .with_component<C4>()
                         .with_component<ToRemove>()
                         .build();

        world.add_systems()
            .add_system(&sys_c1, "C1", {})
            .add_system(&sys_c4, "C4", {"C1"})
            .done();

        auto world_res = world.find<WorldResource>()->get<WorldResource>(0);

        world.build_entity() // EID = 0;
            .build();

        world.build_entity() // EID = 1 (TARGET);
            .with<C1>({})    // idx = 1
            .build();

        world.build_entity() // EID = 2
            .with<C1>({})    // idx = 1
            .build();

        world.build_entity() // EID = 3
            .with<C1>({})    // idx = 2
            .build();

        world.build_entity() // EID = 4 (TARGET)
            .with<C1>({})    // idx = 3
            .with<C4>({})    // idx = 0
            .build();

        world.build_entity() // EID = 5
            .build();

        world.build_entity() // EID = 6
            .build();

        Entity *e = world.find<Entity>()->get<Entity>(1);
        world_res->remove_entity(e);
        world.dispatch();
        e = world.find<Entity>()->get<Entity>(4);
        world_res->remove_entity(e);
        world.dispatch();
    }
}
