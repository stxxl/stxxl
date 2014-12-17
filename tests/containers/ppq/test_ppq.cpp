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

#include <limits>
#include <stxxl/bits/containers/parallel_priority_queue.h>
#include <stxxl/timer>

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
    1* 1024L* 1024L* 1024L,              /* RamSize */
    0                                    /* MaxItems */
    >;

typedef stxxl::parallel_priority_queue<
        my_type, my_cmp,
        STXXL_DEFAULT_ALLOC_STRATEGY,
        STXXL_DEFAULT_BLOCK_SIZE(ValueType), /* BlockSize */
        256* 1024L* 1024L                    /* RamSize */
        > ppq_type;

int main()
{
    ppq_type ppq;

    const uint64 volume = 256 * 1024 * 1024;

    uint64 nelements = volume / sizeof(my_type);

    {
        scoped_print_timer timer("Filling PPQ",
                                 nelements * sizeof(my_type));

        for (uint64 i = 0; i < nelements; i++)
        {
            if ((i % (1024 * 1024)) == 0)
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
            STXXL_CHECK_EQUAL(ppq.top().key,int(i + 1));

            ppq.pop();
            if ((i % (1024 * 1024)) == 0)
                STXXL_MSG("Element " << i << " popped");
        }
    }

    STXXL_CHECK_EQUAL(ppq.size(),0);
    STXXL_CHECK(ppq.empty());

    return 0;
}
