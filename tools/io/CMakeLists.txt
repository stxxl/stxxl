###############################################################################
# CMakeLists.txt
#
# Cmake file for stxxl/tools/io
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

stxxl_build_extra_tool(benchmark_configured_disks)
stxxl_build_extra_tool(benchmark_disk_and_flash)
stxxl_build_extra_tool(benchmark_random_block_access)
stxxl_build_extra_tool(flushbuffers)
stxxl_build_extra_tool(gen_file)
stxxl_build_extra_tool(iobench_scatter_in_place)
stxxl_build_extra_tool(test_cancel)
stxxl_build_extra_tool(test_io)
stxxl_build_extra_tool(test_io_sizes)
stxxl_build_extra_tool(verify_disks)

stxxl_build_extra_tool(benchmark_disks)

if(NOT MSVC)
  stxxl_build_extra_tool(sd_test)
endif(NOT MSVC)

# TODO: cppunit test!
if(CPPUNIT_FOUND AND BUILD_TESTS)

  add_executable(unittest_io unittest.cpp)
  target_link_libraries(unittest_io ${STXXL_LIBS} ${CPPUNIT_LIBRARIES})
  add_test(unittest_io unittest_io)

endif(CPPUNIT_FOUND AND BUILD_TESTS)
