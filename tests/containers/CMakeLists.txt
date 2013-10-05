###############################################################################
# CMakeLists.txt
#
# Cmake file for stxxl/tests/containers/
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

add_subdirectory(btree)

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
stxxl_build_test(test_sequence)
stxxl_build_test(test_sorter)
stxxl_build_test(test_stack)
stxxl_build_test(test_vector)
stxxl_build_test(test_vector_buf)
stxxl_build_test(test_vector_export)
stxxl_build_test(test_vector_sizes)

add_define(test_many_stacks "STXXL_VERBOSE_LEVEL=1")
add_define(test_ext_merger "STXXL_VERBOSE_LEVEL=0")
add_define(test_ext_merger2 "STXXL_VERBOSE_LEVEL=0")

stxxl_test(test_deque 3333333)
stxxl_test(test_ext_merger)
stxxl_test(test_ext_merger2)
stxxl_test(test_iterators)
stxxl_test(test_many_stacks 42)
stxxl_test(test_matrix 4)
stxxl_test(test_matrix 5)
stxxl_test(test_migr_stack)
stxxl_test(test_pqueue)
stxxl_test(test_queue)
stxxl_test(test_queue2 200)
stxxl_test(test_sequence)
stxxl_test(test_sorter)
stxxl_test(test_stack 1024)
stxxl_test(test_vector)
stxxl_test(test_vector_buf)
stxxl_test(test_vector_export)
stxxl_test(test_vector_sizes "${STXXL_TMPDIR}/out" syscall)
if(NOT MSVC)
  stxxl_test(test_vector_sizes "${STXXL_TMPDIR}/out" mmap)
endif(NOT MSVC)
if(USE_BOOST)
  #-tb: boostfd does not work right with vector, because boostfd does not
  #correctly support set_size() (truncate and extending of files). FIXME
  #stxxl_test(test_vector_sizes "${STXXL_TMPDIR}/out" boostfd)
endif(USE_BOOST)

# TESTS_MAP
stxxl_build_test(test_map)
stxxl_build_test(test_map_random)

stxxl_test(test_map 16)
stxxl_test(test_map_random 2000)

#TODO TESTS_BDB		 = berkeley_db_benchmark
#TODO TESTS_LEDASM	 = leda_sm_pq_benchmark leda_sm_stack_benchmark
#TODO TESTS_TPIE	 = tpie_stack_benchmark

