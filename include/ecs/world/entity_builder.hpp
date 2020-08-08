#ifndef ecs_entity_builder_hpp
#define ecs_entity_builder_hpp
#include <ecs/world.hpp>

namespace ecs::world
{
    /**
     * @brief EntityBuilder is a builder class which constructs an entity with components
     * 
     * In order to more easily allow for entities to have a set of components this 
     * builder class is used. Components can easily be added by using .with<T>(T &&t),
     * which comsumes t. Upon calling .build() the Entity is added to the World which 
     * it's a part of. 
     * 
     * TODO: Delay adding the components until .build() is called.
     * 
     */
    class World::EntityBuilder
    {
    private:
        Entity entity;
        World *world_ptr;

    public:
        EntityBuilder(World *ptr);
        ~EntityBuilder(){};

        template <class T>
        EntityBuilder &with(T &&t);
        void build();
    };

    /**
     * @brief Construct a new EntityBuilder object
     * 
     * @param ptr - A pointer to the World where the entity and its components will be added.
     */
    World::EntityBuilder::EntityBuilder(World *ptr) : entity(ptr->get_eid(), ptr->count_components())
    {
        this->world_ptr = ptr;
    }

    /**
     * @brief Add a new component to the Entity being built.
     * 
     * @tparam T - The type of the component.
     * @param t - The component instance.
     * @return World::EntityBuilder& - This builder is returned.
     */
    template <class T>
    World::EntityBuilder &World::EntityBuilder::with(T &&t)
    {
        RegistryNode *node = world_ptr->find<T>();
        entity.add_component(world_ptr->get_cid<T>(), node->size<T>());
        node->push<T>(std::move(t));
        return *this;
    }

    /**
     * @brief Finishes building the entity and destroys the builder.
     * 
     */
    void World::EntityBuilder::build()
    {
        RegistryNode *node = world_ptr->find<Entity>();
        entity.add_component(world_ptr->get_cid<Entity>(), node->size<Entity>());
        world_ptr->add_entity(std::move(entity));
    }

    /**
     * @brief Makes an EntityBuilder for this world.
     * 
     * @return World::EntityBuilder 
     */
    World::EntityBuilder World::build_entity()
    {
        World::EntityBuilder entity_builder(this);
        return entity_builder;
    }

} // namespace ecs::world

#endif