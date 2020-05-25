#ifndef ecs_registry_hpp
#define ecs_registry_hpp
#include <iostream>
#include <memory>
#include <vector>
#include <ecs/entity.hpp>

namespace ecs::registry {

class RegistryNode
{
private:
    std::shared_ptr<void> data;
    size_t data_hash_code;
public:
    std::shared_ptr<RegistryNode> next;
    RegistryNode();
    ~RegistryNode();

    template<class T>
    static std::shared_ptr<RegistryNode> create();
    template<class T>
    bool check_type();
    template<class T>
    std::shared_ptr<std::vector<T>> get_array();

    template<class T>
    void push(T&& t);
    template<class T>
    const T& get(size_t i);
    template<class T>
    void set(size_t i, T&& t);

};

RegistryNode::RegistryNode()
{
    this->data = nullptr;
    this->next = nullptr;
    this->data_hash_code = 0;
}

RegistryNode::~RegistryNode()
{
    std::cout << "Deleting node with data: " << data <<std::endl;
}

template<class T>
std::shared_ptr<RegistryNode> RegistryNode::create(){
    auto n_ptr = std::make_shared<RegistryNode>();
    auto v_ptr = std::make_shared<std::vector<T>>();
    n_ptr->data = v_ptr;
    n_ptr->data_hash_code = typeid(T).hash_code();
    return n_ptr;
}

template<class T>
bool RegistryNode::check_type(){
    return typeid(T).hash_code() == this->data_hash_code;
}

template<class T>
std::shared_ptr<std::vector<T>> RegistryNode::get_array(){
    if (!this->check_type<T>()) throw std::runtime_error("Type doesn't match node");
    return std::static_pointer_cast<std::vector<T>>(this->data);
}

template<class T>
void RegistryNode::push(T&& t){
    if (!this->check_type<T>()) throw std::runtime_error("Type doesn't match node");
    std::static_pointer_cast<std::vector<T>>(this->data)->push_back(std::move(t));
}

template<class T>
const T& RegistryNode::get(size_t i){
    if (!this->check_type<T>()) throw std::runtime_error("Type doesn't match node");
    return std::static_pointer_cast<std::vector<T>>(this->data)->at(i);
}

template<class T>
void RegistryNode::set(size_t i, T&& t){
    if (!this->check_type<T>()) throw std::runtime_error("Type doesn't match node");
    std::static_pointer_cast<std::vector<T>>(this->data)->at(i) = t;
}

class Registry
{
private:
    typedef std::shared_ptr<RegistryNode> IterType;

    IterType tail;
public:
    IterType head;
    Registry();
    ~Registry();
    Registry(const Registry& r);
    Registry(Registry&& r);

    Registry& operator=(Registry&) = default;
    // Registry operator=(Registry) = default;

    template<class T>
    void register_component();
    void register_entities();
    
    class Iterator
    {
    public:
        IterType current;
        Iterator() { current = nullptr; }
        ~Iterator() {} 
        static Iterator create(IterType iter) {
            Iterator i;
            i.current = iter;
            return i;
        }
        RegistryNode operator*(){ return *current; }
        void operator++() { current = current->next; }
        bool operator==(Iterator other) { return other.current == current; }
        bool operator!=(Iterator other) { return other.current != current; }
        RegistryNode get() {return *current; }
    };

    Iterator begin(){ return Registry::Iterator::create(this->head); }
    Iterator end(){ return Registry::Iterator::create(nullptr); }
    template<class T>
    Iterator iter_to();

};

Registry::Registry(/* args */)
{
    this->head = nullptr;
    this->tail = nullptr;
}

Registry::~Registry()
{
}

Registry::Registry(const Registry& r){
    this->head = r.head;
    this->tail = r.tail;
}

Registry::Registry(Registry&& r){
    this->head = r.head;
    this->tail = r.tail;

    r.head = nullptr;
    r.tail = nullptr;
}

template<class T>
Registry::Iterator Registry::iter_to(){
    std::cout << "iterating" << std::endl;
    Registry::Iterator iter = this->begin();
    for (iter; iter != this->end(); ++iter){
        if (iter.current->check_type<T>()) break;
        std::cout << "next" << std::endl;
    }
    return iter;
}

template<class T>
void Registry::register_component() {
    // TODO: ensure that T isn't registered
    auto temp = RegistryNode::create<T>();
    if (head == nullptr) {
        head = temp;
    } else {
        tail->next = temp;
    }
    tail = temp;
}

void Registry::register_entities() {
    // TODO: ensure that T isn't registered
    auto temp = RegistryNode::create<ecs::entity::Entity>();
    temp->next = head;
    head = temp;
}

} // Namespace
#endif