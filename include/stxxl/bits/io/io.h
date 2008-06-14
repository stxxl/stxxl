/***************************************************************************
 *            io.h
 *
 *  Sat Aug 24 23:54:40 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#ifndef STXXL_IO_HEADER
#define STXXL_IO_HEADER

#include "stxxl/bits/io/iobase.h"
#include "stxxl/bits/io/syscall_file.h"
#include "stxxl/bits/io/mmap_file.h"
#include "stxxl/bits/io/simdisk_file.h"
#include "stxxl/bits/io/wincall_file.h"
#include "stxxl/bits/io/boostfd_file.h"
#include "stxxl/bits/io/iostats.h"


#ifdef BOOST_MSVC
 #pragma comment (lib, "libstxxl.lib")
#endif

//! \brief \c \<stxxl\> library namespace
__STXXL_BEGIN_NAMESPACE


__STXXL_END_NAMESPACE

#endif // !STXXL_IO_HEADER
