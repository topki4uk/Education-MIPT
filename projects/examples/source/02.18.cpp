//////////////////////////////////////////////////////////////////

#include <iostream>

//////////////////////////////////////////////////////////////////

int main()
{
    unsigned char symbol;
    unsigned char symbol_type;
    std::cin >> symbol;

    if (!(32 <= symbol && symbol <= 127)) {
        std::cout << "Not in matched range!\n";
        return 0;
    }

    if ('0' <= symbol && symbol <= '9')
    {
        symbol_type = '1';
    }
    else if ('A' <= symbol && symbol <= 'Z')
    {
        symbol_type = '2';
    }
    else if ('a' <= symbol && symbol <= 'z')
    {
        symbol_type = '3';
    }
    else if (symbol == '.' || symbol == ',' || symbol == ':' || symbol == ';')
    {
        symbol_type = '4';
    }
    else 
    {
        symbol_type = '5';
    }

    switch (symbol_type)
    {
        case '1':
            std::cout << "This symbol is digit!\n";
            break;
        
        case '2': 
            std::cout << "This symbol is uppercase letter!\n";
            break;
        
        case '3': 
            std::cout << "This symbol is lowercase letter!\n";
            break;
        
        case '4':
            std::cout << "This symbol is punctuation mark!\n";
            [[ fallthrough ]];

        case '5':
            std::cout << "This symbol is from other scope!\n";
    }

    return 0;
}

//////////////////////////////////////////////////////////////////