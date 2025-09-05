///////////////////////////

#include <cassert>
#include <iostream>
#include <cmath>

///////////////////////////

int main()
{
	const double SQRT_FIVE = std::sqrt(5);
	int sequence_index;
	std::cin >> sequence_index;

	double sequence_number = (std::pow((1 + SQRT_FIVE) / 2, sequence_index) - std::pow((1 - SQRT_FIVE) / 2, sequence_index)) / SQRT_FIVE;
	std::cout << sequence_number << '\n';

	return 0;
}

///////////////////////////