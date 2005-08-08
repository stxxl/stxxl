#ifndef IO_HEADER
#define IO_HEADER

/***************************************************************************
 *            io.h
 *
 *  Sat Aug 24 23:54:40 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "iobase.h"
#include "syscall_file.h"
#include "mmap_file.h"
#include "simdisk_file.h"
#include "wincall_file.h"
#include "iostats.h"

#ifdef BOOST_MSVC
#pragma comment (lib, "libstxxl.lib")
#endif

//! \brief \c \<stxxl\> library namespace
__STXXL_BEGIN_NAMESPACE


__STXXL_END_NAMESPACE

#endif
