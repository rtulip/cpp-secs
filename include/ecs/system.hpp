#ifndef ecs_system_hpp
#define ecs_system_hpp

#include <tuple>
#include <vector>
#include <ecs/world.hpp>
#include <ecs/entity.hpp>

using ecs::dispatch::Executable;
using ecs::entity::bitset;
using ecs::world::World;

namespace ecs::system
{

    /**
     * @brief Abstract class to be implemented by stateful systems
     * 
     * Systems have to implement the run() function which operates on an individual 
     * entity which has all of the component parameters. Systems inherit from Executable
     * which provides and implement a generic exec() function which fetches all entities
     * with the component parameters and calls run on each. 
     * 
     * @tparam Params - The components required for this system.
     */
    template <class... Params>
    class System : public Executable
    {
    public:
        using system_data = std::tuple<Params *...>;
        virtual void run(system_data) = 0;
        void exec(World *world_ptr) final
        {
            for (auto data : world_ptr->fetch<Params...>())
                this->run(data);
        }
    };

} // namespace ecs::system

#endif