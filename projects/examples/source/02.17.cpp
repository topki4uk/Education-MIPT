/////////////////////////////////////////////////////////////

#include <iostream>
#include <cmath>

/////////////////////////////////////////////////////////////

int main()
{
    const double EPS = 1e-5;
    double a, b, c;
    double discr;
    std::cin >> a >> b >> c;

    if (a == 0) 
    {
        std::cout << "Not a quadratic equation!\n";
        return 0;
    }

    discr = b * b - 4 * a * c;

    if (discr > EPS) 
    {
        double x1, x2;
        x1 = (-b + std::sqrt(discr)) / (2 * a);
        x2 = (-b - std::sqrt(discr)) / (2 * a);

        std::cout << "Two solutions!\n" << "x1 is " << x1 << ", x2 is " << x2 << '\n';

    } 
    else if (std::abs(discr) < EPS)
    {
        double x;
        x = (-b) / (2 * a);
        
        std::cout << "One solution!\n" << "x is " << x << '\n';
    }
    else 
    {
        std::cout << "No real solutions!\n";
    }

    return 0;
}

/////////////////////////////////////////////////////////////