/***************************************************************************
 *  local/test1.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <limits>

#include <stxxl/vector>
#include <stxxl/random>
#include <stxxl/sort>

struct my_less_int : std::less<int>
{
    int min_value() const { return std::numeric_limits<int>::min(); };
    int max_value() const { return std::numeric_limits<int>::max(); };
};

int main()
{
    // create vector
    stxxl::VECTOR_GENERATOR<int>::result vector;

    // fill vector with random integers
    stxxl::random_number32 random;

    for (size_t i = 0; i < 100*1024*1024; ++i) {
        vector.push_back(random());
    }

    // sort vector using 16 MiB RAM
    stxxl::sort(vector.begin(), vector.end(), my_less_int(), 16*1024*1024);

    // output first and last items:
    std::cout << vector.size() << " items sorted ranging from "
              << vector.front() << " to " << vector.back() << std::endl;

    return 0;
}
