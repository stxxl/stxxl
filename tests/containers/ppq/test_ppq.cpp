/***************************************************************************
 *  tests/containers/ppq/test_ppq.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2005 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#include <iostream>
#include <limits>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/containers/parallel_priority_queue.h>
#include <stxxl/random>
#include <stxxl/timer>

#include <key_with_padding.h>

using foxxll::scoped_print_timer;

using KeyType = int32_t;
constexpr size_t RecordSize = 128;
using my_type = key_with_padding<KeyType, RecordSize>;
using my_cmp = my_type::compare_greater;

using ppq_type = stxxl::parallel_priority_queue<
          my_type, my_cmp,
          foxxll::default_alloc_strategy,
          STXXL_DEFAULT_BLOCK_SIZE(my_type), /* BlockSize */
          512* 64L* 1024L                    /* RamSize */
          >;

void test_simple()
{
    ppq_type ppq(my_cmp(), 128L * 1024L * 1024L, 4);

    const uint64_t volume = 128L * 1024 * 1024;

    uint64_t nelements = volume / sizeof(my_type);

    {
        scoped_print_timer timer("Filling PPQ",
                                 nelements * sizeof(my_type));

        for (uint64_t i = 0; i < nelements; i++)
        {
            if ((i % (128 * 1024)) == 0)
                LOG1 << "Inserting element " << i;

            ppq.push(my_type(int(nelements - i)));
        }
        LOG1 << "Max elements: " << nelements;
    }

    die_unless(ppq.size() == nelements);

    {
        scoped_print_timer timer("Emptying PPQ",
                                 nelements * sizeof(my_type));

        for (uint64_t i = 0; i < nelements; ++i)
        {
            die_unless(!ppq.empty());
            //LOG1 <<  ppq.top() ;
            die_unequal(ppq.top().key, int(i + 1));

            ppq.pop();
            if ((i % (128 * 1024)) == 0)
                LOG1 << "Element " << i << " popped";
        }
    }

    die_unequal(ppq.size(), 0u);
    die_unless(ppq.empty());
}

void test_bulk_pop()
{
    ppq_type ppq(my_cmp(), 128L * 1024L * 1024L, 4);

    const uint64_t volume = 128L * 1024L * 1024L;
    const size_t bulk_size = 1024;

    uint64_t nelements = volume / sizeof(my_type);

    LOG1 << "Running bulk_pop() test. bulk_size = " << bulk_size;

    {
        scoped_print_timer timer("Filling PPQ",
                                 nelements * sizeof(my_type));

        for (uint64_t i = 0; i < nelements; i++)
        {
            if ((i % (256 * 1024)) == 0)
                LOG1 << "Inserting element " << i;

            ppq.push(my_type(int(nelements - i)));
        }
        LOG1 << "Max elements: " << nelements;
    }

    die_unless(ppq.size() == nelements);

    {
        scoped_print_timer timer("Emptying PPQ",
                                 nelements * sizeof(my_type));

        for (uint64_t i = 0; i < nelements; )
        {
            die_unless(!ppq.empty());

            std::vector<my_type> out;
            ppq.bulk_pop(out, bulk_size);

            die_unless(!out.empty());

            for (size_t j = 0; j < out.size(); ++j) {
                //LOG1 <<  ppq.top() ;
                if ((i % (128 * 1024)) == 0)
                    LOG1 << "Element " << i << " popped";
                ++i;
                die_unequal(out[j].key, int(i));
            }
        }
    }

    die_unequal(ppq.size(), 0u);
    die_unless(ppq.empty());
}

void test_bulk_limit(const size_t bulk_size)
{
    ppq_type ppq(my_cmp(), 256L * 1024L * 1024L, 4);

    LOG1 << "Running test_bulk_limit(" << bulk_size << ")";

    int windex = 0;     // continuous insertion index
    int rindex = 0;     // continuous pop index
    stxxl::random_number32_r prng;

    const size_t repeats = 8;

    // first insert 2 * bulk_size items (which are put into insertion heaps)
    // TODO: fix simple push() interface!?

    ppq.bulk_push_begin(4L * bulk_size);
    for (size_t i = 0; i < 4L * bulk_size; ++i)
        ppq.bulk_push(my_type(windex++));
    ppq.bulk_push_end();

    // extract bulk_size items, and reinsert them with higher indexes
    for (size_t r = 0; r < repeats; ++r)
    {
        size_t this_bulk_size = bulk_size / 2 + prng() % bulk_size;

        if (0) // simple procedure
        {
            for (size_t i = 0; i < this_bulk_size; ++i)
            {
                my_type top = ppq.top();
                ppq.pop();
                die_unequal(top.key, rindex);
                ++rindex;

                ppq.push(my_type(windex++));
            }
        }
        else if (0) // bulk-limit procedure
        {
            ppq.limit_begin(my_type(windex), bulk_size);

            for (size_t i = 0; i < this_bulk_size; ++i)
            {
                my_type top = ppq.limit_top();
                ppq.limit_pop();
                die_unequal(top.key, rindex);
                ++rindex;

                ppq.limit_push(my_type(windex++));
            }

            ppq.limit_end();
        }
        else
        {
            std::vector<my_type> work;
            ppq.bulk_pop_limit(work, my_type(windex));

            ppq.bulk_push_begin(this_bulk_size);

            // not parallel!
            for (size_t i = 0; i < work.size(); ++i)
            {
                die_unequal(work[i].key, rindex);
                ++rindex;
                ppq.bulk_push(my_type(windex++));
            }

            ppq.bulk_push_end();
        }
    }

    die_unequal(ppq.size(), static_cast<size_t>(windex - rindex));

    // extract last items
    for (size_t i = 0; i < 4L * bulk_size; ++i)
    {
        my_type top = ppq.top();
        ppq.pop();
        die_unequal(top.key, rindex);
        ++rindex;
    }

    die_unless(ppq.empty());
}

int main()
{
    foxxll::stats* stats = foxxll::stats::get_instance();
    foxxll::stats_data stats_begin(*stats);
    test_simple();
    std::cout << "Stats after test_simple :" << (foxxll::stats_data(*stats) - stats_begin);
    stats_begin = *foxxll::stats::get_instance();
    test_bulk_pop();
    std::cout << "Stats after bulk_pop :" << (foxxll::stats_data(*stats) - stats_begin);
    stats_begin = *foxxll::stats::get_instance();
    test_bulk_limit(1000);
    std::cout << "Stats after bulk_limit_1000 :" << (foxxll::stats_data(*stats) - stats_begin);
    stats_begin = *foxxll::stats::get_instance();
    test_bulk_limit(1024 * 1024);
    std::cout << "Stats after bulk_limit_1M: " << (foxxll::stats_data(*stats) - stats_begin);
    return 0;
}
