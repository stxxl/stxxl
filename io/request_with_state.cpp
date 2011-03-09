/***************************************************************************
 *  io/request_with_state.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2005, 2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/request_with_state.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/disk_queues.h>

__STXXL_BEGIN_NAMESPACE


request_with_state::~request_with_state()
{
    STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request_with_state::~(), ref_cnt: " << ref_cnt);

    assert(_state() == DONE || _state() == READY2DIE);

    // if(_state() != DONE && _state()!= READY2DIE )
    // STXXL_ERRMSG("WARNING: serious stxxl inconsistency: Request is being deleted while I/O not finished. "<<
    //              "Please submit a bug report.");

    // _state.wait_for (READY2DIE); // does not make sense ?
}

void request_with_state::wait(bool measure_time)
{
    STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request_with_state::wait()");

    stats::scoped_wait_timer wait_timer(get_type() == READ ? stats::WAIT_OP_READ : stats::WAIT_OP_WRITE, measure_time);

    _state.wait_for(READY2DIE);

    check_errors();
}

bool request_with_state::cancel()
{
    STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request_with_state::cancel() " << file_ << " " << buffer << " " << offset);

    if (file_)
    {
        request_ptr rp(this);
        if (disk_queues::get_instance()->cancel_request(rp, file_->get_queue_id()))
        {
            _state.set_to(DONE);
            notify_waiters();
            file_->delete_request_ref();
            file_ = 0;
            _state.set_to(READY2DIE);
            return true;
        }
    }
    return false;
}

bool request_with_state::poll()
{
    const request_state s = _state();

    check_errors();

    return s == DONE || s == READY2DIE;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
