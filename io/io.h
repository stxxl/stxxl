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
#include "iostats.h"


//! \brief \c \<stxxl\> library namespace
__STXXL_BEGIN_NAMESPACE

disk_queues * disk_queues::instance = NULL;
stats * stats::instance = NULL;
debugmon * debugmon::instance = NULL;

#ifdef COUNT_WAIT_TIME
double stxxl::wait_time_counter = .0;
#endif
	

__STXXL_END_NAMESPACE

#endif
