#include <sys/types.h>
#include <iostream>

int main()
{
    std::cout << "Size of 'off_t' type is " << sizeof(off_t) * 8 << " bits." << std::endl;
}
