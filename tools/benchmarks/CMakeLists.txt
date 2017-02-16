############################################################################
#  tools/benchmarks/CMakeLists.txt
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

stxxl_build_test(benchmark_naive_matrix)
stxxl_build_test(matrix_benchmark)
stxxl_build_test(monotonic_pq)
stxxl_build_test(pq_benchmark)
stxxl_build_test(stack_benchmark)

add_define(benchmark_naive_matrix "STXXL_VERBOSE_LEVEL=0")
