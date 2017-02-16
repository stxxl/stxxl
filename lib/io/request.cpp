/***************************************************************************
 *  lib/io/request.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <ostream>

#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/request.h>

namespace stxxl {

request::request(
    const completion_handler& on_complete,
    file* file, void* buffer, offset_type offset, size_type bytes,
    read_or_write op)
    : on_complete_(on_complete),
      file_(file), buffer_(buffer), offset_(offset), bytes_(bytes),
      op_(op)
{
    STXXL_VERBOSE3_THIS("request::(...), ref_cnt=" << get_reference_count());
    file_->add_request_ref();
}

request::~request()
{
    STXXL_VERBOSE3_THIS("request::~request(), ref_cnt=" << get_reference_count());
}

void request::check_alignment() const
{
    if (offset_ % STXXL_BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Offset is not aligned: modulo " <<
                     STXXL_BLOCK_ALIGN << " = " << offset_ % STXXL_BLOCK_ALIGN);

    if (bytes_ % STXXL_BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Size is not a multiple of " <<
                     STXXL_BLOCK_ALIGN << ", = " << bytes_ % STXXL_BLOCK_ALIGN);

    if (unsigned_type(buffer_) % STXXL_BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Buffer is not aligned: modulo " <<
                     STXXL_BLOCK_ALIGN << " = " << unsigned_type(buffer_) % STXXL_BLOCK_ALIGN <<
                     " (" << buffer_ << ")");
}

void request::check_nref_failed(bool after)
{
    STXXL_ERRMSG("WARNING: serious error, reference to the request is lost " <<
                 (after ? "after" : "before") << " serve()" <<
                 " nref=" << get_reference_count() <<
                 " this=" << this <<
                 " offset=" << offset_ <<
                 " buffer=" << buffer_ <<
                 " bytes=" << bytes_ <<
                 " op=" << ((op_ == READ) ? "READ" : "WRITE") <<
                 " file=" << file_ <<
                 " iotype=" << file_->io_type()
                 );
}

const char* request::io_type() const
{
    return file_->io_type();
}

std::ostream& request::print(std::ostream& out) const
{
    out << "File object address: " << file_;
    out << " Buffer address: " << static_cast<void*>(buffer_);
    out << " File offset: " << offset_;
    out << " Transfer size: " << bytes_ << " bytes";
    out << " Type of transfer: " << ((op_ == READ) ? "READ" : "WRITE");
    return out;
}

std::ostream& operator << (std::ostream& out, const request& req)
{
    return req.print(out);
}

} // namespace stxxl
// vim: et:ts=4:sw=4
