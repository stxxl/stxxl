/***************************************************************************
 *  tests/algo/test_asch.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/algo/async_schedule.h>
#include <stxxl/bits/verbose.h>

#include <cstdlib>
#include <random>

// Test async schedule algorithm

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        STXXL_ERRMSG("Usage: " << argv[0] << " D L m seed");
        return -1;
    }
    const size_t D = strtoul(argv[1], NULL, 0);
    const size_t L = strtoul(argv[2], NULL, 0);
    const size_t m = atoi(argv[3]);
    uint32_t seed = atoi(argv[4]);
    size_t* disks = new size_t[L];
    size_t* prefetch_order = new size_t[L];
    size_t* count = new size_t[D];

    for (size_t i = 0; i < D; i++)
        count[i] = 0;

    std::default_random_engine rnd(seed);
    for (size_t i = 0; i < L; i++)
    {
        disks[i] = rnd() % D;
        count[disks[i]]++;
    }

    for (size_t i = 0; i < D; i++)
        std::cout << "Disk " << i << " has " << count[i] << " blocks" << std::endl;

    stxxl::compute_prefetch_schedule(disks, disks + L, prefetch_order, m, D);

    STXXL_MSG("Prefetch order:");
    for (size_t i = 0; i < L; ++i) {
        STXXL_MSG("request " << prefetch_order[i] << "  on disk " << disks[prefetch_order[i]]);
    }
    STXXL_MSG("Request order:");
    for (size_t i = 0; i < L; ++i) {
        size_t j;
        for (j = 0; prefetch_order[j] != i; ++j) ;
        STXXL_MSG("request " << i << "  on disk " << disks[i] << "  scheduled as " << j);
    }

    delete[] count;
    delete[] disks;
    delete[] prefetch_order;
}

// vim: et:ts=4:sw=4
