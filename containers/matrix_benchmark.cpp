/***************************************************************************
 *  containers/matrix_benchmark.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#include <limits>

// Thanks Daniel Russel, Stanford University
#include <Argument_helper.h>

#include <stxxl/vector>
#include <stxxl/stream>
#include <stxxl/bits/containers/matrix.h>

using namespace stxxl;

int main(int argc, char **argv)
{

    #ifndef STXXL_MATRIX_BLOCK_ORDER
    const int block_order = 1568; // must be a multiple of 32, assuming at least 4 bytes element size
    #else
    const int block_order = STXXL_MATRIX_BLOCK_ORDER; // must be a multiple of 32, assuming at least 4 bytes element size
    #endif

    int rank = 10000;
    int internal_memory_megabyte = 256;
    int mult_algo_num = 5;
    int sched_algo_num = 2;
    int internal_memory_byte = 0;

    dsr::Argument_helper ah;
    ah.new_named_int('r',  "rank", "N","rank of the matrices   default", rank);
    ah.new_named_int('m', "memory", "L", "internal memory to use (in megabytes)   default", internal_memory_megabyte);
    ah.new_named_int('a', "mult-algo", "N", "use multiplication-algorithm number N\n  available are:\n   0: naive_multiply_and_add\n   1: recursive_multiply_and_add\n   2: strassen_winograd_multiply_and_add\n   3: multi_level_strassen_winograd_multiply_and_add\n   4: strassen_winograd_multiply (block-interleaved pre- and postadditions)\n   5: strassen_winograd_multiply_and_add_interleaved (block-interleaved preadditions)\n   6: multi_level_strassen_winograd_multiply_and_add_block_grained\n  default", mult_algo_num);
    ah.new_named_int('s', "scheduling-algo", "N", "use scheduling-algorithm number N\n  available are:\n   0: online LRU\n   1: offline LFD\n   2: offline LRU prefetching\n  default", sched_algo_num);
    ah.new_named_int('c', "memory-byte", "L", "internal memory to use (in bytes)   no default", internal_memory_byte);

    ah.set_description("stxxl matrix test");
    ah.set_author("Raoul Steffen, R-Steffen@gmx.de");
    ah.process(argc, argv);

    int_type internal_memory;
    if (internal_memory_byte)
        internal_memory = internal_memory_byte;
    else
        internal_memory = int_type(internal_memory_megabyte) * 1048576;

    STXXL_MSG("multiplying two full double matrices of rank " << rank << ", block order " << block_order
            << " using " << internal_memory_megabyte << "MiB internal memory, multiplication-algo "
            << mult_algo_num << ", scheduling-algo " << sched_algo_num);

    typedef double value_type;

    typedef block_scheduler< matrix_swappable_block<value_type, block_order> > bst;
    typedef matrix<value_type, block_order> mt;
    typedef mt::row_major_iterator mitt;
    typedef mt::const_row_major_iterator cmitt;

    bst * b_s = new bst(internal_memory); // the block_scheduler may use internal_memory byte for caching
    bst & bs = *b_s;
    mt * a = new mt(bs, rank, rank),
       * b = new mt(bs, rank, rank),
       * c = new mt(bs, rank, rank);
    stats_data stats_before, stats_after;
    matrix_operation_statistic_data matrix_stats_before, matrix_stats_after;

    STXXL_MSG("writing input matrices");
    for (mitt mit = a->begin(); mit != a->end(); ++mit)
        *mit = 1;
    for (mitt mit = b->begin(); mit != b->end(); ++mit)
        *mit = 1;

    bs.flush();
    STXXL_MSG("start of multiplication");
    matrix_stats_before.set();
    stats_before = *stats::get_instance();
    if (mult_algo_num >= 0)
        *c = a->multiply(*b, mult_algo_num, sched_algo_num);
    else
        *c = a->multiply_internal(*b, sched_algo_num);
    bs.flush();
    stats_after = *stats::get_instance();
    matrix_stats_after.set();
    STXXL_MSG("end of multiplication");

    matrix_stats_after = matrix_stats_after - matrix_stats_before;
    STXXL_MSG(matrix_stats_after);
    stats_after = stats_after - stats_before;
    STXXL_MSG(stats_after);
    {
        int_type num_err = 0;
        for (cmitt mit = c->cbegin(); mit != c->cend(); ++mit)
            num_err += (*mit != rank);
        if (num_err)
            STXXL_ERRMSG("c had " << num_err << " errors");
    }

    delete a;
    delete b;
    delete c;
    delete b_s;

    STXXL_MSG("end of test");
    std::cout << "@";
    std::cout << " ra " << rank << " bo " << block_order << " im " << internal_memory_megabyte
            << " ma " << mult_algo_num << " sa " << sched_algo_num;
    std::cout << " mu " << matrix_stats_after.block_multiplication_calls
            << " mus " << matrix_stats_after.block_multiplications_saved_through_zero
            << " ad " << matrix_stats_after.block_addition_calls
            << " ads " << matrix_stats_after.block_additions_saved_through_zero;
    std::cout  << " t " << stats_after.get_elapsed_time()
            << " r " << stats_after.get_reads() << " w " << stats_after.get_writes()
            << " rt " << stats_after.get_read_time() << " rtp " << stats_after.get_pread_time()
            << " wt " << stats_after.get_write_time() << " wtp " << stats_after.get_pwrite_time()
            << " rw " << stats_after.get_wait_read_time() << " ww " << stats_after.get_wait_write_time();
    std::cout << std::endl;
    return 0;
}
