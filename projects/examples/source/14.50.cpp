/////////////////////////////////////////////////////////////////////////////////////

// chapter : Parallelism

/////////////////////////////////////////////////////////////////////////////////////

// section : Atomics

/////////////////////////////////////////////////////////////////////////////////////

// content : Lock-Free Stacks
//
// content : Library Boost.Lockfree
//
// content : Microbenchmarking

/////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <cmath>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <ranges>
#include <thread>
#include <vector>

/////////////////////////////////////////////////////////////////////////////////////

#include <boost/lockfree/stack.hpp>
#include <boost/noncopyable.hpp>

/////////////////////////////////////////////////////////////////////////////////////

#include <benchmark/benchmark.h>

/////////////////////////////////////////////////////////////////////////////////////

#include "08.35.hpp"

/////////////////////////////////////////////////////////////////////////////////////

template < typename T > class Stack_v1 : private boost::noncopyable
{
public :

    Stack_v1(std::size_t capacity)
    {
        m_vector.reserve(capacity);
    }

//  --------------------------------------------------

    void push(T x)
    {
        std::scoped_lock < std::mutex > lock(m_mutex);

        m_vector.push_back(x);
    }

//  --------------------------------------------------

    auto top_and_pop(T & x)
    {
        std::scoped_lock < std::mutex > lock(m_mutex);

        if (!std::empty(m_vector))
        {
            x = m_vector.back();

            m_vector.pop_back();

            return true;
        }
        else
        {
            return false;
        }
    }

private :

    std::vector < T > m_vector;

//  --------------------------------------------------

    mutable std::mutex m_mutex;
};

/////////////////////////////////////////////////////////////////////////////////////

template < typename T > class Stack_v2 : private boost::noncopyable
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

            m_size.store(0, std::memory_order::relaxed);

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
                m_size.fetch_add(size, std::memory_order::relaxed);
            }
        }

    //  -----------------------------------------------------------------------------

        std::atomic < Retired_Node * > m_head = nullptr;

        std::atomic < std::size_t > m_size = 0;
    };

public :

   ~Stack_v2()
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

template < typename T > using Stack_v3 = boost::lockfree::stack < T > ;

/////////////////////////////////////////////////////////////////////////////////////

class Task
{
public :

    virtual ~Task() = default;

//  ------------------------------------------

    auto operator()(std::barrier <> & barrier)
    {
        barrier.arrive_and_wait();

        Timer timer;

        test();

        return timer.elapsed().count();
    }

//  ------------------------------------------

    virtual void test() = 0;
};

/////////////////////////////////////////////////////////////////////////////////////

template < typename S > class Task_v1 : public Task
{
public :

    Task_v1(S & stack) : m_stack(stack) {}

//  ---------------------------------------------------------------------

    void test() override
    {
        for (auto i = 1; i < (1 << 10) + 1; ++i)
        {
            m_stack.push(i);
        }
    }

private :

    S & m_stack;
};

/////////////////////////////////////////////////////////////////////////////////////

template < typename S > class Task_v2 : public Task
{
public :

    Task_v2(S & stack) : m_stack(stack) {}

//  ---------------------------------------------------------------------

    void test() override
    {
        auto x = 0;

        for (auto i = 0uz; i < 1 << 10; ++i)
        {
            m_stack.top_and_pop(x);

            for (auto j = 0uz; j < 1 << 10; ++j)
            {
                x += std::pow(std::sin(x), 2) + std::pow(std::cos(x), 2);
            }
        }

        benchmark::DoNotOptimize(x);
    }

private :

    S & m_stack;
};

/////////////////////////////////////////////////////////////////////////////////////

template < typename S > class Task_v3 : public Task
{
public :

    Task_v3(S & stack) : m_stack(stack) {}

//  ---------------------------------------------------------------------

    void test() override
    {
        auto x = 0;

        for (auto i = 0uz; i < 1 << 10; ++i)
        {
            m_stack.pop(x);

            for (auto j = 0uz; j < 1 << 10; ++j)
            {
                x += std::pow(std::sin(x), 2) + std::pow(std::cos(x), 2);
            }
        }

        benchmark::DoNotOptimize(x);
    }

private :

    S & m_stack;
};

/////////////////////////////////////////////////////////////////////////////////////

void test(benchmark::State & state)
{
    auto argument = state.range(0);

    auto concurrency = std::max(std::thread::hardware_concurrency(), 2u);

    std::vector < std::future < double > > futures(concurrency);

    auto size = concurrency * (1 << 10);

    Stack_v1 < int > stack_v1(2 * size);

    Stack_v2 < int > stack_v2;

    Stack_v3 < int > stack_v3(2 * size);

    for (auto i = 1uz; i < size + 1; ++i)
    {
        stack_v1.push(i);

        stack_v2.push(i);

        stack_v3.push(i);
    }

    std::shared_ptr < Task > task_1;

    std::shared_ptr < Task > task_2;

    switch (argument)
    {
        case 1 :
        {
            task_1 = std::make_shared < Task_v1 < Stack_v1 < int > > > (stack_v1);

            task_2 = std::make_shared < Task_v2 < Stack_v1 < int > > > (stack_v1);

            break;
        }

        case 2 :
        {
            task_1 = std::make_shared < Task_v1 < Stack_v2 < int > > > (stack_v2);

            task_2 = std::make_shared < Task_v2 < Stack_v2 < int > > > (stack_v2);

            break;
        }

        case 3 :
        {
            task_1 = std::make_shared < Task_v1 < Stack_v3 < int > > > (stack_v3);

            task_2 = std::make_shared < Task_v3 < Stack_v3 < int > > > (stack_v3);

            break;
        }
    }

    std::barrier <> barrier(concurrency + 1);

    auto lambda = [](auto & future){ return future.get(); };

    for (auto element : state)
    {
        for (auto i = 0uz; i < concurrency / 2; ++i)
        {
            futures[i] = std::async
            (
                std::launch::async, &Task::operator(), task_1, std::ref(barrier)
            );
        }

        for (auto i = concurrency / 2; i < concurrency; ++i)
        {
            futures[i] = std::async
            (
                std::launch::async, &Task::operator(), task_2, std::ref(barrier)
            );
        }

        barrier.arrive_and_wait();

        auto time = *std::ranges::fold_left_first
        (
            std::views::transform(futures, lambda), std::plus()
        );

        state.SetIterationTime(time / concurrency);

		benchmark::DoNotOptimize(*task_1);

        benchmark::DoNotOptimize(*task_2);
    }
}

/////////////////////////////////////////////////////////////////////////////////////

BENCHMARK(test)->Arg(1)->Arg(2)->Arg(3);

/////////////////////////////////////////////////////////////////////////////////////

int main()
{
    benchmark::RunSpecifiedBenchmarks();
}

/////////////////////////////////////////////////////////////////////////////////////