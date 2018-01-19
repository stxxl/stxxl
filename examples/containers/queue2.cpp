/***************************************************************************
 *  examples/containers/queue2.cpp
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

#include <stxxl/queue>

using data_type = unsigned int;

int main()
{
    // template parameter <value_type, block_size, allocation_strategy, size_type>
    using a_queue = stxxl::queue<data_type>;

    // construct queue with default parameters
    a_queue my_queue;

    std::mt19937 randgen;
    std::uniform_int_distribution<data_type> distr_value;

    const size_t number_of_elements = 64 * 1024 * 1024;

    // push random values in the queue
    for (size_t i = 0; i < number_of_elements; i++)
        my_queue.push(distr_value(randgen));

    const data_type last_inserted = my_queue.back();
    LOG1 << "last element inserted: " << last_inserted;

    // identify smaller element than first_inserted, search in growth-direction (front->back)
    for ( ; !my_queue.empty(); my_queue.pop())
    {
        if (last_inserted > my_queue.front())
        {
            LOG1 << "found smaller element: " << my_queue.front() << " than last inserted element";
            break;
        }

        std::cout << my_queue.front() << " " << std::endl;
    }

    return 0;
}
