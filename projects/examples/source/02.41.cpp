//////////////////////////////////////////////////////////////////////////////////////////////

// support : www.cs.usfca.edu/~galles/visualization/RecFact.html

//////////////////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <numeric>


int mygcd(int a, int b)
{
	while (b != 0) {
		int tmp = a;
		a = b;
		b = tmp % a;
	}
	return a;
}

int mylcm(int a, int b)
{
	int t = a % b;
	if (t == 0)
	{
		return a;
	}
	return a * mylcm(b, t) / t;
}

int main()
{
	assert(std::gcd(5, 2) == mygcd(5, 2));
	assert(std::gcd(12, 4) == mygcd(12, 4));
	assert(std::gcd(14, 63) == mygcd(14, 63));
	assert(std::gcd(23, 5) == mygcd(23, 5));
	assert(std::gcd(7, 56) == mygcd(7, 56));

	assert(std::lcm(5, 2) == mylcm(5, 2));
	assert(std::lcm(13, 4) == mylcm(13, 4));
	assert(std::lcm(16, 9) == mylcm(16, 9));
	assert(std::lcm(14, 12) == mylcm(14, 12));
	assert(std::lcm(13, 19) == mylcm(13, 19));

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////