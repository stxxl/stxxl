/***************************************************************************
 *  tools/benchmarks/matrix_benchmark.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cassert>
#include <iostream>
#include <limits>

#include <tlx/logger.hpp>

#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/containers/matrix.h>
#include <stxxl/stream>
#include <stxxl/vector>

int main(int argc, char** argv)
{
    #ifndef STXXL_MATRIX_BLOCK_ORDER
    const int block_order = 1568;                     // must be a multiple of 32, assuming at least 4 bytes element size
    #else
    const int block_order = STXXL_MATRIX_BLOCK_ORDER; // must be a multiple of 32, assuming at least 4 bytes element size
    #endif

    int rank = 10000;
    size_t internal_memory = 256 * 1024 * 1024;
    int mult_algo_num = 5;
    int sched_algo_num = 2;

    stxxl::cmdline_parser cp;
    cp.add_int('r', "rank", "N", rank,
               "rank of the matrices, default: 10000");
    cp.add_bytes('m', "memory", "L", internal_memory,
                 "internal memory to use, default: 256 MiB");
    cp.add_int('a', "mult-algo", "N", mult_algo_num,
               "use multiplication-algorithm number N\n"
               "available are:\n"
               "   0: naive_multiply_and_add\n"
               "   1: recursive_multiply_and_add\n"
               "   2: strassen_winograd_multiply_and_add\n"
               "   3: multi_level_strassen_winograd_multiply_and_add\n"
               "   4: strassen_winograd_multiply (block-interleaved pre- and postadditions)\n"
               "   5: strassen_winograd_multiply_and_add_interleaved (block-interleaved preadditions)\n"
               "   6: multi_level_strassen_winograd_multiply_and_add_block_grained\n"
               "   -1: internal multiplication\n"
               "   -2: pure BLAS\n"
               "  default: 5");
    cp.add_int('s', "scheduling-algo", "N", sched_algo_num,
               "use scheduling-algorithm number N\n"
               "  available are:\n"
               "   0: online LRU\n"
               "   1: offline LFD\n"
               "   2: offline LRU prefetching\n"
               "  default: 2");

    cp.set_description("stxxl matrix test");
    cp.set_author("Raoul Steffen <R-Steffen@gmx.de>");

    if (!cp.process(argc, argv))
        return -1;

    LOG1 << "multiplying two full double matrices of rank " <<
        rank << ", block order " << block_order <<
        " using " << internal_memory / 1024 / 1024 << "MiB internal memory, multiplication-algo " <<
        mult_algo_num << ", scheduling-algo " << sched_algo_num;

    using value_type = double;

    foxxll::stats_data stats_before, stats_after;
    stxxl::matrix_operation_statistic_data matrix_stats_before, matrix_stats_after;

    if (mult_algo_num == -2)
    {
        const size_t size = rank * rank;
        value_type* A = new value_type[size];
        value_type* B = new value_type[size];
        value_type* C = new value_type[size];
        // write A and B
        for (size_t i = 0; i < size; ++i)
            A[i] = B[i] = 1;
        // evict A and B by accessing lots of memory
        size_t int_mem_size = 50 * (size_t(1) << 30) / sizeof(size_t);
        assert(int_mem_size > 0);
        size_t* D = new size_t[int_mem_size];
        for (size_t i = 0; i < int_mem_size; ++i)
            D[i] = 1;
        delete[] D;
        #if STXXL_BLAS
        stats_before = *foxxll::stats::get_instance();
        gemm_wrapper(rank, rank, rank,
                     value_type(1), false, A,
                     false, B,
                     value_type(0), false, C);
        stats_after = *foxxll::stats::get_instance();
        #else
        LOG1 << "internal multiplication is only available for testing with blas";
        #endif
        delete[] A;
        delete[] B;
        delete[] C;
    }
    else
    {
        using bst = foxxll::block_scheduler<stxxl::matrix_swappable_block<value_type, block_order> >;
        using mt = stxxl::matrix<value_type, block_order>;
        using mitt = mt::row_major_iterator;
        using cmitt = mt::const_row_major_iterator;

        bst* b_s = new bst(internal_memory);  // the block_scheduler may use internal_memory byte for caching
        bst& bs = *b_s;
        mt* a = new mt(bs, rank, rank),
            * b = new mt(bs, rank, rank),
            * c = new mt(bs, rank, rank);

        LOG1 << "writing input matrices";
        for (mitt mit = a->begin(); mit != a->end(); ++mit)
            *mit = 1;
        for (mitt mit = b->begin(); mit != b->end(); ++mit)
            *mit = 1;

        bs.flush();
        LOG1 << "start of multiplication";
        matrix_stats_before.set();
        stats_before = *foxxll::stats::get_instance();
        if (mult_algo_num >= 0)
            *c = a->multiply(*b, mult_algo_num, sched_algo_num);
        else
            *c = a->multiply_internal(*b, sched_algo_num);
        bs.flush();
        stats_after = *foxxll::stats::get_instance();
        matrix_stats_after.set();
        LOG1 << "end of multiplication";

        matrix_stats_after = matrix_stats_after - matrix_stats_before;
        LOG1 << matrix_stats_after;
        stats_after = stats_after - stats_before;
        LOG1 << stats_after;
        {
            size_t num_err = 0;
            for (cmitt mit = c->cbegin(); mit != c->cend(); ++mit)
                num_err += (*mit != rank);
            if (num_err)
                LOG1 << "c had " << num_err << " errors";
        }

        delete a;
        delete b;
        delete c;
        delete b_s;
    }

    LOG1 << "end of test";
    std::cout << "@";
    std::cout << " ra " << rank << " bo " << block_order << " im " << internal_memory / 1024 / 1024
              << " ma " << mult_algo_num << " sa " << sched_algo_num;
    std::cout << " mu " << matrix_stats_after.block_multiplication_calls
              << " mus " << matrix_stats_after.block_multiplications_saved_through_zero
              << " ad " << matrix_stats_after.block_addition_calls
              << " ads " << matrix_stats_after.block_additions_saved_through_zero;
    std::cout << " t " << stats_after.get_elapsed_time()
              << " r " << stats_after.get_read_count() << " w " << stats_after.get_write_count()
              << " rt " << stats_after.get_read_time() << " rtp " << stats_after.get_pread_time()
              << " wt " << stats_after.get_write_time() << " wtp " << stats_after.get_pwrite_time()
              << " rw " << stats_after.get_wait_read_time() << " ww " << stats_after.get_wait_write_time()
              << " iotp " << stats_after.get_pio_time();
    std::cout << std::endl;
    return 0;
}
