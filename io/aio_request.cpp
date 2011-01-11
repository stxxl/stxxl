/***************************************************************************
 *  io/aio_request.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/aio_request.h>
#include <stxxl/bits/io/disk_queues.h>

#if STXXL_HAVE_AIO_FILE

#include <sys/syscall.h>


__STXXL_BEGIN_NAMESPACE

void aio_request::completed(bool posted, bool canceled)
{
    if (!canceled)
    {
        if (type == READ)
            stats::get_instance()->read_finished();
        else
            stats::get_instance()->write_finished();
    }
    else if (posted)
    {
        if (type == READ)
            stats::get_instance()->read_canceled(bytes);
        else
            stats::get_instance()->write_canceled(bytes);
    }
    request_impl_basic::completed(canceled);
}

void aio_request::fill_control_block()
{
    aio_file * af = dynamic_cast<aio_file *>(file_);

    memset(&cb, 0, sizeof(cb));
    cb.aio_data = reinterpret_cast<__u64>(new request_ptr(this));       // indirection, so the I/O system retains an auto_ptr reference
    cb.aio_fildes = af->file_des;
    cb.aio_lio_opcode = (type == READ) ? IOCB_CMD_PREAD : IOCB_CMD_PWRITE;
    cb.aio_reqprio = 0;
    cb.aio_buf = reinterpret_cast<__u64>(buffer);
    cb.aio_nbytes = bytes;
    cb.aio_offset = offset;
}

//! \brief Submits an I/O request to the OS
//! \returns false if submission fails
bool aio_request::post()
{
    fill_control_block();
    iocb * cb_pointer = &cb;
    double now = timestamp();	// io_submit might considerable time, so we have to remember the current time before
    aio_queue * queue = dynamic_cast<aio_queue*>(disk_queues::get_instance()->get_queue(get_file()->get_queue_id()));
    int success = syscall(SYS_io_submit, queue->get_io_context(), 1, &cb_pointer);
    if (success == 1)
    {
    	if (type == READ)
    		stats::get_instance()->read_started(bytes, now);
    	else
    		stats::get_instance()->write_started(bytes, now);
    }
    else if (success == -1 && errno != EAGAIN)
        STXXL_THROW2(io_error, "io_submit()");

    return success == 1;
}

//! \brief Cancel the request
//!
//! Routine is called by user, as part of the request interface.
bool aio_request::cancel()
{
    request_ptr req(this);
    aio_queue * queue = dynamic_cast<aio_queue*>(disk_queues::get_instance()->get_queue(get_file()->get_queue_id()));
    return queue->cancel_request(req);
}

//! \brief Cancel already posted request
bool aio_request::cancel_aio()
{
    io_event event;
    aio_queue * queue = dynamic_cast<aio_queue*>(disk_queues::get_instance()->get_queue(get_file()->get_queue_id()));
    int result = syscall(SYS_io_cancel, queue->get_io_context(), &cb, &event);
    if (result == 0)    //successfully canceled
        queue->handle_events(&event, 1, true);
    return result == 0;
}

__STXXL_END_NAMESPACE

#endif // #if STXXL_HAVE_AIO_FILE
// vim: et:ts=4:sw=4
