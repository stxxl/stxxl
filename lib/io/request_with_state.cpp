/***************************************************************************
 *  lib/io/request_with_state.cpp
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

#include <stxxl/bits/common/shared_state.h>
#include <stxxl/bits/io/disk_queues.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/iostats.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/request_with_state.h>
#include <stxxl/bits/singleton.h>
#include <stxxl/bits/verbose.h>

#include <cassert>

namespace stxxl {

request_with_state::~request_with_state()
{
    STXXL_VERBOSE3_THIS("request_with_state::~(), ref_cnt: " << get_reference_count());

    assert(state_() == DONE || state_() == READY2DIE);

    // if(state_() != DONE && state_()!= READY2DIE )
    // STXXL_ERRMSG("WARNING: serious stxxl inconsistency: Request is being deleted while I/O not finished. "<<
    //              "Please submit a bug report.");

    // state_.wait_for (READY2DIE); // does not make sense ?
}

void request_with_state::wait(bool measure_time)
{
    STXXL_VERBOSE3_THIS("request_with_state::wait()");

    stats::scoped_wait_timer wait_timer(
        op_ == READ ? stats::WAIT_OP_READ : stats::WAIT_OP_WRITE, measure_time);

    state_.wait_for(READY2DIE);

    check_errors();
}

bool request_with_state::cancel()
{
    STXXL_VERBOSE3_THIS("request_with_state::cancel() "
                        << file_ << " " << buffer_ << " " << offset_);

    if (file_)
    {
        request_ptr rp(this);
        if (disk_queues::get_instance()->cancel_request(rp, file_->get_queue_id()))
        {
            state_.set_to(DONE);
            notify_waiters();
            file_->delete_request_ref();
            file_ = 0;
            state_.set_to(READY2DIE);
            return true;
        }
    }
    return false;
}

bool request_with_state::poll()
{
    const request_state s = state_();

    check_errors();

    return s == DONE || s == READY2DIE;
}

void request_with_state::completed(bool canceled)
{
    STXXL_VERBOSE3_THIS("request_with_state::completed()");
    state_.set_to(DONE);
    if (!canceled) {
        if (on_complete_)
            on_complete_(this);
    }
    notify_waiters();
    file_->delete_request_ref();
    file_ = 0;
    state_.set_to(READY2DIE);
}

} // namespace stxxl

// vim: et:ts=4:sw=4
