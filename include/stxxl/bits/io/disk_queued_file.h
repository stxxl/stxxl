/***************************************************************************
 *  include/stxxl/bits/io/disk_queued_file.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_HEADER_IO_DISK_QUEUED_FILE
#define STXXL_HEADER_IO_DISK_QUEUED_FILE

#include <stxxl/bits/io/file.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of some file methods based on request_impl_basic
class disk_queued_file : public virtual file
{
	int queue_id, allocator_id;
public:
    disk_queued_file(int queue_id, int allocator_id) : queue_id(queue_id), allocator_id(allocator_id)
    { }
    request_ptr aread(
        void * buffer,
        offset_type pos,
        size_type bytes,
        const completion_handler & on_cmpl);
    request_ptr awrite(
        void * buffer,
        offset_type pos,
        size_type bytes,
        const completion_handler & on_cmpl);

    virtual int get_queue_id() const
    {
        return queue_id;
    }

    virtual int get_allocator_id() const
    {
        return allocator_id;
    }
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_HEADER_IO_DISK_QUEUED_FILE
// vim: et:ts=4:sw=4
