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
#include <functional>
#include <algorithm>
#include <mutex>

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
    class World;

    /**
     * @brief A wrapper around a World* for use as a resource.
     * 
     * In order for systems to interact with the underlying World, this Resource can be
     * used. This resource provides methods for adding & removing entities and components
     * from the World. 
     * 
     * Operations done through the WorldResource can bring some safety risks, thus it is
     * important to ensure that this resource is only used when needed and that the 
     * guidelines are followed for its use. 
     * 
     */
    class WorldResource
    {
    private:
        World *world_ptr;
        std::mutex mutex_guard;
        std::vector<std::pair<size_t, std::function<void()>>> remove_functions;
        std::vector<std::pair<size_t, size_t>> remove_indicies;

    public:
        WorldResource(World *world_pointer);
        WorldResource(WorldResource &&other);
        WorldResource &operator=(WorldResource &&other);
        ~WorldResource() = default;
        template <class T>
        void remove_entity_component(Entity *);
        template <class... Ts>
        void invalidate_entity_components(Entity *e);
        template <class T>
        void invalidate_entity_component(Entity *e);
        void remove_entity(Entity *);
        void stage_entity_for_removal(Entity *e);
        void merge();
        World *world() { return this->world_ptr; }
    };

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
        void add_entity(Entity &&entity);

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
            WorldResource res(this);
            this->add_resource<WorldResource>(std::move(res));
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

            auto world_res_node = this->find<WorldResource>();
            WorldResource res(this);
            world_res_node->set<WorldResource>(0, std::move(res));
        }

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

        friend class WorldResource;
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
     * If an entity which has components <A, B, ...> has been flagged for removal, then
     * the components <A, B, ...> are staged for inavlidation. Once all of an Entity's
     * components have been removed by fetch<Ts...> calls, then it can be staged for
     * removal entirely.
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
        for (auto &e : *(node->iter<Entity>()))
        {
            if (e.has_component(m))
            {
                if (e.is_flagged_for_removal())
                {
                    auto world_res = this->find<WorldResource>()->get<WorldResource>(0);
                    if (e.is_alive())
                    {
                        world_res->invalidate_entity_components<Ts...>(&e);
                    }
                    else
                    {
                        world_res->stage_entity_for_removal(&e);
                    }
                }
                else
                {
                    auto tuple = std::make_tuple(this->get<Ts>(&e)...);
                    vec.push_back(tuple);
                }
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

    /**
     * @brief Construct a new World Resource:: World Resource object
     * 
     * @param world_pointer - A pointer to the World.
     */
    WorldResource::WorldResource(World *world_pointer)
    {
        this->world_ptr = world_pointer;
    }

    WorldResource::WorldResource(WorldResource &&other)
    {
        this->world_ptr = other.world_ptr;
    }

    WorldResource &WorldResource::operator=(WorldResource &&other)
    {
        this->world_ptr = other.world_ptr;
        return *this;
    }

    /**
     * @brief Stage an Entity's component for removal.
     *  
     * Removing a component from an Entity is hard because its' component can be in the
     * middle of the vector of components, thus removing the component will affect other
     * entities accessing their own components. Additionally, because systems are run,
     * the pointer to every component used by a system must remain valid while the system
     * is being run.
     * 
     * For these reasons, calling this function does NOT remove the component instantly,
     * rather, the removal is postponed until after all concurrent system's have joined.
     * 
     * @tparam T - The type of the component to be removed. 
     * @param e - A pointer to the Entity to operate on.
     */
    template <class T>
    void WorldResource::remove_entity_component(Entity *e)
    {
        // Get the component id of T.
        size_t cid = this->world_ptr->get_cid<T>();

        size_t ENTITY_CID = this->world_ptr->get_cid<Entity>();
        if (cid == ENTITY_CID)
            throw std::runtime_error("Cannot remove the Entity component from an Entity!");

        // If the entity doesn't have a T component, throw a runtime error. This is to
        // help ensure that a system is only operating on the entity it's working with.
        if (!e->has_component(cid))
            throw std::runtime_error("Cannot remove a component from an entity if it doesn't have it.");

        // Get the index of the component to be removed.
        size_t entity_component_index = e->get_component(cid);

        // Fetch the RegistryNode for the component
        RegistryNode *node_ptr = this->world_ptr->find<T>();

        // Create a pair of the component index and the component id.
        this->remove_indicies.push_back(std::make_pair(entity_component_index, cid));

        // Create a lambda function to delete the component instance, and remove the
        // entity's knowledge of the component.
        auto f = [node_ptr, entity_component_index, e, cid]() {
            e->remove_component(cid);                   // Forget the component
            node_ptr->erase<T>(entity_component_index); // Delete the component
        };

        // Add the lambda function to be called later paired with the component index.
        this->remove_functions.push_back(std::make_pair(entity_component_index, std::move(f)));
    }

    /**
     * @brief Mark an Entity's component as invalid.
     * 
     * This is a helper function used for removing Entities gradually. Unlike with a 
     * component removal, the Entity can still be fetched for component T, but it will 
     * not be operated on. This will allow for Components to be destructed over system
     * execution, until all of an Entity's components have been removed.
     * 
     * @tparam T 
     * @param e 
     */
    template <class T>
    void WorldResource::invalidate_entity_component(Entity *e)
    {

        // Get the component id of T.
        size_t cid = this->world_ptr->get_cid<T>();

        size_t ENTITY_CID = this->world_ptr->get_cid<Entity>();
        // If trying to remove the Entity Component don't do anything.
        if (cid == ENTITY_CID)
        {
            return;
        }

        // If the Entity no longer has a valid T component, return and don't do anything.
        // Otherwise, set the component to invalid.
        if (!e->has_valid_component(cid))
            return;

        // Get the index of the component to be removed.
        size_t entity_component_index = e->get_component(cid);

        // Fetch the RegistryNode for the component
        RegistryNode *node_ptr = this->world_ptr->find<T>();

        // Create a pair of the component index and the component id.
        this->remove_indicies.push_back(std::make_pair(entity_component_index, cid));

        // Create a lambda function to delete the component instance, and remove the
        // entity's knowledge of the component.
        auto f = [node_ptr, entity_component_index, e, cid]() {
            e->invalidate_component(cid);               // Invalidate the component
            node_ptr->erase<T>(entity_component_index); // Delete the component
        };

        // Add the lambda function to be called later paired with the component index.
        this->remove_functions.push_back(std::make_pair(entity_component_index, std::move(f)));
    }

    /**
     * @brief Invalidates a set of components for an Entity.
     * 
     * This is used during a fetch<Ts... > call and allows for Systems to destruct 
     * components of Entitys which have been marked for removal.
     * 
     * @tparam Ts 
     * @param e 
     */
    template <class... Ts>
    void WorldResource::invalidate_entity_components(Entity *e)
    {
        (this->invalidate_entity_component<Ts>(e), ...);
    }

    /**
     * @brief Flags an entity for removal.
     * 
     * Because an Entity is not aware of the types of it's components, an Entity cannot
     * be destroyed instantly, but rather it must be destroyed over time by different
     * systems execution. 
     * 
     * As systems are exeucted, a fetch<Ts ...> call is made, and if the Entity would 
     * meet the requirements to be executed, the components will be invalidated and 
     * destroyed instead of being executed. Once the Entity is only comprised of the 
     * Entity component and nothing else, it can then be staged for removal. 
     * 
     * Note: 
     *      It is possible that an Entity may never be destroyed until the end of 
     *      program execution. Should the programmer choose to make systems which remove 
     *      the components of an Entity in such a way that there are components which are
     *      never used in systems, then the Entity will be forever flagged for removal, 
     *      but never staged. 
     *      
     *      This is an unlikely situation, and could be resolved with additional work, 
     *      but is beyond the scope of this project. The overall cost of this situation
     *      is a little bit of overhead, but doesn't cause any memory leaks or undefined
     *      behaviour.
     * 
     * @param e 
     */
    void WorldResource::remove_entity(Entity *e)
    {
        e->flag_for_removal();
    }

    /**
     * @brief Stage an entity for removal
     * 
     * This assumes that all but the Entity component itself has been removed. 
     * 
     * @param e 
     */
    void WorldResource::stage_entity_for_removal(Entity *e)
    {

        // --- CRITICAL SECTION ---
        this->mutex_guard.lock();
        if (e->is_staged_for_removal())
        {
            this->mutex_guard.unlock();
            return;
        }
        e->set_staged_for_removal();
        this->mutex_guard.unlock();
        // --- END OF CRITICAL SECTION ---

        // Get the component id of T.
        size_t cid = this->world_ptr->get_cid<Entity>();

        // Get the index of the component to be removed.
        size_t entity_component_index = e->get_component(cid);

        // Fetch the RegistryNode for the component
        RegistryNode *node_ptr = this->world_ptr->find<Entity>();

        // Create a pair of the component index and the component id.
        this->remove_indicies.push_back(std::make_pair(entity_component_index, cid));

        // Create a lambda function to delete the component instance, and remove the
        // entity's knowledge of the component.
        auto f = [node_ptr, entity_component_index, e, cid]() {
            node_ptr->erase<Entity>(entity_component_index); // Delete the Entity Component
        };

        // Add the lambda function to be called later paired with the component index.
        this->remove_functions.push_back(std::make_pair(entity_component_index, std::move(f)));
    }

    /**
     * @brief Performes all the removals from systems after their execution.
     */
    void WorldResource::merge()
    {
        // If nothing to remove, return early for faster execution.
        if (this->remove_functions.size() == 0)
            return;

        // Create lambda functions for sorting the lists in index decending order.
        auto function_sorting = [](std::pair<size_t, std::function<void()>> lhs, std::pair<size_t, std::function<void()>> rhs) {
            return std::get<0>(lhs) > std::get<0>(rhs);
        };
        auto index_sorting = [](std::pair<size_t, size_t> lhs, std::pair<size_t, size_t> rhs) {
            return std::get<0>(lhs) > std::get<0>(rhs);
        };

        // Sort the lists in index decending order.
        // Since these components are removed by index, it's important that components
        // with the largest index are removed first. This is why these sorts are needed.
        std::sort(this->remove_functions.begin(), this->remove_functions.end(), function_sorting);
        std::sort(this->remove_indicies.begin(), this->remove_indicies.end(), index_sorting);

        // Actually remove all the components.
        for (auto function_pair : this->remove_functions)
        {
            size_t idx = std::get<0>(function_pair);
            auto f = std::get<1>(function_pair);
            f();
        }

        // Adjust indicies of every Entity effected.
        RegistryNode *entity_node = this->world_ptr->find<Entity>();

        for (auto &e : *(entity_node->iter<Entity>()))
        {
            for (auto index_pair : this->remove_indicies)
            {
                size_t idx = std::get<0>(index_pair);
                size_t cid = std::get<1>(index_pair);
                // The component MUST be valid, otherwise it may have been removed already.
                if (e.has_valid_component(cid))
                {
                    size_t e_idx = e.get_component(cid);
                    if (e_idx > idx)
                    {
                        e.decrement_component(cid);
                    }
                }
            }
        }

        // Empty the lists for the next systems.
        this->remove_functions.clear();
        this->remove_indicies.clear();
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
        RegistryNode *world_res_node = this->find<WorldResource>();
        WorldResource *world_res = world_res_node->get<WorldResource>(0);
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

            // Perform any removals required now that all threads have joined.
            world_res->merge();
        }
    }
} // namespace ecs::world
#endif