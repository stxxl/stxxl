###############################################################################
# CMakeLists.txt
#
# Cmake file for stxxl/tools/containers
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

stxxl_build_test(bench_pqueue)
stxxl_build_test(benchmark_naive_matrix)
stxxl_build_test(copy_file)
stxxl_build_test(matrix_benchmark)
stxxl_build_test(monotonic_pq)
stxxl_build_test(pq_benchmark)
stxxl_build_test(stack_benchmark)
stxxl_build_test(test_deque)
stxxl_build_test(test_ext_merger)
stxxl_build_test(test_ext_merger2)
stxxl_build_test(test_iterators)
stxxl_build_test(test_many_stacks)
stxxl_build_test(test_matrix)
stxxl_build_test(test_migr_stack)
stxxl_build_test(test_pqueue)
stxxl_build_test(test_queue)
stxxl_build_test(test_queue2)
stxxl_build_test(test_stack)
stxxl_build_test(test_vector)
stxxl_build_test(test_vector_export)
stxxl_build_test(test_vector_sizes)
stxxl_build_test(write_vector)
stxxl_build_test(write_vector2)

add_define(test_many_stacks "STXXL_VERBOSE_LEVEL=1")
add_define(test_ext_merger "STXXL_VERBOSE_LEVEL=1")
add_define(test_ext_merger2 "STXXL_VERBOSE_LEVEL=1")
add_define(bench_pqueue "STXXL_VERBOSE_LEVEL=0")
add_define(benchmark_naive_matrix "STXXL_VERBOSE_LEVEL=0")

# TESTS_MAP
stxxl_build_test(test_map)
stxxl_build_test(test_map_random)

#TODO TESTS_BDB		 = berkeley_db_benchmark
#TODO TESTS_LEDASM	 = leda_sm_pq_benchmark leda_sm_stack_benchmark
#TODO TESTS_TPIE	 = tpie_stack_benchmark

add_subdirectory(btree)
