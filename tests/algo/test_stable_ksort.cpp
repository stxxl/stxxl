/***************************************************************************
 *  tests/algo/test_stable_ksort.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003, 2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

//! \example algo/test_stable_ksort.cpp
//! This is an example of how to use \c stxxl::ksort() algorithm

#include <vector>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/mng.hpp>

#include <stxxl/bits/defines.h>
#include <stxxl/stable_ksort>
#include <stxxl/vector>

#include <key_with_padding.h>

using my_type = key_with_padding<unsigned, 128>;

int main()
{
#if STXXL_PARALLEL_MULTIWAY_MERGE
    LOG1 << "STXXL_PARALLEL_MULTIWAY_MERGE";
#endif
    unsigned memory_to_use = 22 * STXXL_DEFAULT_BLOCK_SIZE(my_type);
    using vector_type = stxxl::vector<my_type>;
    const uint64_t n_records = 2 * 16 * uint64_t(STXXL_DEFAULT_BLOCK_SIZE(uint64_t)) / sizeof(my_type);
    vector_type v(n_records);

    stxxl::random_number32 rnd;
    LOG1 << "Filling vector... " << rnd() << " " << rnd() << " " << rnd();
    for (vector_type::size_type i = 0; i < v.size(); i++)
        v[i].key = (rnd() / 2) * 2;

    LOG1 << "Checking order...";
    die_unless(!stxxl::is_sorted(v.cbegin(), v.cend()));

    LOG1 << "Sorting...";
    stxxl::stable_ksort(v.begin(), v.end(), my_type::key_extract(), memory_to_use);

    LOG1 << "Checking order...";
    die_unless(stxxl::is_sorted(v.cbegin(), v.cend()));

    return 0;
}
