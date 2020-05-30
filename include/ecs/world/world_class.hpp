#ifndef ecs_world_class_hpp
#define ecs_world_class_hpp
#include <iostream>
#include <memory>
#include <vector>
#include <tuple>
#include <ecs/registry.hpp>
#include <ecs/entity.hpp>

using ecs::entity::bitset;
using ecs::entity::Entity;
using ecs::registry::RegistryNode;

namespace ecs::dispatch
{
    /**
     * @brief Executables are an abstarct class which defines an exec function
     * 
     * The idea behind this is that Systems require a parameter pack, thus Systems which
     * require different parameter types, cannot be stored in a container (easily). 
     * 
     * Regardless of the parameters the Systems use, the functionality to fetch and run
     * each system can be abstracted away into a single exec() function. That is what 
     * this abstract class provides. Now, a dispatcher can be a container of Executable*
     * and call the exec() function as defined by the System class, meaning the user does
     * not have to do any additional implementation beyond the System::run() function. 
     * 
     */
    class Executable;
} // namespace ecs::dispatch

namespace ecs::world
{
    /**
     * World is a container class for different components.
     * 
     * The idea is that at runtime, a World is constructed with all the components which
     * are desired. After building the World, components will not be registered or removed.
     * 
     */
    class World
    {
    private:
        size_t next_eid;
        std::vector<RegistryNode> nodes;
        std::unordered_map<size_t, size_t> node_index_lookup;
        std::vector<ecs::dispatch::Executable *> systems;

        template <class T>
        void register_component();
        template <class T>
        bool has_component() const;
        size_t get_eid();

        template <class T>
        T *get(Entity *e);
        size_t count_components() const;

        World(/* args */) = default; //! World constructor is private. Use World::create().

    public:
        ~World() = default;
        World(const World &world) = delete;
        World(World &&world) = default;

        void add_entity(Entity &&entity);
        void add_system(ecs::dispatch::Executable *exe_ptr);
        void dispatch();

        template <class T>
        const RegistryNode *find() const;
        template <class T>
        RegistryNode *find();
        template <class T>
        size_t get_cid() const;

        template <class... Ts>
        bitset mask() const;
        template <class... Ts>
        std::vector<std::tuple<Ts *...>> fetch();

        class WorldBuilder;
        static WorldBuilder create();

        class EntityBuilder;
        EntityBuilder build_entity();
    };

    /**
     * @brief Returns the eid for a new entity. 
     * 
     * It is assumed that each Entity has a unique id, and that this function is used to 
     * generate the eid for the Entity.
     * 
     * TODO: Need to have a better way to distribute and track active entitiy id's, but
     * for the moment this will do. 
     * 
     * @return size_t 
     */
    size_t World::get_eid()
    {
        return this->next_eid++;
    }

    /**
     * @brief Returns the component id (cid) of a component.
     * 
     * @tparam T - The component thats being looked for.
     * @return size_t - The component id
     * 
     * @exception Throws a runtime exception if the component isn't registered to the world.
     */
    template <class T>
    size_t World::get_cid() const
    {
        if (!this->has_component<T>())
            throw std::runtime_error("Component is not registered");
        return this->node_index_lookup.at(typeid(T).hash_code());
    }

    /**
     * @brief Getter function for the number of registered components.
     * 
     * @return size_t - The number of components registered to the world.
     */
    size_t World::count_components() const
    {
        return this->nodes.size();
    }

    /**
     * @brief Getter function to an Entity's instance of a component.
     * 
     * @tparam T - The component type to get.
     * @param e - A pointer to the Entity in question.
     * @return T* - A pointer to the Entity's component.
     * 
     * TODO: Bounds need to be checked. 
     */
    template <class T>
    T *World::get(Entity *e)
    {
        size_t cid = this->get_cid<T>();
        size_t idx = e->get_component(cid);
        RegistryNode *node = this->find<T>();

        return node->get<T>(idx);
    }

    /**
     * @brief Creates a bitmask for a set of components
     * 
     * @tparam Ts - The set of components
     * @return bitset - The mask
     */
    template <class... Ts>
    bitset World::mask() const
    {
        bitset bits = bitset(this->count_components());
        (bits.set(this->get_cid<Ts>()), ...);
        return bits;
    }

    /**
     * @brief Builds a vector of tuples of pointers to components for systems to iterate over.
     * 
     * Systems work on a set of components< A, B, ...>, and are intended to run over a tuple
     * of component pointers <A*, B*, ...>, where each element in the tuple belongs to a
     * single Entity. This function aims to construct the tuples of system_data and group
     * them into a vector which can then be iterated over.
     * 
     * @tparam Ts - The set of components to be fetched.
     * @return std::vector<std::tuple<Ts *...>> 
     */
    template <class... Ts>
    std::vector<std::tuple<Ts *...>> World::fetch()
    {
        std::vector<std::tuple<Ts *...>> vec;
        bitset m = this->mask<Ts...>();
        auto node = this->find<Entity>();
        for (auto e : *(node->iter<Entity>()))
        {
            if (e.has_component(m))
            {
                auto tuple = std::make_tuple(this->get<Ts>(&e)...);
                vec.push_back(tuple);
            }
        }
        return std::move(vec);
    }

    /**
     * @brief Adds a built Entity to the World.
     * 
     * This consumes the Entity
     * 
     * @param entity - The built Entity.
     */
    void World::add_entity(Entity &&entity)
    {
        auto node = this->find<Entity>();
        node->push<Entity>(std::move(entity));
    }

    /**
     * @brief Adds a system to the dispathcer
     * 
     * TODO: Fully impelment a dependency tree for the dispatcher to understand which
     * systems can be executed in parallel.
     * 
     * @param exe_ptr 
     */
    void World::add_system(ecs::dispatch::Executable *exe_ptr)
    {
        this->systems.push_back(exe_ptr);
    }

    /**
     * @brief Checks if a component is registered.
     * 
     * @tparam T - The component in question.
     * @return true - The component is registered.
     * @return false - The component is not registered.
     */
    template <class T>
    bool World::has_component() const
    {
        return this->node_index_lookup.find(typeid(T).hash_code()) != this->node_index_lookup.end();
    }

    /**
     * @brief Registers a component to the World.
     * 
     * @tparam T - The type to be registered.
     */
    template <class T>
    void World::register_component()
    {
        if (this->has_component<T>())
            throw std::runtime_error("Component is already registered");
        this->node_index_lookup.emplace(typeid(T).hash_code(), this->nodes.size());
        this->nodes.push_back(RegistryNode::create<T>());
    }

    /**
     * @brief Getter function (immutable) for a Registry Node of a particular Type 
     * 
     * @tparam T - The type being looked for
     * @return const RegistryNode* 
     */
    template <class T>
    const RegistryNode *World::find() const
    {
        if (!this->has_component<T>())
            throw std::runtime_error("Component is not registered");
        size_t idx = this->node_index_lookup.at(typeid(T).hash_code());
        return &this->nodes.at(idx);
    }

    /**
     * @brief Getter function (mutable) for a Registry Node of a particular Type 
     * 
     * @tparam T - The type being looked for
     * @return RegistryNode* 
     */
    template <class T>
    RegistryNode *World::find()
    {
        if (!this->has_component<T>())
            throw std::runtime_error("Component is not registered");
        size_t idx = this->node_index_lookup.at(typeid(T).hash_code());
        return &this->nodes.at(idx);
    }

} // namespace ecs::world

namespace ecs::dispatch
{
    class Executable
    {
    public:
        virtual void exec(ecs::world::World *) = 0;
    };
} // namespace ecs::dispatch

namespace ecs::world
{
    /**
     * @brief Runs each system which has been added to the world in order.
     * 
     */
    void World::dispatch()
    {
        for (auto sys : this->systems)
            sys->exec(this);
    }
} // namespace ecs::world
#endif