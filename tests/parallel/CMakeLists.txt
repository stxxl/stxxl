############################################################################
#  tests/parallel/CMakeLists.txt
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2014-2015 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

stxxl_build_test(test_multiway_merge)
stxxl_build_test(test_multiway_mergesort)
stxxl_build_test(bench_multiway_merge)

stxxl_test(test_multiway_merge)
stxxl_test(test_multiway_mergesort)

add_define(test_multiway_merge "MCSTL_ASSERTIONS=1")
add_define(test_multiway_mergesort "MCSTL_ASSERTIONS=1")
