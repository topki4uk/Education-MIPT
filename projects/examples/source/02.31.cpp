//////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

//////////////////////////////////////////////////////////////////////

void nextcollatz(unsigned long long int& number) {
	if (number % 2 == 0)
	{
		number /= 2;
	} 
	else 
	{
		number *= 3;
		number++;
	}
}

int main()
{
	std::vector <int> collatz_length(100, 0);
	int seq_length = 0;
	int count = 0;
	unsigned long long int collatz = 1;

	while (count < 100)
	{
		if (collatz == 1)
		{
			collatz_length[count] = seq_length;
			seq_length = 0;
			count++;
			collatz = count + 1;
		}
		else if (collatz < 100 && collatz_length[collatz - 1] != 0)
		{
			collatz_length[count] = collatz_length[collatz - 1] + seq_length;
			seq_length = 0;
			count++;
			collatz = count + 1;
		}
		else 
		{
			nextcollatz(collatz);
			seq_length++;
		}
	}

	int max_ind = 0;
	for (auto i = 0uz; i < 100; i++)
	{
		if (collatz_length[max_ind] < collatz_length[i])
		{
			max_ind = i;
		}
	}

	std::cout << max_ind + 1 << ": " << collatz_length[max_ind] << '\n';
	return 0;
}

//////////////////////////////////////////////////////////////////////