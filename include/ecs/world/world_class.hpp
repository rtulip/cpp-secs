#ifndef ecs_world_class_hpp
#define ecs_world_class_hpp
#include <iostream>
#include <memory>
#include <vector>
#include <tuple>
#include <ecs/registry.hpp>
#include <ecs/entity.hpp>
#include <string>
#include <iostream>
#include <thread>

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

    // A container for Executables which can be run in parallel
    typedef std::vector<Executable *> DispatcherStage;
    // A container of 'Stages' to be run in order.
    typedef std::vector<DispatcherStage> DispatcherContainer;

    /**
     * @brief Class to build a DispatcherContainer based on user described dependencies
     * 
     * Dependencies MUST be programmer defined. This is required to allow the programmer
     * to specify the order in which systems are run, even if the signature is mutually
     * exclusive. 
     * 
     * It is the PROGRAMMER'S responsibility to ensure that they do not introduce data
     * races. Systems with one or more shared component signatures MUST have a dependency
     * relationship, either directly or indirectly.
     * 
     * The resuting DispatcherContainer is built using a BFS Topological sort.
     * 
     * Example: 
     *  Consider the following Systems:
     *      System A<Com_1, Com_2>
     *      System B<Com_2>
     *      System C<Com_1>
     * 
     *  Systems B and C both share a component signature with System A, thus there must 
     *  be a defined order. 
     * 
     *  Dependencies can be defined directly:
     *      [ B, C ] -> [ A ] (A depends on both B, C. B & C can be run in parallel safely.)
     *  or indirectly:
     *      [ A ] -> [ B ] -> [ C ] (C depends on B, which depends on A) 
     * 
     */
    class DispatcherContainerBuilder
    {
    private:
        std::unordered_map<std::string, uint> counts;
        std::unordered_map<std::string, std::vector<std::string>> edges;
        std::unordered_map<std::string, Executable *> systems;
        DispatcherContainer *container_ref;

    public:
        DispatcherContainerBuilder(DispatcherContainer *ref) { this->container_ref = ref; };
        ~DispatcherContainerBuilder() = default;

        DispatcherContainerBuilder &add_system(Executable *exe_ptr, const std::string exe_name, std::initializer_list<std::string> deps);
        void done();
    };

    /**
     * @brief Adds a System to be scheuled by the dispatcher.
     * 
     * It is currently required that Systems are added in an order such that the System's
     * dependencies have already been defined. If a dependency is not found, a runtime
     * error is thrown. This is sufficient to ensure that the dependency graph is acyclic.
     * 
     * @param exe_ptr   The System pointer
     * @param exe_name  A unique identifier for the system. Used to specify dependencies
     * @param deps      A list of dependencies.     
     * @return DispatcherContainerBuilder& 
     */
    DispatcherContainerBuilder &DispatcherContainerBuilder::add_system(
        Executable *exe_ptr,
        const std::string exe_name,
        std::initializer_list<std::string> deps)
    {
        this->systems[exe_name] = exe_ptr;
        this->edges[exe_name] = std::vector<std::string>();
        this->counts[exe_name] = 0;
        for (auto dep : deps)
        {
            this->edges[exe_name].push_back(dep);
            if (this->counts.find(dep) == this->counts.end())
                throw std::runtime_error("Dependency not found");
            this->counts[dep]++;
        }
        return *this;
    }

    /**
     * @brief Finish specifying systems to add to the Dispatcher.
     * 
     * The DispatcherContainer is built using a BFS Topological sort. The algorithm works
     * as follows:
     *  
     *  1) Find all Verticies with an 'in degree' of 0
     *  2) Add each of the found Verticies to the next 'Stage'.
     *  3) Remove each found Vertex from the graph.
     *  4) Repeat steps 1-3 until the graph is empty. 
     * 
     * An additional check to ensure that the graph is acyclic is done. If there are no
     * found Verticies with an 'in degree' of 0 AND the graph isn't empty, then there is
     * a cycle in the graph. In this situation a runtime error is thrown. 
     * 
     * Since the dependencies are defined 'bottom up', this topological sort will find
     * the systems to execute last first. Since building this DispatcherContainer is to
     * be done once per world, the execution time isn't super critical. As such I'm using
     * the costly 'insert' to put each stage before the last, such that the 
     * DispatcherContainer order makes sense. If this is deemed unnessisary, container 
     * can be iterated in reverse.
     * 
     */
    void DispatcherContainerBuilder::done()
    {
        while (!this->systems.empty())
        {
            // Add empty stage
            ecs::dispatch::DispatcherStage stage;

            // Find all keys with out degree 0
            std::vector<std::string> in_degree_zero_keys;
            for (auto key : this->counts)
            {
                if (key.second == 0)
                    in_degree_zero_keys.push_back(key.first);
            }

            // Remove the keys from counts.
            for (auto key : in_degree_zero_keys)
                this->counts.erase(key);

            // Check for cycles - Kept as a failsafe.
            if (in_degree_zero_keys.size() == 0 && !this->systems.empty())
                throw std::runtime_error("Detected cycle in dependency graph!");

            // Remove the systems with in degree 0
            for (auto key : in_degree_zero_keys)
            {
                // Decrease the in degree of each element pointed by key
                for (auto edge : this->edges[key])
                {
                    this->counts[edge]--;
                }

                // Remove the key
                this->edges.erase(key);
                // Remove the system from the graph, and add it to the stage.
                stage.push_back(this->systems[key]);
                this->systems.erase(key);
            }

            // Insert the stage into the dependency graph.
            container_ref->insert(container_ref->begin(), stage);
        }
    }
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
        ecs::dispatch::DispatcherContainer systems;
        ecs::entity::bitset component_mask;

        template <class T>
        void register_component();
        template <class T>
        void add_resource(T &&t);
        template <class T>
        void add_resource(T &t);
        template <class T>
        bool has_component() const;
        size_t get_eid();

        template <class T>
        T *get(Entity *e);
        size_t count_components() const;

        World(/* args */) //! World constructor is private. Use World::create().
        {
            this->next_eid = 0;
            this->nodes = std::vector<RegistryNode>();
            this->node_index_lookup = std::unordered_map<size_t, size_t>();
            this->systems = ecs::dispatch::DispatcherContainer();
            this->component_mask = ecs::entity::bitset(0);
            this->register_component<Entity>();
            this->add_resource<World *>(this);
        }

    public:
        ~World() = default;
        World(const World &world) = delete;
        World(World &&world)
        {
            this->next_eid = world.next_eid;
            this->nodes = std::move(world.nodes);
            this->node_index_lookup = std::move(world.node_index_lookup);
            this->systems = std::move(world.systems);
            this->component_mask = std::move(world.component_mask);

            auto world_res_node = this->find<World *>();
            auto this_ptr_copy = this;
            world_res_node->set<World *>(0, std::move(this_ptr_copy));
        }

        void add_entity(Entity &&entity);
        ecs::dispatch::DispatcherContainerBuilder add_systems();
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
        friend class WorldBuilder;
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
        RegistryNode *node = this->find<T>();
        if (node->is_resource())
            return node->get<T>(0);

        size_t cid = this->get_cid<T>();
        size_t idx = e->get_component(cid);

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
        return bits & this->component_mask;
    }

    /**
     * @brief Builds a vector of tuples of pointers to components for systems to iterate over.
     * 
     * Systems work on a set of components <A, B, ...>, and are intended to run over a
     * tuple of component pointers <A*, B*, ...>, where each element in the tuple belongs
     * to asingle Entity. This function aims to construct the tuples of system_data and
     * group them into a vector which can then be iterated over.
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
    ecs::dispatch::DispatcherContainerBuilder World::add_systems()
    {
        ecs::dispatch::DispatcherContainerBuilder builder(&this->systems);
        return builder;
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
     * @brief Adds a resource to the World.
     * 
     * @tparam T - The type to be added.
     */
    template <class T>
    void World::add_resource(T &&t)
    {
        if (this->has_component<T>())
            throw std::runtime_error("Already have a resource of this type!");
        this->node_index_lookup.emplace(typeid(T).hash_code(), this->nodes.size());
        this->nodes.push_back(RegistryNode::create_resource<T>(std::move(t)));
    }

    /**
     * @brief Adds a resource to the World.
     * 
     * @tparam T - The type to be added.
     */
    template <class T>
    void World::add_resource(T &t)
    {
        if (this->has_component<T>())
            throw std::runtime_error("Already have a resource of this type!");
        this->node_index_lookup.emplace(typeid(T).hash_code(), this->nodes.size());
        this->nodes.push_back(RegistryNode::create_resource<T>(t));
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
        for (auto stage : this->systems)
        {
            /**
             * @brief Start a thread for every `stage` in the dispatcher
             * 
             * Safety:
             *  - The DispatcherBuilder will ensure that systems will run after their
             *    dependencies have finished executing. 
             *  - It is the PROGRAMMER'S responsibility to ensure that the dependencies
             *    of systems are properly specified.
             * 
             */
            std::vector<std::thread> threads;
            for (auto sys : stage)
            {
                auto lambda_f = [sys](ecs::world::World *w) {
                    sys->exec(w);
                };
                std::thread t(lambda_f, this);
                threads.push_back(std::move(t));
            }

            // Wait for each thread to join before moving to the next stage.
            for (int i = 0; i < threads.size(); i++)
            {
                threads.at(i).join();
            }
        }
    }
} // namespace ecs::world
#endif