/***************************************************************************
 *  tests/containers/test_pqueue.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

//! \example containers/test_pqueue.cpp
//! This is an example of how to use \c stxxl::PRIORITY_QUEUE_GENERATOR
//! and \c stxxl::priority_queue

#include <iostream>
#include <limits>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/priority_queue>
#include <stxxl/timer>

#include <key_with_padding.h>

using foxxll::scoped_print_timer;

constexpr size_t volume = 128 * 1024; // in KiB

using KeyType = int;
constexpr size_t RecordSize = 128;
using my_type = key_with_padding<KeyType, RecordSize>;

// Here we test that the PQ can deal with not default constructable comparators;
// the function is provided by our default comparator infrastructure
struct comp_without_def_construct : public my_type::compare_greater {
    comp_without_def_construct() = delete;
    explicit comp_without_def_construct(int) { }
};

// forced instantiation
template class stxxl::PRIORITY_QUEUE_GENERATOR<
        my_type, comp_without_def_construct, 32* 1024* 1024, volume / sizeof(my_type)
        >;

int main()
{
    using gen = stxxl::PRIORITY_QUEUE_GENERATOR<
              my_type, comp_without_def_construct, 700* 1024, volume / sizeof(my_type)
              >;
    using pq_type = gen::result;
    using block_type = pq_type::block_type;

    LOG1 << "Block size: " << size_t(block_type::raw_size);
    LOG1 << "AI: " << gen::AI;
    LOG1 << "X : " << gen::X;
    LOG1 << "N : " << gen::N;
    LOG1 << "AE: " << gen::AE;

    const unsigned mem_for_pools = 128 * 1024 * 1024;
    foxxll::read_write_pool<block_type> pool(
        (mem_for_pools / 2) / block_type::raw_size,
        (mem_for_pools / 2) / block_type::raw_size);
    pq_type p(pool, comp_without_def_construct { 1 });

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());

    size_t nelements = 1024 * volume / sizeof(my_type);
    LOG1 << "Internal memory consumption of the priority queue: " << p.mem_cons() << " B";
    LOG1 << "Max elements: " << nelements;

    {
        scoped_print_timer timer("Filling PQ",
                                 nelements * sizeof(my_type));

        for (size_t i = 0; i < nelements; i++)
        {
            if ((i % (1024 * 1024)) == 0)
                LOG1 << "Inserting element " << i;
            p.push(my_type(int(nelements - i)));
        }
    }

    die_unless(p.size() == (size_t)nelements);

#if 0
    // test swap
    pq_type p1(pool);
    std::swap(p, p1);
    std::swap(p, p1);
#endif

    LOG1 << "Internal memory consumption of the priority queue: " << p.mem_cons() << " B";

    {
        scoped_print_timer timer("Emptying PPQ",
                                 nelements * sizeof(my_type));

        for (size_t i = 0; i < nelements; ++i)
        {
            die_unless(!p.empty());
            //LOG1 <<  p.top() ;
            die_unless(p.top().key == int(i + 1));
            p.pop();
            if ((i % (1024 * 1024)) == 0)
                LOG1 << "Element " << i << " popped";
        }
    }

    die_unless(p.size() == 0);
    die_unless(p.empty());

    LOG1 << "Internal memory consumption of the priority queue: " << p.mem_cons() << " B";

    std::cout << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    return 0;
}
