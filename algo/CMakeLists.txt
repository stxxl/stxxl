###############################################################################
# CMakeLists.txt
#
# Cmake file for stxxl/algo
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

stxxl_build_test(test_scan)
stxxl_build_test(test_sort)
stxxl_build_test(test_ksort)
stxxl_build_test(test_stable_ksort)
stxxl_build_test(test_random_shuffle)
stxxl_build_test(test_bad_cmp)

add_define(test_ksort "STXXL_VERBOSE_LEVEL=1" "STXXL_CHECK_ORDER_IN_SORTS")
add_define(test_sort "STXXL_VERBOSE_LEVEL=0")
add_define(test_bad_cmp "STXXL_VERBOSE_LEVEL=0")
add_define(test_random_shuffle "STXXL_VERBOSE_LEVEL=0")

stxxl_build_extra_tool(test_asch)
stxxl_build_extra_tool(sort_file)
stxxl_build_extra_tool(copy_and_sort_file)

stxxl_build_extra_test(test_sort_all_parameters)
stxxl_build_extra_test(test_ksort_all_parameters)
stxxl_build_extra_test(test_stable_ksort_all_parameters)

if(BUILD_EXTRAS)
  add_define(test_ksort_all_parameters "STXXL_VERBOSE_STABLE_KSORT=STXXL_VERBOSE0" "STXXL_VERBOSE_LEVEL=0" "STXXL_CHECK_ORDER_IN_SORTS")
endif(BUILD_EXTRAS)

#TODO: TESTS_NON_MSVC		 = test_parallel_sort

# Left-overs from GNUmakefile

# ifeq ($(strip $(USE_MCSTL)),yes)
# # ICPC 11.0: internal error: 0_1374 (issue 539748)
# $(call reduce_optimization,3,0,test_sort,Version_11.0,any,any)
# $(call reduce_optimization,3,0,test_ksort,Version_11.0,any,any)
# $(call reduce_optimization,3,0,sort_file,Version_11.0,any,any)
# $(call reduce_optimization,3,0,test_parallel_sort,Version_11.0,any,any)
#
# # ICPC 10.0: internal error: backend signals (issue 466173)
# $(call reduce_optimization,3,1,async_schedule,Version_10.0,32-bit,any,o)
# $(call reduce_optimization,3,1,test_scan,Version_10.0,32-bit,any)
# $(call reduce_optimization,3,1,test_sort,Version_10.0,32-bit,any)
# $(call reduce_optimization,3,1,test_ksort,Version_10.0,32-bit,any)
# $(call reduce_optimization,3,1,test_stable_ksort,Version_10.0,32-bit,any)
# $(call reduce_optimization,3,1,sort_file,Version_10.0,32-bit,any)
# $(call reduce_optimization,3,1,test_random_shuffle,Version_10.0,32-bit,any)
# $(call reduce_optimization,3,1,test_parallel_sort,Version_10.0,32-bit,any)
# endif
