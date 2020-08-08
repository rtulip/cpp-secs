#ifndef ecs_registry_hpp
#define ecs_registry_hpp
#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <ecs/entity.hpp>

namespace ecs::registry
{

    /**
     * @brief A non-templated data structure to hold a generic type.
     * 
     * The RegistryNode is at the core of this ECS design. The aim of the RegistryNode
     * is to be a generic container, which isn't templated. This requires the data to be
     * held in a void*, which will has some safety concerns which will be discussed 
     * later. In order to have RegistryNode not be templated, accessing and the data of 
     * the RegistryNode is templated in exchange. 
     * 
     * Safety:
     * Since the pointer to the RegistryNode data is a void* its easy for undefined 
     * behaviour to creep into the project. To ensure that this data structure is used
     * safely the following invariants must be upheld.
     * 
     *      1. For a given RegistryNode, it's data will be a pointer to a vector<T>
     *      2. T is determined when the RegistryNode is constructed.
     *      3. T will NOT change at runtime.
     * 
     * Variants:
     *      A RegistryNode can be used in two ways: as a Component container, or a
     *      Resource container. The only difference is that a Resource will only have one
     *      element in the vector, and any get() call will return a pointer to the one
     *      element, whereas a Component container will have many elements which can be
     *      accessed with a get() call. 
     * 
     *      The following assumptions must be upheld for Resource RegistryNodes:
     *          - There is ALWAYS 1 element in the 0th position of the vector.
     *          - No elements are added or removed from the vector<T> after construction.
     * 
     */
    class RegistryNode
    {
    private:
        std::shared_ptr<void> data;
        const size_t data_hash_code;
        template <class T>
        bool check_type();
        template <class T>
        std::shared_ptr<std::vector<T>> cast();
        RegistryNode(size_t hash_code);

    public:
        ~RegistryNode() = default;

        enum class Type
        {
            Component,
            Resource,
            Unknown,
        } NodeType;

        template <class T>
        static RegistryNode create();
        template <class T>
        static RegistryNode create_resource(T &t);
        template <class T>
        static RegistryNode create_resource(T &&t);
        template <class T>
        void push(T &&t);
        template <class T>
        T *get(size_t i);
        template <class T>
        void erase(size_t i);
        template <class T>
        void set(size_t i, T &&t);
        template <class T>
        size_t size();
        template <class T>
        std::shared_ptr<std::vector<T>> iter();

        size_t get_hash();
        bool is_resource();
    };

    /**
     * @brief UNSAFE Constructor of a new Registry Node:: Registry Node object
     * 
     * Safety:
     *      This function is UNSAFE, as the associated type T cannot be resolved,
     *      which breaks the 2nd invariant.
     */
    RegistryNode::RegistryNode(size_t hash_code) : data_hash_code(hash_code)
    {
        this->data = nullptr;
        this->NodeType = RegistryNode::Type::Unknown;
    }

    /**
     * @brief A safe constructor of a Component RegistryNode.
     * 
     * Safety:
     *      The first invariant is upheld, as this constructs a shared pointer to a 
     *      vector<T> and assigns it to the RegistryNode's data.
     * 
     *      The second invariant is upheld, by associating the type T with this 
     *      RegistryNode through the type hash_code. typeid(T).hash_code() is assigned
     *      to the RegistryNode's data_hash_code.     
     * 
     *      So long as the data_hash_code is checked for EVERY manipulation of the data,
     *      the second invariant will be upheld.
     *      
     *      The third invariant is upheld because the data_hash_code is a const member.
     * 
     * @tparam T - The type to be associated with the new RegistryNode
     * @return RegistryNode - The RegistryNode associated with the type T.
     */
    template <class T>
    RegistryNode RegistryNode::create()
    {
        RegistryNode node(typeid(T).hash_code());

        auto v_ptr = std::make_shared<std::vector<T>>();
        node.data = v_ptr;
        node.NodeType = RegistryNode::Type::Component;
        return node;
    }

    /**
     * @brief A safe constructor of a Resource RegistryNode.
     * 
     * Safety:
     *      The first invariant is upheld, as this constructs a shared pointer to a 
     *      vector<T> and assigns it to the RegistryNode's data.
     * 
     *      The second invariant is upheld, by associating the type T with this 
     *      RegistryNode through the type hash_code. typeid(T).hash_code() is assigned
     *      to the RegistryNode's data_hash_code.     
     * 
     *      So long as the data_hash_code is checked for EVERY manipulation of the data,
     *      the second invariant will be upheld.
     *      
     *      The third invariant is upheld because the data_hash_code is a const member.
     *  
     *      Additionally, t is moved into the node here, thus upholding the existance of
     *      the 0th element in the vector.
     * 
     * @tparam T - The type to be associated with the new RegistryNode
     * @return RegistryNode - The RegistryNode associated with the type T.
     */
    template <class T>
    RegistryNode RegistryNode::create_resource(T &t)
    {
        RegistryNode node(typeid(T).hash_code());

        auto v_ptr = std::make_shared<std::vector<T>>();
        node.data = v_ptr;
        node.NodeType = RegistryNode::Type::Resource;
        node.cast<T>()->push_back(t);
        return node;
    }

    /**
     * @brief A safe constructor of a Resource RegistryNode.
     * 
     * Safety:
     *      The first invariant is upheld, as this constructs a shared pointer to a 
     *      vector<T> and assigns it to the RegistryNode's data.
     * 
     *      The second invariant is upheld, by associating the type T with this 
     *      RegistryNode through the type hash_code. typeid(T).hash_code() is assigned
     *      to the RegistryNode's data_hash_code.     
     * 
     *      So long as the data_hash_code is checked for EVERY manipulation of the data,
     *      the second invariant will be upheld.
     *      
     *      The third invariant is upheld because the data_hash_code is a const member.
     *  
     *      Additionally, t is moved into the node here, thus upholding the existance of
     *      the 0th element in the vector.
     * 
     * @tparam T - The type to be associated with the new RegistryNode
     * @return RegistryNode - The RegistryNode associated with the type T.
     */
    template <class T>
    RegistryNode RegistryNode::create_resource(T &&t)
    {
        RegistryNode node(typeid(T).hash_code());

        auto v_ptr = std::make_shared<std::vector<T>>();
        node.data = v_ptr;
        node.NodeType = RegistryNode::Type::Resource;
        node.cast<T>()->push_back(std::move(t));
        return node;
    }

    /**
     * @brief A quick check to see if T matches this registry node.
     * 
     * Safety:
     *      This function is used to validate that type T is associated with this 
     *      RegistryNode. This is done by comparing the hash code of T to the 
     *      RegistryNode's data_hash_code. If they match, then it is safe to cast the
     *      data to a vector<T>.
     * 
     * @tparam T 
     * @return true - T is associated with this RegistryNode.
     * @return false - T is not associated with thie RegistryNode.
     */
    template <class T>
    bool RegistryNode::check_type()
    {
        return typeid(T).hash_code() == this->data_hash_code;
    }

    /**
     * @brief The safe accessor function to the data vector.
     * 
     * Safety:
     *      This function is a safe accessor because it upholds all invariants.
     * 
     *      1. T is checked to be the appropriate associated type, otherwise a runtime
     *         error is thrown.
     *      2. The data pointer is ALWAYS cast to a vector<T> if T is correct.
     *     
     *      So long as this function is ALWAYS used before accessing the data vector,
     *      then the RegistryNode can be modified and accessed safely.
     * 
     * @tparam T 
     * @return std::shared_ptr<std::vector<T>> 
     */
    template <class T>
    std::shared_ptr<std::vector<T>> RegistryNode::cast()
    {
        if (!this->check_type<T>())
            throw std::runtime_error("Type doesn't match node");
        if (this->NodeType == RegistryNode::Type::Unknown)
            throw std::runtime_error("RegistryNode formed improperly and has an unknown NodeType");
        return std::static_pointer_cast<std::vector<T>>(this->data);
    }

    /**
     * @brief A safe function to add a T to the end of the data vector.
     * 
     * This comsumes t.
     * 
     * Safety:
     *      This function uses cast<T> to modify the RegistryNode data pointer, thus all
     *      invariants are upheld.
     *  
     *      This function does nothing when operating on a Resource RegistryNode, 
     *      assuring that no elements are added to the resource.
     * 
     * @tparam T - The associated type of this RegistryNode
     * @param t - An instance of T
     */
    template <class T>
    void RegistryNode::push(T &&t)
    {
        if (this->NodeType == RegistryNode::Type::Component)
            this->cast<T>()->push_back(std::move(t));
    }

    /**
     * @brief A safe accessor to data[i]
     * 
     * Returns a pointer to the T
     * 
     * Safety:
     *      This function uses cast<T> to access the RegistryNode data pointer, thus all 
     *      invariants are upheld.
     * 
     *      Bounds are NOT checked at this point yet.
     * 
     * TODO: Check Bounds.
     * 
     * @tparam T - The type to be associated with the new RegistryNode
     * @param i - The index.
     * @return T* - A pointer to the T.
     */
    template <class T>
    T *RegistryNode::get(size_t i)
    {
        switch (this->NodeType)
        {
        case RegistryNode::Type::Component:
            return &((*this->cast<T>())[i]);
            break;
        case RegistryNode::Type::Resource:
            return &((*this->cast<T>())[0]);
            break;
        default:
            throw std::runtime_error("Cannot get from a RegistryNode with Unknown Type");
            break;
        }
    }

    /**
     * @brief A way to remove an element from a Component Registry
     * 
     * Removes the ith element from the RegistryNode vector if it is a Component Type.
     * Note:
     *      This will affect the indicies of entities who's component id is higher than i
     *      Because RegistryNodes are unaware of others, this MUST be handled properly
     * 
     * Safety: 
     *      This function uses cast<T> to access the RegistryNode data pointer, thus all 
     *      invariants are upheld.
     * 
     *      Bounds are NOT checked at this point yet.
     * 
     *      Additionally, no operation is done on a RegistryNode of Resource type, thus
     *      upholding the requirement that there is ALWAYS a 0th element in the node.
     * @tparam T - The type to be associated with the new RegistryNode
     * @param i - The index
     */
    template <class T>
    void RegistryNode::erase(size_t i)
    {
        switch (this->NodeType)
        {
        case RegistryNode::Type::Component:
            auto vec_ptr = this->cast<T>();
            vec_ptr->erase(vec_ptr->begin() + i);
        }
    }

    /**
     * @brief A safe setter function to data[i]
     * 
     * Safety:
     *      This function uses cast<T> to access the RegistryNode data pointer, thus all
     *      invariants are upheld.
     * 
     * @tparam T - The type to be associated with the new RegistryNode
     * @param i - The index.
     * @param t - The T instance.
     */
    template <class T>
    void RegistryNode::set(size_t i, T &&t)
    {
        switch (this->NodeType)
        {
        case RegistryNode::Type::Component:
            this->cast<T>()->at(i) = std::move(t);
            break;
        case RegistryNode::Type::Resource:
            this->cast<T>()->at(0) = std::move(t);
            break;
        }
    }

    /**
     * @brief A safe function to get the number of elements in the data vector.
     * 
     * Sasfety:
     *      This function uses cast<T> to access the RegistryNode data pointer, thus all
     *      invariants are upheld
     * 
     * @tparam T - The type to be associated with the new RegistryNode
     * @return size_t - the number of T in the vector.
     */
    template <class T>
    size_t RegistryNode::size()
    {
        return this->cast<T>()->size();
    }

    /**
     * @brief A safe accessor to the Registry node data vector.
     * 
     *  The intended use is to use this for iterating over the elements in the vector.
     *  
     *  TODO: Make this return a vector iterator, rather than the vector itself. 
     *  
     *  Safety:
     *      This function uses cast<T> to access the RegistryNode data pointer, thus all
     *      invariants are upheld.
     * 
     * @tparam T - The type to be associated with the new RegistryNode
     * @return std::shared_ptr<std::vector<T>> 
     */
    template <class T>
    std::shared_ptr<std::vector<T>> RegistryNode::iter()
    {
        return this->cast<T>();
    }

    /**
     * @brief Getter function for the data hash code
     * 
     * @return size_t - The data hash code
     */
    size_t RegistryNode::get_hash()
    {
        return this->data_hash_code;
    }

    /**
     * @brief Function to check if a RegistryNode is a resource
     * 
     * @return true
     * @return false 
     */
    bool RegistryNode::is_resource()
    {
        return this->NodeType == RegistryNode::Type::Resource;
    }

} // namespace ecs::registry
#endif