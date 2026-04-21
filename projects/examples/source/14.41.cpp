////////////////////////////////////////////////////////////////////////////////////

// chapter : Parallelism

////////////////////////////////////////////////////////////////////////////////////

// section : Atomics

////////////////////////////////////////////////////////////////////////////////////

// content : Thread Launch Synchronization

////////////////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <syncstream>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////

using namespace std::literals;

////////////////////////////////////////////////////////////////////////////////////

class Entity
{
public :

    void test() const
    {
        trace(); m_x.wait(false);

        trace(); 
    }

//  --------------------------------------------------------------------------------

    void release() const
    {
        m_x = true;

        m_x.notify_all();
    }

private :

    void trace() const
    {
        auto id = std::this_thread::get_id();

        std::osyncstream(std::cout) << std::format("Entity::trace : id = {}\n", id);
    }

//  --------------------------------------------------------------------------------

    mutable std::atomic < bool > m_x = false;
};

////////////////////////////////////////////////////////////////////////////////////

int main()
{
    Entity entity;

//  -----------------------------------------------

    std::jthread jthread_1(&Entity::test, &entity);

    std::jthread jthread_2(&Entity::test, &entity);

//  -----------------------------------------------

    std::this_thread::sleep_for(1s);

//  -----------------------------------------------

    entity.release();
}

////////////////////////////////////////////////////////////////////////////////////