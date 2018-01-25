/***************************************************************************
 *  examples/containers/vector2.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Daniel Feist <daniel.feist@student.kit.edu>
 *  Copyright (C) 2018 Manuel Penschuck <stxxl@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <random>

#include <tlx/logger.hpp>

#include <foxxll/common/timer.hpp>

#include <stxxl/vector>

using data_type = unsigned int;

int main()
{
    // template parameter <value_type, page_size, number_of_pages, block_size, alloc_strategy, paging_strategy>
    using vector_type = stxxl::vector<data_type, 4, stxxl::lru_pager<8>, 1*1024*1024, foxxll::random_cyclic>;
    vector_type my_vector;

    unsigned int counter = 0;
    uint64_t number_of_elements = 32 * 1024 * 1024;

    std::mt19937_64 randgen;
    std::uniform_int_distribution<data_type> distr;

    // fill vector with random integers
    for (uint64_t i = 0; i < number_of_elements; ++i)
        my_vector.push_back(distr(randgen));

    // construct iterator
    auto iter = my_vector.cbegin();

    // use iterator to advance my_vector and calculate number of even elements
    for (uint64_t j = 0; j < my_vector.size(); j++, iter++)
        counter += (*iter % 2 == 0);  // is my_vector's current element even?

    LOG1 << "found " << counter << " even numbers in V";
    return 0;
}
