/////////////////////////////////////////////////////////////////////////////////////////////

// chapter : Parallelism

/////////////////////////////////////////////////////////////////////////////////////////////

// section : Processes

/////////////////////////////////////////////////////////////////////////////////////////////

// content : Process Communication
//
// content : Library Boost.Interpocess
//
// content : Shared Memory
//
// content : Memory Mapped Files

/////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <thread>

/////////////////////////////////////////////////////////////////////////////////////////////

using namespace std::literals;

/////////////////////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

/////////////////////////////////////////////////////////////////////////////////////////////

#include <sys/wait.h>
#include <unistd.h>

/////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	auto path = "shared_memory";

//  -----------------------------------------------------------------------------------------

	boost::interprocess::shared_memory_object::remove(path);

//  -----------------------------------------------------------------------------------------

    if (auto id = fork(); id != 0)
    {
        boost::interprocess::shared_memory_object storage
        (
		    boost::interprocess::create_only, path, boost::interprocess::read_write
        );

    //  -------------------------------------------------------------------------------------

        storage.truncate(1 << 10);

    //  -------------------------------------------------------------------------------------

        boost::interprocess::mapped_region mapping(storage, boost::interprocess::read_write);

    //  -------------------------------------------------------------------------------------

        auto begin = static_cast < std::byte * > (mapping.get_address());

    //  -------------------------------------------------------------------------------------

        std::ranges::fill(begin, std::next(begin, mapping.get_size()), std::byte(1));

    //  -------------------------------------------------------------------------------------

        wait(nullptr);

    //  -------------------------------------------------------------------------------------

        boost::interprocess::shared_memory_object::remove(path);
    }
    else
    {
        std::this_thread::sleep_for(1s);

    //  ------------------------------------------------------------------------------------

        boost::interprocess::shared_memory_object storage
        (
		    boost::interprocess::open_only, path, boost::interprocess::read_only
        );

    //  ------------------------------------------------------------------------------------

        boost::interprocess::mapped_region mapping(storage, boost::interprocess::read_only);

    //  ------------------------------------------------------------------------------------

        auto begin = static_cast < std::byte * > (mapping.get_address());

    //  ------------------------------------------------------------------------------------

        auto lambda = [](auto x){ assert(x == std::byte(1)); };

    //  ------------------------------------------------------------------------------------

        std::ranges::for_each(begin, std::next(begin, mapping.get_size()), lambda);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////