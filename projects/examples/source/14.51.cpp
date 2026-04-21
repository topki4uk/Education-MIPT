//////////////////////////////////////////////////////////////////////////////////

// chapter : Parallelism

//////////////////////////////////////////////////////////////////////////////////

// section : Atomics

//////////////////////////////////////////////////////////////////////////////////

// content : Lock-Free Stacks
//
// content : Library Boost.Lockfree
//
// content : Microbenchmarking

//////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
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

//////////////////////////////////////////////////////////////////////////////////

#include <boost/lockfree/stack.hpp>
#include <boost/noncopyable.hpp>

//////////////////////////////////////////////////////////////////////////////////

#include <benchmark/benchmark.h>

//////////////////////////////////////////////////////////////////////////////////

#include "08.35.hpp"

//////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////

template < typename T > class Stack_v2 : private boost::noncopyable
{
private :

    struct Node
    {
        T x = T();

        Node * next = nullptr;
    };

public :

   ~Stack_v2()
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

//////////////////////////////////////////////////////////////////////////////////

template < typename T > using Stack_v3 = boost::lockfree::stack < T > ;

//////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////

BENCHMARK(test)->Arg(1)->Arg(2)->Arg(3);

//////////////////////////////////////////////////////////////////////////////////

int main()
{
    benchmark::RunSpecifiedBenchmarks();
}

//////////////////////////////////////////////////////////////////////////////////