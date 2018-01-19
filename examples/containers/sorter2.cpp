/***************************************************************************
 *  examples/containers/sorter2.cpp
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
#include <limits>
#include <random>
#include <utility>

#include <tlx/logger.hpp>

#include <foxxll/common/timer.hpp>

#include <stxxl/bits/common/comparator.h>
#include <stxxl/sorter>

using data_type = int;
using two_integer_t = std::pair<data_type, data_type>;
using comparator_t = stxxl::comparator<two_integer_t>;

int main()
{
    // template parameter <ValueType, CompareType, BlockSize(optional), AllocStr(optional)>
    using sorter_type = stxxl::sorter<two_integer_t, comparator_t, 1*1024*1024>;

    // create sorter object (CompareType(), MainMemoryLimit)
    sorter_type int_sorter(comparator_t(), 64 * 1024 * 1024);

    std::mt19937 randgen;
    std::uniform_int_distribution<data_type> distr(0, 99999);

    {
        const size_t elements_to_push = 1000;
        foxxll::scoped_print_timer timer("Push", elements_to_push * sizeof(two_integer_t));

        // insert random numbers from [0,100000)
        for (size_t i = 0; i < elements_to_push; ++i)
            int_sorter.push({ distr(randgen), static_cast<data_type>(i) });
    }

    {
        foxxll::scoped_print_timer timer("Push", int_sorter.size() * sizeof(two_integer_t));
        int_sorter.sort();  // switch to output state and sort
    }

    // echo sorted elements
    for ( ; !int_sorter.empty(); ++int_sorter)
    {
        std::cout << int_sorter->second << " ";
    }

    return 0;
}
