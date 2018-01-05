/***************************************************************************
 *  tests/containers/test_sorter.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2012 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096
//! \example containers/test_sorter.cpp
//! This is an example of how to use \c stxxl::sorter() container

#include <limits>
#include <stxxl/sorter>

#include <key_with_padding.h>

using KeyType = unsigned;
constexpr size_t RecordSize = 16;
using my_type = key_with_padding<KeyType, RecordSize>;
using Comparator = my_type::compare_less;

// forced instantiation
template class stxxl::sorter<my_type, Comparator>;

int main()
{
#if STXXL_PARALLEL_MULTIWAY_MERGE
    STXXL_MSG("STXXL_PARALLEL_MULTIWAY_MERGE");
#endif
    unsigned memory_to_use = 64 * STXXL_DEFAULT_BLOCK_SIZE(my_type);
    enum { block_size = STXXL_DEFAULT_BLOCK_SIZE(my_type) };

    // comparator object used for sorters
    Comparator cmp;

    // construct sorter type: stxxl::sorter<ValueType, ComparatorType, BlockSize>
    using sorter_type = stxxl::sorter<my_type, Comparator, block_size>;

    {
        // test small number of items that can be sorted internally

        sorter_type s(cmp, memory_to_use);

        // put in some items
        s.push(my_type(42));
        s.push(my_type(0));
        s.push(my_type(23));

        // finish input, switch to sorting stage.
        s.sort();

        STXXL_CHECK(*s == my_type(0));
        ++s;
        STXXL_CHECK(*s == my_type(23));
        ++s;
        STXXL_CHECK(*s == my_type(42));
        ++s;
        STXXL_CHECK(s.empty());
    }

    {
        // large test with 192 * 4 KiB items

        const uint64_t n_records = uint64_t(192) * STXXL_DEFAULT_BLOCK_SIZE(my_type) / sizeof(my_type);

        sorter_type s(cmp, memory_to_use);

        stxxl::random_number32 rnd;

        STXXL_MSG("Filling sorter..., input size = " << n_records << " elements (" << ((n_records * sizeof(my_type)) >> 10) << " KiB)");

        for (uint64_t i = 0; i < n_records; i++)
        {
            STXXL_CHECK(s.size() == i);

            s.push(my_type(1 + (rnd() % 0xfffffff)));
        }

        // finish input, switch to sorting stage.
        s.sort();

        STXXL_MSG("Checking order...");

        STXXL_CHECK(!s.empty());
        STXXL_CHECK(s.size() == n_records);

        my_type prev = *s;      // get first item
        ++s;

        uint64_t count = n_records - 1;

        while (!s.empty())
        {
            STXXL_CHECK(s.size() == count);

            if (!(prev <= *s)) STXXL_MSG("WRONG");
            STXXL_CHECK(prev <= *s);

            ++s;
            --count;
        }
        STXXL_MSG("OK");

        // rewind and read output again
        s.rewind();

        STXXL_MSG("Checking order again...");

        STXXL_CHECK(!s.empty());
        STXXL_CHECK(s.size() == n_records);

        prev = *s;      // get first item
        ++s;

        while (!s.empty())
        {
            if (!(prev <= *s)) STXXL_MSG("WRONG");
            STXXL_CHECK(prev <= *s);

            ++s;
        }
        STXXL_MSG("OK");

        STXXL_CHECK(s.size() == 0);

        STXXL_MSG("Done");
    }

    return 0;
}
// vim: et:ts=4:sw=4
