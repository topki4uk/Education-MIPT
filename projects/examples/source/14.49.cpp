/////////////////////////////////////////////////////////////////////////////////////

// chapter : Parallelism

/////////////////////////////////////////////////////////////////////////////////////

// section : Atomics

/////////////////////////////////////////////////////////////////////////////////////

// content : Lock-Free Programming
//
// content : Thread-Safe Lock-Free Stacks
//
// content : Hazard Pointers
//
// content : Garbage Collectors

/////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>

/////////////////////////////////////////////////////////////////////////////////////

#include <boost/noncopyable.hpp>

/////////////////////////////////////////////////////////////////////////////////////

template < typename T > class Stack : private boost::noncopyable
{
private :

    struct Node
    {
        T x = T();

        Node * next = nullptr;
    };

//  ---------------------------------------------------------------------------------

    struct Pointer 
    {
        std::atomic < std::thread::id > id = std::thread::id();

        std::atomic < Node * > node = nullptr;
    };

//  ---------------------------------------------------------------------------------

    class Handler
    {
    public :

        Handler() : m_pointer(nullptr)
        {
            for (auto & pointer : s_pointers)
            {
                std::thread::id id;

                if
                (
                    pointer.id.compare_exchange_strong
                    (
                        id, std::this_thread::get_id(),

                        std::memory_order::acquire,

                        std::memory_order::relaxed
                    )
                )
                {
                    m_pointer = &pointer;

                    break;
                }
            }
        }

    //  -----------------------------------------------------------------------

       ~Handler()
        {
            m_pointer->node.store(nullptr, std::memory_order::relaxed);

            m_pointer->id.store(std::thread::id(), std::memory_order::release);
        }

    //  -----------------------------------------------------------------------

        auto & get() const 
        {
            return m_pointer->node;
        }

    private :

        Pointer * m_pointer = nullptr;
    };

//  ---------------------------------------------------------------------------------

    class Storage
    {
    private :

        class Retired_Node
        {
        public :

           ~Retired_Node()
            {
                delete node;
            }

        //  ------------------------------

            Node * node = nullptr;

            Retired_Node * next = nullptr;
        };

    public :

        void push_back(Node * node)
        {
            push_back(new Retired_Node(node, nullptr));

            m_size.fetch_add(1, std::memory_order::relaxed);
        }

    //  -----------------------------------------------------------------------------

        void try_clear() 
        {
            if (m_size.load(std::memory_order::relaxed) >= std::size(s_pointers) * 2)
            {
                clear();
            }
        }

    private :

        void push_back(Retired_Node * node) 
        {
            node->next = m_head.load(std::memory_order::relaxed);

            while 
            (
                !m_head.compare_exchange_weak
                (
                    node->next, node,

                    std::memory_order::release,

                    std::memory_order::relaxed
                )
            );
        }

    //  -----------------------------------------------------------------------------

        void clear() 
        {
            auto node = m_head.exchange(nullptr, std::memory_order::acquire);

            m_size.store(0, std::memory_order_relaxed);

            auto size = 0uz;

            while (node)
            {
                auto next = node->next;

                if (!has_pointers(node->node)) 
                {
                    delete node;
                }
                else 
                {
                    push_back(node);

                    ++size;
                }

                node = next;
            }

            if (size > 0) 
            {
                m_size.fetch_add(size, std::memory_order_relaxed);
            }
        }

    //  -----------------------------------------------------------------------------

        std::atomic < Retired_Node * > m_head = nullptr;

        std::atomic < std::size_t > m_size = 0;
    };

public :

   ~Stack()
    {
        T x = T();

        while (top_and_pop(x));
    }

//  ---------------------------------------------------------------------------------

    void push(T x)
    {
        auto node = new Node(x, nullptr);

        node->next = m_head.load(std::memory_order::relaxed);

        while
        (
            !m_head.compare_exchange_weak
            (
                node->next, node, 

                std::memory_order::release, 

                std::memory_order::relaxed
            )
        );
    }

//  ---------------------------------------------------------------------------------

    auto top_and_pop(T & x)
    {
        auto & pointer = get_pointer();

        auto head = m_head.load(std::memory_order::acquire);

        do 
        {
            Node * node = nullptr;

            do 
            {
                node = head;

                pointer.store(head);

                head = m_head.load(std::memory_order::acquire);
            }
            while (head != node);
        }
        while
        (
            head && !m_head.compare_exchange_strong
            (
                head, head->next,

                std::memory_order::acquire,

                std::memory_order::relaxed
            )
        );

        pointer.store(nullptr, std::memory_order::release);

        if (head)
        {
            x = head->x;

            if (has_pointers(head))
            {
                m_storage.push_back(head);
            }
            else
            {
                delete head;
            }

            m_storage.try_clear();

            return true;
        }

        return false;
    }

private :

    auto & get_pointer() 
    {
        thread_local static Handler handler;

        return handler.get();
    }

//  ---------------------------------------------------------------------------------

    static auto has_pointers(Node * node)
    {
        for (auto & pointer : s_pointers)
        {
            if (pointer.id.load(std::memory_order::acquire) != std::thread::id())
            {
                if (pointer.node.load(std::memory_order::relaxed) == node) 
                {
                    return true;
                }
            }
        }

        return false;
    }

//  ---------------------------------------------------------------------------------

    std::atomic < Node * > m_head = nullptr;

    Storage m_storage;

//  ---------------------------------------------------------------------------------

    static inline std::array < Pointer, 64 > s_pointers = {};
};

/////////////////////////////////////////////////////////////////////////////////////

void produce(Stack < int > & stack)
{
    for (auto i = 1; i < (1 << 10) + 1; ++i)
    {
        stack.push(i);
    }
}

/////////////////////////////////////////////////////////////////////////////////////

void consume(Stack < int > & stack)
{
    auto x = 0;

    for (auto i = 0uz; i < 1 << 10; ++i)
    {
        stack.top_and_pop(x);
    }
}

/////////////////////////////////////////////////////////////////////////////////////

int main()
{
    Stack < int > stack;

//  ------------------------------------------------

    std::jthread thread_1(produce, std::ref(stack));

    std::jthread thread_2(consume, std::ref(stack));
}

/////////////////////////////////////////////////////////////////////////////////////