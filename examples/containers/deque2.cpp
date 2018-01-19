/***************************************************************************
 *  examples/containers/deque2.cpp
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
#include <random>

#include <tlx/logger.hpp>

#include <stxxl/deque>

using data_type = unsigned int;

int main()
{
    stxxl::deque<data_type> my_deque;

    const size_t number_of_elements = 8 * 1024 * 1024;

    std::mt19937 randgen;
    std::uniform_int_distribution<data_type> distr_value;
    std::uniform_int_distribution<size_t> distr_index(0, number_of_elements - 1);

    // fill deque with random integer values
    for (uint64_t i = 0; i < number_of_elements; i++)
        my_deque.push_front(distr_value(randgen));

    auto deque_iterator = my_deque.begin();

    // Access random element x at position p(x) in the deque
    const auto rand_idx = distr_index(randgen);
    const auto pivot = my_deque[rand_idx];

    // Count number of smaller elements from the front to p(x) - 1
    size_t smaller_left = 0;
    for (unsigned int j = 0; j < rand_idx; ++j, ++deque_iterator)
        smaller_left += (*deque_iterator < pivot);

    ++deque_iterator;

    // Count number of smaller elements from p(x) + 1 to the end
    size_t smaller_right = 0;
    for (uint64_t k = rand_idx + 1; k < number_of_elements - 1; ++k, ++deque_iterator)
        smaller_right += (*deque_iterator < pivot);

    LOG1 << "smaller left: " << smaller_left << ", smaller right: " << smaller_right;
    return 0;
}
