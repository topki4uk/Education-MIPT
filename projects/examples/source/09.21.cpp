////////////////////////////////////////////////////////////////////////////////

// chapter : Memory

////////////////////////////////////////////////////////////////////////////////

// section : Memory

////////////////////////////////////////////////////////////////////////////////

// content : Uninitialized Memory
//
// content : Functions operator new and operator delete
//
// content : Enumeration std::align_val_t
//
// content : Placement Operator new
//
// content : Memory Laundering
//
// content : Function std::launder
//
// content : Functions std::construct_at and std::destroy_at

////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <print>

////////////////////////////////////////////////////////////////////////////////

class Entity
{
public :

	Entity(int x) : m_x(x)
	{
		std::print("Entity:: Entity : m_x = {}\n", m_x);
	}

//  ----------------------------------------------------

   ~Entity()
	{
		std::print("Entity::~Entity : m_x = {}\n", m_x);
	}

//  ----------------------------------------------------

	auto get() const
	{
		return m_x;
	}

private :

	int const m_x = 0;
};

////////////////////////////////////////////////////////////////////////////////

int main()
{
	auto storage = static_cast < std::byte * >
	(
		operator new(sizeof(Entity), std::align_val_t(alignof(Entity)))
	);

//  ----------------------------------------------------------------------------

	auto entity_1 = new (storage) Entity(1);

//  ----------------------------------------------------------------------------

	entity_1->~Entity();

//  ----------------------------------------------------------------------------

	auto entity_2 = std::construct_at(std::bit_cast < Entity * > (storage), 2);

//  ----------------------------------------------------------------------------

//	assert(entity_1->get() == 2); // bad

//  ----------------------------------------------------------------------------

	assert(std::launder(entity_1)->get() == 2);

//  ----------------------------------------------------------------------------

	std::destroy_at(entity_2);

//  ----------------------------------------------------------------------------

	operator delete(storage, sizeof(Entity), std::align_val_t(alignof(Entity)));
}

////////////////////////////////////////////////////////////////////////////////