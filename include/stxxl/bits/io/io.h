/***************************************************************************
 *  include/stxxl/bits/io/io.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_HEADER
#define STXXL_IO_HEADER

#include <stxxl/bits/io/iobase.h>
#include <stxxl/bits/io/syscall_file.h>
#include <stxxl/bits/io/mmap_file.h>
#include <stxxl/bits/io/simdisk_file.h>
#include <stxxl/bits/io/wincall_file.h>
#include <stxxl/bits/io/boostfd_file.h>
#include <stxxl/bits/io/mem_file.h>
#include <stxxl/bits/io/fileperblock_file.h>
#include <stxxl/bits/io/iostats.h>


#ifdef BOOST_MSVC
 #pragma comment (lib, "libstxxl.lib")
#endif

//! \brief \c \<stxxl\> library namespace
__STXXL_BEGIN_NAMESPACE

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_HEADER
