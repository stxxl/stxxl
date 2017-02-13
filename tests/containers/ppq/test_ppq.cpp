/***************************************************************************
 *  tests/containers/ppq/test_ppq.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
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
#include <limits>
#include <stxxl/bits/containers/parallel_priority_queue.h>
#include <stxxl/random>
#include <stxxl/timer>

using stxxl::uint64;
using stxxl::uint64;
using stxxl::scoped_print_timer;

#define RECORD_SIZE 128

struct my_type
{
    typedef int key_type;
    key_type key;
    char data[RECORD_SIZE - sizeof(key_type)];
    my_type() { }
    explicit my_type(key_type k) : key(k)
    {
#if STXXL_WITH_VALGRIND
        memset(data, 0, sizeof(data));
#endif
    }

    friend std::ostream& operator << (std::ostream& o, const my_type& obj)
    {
        return o << obj.key;
    }
};

struct my_cmp : std::binary_function<my_type, my_type, bool> // greater
{
    bool operator () (const my_type& a, const my_type& b) const
    {
        return a.key > b.key;
    }

    my_type min_value() const
    {
        return my_type(std::numeric_limits<my_type::key_type>::max());
    }
};

// forced instantiation
template class stxxl::parallel_priority_queue<
        my_type, my_cmp,
        STXXL_DEFAULT_ALLOC_STRATEGY,
        STXXL_DEFAULT_BLOCK_SIZE(ValueType), /* BlockSize */
        1* 64L * 1024L* 1024L,              /* RamSize */
        0                                    /* MaxItems */
        >;

typedef stxxl::parallel_priority_queue<
        my_type, my_cmp,
        STXXL_DEFAULT_ALLOC_STRATEGY,
        STXXL_DEFAULT_BLOCK_SIZE(ValueType), /* BlockSize */
        512* 64L* 1024L                    /* RamSize */
        > ppq_type;

void test_simple()
{
    ppq_type ppq(my_cmp(), 128L * 1024L * 1024L, 4);

    const uint64 volume = 128L * 1024 * 1024;

    uint64 nelements = volume / sizeof(my_type);

    {
        scoped_print_timer timer("Filling PPQ",
                                 nelements * sizeof(my_type));

        for (uint64 i = 0; i < nelements; i++)
        {
            if ((i % (128 * 1024)) == 0)
                STXXL_MSG("Inserting element " << i);

            ppq.push(my_type(int(nelements - i)));
        }
        STXXL_MSG("Max elements: " << nelements);
    }

    STXXL_CHECK(ppq.size() == nelements);

    {
        scoped_print_timer timer("Emptying PPQ",
                                 nelements * sizeof(my_type));

        for (uint64 i = 0; i < nelements; ++i)
        {
            STXXL_CHECK(!ppq.empty());
            //STXXL_MSG( ppq.top() );
            STXXL_CHECK_EQUAL(ppq.top().key, int(i + 1));

            ppq.pop();
            if ((i % (128 * 1024)) == 0)
                STXXL_MSG("Element " << i << " popped");
        }
    }

    STXXL_CHECK_EQUAL(ppq.size(), 0);
    STXXL_CHECK(ppq.empty());
}

void test_bulk_pop()
{
    ppq_type ppq(my_cmp(), 128L * 1024L * 1024L, 4);

    const uint64 volume = 128L * 1024L * 1024L;
    const uint64 bulk_size = 1024;

    uint64 nelements = volume / sizeof(my_type);

    STXXL_MSG("Running bulk_pop() test. bulk_size = " << bulk_size);

    {
        scoped_print_timer timer("Filling PPQ",
                                 nelements * sizeof(my_type));

        for (uint64 i = 0; i < nelements; i++)
        {
            if ((i % (256 * 1024)) == 0)
                STXXL_MSG("Inserting element " << i);

            ppq.push(my_type(int(nelements - i)));
        }
        STXXL_MSG("Max elements: " << nelements);
    }

    STXXL_CHECK(ppq.size() == nelements);

    {
        scoped_print_timer timer("Emptying PPQ",
                                 nelements * sizeof(my_type));

        for (uint64 i = 0; i < nelements; )
        {
            STXXL_CHECK(!ppq.empty());

            std::vector<my_type> out;
            ppq.bulk_pop(out, bulk_size);

            STXXL_CHECK(!out.empty());

            for (uint64 j = 0; j < out.size(); ++j) {
                //STXXL_MSG( ppq.top() );
                if ((i % (128 * 1024)) == 0)
                    STXXL_MSG("Element " << i << " popped");
                ++i;
                STXXL_CHECK_EQUAL(out[j].key, int(i));
            }
        }
    }

    STXXL_CHECK_EQUAL(ppq.size(), 0);
    STXXL_CHECK(ppq.empty());
}

void test_bulk_limit(const size_t bulk_size)
{
    ppq_type ppq(my_cmp(), 256L * 1024L * 1024L, 4);

    STXXL_MSG("Running test_bulk_limit(" << bulk_size << ")");

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
                STXXL_CHECK_EQUAL(top.key, rindex);
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
                STXXL_CHECK_EQUAL(top.key, rindex);
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
                STXXL_CHECK_EQUAL(work[i].key, rindex);
                ++rindex;
                ppq.bulk_push(my_type(windex++));
            }

            ppq.bulk_push_end();
        }
    }

    STXXL_CHECK_EQUAL(ppq.size(), static_cast<size_t>(windex - rindex));

    // extract last items
    for (size_t i = 0; i < 4L * bulk_size; ++i)
    {
        my_type top = ppq.top();
        ppq.pop();
        STXXL_CHECK_EQUAL(top.key, rindex);
        ++rindex;
    }

    STXXL_CHECK(ppq.empty());
}

int main()
{
    stxxl::stats * stats = stxxl::stats::get_instance();
    stxxl::stats_data stats_begin(*stats);
    test_simple();
    std::cout << "Stats after test_simple :" << (stxxl::stats_data(*stats) - stats_begin);
    stats_begin = *stxxl::stats::get_instance();
    test_bulk_pop();
    std::cout << "Stats after bulk_pop :" << (stxxl::stats_data(*stats) - stats_begin);
    stats_begin = *stxxl::stats::get_instance();
    test_bulk_limit(1000);
    std::cout << "Stats after bulk_limit_1000 :" << (stxxl::stats_data(*stats) - stats_begin);
    stats_begin = *stxxl::stats::get_instance();
    test_bulk_limit(1024 * 1024);
    std::cout << "Stats after bulk_limit_1M: " << (stxxl::stats_data(*stats) - stats_begin);
    return 0;
}
