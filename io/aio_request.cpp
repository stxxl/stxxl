/***************************************************************************
 *  io/aio_request.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iomanip>
#include <stxxl/bits/io/aio_request.h>
#include <stxxl/bits/io/file.h>

#ifndef STXXL_THREAD_ID
#define STXXL_THREAD_ID pthread_self()
#endif


__STXXL_BEGIN_NAMESPACE

const char* aio_request::io_type() const
{
    return file_->io_type();
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
