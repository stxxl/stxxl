###############################################################################
# CMakeLists.txt
#
# Cmake file for stxxl/tests/common
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

stxxl_build_test(test_argument_helper)
stxxl_build_test(test_counting_ptr)
stxxl_build_test(test_globals)
stxxl_build_test(test_log2)
stxxl_build_test(test_manyunits test_manyunits2)
stxxl_build_test(test_random)
stxxl_build_test(test_uint_types)

stxxl_test(test_argument_helper)
stxxl_test(test_counting_ptr)
stxxl_test(test_globals)
stxxl_test(test_log2)
stxxl_test(test_manyunits)
stxxl_test(test_random)
stxxl_test(test_uint_types)
