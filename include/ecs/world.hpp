#ifndef ecs_world_hpp
#define ecs_world_hpp

/** \mainpage SECS - A Stateful Entity Component System
 * @author Robbie Tulip - V00846133 (rtulip@uvic.ca)
 * 
 * ## Introduction
 * This is my final project of UVic SEng 475 where I explored the creation of an Entity
 * Component System with Stateful Systems in C++. On this main page, I'll provide an
 * overview of what an ECS is, where it is used, and why. Then I'll go into more specific
 * implementation details, summarizing the different parts of this ECS, and I'll finish
 * with work which could be done to further improve this implementation. Along the way
 * I'll provide code examples demonstrating how this library can be used.
 * 
 * ## What is an ECS
 * An Entity Component System (ECS) is a common example of what is known as Data Oriented
 * Design (DOD), where in order to improve efficiency, the underlying data is considered
 * from the start of building a program.
 * 
 * In short, an ECS is a design pattern built up on three parts: Entities, Components,
 * and Systems.
 * 
 * Components are simply instances of data. Entities are an identifier, which group
 * components together to act as an 'Object', and systems are behaviour which operate on
 * a set of components. 
 * 
 * Entity Component Systems are a commonly used pattern in game design, where inheritance
 * trees to define behaviour get very deep and complex. 
 * <a href="https://medium.com/ingeniouslysimple/entities-components-and-systems-89c31464240d">Mark Jordan</a>
 * wrote a short blog 
 * post which provides a good example of an Object Oriented inheritance tree which could
 * be seen in game development, as well as provides some example types of objects which
 * are desireable, but don't fit well into the example inheritance structure.
 * 
 * ## Introduction to SECS
 * SECS is built up upon just a few core classes which build the behaviour of the ECS. 
 * The ecs::world::World class, is an interface into the ECS, and provides means to work
 * with entities, components, and systems. The ecs::system::System class can be inherited
 * to easily define how a system is run. The ecs::entity::Entity class is a component 
 * which is automatically added to the ecs::world::World, and works as an identifier and 
 * bookkeeper for other associated components.
 * 
 * SECS garantees that all components of the same type are stored contiguously in memory,
 * for more cache-friendly code. Additionally, SECS performs analysis on 
 * ecs::system::System dependencies and will automatically run ecs::system::System's in
 * parallel when possible. SECS also allows for users to define quasi-global variables,
 * called 'Resources'. On top of all of this, SECS provides an easy API for building the
 * ecs::world::World, building entities comprising of different components, adding 
 * resources, as well as defining and executing systems. 
 * 
 * Below is a quick example of how SECS can be used to make a System, register components
 * to the World, build entities with different sets of components, and run the system. 
 * 
 * ```cpp
 * 
 * #include <ecs/world.hpp>
 * #include <ecs/system.hpp>
 * 
 * // A Position Component
 * struct Position 
 * {
 *     float x;
 *     float y;
 * };
 * // A Velocity Component
 * struct Velocity 
 * {
 *     float dx; 
 *     float dy;
 * };
 * // A sample resource for a friction variable
 * struct FrictionResource
 * {
 *     float friction;
 * };
 * 
 * // System which operates on each Entity with Position and Velocity. 
 * struct MovementSystem : ecs::system::System<Position, Velocity, FrictionResource> 
 * {
 *     // The function which is called if an Entity has both Position and Velocity.
 *     // system_data is a type alias around a tuple of pointers to the Entity data 
 *     // members to make the API nicer. A pointer to the same FrictionResource component
 *     // is returned for every Entity with Position and Velocity.
 *     void run(system_data data)
 *     {
 *         // Access elements in the system_data tuple.
 *         Position* pos = std::get<0>(data);
 *         auto vel = std::get<1>(data);
 *         auto friction = std::get<2>(data);
 *         // Increase the Position in the x & y by the velocity in each direction.
 *         pos->x += vel->dx - friction->friction;
 *         pos->y += vel->dy - friction->friction;
 *     }
 * };
 * 
 * int main()
 * {
 *     MovementSystem move_sys;
 * 
 *     // Define the 'scope' of the world, i.e. which components can be used.
 *     auto world = ecs::world::World::create()
 *                      .with<Position>()
 *                      .with<Velocity>()
 *                      .add_resource<FrictionResource>({0.5})
 *                      .build();
 * 
 *     // Add systems to the world & determine execution order.
 *     world.add_systems()
 *          // Adds the movement system, gives a name, and no dependencies.
 *          .add_system(&move_sys, "Movement System", {})
 *          .done();
 * 
 *     // Build some Entities
 *     for (float i = 0; i < 100; i++)
 *     {
 *          if (i % 2 == 0)
 *          {
 *              // Some have both Position & Velocity 
 *              world.build_entity()
 *                  .with<Position>({i, i*2})
 *                  .with<Velocity>({i*3, i})
 *                  .build();
 *          } 
 *          else
 *          {
 *              // Some only have Position
 *              world.build_entity()
 *                  .with<Position>({i*2, i})
 *                  .build();
 *          }
 *     }
 * 
 *     // Run all the systems. In this case there's only one.
 *     world.dispatch();
 *     
 * }
 * ```
 * 
 * This examples shows most of the features which SECS provides. There are some more 
 * advanced features which will be covered later in more detail. 
 * 
 * From here, the different classes will be borken down into more detail, including 
 * important notes on their implementation. 
 * 
 * ## The World Class
 * This class holds two important pieces of data: the component registry, and the
 * dispatcher.
 * 
 * ### Component & Resource Storage
 * The component registry is the storage container for each of the components and 
 * Resources. It can be thought of as a 2 dimensional array, where each column holds a
 * different data type. Behind the scenes, the component registry uses a class, 
 * ecs::registry::RegistryNode, to keep components of the same type contiguous in memory. 
 * 
 * The ecs::registry::RegistryNode class is a non-templated class, which behaves as a 
 * wrapper around the std::vector class. In order for a RegistryNode to be non-templated,
 * each function that operates on a ecs::registry::RegistryNode is templated in exchange.
 * This allows the World to have a single container (std::vector) of 
 * ecs::registry::RegistryNode for each component and resource. 
 * 
 * The ecs::registry::RegistryNode class handles resources in the same way as components,
 * but it ensuresonly one instance of a resource is kept at any given time, and accessing
 * a resource ignores always returns a pointer to the single instance.
 * 
 * ### System Dispatching
 * The ecs::world::World ecs::dispatch::DispatcherContainer is a simple data structure
 * which groups Systems into 'stages' (which can be executed in parallel). Under the 
 * hood, this is a vector of vectors of ecs::dispatch::Executable pointers, where the
 * ecs::dispatch::Executable class is used to generalize ecs::system::System execution,
 * without template arguments. The ecs::system::System and ecs::dispatch::Executable
 * classes will be discussed in more detail later. 
 * 
 * When adding an ecs::system::System to the ecs::world::World, you have to provide the
 * following: a pointer to the ecs::system::System, an identifier, and an initilizer list
 * of dependencies. Using this information, a graph is constructed. BFS topological sort
 * is then used to find systems which can be executed in parallel. The process is simple: 
 * find all nodes with in-degree of zero, add them to a ecs::dispatch::DispatchStage, 
 * remove them from the graph, and repeat until no nodes are left in the graph. Each 
 * 
 * ### World Builder
 * Creating a ecs::world::World is done via the ecs::world::WorldBuilder class. This 
 * class provides functions to register components & add resources to the World. This
 * helps remove complexity of altering the underlying data structures at of the World 
 * after it has been 'built'.
 * 
 * ### Default Components & Resources
 * When you build a ecs::world::World, the ecs::entity::Entity component and the 
 * ecs::world::WorldResource resource are added in addition to the components and 
 * resources defined by the programmer. The ecs::entity::Entity component is a glorified
 * identifier, and is what is used to keep track of each ec::entity::Entity and it's 
 * related components. The ecs::world::WorldResource is a resource which provides means 
 * to operate on the ecs::world::World itself within systems. It provides an API for
 * altering entities, and an accessor to the ecs::world::World itself. This is one of
 * the areas in the project which can be worked on the most to further restrict which
 * ecs::world::World functions can be used within Systems to ensure that behaviour is 
 * well defined. By exposing the World* to systems, it may be possible for the programmer
 * to cause undefined behaviour. For the moment, functions which operate on the world
 * have been labeld as 'System Safe' if they are safe to use in a System, and it is the 
 * programmers responsibility to ensure that only 'System Safe' functions are used in
 * System's run function.
 * 
 * Below is an example of a system which uses the ecs::world::WorldResource to remove all
 * Entities which have a 'ToRemove' component from the ecs::world::World.
 * 
 * ```cpp
 * 
 * struct ToRemove {};
 * 
 * struct EntityRemover : ecs::system::System<ToRemove, ecs::entity::Entity, ecs::world::WorldResource> 
 * {
 *      void run(system_data data)
 *      {
 *          auto entity = std::get<1>(data);
 *          auto world_res = std::get<2>(data);
 *          
 *          world_res->remove_entity(entity);
 *      }
 * }
 * 
 * ```
 * 
 * ## The Entity Class
 * As mentioned earlier, ecs::entity::Entity is an identifier to group components 
 * together into an 'Object'. In SECS, the ecs::entity::Entity class is itself a 
 * component, which is added by default to the World. 
 * 
 * The ecs::entity::Entity class is slightly more than just an identifier in SECS. I 
 * chose to have the ecs::entity::Entity class also keep track of the components which
 * are associated with it. This was a simple model to understand when implementing SECS,
 * but it is not the most performant solution. 
 * 
 * This implementation of the ecs::entity::Entity class makes executing systems much 
 * slower in the worse case as described below.
 * 
 * ## The System and Executable Classes
 * The ecs::system::System class is a templated class which can be inherited. All that is
 * required is that the function ecs::system::System::run() is defined. 
 * ecs::system::System::system_data is an alias type to std::tuple<Ts*...> where Ts... 
 * are the variadic template arguments of the ecs::system::System. The system_data 
 * provided to the run() function are a tuple of pointers for a given ecs::entity::Entity.
 * 
 * The ecs::dispatch::Executable class is an implementation detail which is used to hide
 * the template parameters of the ecs::system::System class from the ecs::world::World
 * class. This class provides one method, ecs::dispatch::Executable::exec(ecs::world::World*),
 * which has a default implementation in the System class. exec() is the function which
 * performes the 'run' function on all Entities with the correct components. This allows
 * for a neat pattern, where Template arguments can be 'hidden', because the  class/stuct
 * impelemting System doesn't need to be templated, as it only inherits a templated 
 * class, and ecs::dispatch::Executable isn't templated.
 * 
 * The ecs::system::System class's implementation of ecs::dispatch::Executable::exec()
 * is very simple, and can be seen below:
 * ```cpp
 * template <class ... Params>
 * void System<Params...>::exec(World *world_ptr) final
 * { 
 *     for (auto data : world_ptr->fetch<Params...>())
 *         this->run(data);
 * }
 * ```
 * Notably, this is where the performance cost of the implementation of the 
 * ecs::entity::Entity class is relevant. the ecs::world::World::fetch<Params...>() 
 * function has to loop through every Entity to find if it has the relevant components. 
 * Thus executing a System is O(n), where n is the number of entities in the world.
 * 
 * An alternate solution would be to have the RegistryNodes track the Entity when a 
 * component is added to the RegistryNode. Since every component has a separate 
 * RegistryNode, you could perform an intersection of the Entities found in each registry
 * nodes which would still be O(n), but the overall amount of work would be reduced, even
 * if not on an exponential scale.
 * 
 * ## Systems interacting with Entities
 * A particular challange of this project was allowing systems to operate on entites and
 * the world. Using the ecs::world::WorldResource, a programmer can now perform any of 
 * the following operations: build a new entity, add a component to an existing entity,
 * remove a component from an existing entity, and remove an entity from the world
 * altogether. 
 * 
 * The fact that systems may be executing in parallel makes adding/removing components 
 * particularly tricky. As such, all modifications to the world is done between dispatch
 * stages. As such the ecs::world::WorldResource keeps vectors of lambda functions which
 * perform the insertion/removal of the components into the world. These lambda functions
 * are run  after every system in a stage has finished executing.  
 * 
 * Adding components is fairly easy, and the order in which the additions are done does
 * not matter. Removing components is much more difficult, because ecs::entity::Entity 
 * tracks the vector index of their components. For this reason, it is important that 
 * components are removed from highest index to lowest index. Additionally, since the
 * ecs::entity::Entity class is a component itself, it was important that Entities are
 * removed last over other components, otherwise a number of problems can arise when 
 * removing components. 
 * 
 * Removing entities all together is particularly challenging, because an 
 * ecs::entity::Entity is not aware of the underlying types of it's components. As such,
 * entites are removed gradually over time. If a ecs::system::System *would* operate on 
 * an entity, and it has been flagged for removal, instead, the components are removed
 * at the end of the dispatch stage. Thus it is theoretically possible for an Entity to 
 * never fully be removed if it has components which are never operated on by Systems.
 * 
 * Another factor to consider is that because the underlying data structure of the 
 * RegistryNode is a std::vector, a resize could occur when adding components. This is
 * why adding components had to be done after a dispatch stage, else it was possible for
 * pointers to components, which are returned by World::fetch<Ts...>() (which is used in
 * System<Ts...>::exec()), to point to memory which is no longer valid.
 * 
 * ## Conclusion
 * Overall this has been a really interesting project. I've had the oppertunity to 
 * explore an interesting design pattern to better understand how it works. Additionally,
 * this has been a great chance to build upon my C++ skills. 
 * 
 * There are certainly many improvements and optimizations which could be made, but due
 * to the limited time of this project, I'm happy with where I've ended up. I've been
 * able to implement all of the core pieces originally specified in the proposal, as well
 * as all of my additional goals, including parallel system execution, and resources, 
 * which I originally thought was beyond the scope of the project, but was ultimately 
 * required for performing operations on entities and the world from systems.
 * 
 * For an even more detailed break down of all the different pieces which all had to come
 * together for this project to work, every class and function has been documented.
 */

#include <ecs/world/world_class.hpp>
#include <ecs/world/world_builder.hpp>
#include <ecs/world/entity_builder.hpp>

#endif