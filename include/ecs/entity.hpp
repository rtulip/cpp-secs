#ifndef ecs_entity_hpp
#define ecs_entity_hpp
#include <boost/dynamic_bitset.hpp>
#include <unordered_map>

namespace ecs::entity
{
    using bitset = boost::dynamic_bitset<>;
    /**
     * @brief Entities are a sophisticated identifier.
     * 
     * In this implementation, Entities have a reference to which components they have
     * in a bitset. Additionally, the index of each component is kept in a unorderd map
     * for quick lookup. 
     * 
     * This is not an ideal implementation of an Entity in an ECS. Ideally, Entities are
     * only an identifier. 
     * 
     * TODO: There should be a way to track generations and merge changes after systems
     * are run.
     * TODO: Look into exactly how to separate the index lookup from Entities. 
     * 
     */
    class Entity
    {
    private:
        size_t id;
        bitset components;
        std::unordered_map<size_t, size_t> index_lookup;

    public:
        Entity(size_t eid, size_t n_components);
        ~Entity() = default;
        size_t eid() const;
        void add_component(size_t cid, size_t idx);
        bool has_component(size_t cid) const;
        bool has_component(bitset mask) const;
        size_t get_component(size_t cid) const;
    };

    /**
     * @brief Construct a new Entity object
     * 
     * @param eid - The Entity id of this Entity.
     * @param n_components - The number of components registered in the world.
     */
    Entity::Entity(size_t eid, size_t n_components)
    {
        id = eid;
        components = bitset(n_components);
    }

    /**
     * @brief Getter function for the Entity eid.
     * 
     * @return size_t - the Eid.
     */
    size_t Entity::eid() const
    {
        return this->id;
    }

    /**
     * @brief Adds a component to the Entity. 
     * 
     * @param cid - The component id.
     * @param idx - The index of the component.
     */
    void Entity::add_component(size_t cid, size_t idx)
    {
        this->index_lookup.emplace(cid, idx);
        this->components[cid] = 1;
    }

    /**
     * @brief Checks if this Entity has a component
     * 
     * @param cid - The component id.
     * @return true - The Entity has the component.
     * @return false - The Entity does not have the component.
     * 
     * TODO: Check bounds.
     */
    bool Entity::has_component(size_t cid) const
    {
        return this->components[cid];
    }

    /**
     * @brief Checks if this Entity has ALL components in the mask.
     * 
     * @param mask - The bitmask of components
     * @return true - The Entity has all components.
     * @return false - The Entity does not have all components.
     */
    bool Entity::has_component(bitset mask) const
    {
        return (this->components & mask) == mask;
    }

    /**
     * @brief Getter function for the component index.
     * 
     * @param cid - The component id in question.
     * @return size_t - THe index of the component.
     */
    size_t Entity::get_component(size_t cid) const
    {
        return this->index_lookup.at(cid);
    }

} // namespace ecs::entity

#endif