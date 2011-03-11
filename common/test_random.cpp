/***************************************************************************
 *  common/test_random.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright © 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <cassert>
#include <stxxl/random>

int main()
{
    //stxxl::set_seed(42);
    std::cout << "seed = " << stxxl::get_next_seed() << std::endl;

    stxxl::srandom_number32(stxxl::get_next_seed());
#ifndef BOOST_MSVC
    srand48(time(NULL));
#endif
    stxxl::random_number32 random_number32;
    stxxl::random_number32_r random_number32_r;
    stxxl::random_uniform_fast random_uniform_fast;
    stxxl::random_uniform_slow random_uniform_slow;
    stxxl::random_number<> random_number_fast;
    stxxl::random_number<stxxl::random_uniform_slow> random_number_slow;
    stxxl::random_number64 random_number64;

    for (int i = 0; i < 3; ++i) {
#ifndef BOOST_MSVC
        std::cout << "d48 " << drand48() << std::endl;
#endif
        std::cout << "r32 " << random_number32() << std::endl;
        std::cout << "r3r " << random_number32_r() << std::endl;
        std::cout << "ruf " << random_uniform_fast() << std::endl;
        std::cout << "rus " << random_uniform_slow() << std::endl;
        std::cout << "rnf " << random_number_fast(42) << std::endl;
        std::cout << "rns " << random_number_slow(42) << std::endl;
        std::cout << "r64 " << random_number64() << std::endl;
        std::cout << std::endl;
    }

    stxxl::set_seed(42);
    assert(stxxl::get_next_seed() == 42);
    assert(stxxl::get_next_seed() != 42);
}
