#include <iostream>
#include <stxxl/random>

int main()
{
    //stxxl::set_seed(42);
    std::cout << "seed = " << stxxl::get_next_seed() << std::endl;
    srand48(time(NULL));
    stxxl::random_number32 random_number32;
    stxxl::random_uniform_fast random_uniform_fast;
    stxxl::random_uniform_slow random_uniform_slow;
    stxxl::random_number<> random_number_fast;
    stxxl::random_number<stxxl::random_uniform_slow> random_number_slow;
    stxxl::random_number64 random_number64;

    for (int i = 0; i < 3; ++i) {
        std::cout << "d48 " << drand48() << std::endl;
        std::cout << "r32 " << random_number32() << std::endl;
        std::cout << "ruf " << random_uniform_fast() << std::endl;
        std::cout << "rus " << random_uniform_slow() << std::endl;
        std::cout << "rnf " << random_number_fast(42) << std::endl;
        std::cout << "rns " << random_number_slow(42) << std::endl;
        std::cout << "r64 " << random_number64() << std::endl;
        std::cout << std::endl;
    }
}
