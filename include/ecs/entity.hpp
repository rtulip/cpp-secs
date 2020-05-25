#ifndef ecs_entity_hpp
#define ecs_entity_hpp
#include <boost/dynamic_bitset.hpp>
#include <unordered_map>

using bitset = boost::dynamic_bitset<>;
namespace ecs::entity
{

    class Entity
    {
    private:
        size_t id;
        bitset components;
        std::unordered_map<size_t, size_t> index_lookup;

    public:
        Entity(size_t eid, size_t n_components);
        ~Entity();
        size_t eid();
        void add_component(size_t cid);
        bool has_component(size_t cid);
    };

    Entity::Entity(size_t eid, size_t n_components)
    {
        id = eid;
        components = bitset(n_components);
    }

    Entity::~Entity()
    {
    }

    size_t Entity::eid()
    {
        return this->id;
    }

    void Entity::add_component(size_t cid)
    {
        this->components[cid] = 1;
    }

    bool Entity::has_component(size_t cid)
    {
        return this->components[cid];
    }

} // namespace ecs::entity

#endif