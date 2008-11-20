/***************************************************************************
 *  include/stxxl/bits/io/iobase.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IOBASE_HEADER
#define STXXL_IOBASE_HEADER

#include <set>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/state.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/disk_queues.h>


__STXXL_BEGIN_NAMESPACE

//! \defgroup iolayer I/O primitives layer
//! Group of classes which enable abstraction from operating system calls and support
//! system-independent interfaces for asynchronous I/O.
//! \{

//! \brief Default completion handler class

struct default_completion_handler
{
    //! \brief An operator that does nothing
    void operator () (request *) { }
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IOBASE_HEADER
// vim: et:ts=4:sw=4
