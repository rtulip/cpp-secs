#ifndef ecs_entity_hpp
#define ecs_entity_hpp
#include <boost/dynamic_bitset.hpp>
#include <unordered_map>
#include <atomic>

namespace ecs::entity
{
    using bitset = boost::dynamic_bitset<>;
    enum EntityState
    {
        ACTIVE,
        TO_REMOVE,
        STAGED_FOR_REMOVAL
    };
    /**
     * @brief Entities are a sophisticated identifier.
     * 
     * In this implementation, Entities have a reference to which components they have
     * in a bitset. Additionally, the index of each component is kept in a unorderd map
     * for quick lookup.
     * 
     */
    class Entity
    {
    private:
        size_t id;
        bitset components;
        bitset valid;
        EntityState state;
        std::unordered_map<size_t, size_t> index_lookup;

    public:
        Entity(size_t eid, size_t n_components);
        ~Entity() = default;
        size_t eid() const;
        void add_component(size_t cid, size_t idx);
        void remove_component(size_t cid);
        void invalidate_component(size_t cid);
        bool has_component(size_t cid) const;
        bool has_valid_component(size_t cid) const;
        bool has_component(bitset mask) const;
        bool has_valid_component(bitset mask) const;
        size_t get_component(size_t cid) const;
        size_t decrement_component(size_t cid);
        bool is_alive() const;
        void flag_for_removal();
        bool is_flagged_for_removal() const;
        void set_staged_for_removal();
        bool is_staged_for_removal() const;
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
        valid = bitset(n_components);
        state = EntityState::ACTIVE;
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
        this->valid[cid] = 1;
    }

    /**
     * @brief Removes a component from an entity
     * 
     * @param cid - The component id.
     */
    void Entity::remove_component(size_t cid)
    {
        this->index_lookup.erase(cid);
        this->components[cid] = 0;
        this->valid[cid] = 0;
    }

    /**
     * @brief Mark a component as invalid.
     * 
     * This is used for the eventual removal of the Entity.
     * 
     * @param cid - The component id
     */
    void Entity::invalidate_component(size_t cid)
    {
        if (this->valid[cid])
        {
            this->index_lookup.erase(cid);
        }
        this->valid[cid] = 0;
    }

    /**
     * @brief Checks if this Entity has a component
     * 
     * Note: The component may not be valid. 
     * 
     * @param cid - The component id.
     * @return true - The Entity has the component.
     * @return false - The Entity does not have the component.
     * 
     */
    bool Entity::has_component(size_t cid) const
    {
        return this->components[cid];
    }

    /**
     * @brief Checks if the Entity has a valid component 
     * 
     * @param cid - The component id
     * @return true 
     * @return false 
     */
    bool Entity::has_valid_component(size_t cid) const
    {
        return this->valid[cid];
    }

    /**
     * @brief Checks if this Entity has ALL components in the mask.
     * 
     * Note: Some or all of the components may not be valid.
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
     * @brief Checks is an entity has a valid set of components
     * 
     * @param mask - The bitmask of components
     * @return true 
     * @return false 
     */
    bool Entity::has_valid_component(bitset mask) const
    {
        return (this->valid & mask) == mask;
    }

    /**
     * @brief Getter function for the component index.
     * 
     * @param cid - The component id in question.
     * @return size_t - The index of the component.
     */
    size_t Entity::get_component(size_t cid) const
    {
        return this->index_lookup.at(cid);
    }

    /**
     * @brief Function to decrease the index of the component by 1
     * 
     * This is used when a component is removed. If the index of that component is lower
     * than this index_lookup, then the index has to be decreased. So long as this is 
     * called once for every index removed which is lower, than the index lookup table
     * will still be correct.
     * 
     * @param cid - The component index
     * @return size_t - The new component index
     */
    size_t Entity::decrement_component(size_t cid)
    {
        return --this->index_lookup.at(cid);
    }

    /**
     * @brief Function to see if all the Entity's components have been removed.
     * 
     * @return true 
     * @return false 
     */
    bool Entity::is_alive() const { return !(this->state != EntityState::ACTIVE && this->valid.count() == 1); }

    /**
     * @brief Function to flag this Entity for removal
     * 
     */
    void Entity::flag_for_removal() { this->state = EntityState::TO_REMOVE; }

    /**
     * @brief Function to check if an entity has been flagged for removal.
     * 
     * @return true 
     * @return false 
     */
    bool Entity::is_flagged_for_removal() const { return this->state != EntityState::ACTIVE; }

    /**
     * @brief Set the Entity state to Staged for removal.
     * 
     */
    void Entity::set_staged_for_removal() { this->state = EntityState::STAGED_FOR_REMOVAL; }

    /**
     * @brief Check if the entity state is staged for removal.
     * 
     * @return true 
     * @return false 
     */
    bool Entity::is_staged_for_removal() const { return this->state == EntityState::STAGED_FOR_REMOVAL; }

} // namespace ecs::entity

#endif