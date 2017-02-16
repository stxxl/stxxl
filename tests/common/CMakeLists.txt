############################################################################
#  tests/common/CMakeLists.txt
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

stxxl_build_test(test_binary_buffer)
stxxl_build_test(test_cmdline)
stxxl_build_test(test_comparator)
stxxl_build_test(test_counting_ptr)
stxxl_build_test(test_external_shared_ptr)
stxxl_build_test(test_globals)
stxxl_build_test(test_log2)
stxxl_build_test(test_manyunits test_manyunits2)
stxxl_build_test(test_random)
stxxl_build_test(test_swap_vector)
stxxl_build_test(test_tuple)
stxxl_build_test(test_uint_types)
stxxl_build_test(test_winner_tree)

stxxl_test(test_binary_buffer)
stxxl_test(test_cmdline)
stxxl_test(test_counting_ptr)
stxxl_test(test_external_shared_ptr)
stxxl_test(test_globals)
stxxl_test(test_log2)
stxxl_test(test_manyunits)
stxxl_test(test_random)
stxxl_test(test_swap_vector)
stxxl_test(test_tuple)
stxxl_test(test_uint_types)
stxxl_test(test_winner_tree)
