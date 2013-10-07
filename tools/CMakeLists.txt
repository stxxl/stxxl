###############################################################################
# CMakeLists.txt
#
# Cmake file for stxxl/tools/
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

stxxl_build_tool(stxxl_tool
  create_files.cpp
  benchmark_disks.cpp
  benchmark_files.cpp
  benchmark_sort.cpp
  benchmark_disks_random.cpp
  )

add_subdirectory(containers)
add_subdirectory(io)
add_subdirectory(utils)

