/***************************************************************************
 *  lib/io/serving_request.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/common/shared_state.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/request_interface.h>
#include <stxxl/bits/io/request_with_state.h>
#include <stxxl/bits/io/serving_request.h>
#include <stxxl/bits/verbose.h>

#include <iomanip>

namespace stxxl {

serving_request::serving_request(
    const completion_handler& on_cmpl,
    file* file, void* buffer, offset_type offset, size_type bytes,
    read_or_write op)
    : request_with_state(on_cmpl, file, buffer, offset, bytes, op)
{
#ifdef STXXL_CHECK_BLOCK_ALIGNING
    // Direct I/O requires file system block size alignment for file offsets,
    // memory buffer addresses, and transfer(buffer) size must be multiple
    // of the file system block size
    check_alignment();
#endif
}

void serving_request::serve()
{
    check_nref();
    STXXL_VERBOSE2_THIS(
        "serving_request::serve(): " <<
            buffer_ << " @ [" <<
            file_ << "|" << file_->get_allocator_id() << "]0x" <<
            std::hex << std::setfill('0') << std::setw(8) <<
            offset_ << "/0x" << bytes_ <<
        (op_ == request::READ ? " READ" : " WRITE"));

    try
    {
        file_->serve(buffer_, offset_, bytes_, op_);
    }
    catch (const io_error& ex)
    {
        error_occured(ex.what());
    }

    check_nref(true);

    completed(false);
}

const char* serving_request::io_type() const
{
    return file_->io_type();
}

} // namespace stxxl
// vim: et:ts=4:sw=4
