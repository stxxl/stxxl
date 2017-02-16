############################################################################
#  tests/io/CMakeLists.txt
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

stxxl_build_test(test_cancel)
stxxl_build_test(test_io)
stxxl_build_test(test_io_sizes)

stxxl_test(test_io "${STXXL_TMPDIR}")

stxxl_test(test_cancel syscall
  "${STXXL_TMPDIR}/testdisk_cancel_syscall")
# TODO: clean up after fileperblock_syscall
stxxl_test(test_cancel fileperblock_syscall
  "${STXXL_TMPDIR}/testdisk_cancel_fpb_syscall")

if(STXXL_HAVE_MMAP_FILE)
  stxxl_test(test_cancel mmap
    "${STXXL_TMPDIR}/testdisk_cancel_mmap")
  stxxl_test(test_cancel fileperblock_mmap
    "${STXXL_TMPDIR}/testdisk_cancel_fpb_mmap")
endif(STXXL_HAVE_MMAP_FILE)

if(STXXL_HAVE_LINUXAIO_FILE)
  stxxl_test(test_cancel linuxaio
    "${STXXL_TMPDIR}/testdisk_cancel_linuxaio")
endif(STXXL_HAVE_LINUXAIO_FILE)

stxxl_test(test_cancel memory
  "${STXXL_TMPDIR}/testdisk_cancel_memory")

stxxl_test(test_io_sizes syscall
  "${STXXL_TMPDIR}/testdisk_io_sizes_syscall" 1073741824)
if(STXXL_HAVE_MMAP_FILE)
  stxxl_test(test_io_sizes mmap
    "${STXXL_TMPDIR}/testdisk_io_sizes_mmap" 1073741824)
endif(STXXL_HAVE_MMAP_FILE)
if(STXXL_HAVE_LINUXAIO_FILE)
  stxxl_test(test_io_sizes linuxaio
    "${STXXL_TMPDIR}/testdisk_io_sizes_linxaio" 1073741824)
endif(STXXL_HAVE_LINUXAIO_FILE)

if(STXXL_HAVE_MMAP_FILE)
  stxxl_build_test(test_mmap)
  stxxl_test(test_mmap)
endif(STXXL_HAVE_MMAP_FILE)
