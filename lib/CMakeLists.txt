###############################################################################
# CMakeLists.txt
#
# Cmake file for libstxxl.a/so/lib
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

set(LIBSTXXL_SOURCES

  common/exithandler.cpp
  common/log.cpp
  common/rand.cpp
  common/seed.cpp
  common/utils.cpp
  common/verbose.cpp
  common/version.cpp

  io/boostfd_file.cpp
  io/create_file.cpp
  io/disk_queued_file.cpp
  io/fileperblock_file.cpp
  io/iostats.cpp
  io/mem_file.cpp
  io/request.cpp
  io/request_queue_impl_1q.cpp
  io/request_queue_impl_qwqr.cpp
  io/request_queue_impl_worker.cpp
  io/request_with_state.cpp
  io/request_with_waiters.cpp
  io/serving_request.cpp
  io/syscall_file.cpp
  io/ufs_file_base.cpp
  io/wbtl_file.cpp
  io/wfs_file_base.cpp
  io/wincall_file.cpp

  mng/config.cpp
  mng/diskallocator.cpp
  mng/mng.cpp
  
  algo/async_schedule.cpp

  )

if(NOT MSVC)

  # additional sources for non Visual Studio builds
  set(LIBSTXXL_SOURCES ${LIBSTXXL_SOURCES}

    io/mmap_file.cpp
    io/simdisk_file.cpp

    )

endif(NOT MSVC)

add_library(stxxl ${LIBSTXXL_SOURCES})

set_target_properties(stxxl PROPERTIES VERSION 1.3.9999)
