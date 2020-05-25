#ifndef ecs_registry_hpp
#define ecs_registry_hpp
#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <ecs/entity.hpp>

namespace ecs::registry
{

    class RegistryNode
    {
    private:
        std::shared_ptr<void> data;
        size_t data_hash_code;

    public:
        std::shared_ptr<RegistryNode> next;
        RegistryNode();
        ~RegistryNode();

        template <class T>
        static RegistryNode create();
        template <class T>
        bool check_type();
        template <class T>
        void push(T &&t);
        template <class T>
        const T &get(size_t i);
        template <class T>
        void set(size_t i, T &&t);
        template <class T>
        size_t size();
    };

    RegistryNode::RegistryNode()
    {
        this->data = nullptr;
        this->next = nullptr;
        this->data_hash_code = 0;
    }

    RegistryNode::~RegistryNode()
    {
        std::cout << "Deleting node with data: " << data << std::endl;
    }

    template <class T>
    RegistryNode RegistryNode::create()
    {
        RegistryNode node;

        auto v_ptr = std::make_shared<std::vector<T>>();
        node.data = v_ptr;
        node.data_hash_code = typeid(T).hash_code();
        return node;
    }

    template <class T>
    bool RegistryNode::check_type()
    {
        return typeid(T).hash_code() == this->data_hash_code;
    }

    template <class T>
    void RegistryNode::push(T &&t)
    {
        if (!this->check_type<T>())
            throw std::runtime_error("Type doesn't match node");
        std::static_pointer_cast<std::vector<T>>(this->data)->push_back(std::move(t));
    }

    template <class T>
    const T &RegistryNode::get(size_t i)
    {
        if (!this->check_type<T>())
            throw std::runtime_error("Type doesn't match node");
        return std::static_pointer_cast<std::vector<T>>(this->data)->at(i);
    }

    template <class T>
    void RegistryNode::set(size_t i, T &&t)
    {
        if (!this->check_type<T>())
            throw std::runtime_error("Type doesn't match node");
        std::static_pointer_cast<std::vector<T>>(this->data)->at(i) = t;
    }

    template <class T>
    size_t RegistryNode::size()
    {
        if (!this->check_type<T>())
            throw std::runtime_error("Type doesn't match node");
        return std::static_pointer_cast<std::vector<T>>(this->data)->size();
    }

} // namespace ecs::registry
#endif