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

#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/file.h>


__STXXL_BEGIN_NAMESPACE

request::request(const completion_handler & on_compl,
            file * file__,
            void * buffer_,
            stxxl::int64 offset_,
            size_t bytes_,
            request_type type_) :
        on_complete(on_compl), ref_cnt(0),
        file_(file__),
        buffer(buffer_),
        offset(offset_),
        bytes(bytes_),
        type(type_)
{
    STXXL_VERBOSE3("request " << static_cast<void *>(this) << ": creation, cnt: " << ref_cnt);
}

request::~request()
{
    STXXL_VERBOSE3("request " << static_cast<void *>(this) << ": deletion, cnt: " << ref_cnt);
}

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
