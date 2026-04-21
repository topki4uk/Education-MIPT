/////////////////////////////////////////////////////////////////////////////

// chapter : Parallelism

/////////////////////////////////////////////////////////////////////////////

// section : Atomics

/////////////////////////////////////////////////////////////////////////////

// content : Lock-Free Programming
//
// content : Thread-Safe Lock-Free Stacks
//
// content : Garbage Collectors

/////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <thread>
#include <tuple>

/////////////////////////////////////////////////////////////////////////////

#include <boost/noncopyable.hpp>

/////////////////////////////////////////////////////////////////////////////

template < typename T > class Stack : private boost::noncopyable
{
private :

    struct Node
    {
        T x = T();

        Node * next = nullptr;
    };

public :

   ~Stack()
    {
        T x = T();

        while (top_and_pop(x));
    }

//  -------------------------------------------------------------------------

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

//  -------------------------------------------------------------------------

    auto top_and_pop(T & x)
    {
        m_counter.fetch_add(1, std::memory_order::acquire);

        auto head = m_head.load(std::memory_order::acquire);

        while 
        (
            head && !m_head.compare_exchange_weak
            (
                head, head->next,

                std::memory_order::acquire,

                std::memory_order::relaxed
            )
        );

        if (head)
        {
            x = head->x;

            try_free(head);

            return true;
        }

        m_counter.fetch_sub(1, std::memory_order::release);

        return false;
    }

private :

    void try_free(Node * head)
    {
        if (m_counter.load(std::memory_order::acquire) == 1)
        {
            auto tail = m_tail.exchange(nullptr, std::memory_order::acquire);

            if (!(m_counter.fetch_sub(1, std::memory_order::acq_rel) - 1))
            {
                free(tail);
            }
            else if (tail)
            {
                save(tail);
            }

            delete head;
        }
        else
        {
            save(head, head);

            m_counter.fetch_sub(1, std::memory_order::release);
        }
    }

//  -------------------------------------------------------------------------

    void free(Node * nodes) const
    {
        while (nodes)
        {
            auto next = nodes->next;

            delete nodes;

            nodes = next;
        }
    }

//  -------------------------------------------------------------------------

    void save(Node * nodes)
    {
        auto end = nodes;

        while(auto next = end->next)
        {
            end = next;
        }

        save(nodes, end);
    }

//  -------------------------------------------------------------------------

    void save(Node * begin, Node * end)
    {
        end->next = m_tail;

        while
        (
            !m_tail.compare_exchange_weak
            (
                end->next, begin,

                std::memory_order::release, 

                std::memory_order::relaxed                
            )
        );
    }

//  -------------------------------------------------------------------------

    std::atomic < Node * > m_head = nullptr, m_tail = nullptr;

    std::atomic < std::size_t > m_counter = 0;
};

/////////////////////////////////////////////////////////////////////////////

void top_and_pop(Stack < int > & stack)
{
    auto x = 0;

    stack.top_and_pop(x);
}

/////////////////////////////////////////////////////////////////////////////

int main()
{
    Stack < int > stack;

//  ---------------------------------------------------------

    stack.push(1);

    stack.push(2);

//  ---------------------------------------------------------

    {
        std::jthread jthread_1(top_and_pop, std::ref(stack));

        std::jthread jthread_2(top_and_pop, std::ref(stack));
    }
}

/////////////////////////////////////////////////////////////////////////////