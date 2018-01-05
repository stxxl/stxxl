/***************************************************************************
 *  tests/algo/test_ksort.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

//! \example algo/test_ksort.cpp
//! This is an example of how to use \c stxxl::ksort() algorithm

#include <foxxll/mng.hpp>
#include <stxxl/ksort>
#include <stxxl/vector>

#include <key_with_padding.h>

using KeyType = uint64_t;
constexpr size_t RecordSize = 2*sizeof(KeyType) + 0;
using my_type = key_with_padding<KeyType, RecordSize, true>;
using get_key = my_type::key_extract;

int main()
{
    STXXL_MSG("Check config...");
    try {
        foxxll::block_manager::get_instance();
    }
    catch (std::exception& e)
    {
        STXXL_MSG("Exception: " << e.what());
        abort();
    }
#if STXXL_PARALLEL_MULTIWAY_MERGE
    STXXL_MSG("STXXL_PARALLEL_MULTIWAY_MERGE");
#endif
    unsigned memory_to_use = 16 * STXXL_DEFAULT_BLOCK_SIZE(my_type);
    using vector_type = stxxl::vector<my_type, 4, stxxl::lru_pager<4> >;
    const uint64_t n_records = 3 * 16 * uint64_t(STXXL_DEFAULT_BLOCK_SIZE(my_type)) / sizeof(my_type);
    vector_type v(n_records);

    stxxl::random_number32 rnd;
    STXXL_MSG("Filling vector... ");
    for (vector_type::size_type i = 0; i < v.size(); i++)
    {
        v[i].key = rnd() + 1;
        v[i].key_copy = v[i].key;
    }

    STXXL_MSG("Checking order...");
    STXXL_CHECK(!stxxl::is_sorted(v.cbegin(), v.cend()));

    STXXL_MSG("Sorting...");
    stxxl::ksort(v.begin(), v.end(), get_key(), memory_to_use);

    STXXL_MSG("Checking order...");
    STXXL_CHECK(stxxl::is_sorted(v.cbegin(), v.cend()));
    STXXL_MSG("Checking content...");
    my_type prev;
    for (vector_type::size_type i = 0; i < v.size(); i++)
    {
        if (v[i].key != v[i].key_copy)
        {
            STXXL_MSG("Bug at position " << i);
            abort();
        }
        if (i > 0 && prev.key == v[i].key)
        {
            STXXL_MSG("Duplicate at position " << i << " key=" << v[i].key);
            //abort();
        }
        prev = v[i];
    }
    STXXL_MSG("OK");

    return 0;
}
