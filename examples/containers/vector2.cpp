/***************************************************************************
 *  examples/containers/vector2.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Daniel Feist <daniel.feist@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>

#include <tlx/logger.hpp>

#include <stxxl/vector>

int main()
{
    // template parameter <value_type, page_size, number_of_pages, block_size, alloc_strategy, paging_strategy>
    using vector_type = stxxl::vector<
              unsigned int, 4, stxxl::lru_pager<8>, 1*1024*1024, foxxll::random_cyclic>;

    vector_type my_vector;
    unsigned int counter = 0;
    unsigned int tmp;
    stxxl::random_number<> rand;
    uint64_t number_of_elements = 32 * 1024 * 1024;

    // fill vector with random integers
    for (uint64_t i = 0; i < number_of_elements; ++i)
    {
        tmp = rand(123456789);  // generate random number from the interval [0,123456789)
        my_vector.push_back(tmp);
    }

    // construct iterator
    vector_type::const_iterator iter = my_vector.begin();

    // use iterator to advance my_vector and calculate number of even elements
    for (uint64_t j = 0; j < my_vector.size(); j++)
    {
        //std::cout << *iter << " ";
        if (*iter % 2 == 0)  // is my_vector's current element even?
        {
            ++counter;
        }
        iter++;
    }

    LOG1 << "found " << counter << " even numbers in V";
    return 0;
}
