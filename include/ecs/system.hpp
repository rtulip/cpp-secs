#ifndef ecs_system_hpp
#define ecs_system_hpp

#include <tuple>

namespace ecs::system
{
    template <class... Params>
    class System
    {
    public:
        using system_data = std::tuple<Params *...>;
        virtual void run(system_data) = 0;
    };
} // namespace ecs::system

#endif