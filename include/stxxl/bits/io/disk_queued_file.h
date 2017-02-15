/***************************************************************************
 *  include/stxxl/bits/io/disk_queued_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_DISK_QUEUED_FILE_HEADER
#define STXXL_IO_DISK_QUEUED_FILE_HEADER

#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/request.h>

namespace stxxl {

//! \addtogroup fileimpl
//! \{

//! Implementation of some file methods based on serving_request.
class disk_queued_file : public virtual file
{
    int queue_id_, allocator_id_;

public:
    disk_queued_file(int queue_id, int allocator_id)
        : queue_id_(queue_id), allocator_id_(allocator_id)
    { }

    request_ptr aread(
        void* buffer, offset_type pos, size_type bytes,
        const completion_handler& on_complete = completion_handler());

    request_ptr awrite(
        void* buffer, offset_type pos, size_type bytes,
        const completion_handler& on_complete = completion_handler());

    virtual int get_queue_id() const
    {
        return queue_id_;
    }

    virtual int get_allocator_id() const
    {
        return allocator_id_;
    }
};

//! \}

} // namespace stxxl

#endif // !STXXL_IO_DISK_QUEUED_FILE_HEADER
// vim: et:ts=4:sw=4
