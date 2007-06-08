#ifndef IO_HEADER
#define IO_HEADER

/***************************************************************************
 *            io.h
 *
 *  Sat Aug 24 23:54:40 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/io/iobase.h"
#include "stxxl/io/syscall_file.h"
#include "stxxl/io/mmap_file.h"
#include "stxxl/io/simdisk_file.h"
#include "stxxl/io/wincall_file.h"
#include "stxxl/io/boostfd_file.h"
#include "stxxl/io/iostats.h"


#ifdef BOOST_MSVC
 #pragma comment (lib, "libstxxl.lib")
#endif

//! \brief \c \<stxxl\> library namespace
__STXXL_BEGIN_NAMESPACE


__STXXL_END_NAMESPACE

#endif
