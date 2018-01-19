/***************************************************************************
 *  examples/containers/stack2.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Daniel Feist <daniel.feist@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <random>
#include <stack>

#include <tlx/logger.hpp>

#include <foxxll/common/timer.hpp>

#include <stxxl/stack>

using data_type = uint64_t;

int main()
{
    // template parameter <data_type, externality, behaviour, blocks_per_page, block_size,
    // internal_stack_type, migrating_critical_size, allocation_strategy, size_type>
    using simple_stack = stxxl::STACK_GENERATOR<data_type, stxxl::external, stxxl::grow_shrink>::result;

    // create stack instance
    simple_stack a_stack;

    std::mt19937_64 randgen;
    std::uniform_int_distribution<data_type> distr;
    uint64_t number_of_elements = 1024 * 1024;

    // routine: 1) push random values on stack and 2) pop all except the lowest value and start again
    for (int k = 0; k < 5; k++) {
        {
            foxxll::scoped_print_timer("Push", number_of_elements * sizeof(data_type));
            for (uint64_t i = 0; i < number_of_elements; i++)
                a_stack.push(distr(randgen));
        }

        {
            foxxll::scoped_print_timer("Push", (number_of_elements - 1) * sizeof(data_type));
            for (uint64_t j = 0; j < number_of_elements - 1; j++)
                a_stack.pop();
        }
    }

    return 0;
}
