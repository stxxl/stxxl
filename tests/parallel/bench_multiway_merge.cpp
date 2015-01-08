/***************************************************************************
 *  tests/parallel/bench_multiway_merge.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <vector>
#include <utility>
#include <omp.h>
#include <cstdlib>
#include <stxxl/bits/common/timer.h>
#include <stxxl/bits/common/rand.h>
#include <stxxl/bits/common/is_sorted.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/bits/parallel/multiway_merge.h>

typedef uint64_t value_type;

// we allocate a list of blocks, each block being a sequence of items.
static const size_t value_size = sizeof(value_type);
static const size_t block_bytes = 2 * 1024 * 1024;
static const size_t block_size = block_bytes / value_size;

template <int Method>
void test_multiway_merge(unsigned int seqnum)
{
    typedef std::vector<value_type> sequence_type;

    size_t total_size = seqnum * block_size;
    size_t total_bytes = seqnum * block_bytes;

    std::less<value_type> cmp;

    std::vector<sequence_type> blocks(seqnum);

    {
        stxxl::scoped_print_timer spt("Filling blocks with random numbers in parallel", total_bytes);

        for (size_t i = 0; i < seqnum; ++i)
            blocks[i].resize(block_size);

#pragma omp parallel
        {
            unsigned int seed = 1234 * omp_get_thread_num();
            stxxl::random_number32_r rnd(seed);
#pragma omp for
            for (size_t i = 0; i < seqnum; ++i)
            {
                for (size_t j = 0; j < block_size; ++j)
                    blocks[i][j] = rnd();

                std::sort(blocks[i].begin(), blocks[i].end(), cmp);
            }
        }
    }

    std::vector<value_type> out;

    {
        stxxl::scoped_print_timer spt("Allocating output buffer", total_bytes);
        out.resize(total_size);
    }

    const char* method_name =
        Method == 0 ? "seq_mwm" :
        Method == 1 ? "seq_gnu_mwm" :
        Method == 2 ? "para_mwm" :
        Method == 3 ? "para_gnu_mwm" :
        "???";

    {
        stxxl::scoped_print_timer spt("Merging", total_bytes);

        typedef std::pair<sequence_type::iterator, sequence_type::iterator> sequence_iterator_pair_type;
        std::vector<sequence_iterator_pair_type> sequences(seqnum);

        for (size_t i = 0; i < seqnum; ++i) {
            sequences[i] =
                sequence_iterator_pair_type(blocks[i].begin(), blocks[i].end());
        }

        if (Method == 0)
        {
            stxxl::parallel::sequential_multiway_merge(sequences.begin(), sequences.end(),
                                                       out.begin(), total_size, cmp,
                                                       false, false);
        }
        else if (Method == 1)
        {
            __gnu_parallel::multiway_merge(sequences.begin(), sequences.end(),
                                           out.begin(), total_size, cmp,
                                           __gnu_parallel::sequential_tag());
        }
        else if (Method == 2)
        {
            stxxl::parallel::multiway_merge(sequences.begin(), sequences.end(),
                                            out.begin(), total_size, cmp,
                                            false);
        }
        else if (Method == 3)
        {
            __gnu_parallel::multiway_merge(sequences.begin(), sequences.end(),
                                           out.begin(), total_size, cmp);
        }

        std::cout << "RESULT"
                  << " seqnum=" << seqnum
                  << " method=" << method_name
                  << " block_size=" << block_size
                  << " total_size=" << total_size
                  << " total_bytes=" << total_bytes
                  << " num_threads=" << omp_get_num_threads()
                  << " time=" << spt.timer().seconds()
                  << "\n";
    }

    STXXL_CHECK(stxxl::is_sorted(out.begin(), out.end(), cmp));
}

template <int Method>
void test_all()
{
    for (unsigned int s = 1; s < 128; s += 1)
        test_multiway_merge<Method>(s);

    for (unsigned int s = 128; s < 256; s += 2)
        test_multiway_merge<Method>(s);

    for (unsigned int s = 256; s < 512; s += 4)
        test_multiway_merge<Method>(s);

    for (unsigned int s = 512; s < 1024; s += 8)
        test_multiway_merge<Method>(s);

    for (unsigned int s = 1024; s < 1024 * 1024; s += 32)
        test_multiway_merge<Method>(s);
}

int main()
{
    test_all<0>();
    test_all<1>();
    test_all<2>();
    test_all<3>();

    return 0;
}
