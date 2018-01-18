/***************************************************************************
 *  tests/algo/test_sort.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

//! \example algo/test_sort.cpp
//! This is an example of how to use \c stxxl::sort() algorithm

#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/mng.hpp>

#include <stxxl/bits/common/padding.h>
#include <stxxl/bits/defines.h>
#include <stxxl/sort>
#include <stxxl/vector>

#include <key_with_padding.h>

constexpr size_t RECORD_SIZE = 8;
using KeyType = unsigned;
using my_type = key_with_padding<KeyType, RECORD_SIZE>;
using cmp = my_type::compare_less;

int main()
{
#if STXXL_PARALLEL_MULTIWAY_MERGE
    LOG1 << "STXXL_PARALLEL_MULTIWAY_MERGE";
#endif
    unsigned memory_to_use = 64 * STXXL_DEFAULT_BLOCK_SIZE(my_type);
    using vector_type = stxxl::vector<my_type>;

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());

    {
        // test small vector that can be sorted internally
        vector_type v(3);
        v[0] = my_type(42);
        v[1] = my_type(0);
        v[2] = my_type(23);
        LOG1 << "small vector unsorted " << v[0] << " " << v[1] << " " << v[2];
        //stxxl::sort(v.begin(), v.end(), cmp(), memory_to_use);
        stxxl::stl_in_memory_sort(v.begin(), v.end(), cmp());
        LOG1 << "small vector sorted   " << v[0] << " " << v[1] << " " << v[2];
        die_unless(stxxl::is_sorted(v.cbegin(), v.cend(), cmp()));
    }

    const uint64_t n_records = uint64_t(192) * uint64_t(STXXL_DEFAULT_BLOCK_SIZE(my_type)) / sizeof(my_type);
    vector_type v(n_records);

    stxxl::random_number32 rnd;
    LOG1 << "Filling vector..., input size = " << v.size() << " elements (" << ((v.size() * sizeof(my_type)) >> 20) << " MiB)";
    for (vector_type::size_type i = 0; i < v.size(); i++)
        v[i].key = 1 + (rnd() % 0xfffffff);

    LOG1 << "Checking order...";
    die_unless(!stxxl::is_sorted(v.cbegin(), v.cend(), cmp()));

    LOG1 << "Sorting (using " << (memory_to_use >> 20) << " MiB of memory)...";
    stxxl::sort(v.begin(), v.end(), cmp(), memory_to_use);

    LOG1 << "Checking order...";
    die_unless(stxxl::is_sorted(v.cbegin(), v.cend(), cmp()));

    LOG1 << "Done, output size=" << v.size();

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    return 0;
}

// vim: et:ts=4:sw=4
