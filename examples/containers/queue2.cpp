/***************************************************************************
 *  examples/containers/queue2.cpp
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

#include <stxxl/queue>
#include <stxxl/random>

int main()
{
    // template parameter <value_type, block_size, allocation_strategy, size_type>
    using a_queue = stxxl::queue<unsigned int>;

    // construct queue with default parameters
    a_queue my_queue;

    unsigned int random;
    stxxl::random_number32 rand32;  // define random number generator
    uint64_t number_of_elements = 64 * 1024 * 1024;

    // push random values in the queue
    for (uint64_t i = 0; i < number_of_elements; i++)
    {
        random = rand32();  // generate random integers from interval [0,2^32)
        my_queue.push(random);
    }

    unsigned int last_inserted = my_queue.back();
    LOG1 << "last element inserted: " << last_inserted;

    // identify smaller element than first_inserted, search in growth-direction (front->back)
    while (!my_queue.empty())
    {
        if (last_inserted > my_queue.front())
        {
            LOG1 << "found smaller element: " << my_queue.front() << " than last inserted element";
            break;
        }
        std::cout << my_queue.front() << " " << std::endl;
        my_queue.pop();
    }

    return 0;
}
