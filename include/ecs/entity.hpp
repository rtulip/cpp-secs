#ifndef ecs_entity_hpp
#define ecs_entity_hpp
#include <boost/dynamic_bitset.hpp>

using bitset = boost::dynamic_bitset<>;
namespace ecs::entity
{

    class Entity
    {
    private:
        size_t id;
        bitset components;

    public:
        Entity(size_t eid, size_t n_components);
        ~Entity();
    };

    Entity::Entity(size_t eid, size_t n_components)
    {
        id = eid;
        components = bitset(n_components);
    }

    Entity::~Entity()
    {
    }

} // namespace ecs::entity

#endif