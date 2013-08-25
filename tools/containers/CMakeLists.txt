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
stxxl_build_test(write_vector)
stxxl_build_test(write_vector2)

add_define(bench_pqueue "STXXL_VERBOSE_LEVEL=0")
add_define(benchmark_naive_matrix "STXXL_VERBOSE_LEVEL=0")

add_library(tpie STATIC IMPORTED)
set_property(TARGET tpie PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/3rdparty/tpie_091905/lib/libtpie.a")

include_directories("${PROJECT_SOURCE_DIR}/3rdparty/tpie_091905/include")
stxxl_build_test(tpie_stack_benchmark)

target_link_libraries(tpie_stack_benchmark ${STXXL_LIBS} tpie)