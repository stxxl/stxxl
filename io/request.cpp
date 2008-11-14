/***************************************************************************
 *  io/request.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/iobase.h>


__STXXL_BEGIN_NAMESPACE


void request::check_alignment() const
{
    if (offset % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Offset is not aligned: modulo " <<
                     BLOCK_ALIGN << " = " << offset % BLOCK_ALIGN);

    if (bytes % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Size is not a multiple of " <<
                     BLOCK_ALIGN << ", = " << bytes % BLOCK_ALIGN);

    if (unsigned_type(buffer) % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Buffer is not aligned: modulo " <<
                     BLOCK_ALIGN << " = " << unsigned_type(buffer) % BLOCK_ALIGN <<
                     " (" << buffer << ")");
}

void request::check_nref_failed(bool after)
{
    STXXL_ERRMSG("WARNING: serious error, reference to the request is lost " <<
                 (after?"after":"before") << " serve" <<
                 " nref=" << nref() <<
                 " this=" << this <<
                 " offset=" << offset <<
                 " buffer=" << buffer <<
                 " bytes=" << bytes <<
                 " type=" << ((type == READ) ? "READ" : "WRITE"));
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
