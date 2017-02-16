/***************************************************************************
 *  lib/io/linuxaio_request.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/linuxaio_request.h>

#if STXXL_HAVE_LINUXAIO_FILE

#include <stxxl/bits/io/disk_queues.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/bits/common/error_handling.h>

#include <unistd.h>
#include <sys/syscall.h>

namespace stxxl {

void linuxaio_request::completed(bool posted, bool canceled)
{
    STXXL_VERBOSE_LINUXAIO("linuxaio_request[" << this << "] completed(" <<
                           posted << "," << canceled << ")");

    if (!canceled)
    {
        if (op_ == READ)
            stats::get_instance()->read_finished();
        else
            stats::get_instance()->write_finished();
    }
    else if (posted)
    {
        if (op_ == READ)
            stats::get_instance()->read_canceled(bytes_);
        else
            stats::get_instance()->write_canceled(bytes_);
    }
    request_with_state::completed(canceled);
}

void linuxaio_request::fill_control_block()
{
    linuxaio_file* af = dynamic_cast<linuxaio_file*>(file_);

    memset(&cb_, 0, sizeof(cb_));
    // indirection, so the I/O system retains a counting_ptr reference
    cb_.aio_data = reinterpret_cast<__u64>(new request_ptr(this));
    cb_.aio_fildes = af->file_des_;
    cb_.aio_lio_opcode = (op_ == READ) ? IOCB_CMD_PREAD : IOCB_CMD_PWRITE;
    cb_.aio_reqprio = 0;
    cb_.aio_buf = static_cast<__u64>((unsigned long)(buffer_));
    cb_.aio_nbytes = bytes_;
    cb_.aio_offset = offset_;
}

//! Submits an I/O request to the OS
//! \returns false if submission fails
bool linuxaio_request::post()
{
    STXXL_VERBOSE_LINUXAIO("linuxaio_request[" << this << "] post()");

    fill_control_block();
    iocb* cb_pointer = &cb_;
    // io_submit might considerable time, so we have to remember the current
    // time before the call.
    double now = timestamp();
    linuxaio_queue* queue = dynamic_cast<linuxaio_queue*>(
        disk_queues::get_instance()->get_queue(file_->get_queue_id()));
    long success = syscall(SYS_io_submit, queue->get_io_context(), 1, &cb_pointer);
    if (success == 1)
    {
        if (op_ == READ)
            stats::get_instance()->read_started(bytes_, now);
        else
            stats::get_instance()->write_started(bytes_, now);
    }
    else if (success == -1 && errno != EAGAIN)
        STXXL_THROW_ERRNO(io_error, "linuxaio_request::post"
                          " io_submit()");

    return success == 1;
}

//! Cancel the request
//!
//! Routine is called by user, as part of the request interface.
bool linuxaio_request::cancel()
{
    STXXL_VERBOSE_LINUXAIO("linuxaio_request[" << this << "] cancel()");

    if (!file_) return false;

    request_ptr req(this);
    linuxaio_queue* queue = dynamic_cast<linuxaio_queue*>(
        disk_queues::get_instance()->get_queue(file_->get_queue_id()));
    return queue->cancel_request(req);
}

//! Cancel already posted request
bool linuxaio_request::cancel_aio()
{
    STXXL_VERBOSE_LINUXAIO("linuxaio_request[" << this << "] cancel_aio()");

    if (!file_) return false;

    io_event event;
    linuxaio_queue* queue = dynamic_cast<linuxaio_queue*>(
        disk_queues::get_instance()->get_queue(file_->get_queue_id()));
    long result = syscall(SYS_io_cancel, queue->get_io_context(), &cb_, &event);
    if (result == 0)    //successfully canceled
        queue->handle_events(&event, 1, true);
    return result == 0;
}

} // namespace stxxl

#endif // #if STXXL_HAVE_LINUXAIO_FILE
// vim: et:ts=4:sw=4
