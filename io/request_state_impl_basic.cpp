/***************************************************************************
 *  io/request_state_impl_basic.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2005, 2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/request_state_impl_basic.h>


__STXXL_BEGIN_NAMESPACE


request_state_impl_basic::~request_state_impl_basic()
{
    STXXL_VERBOSE3("basic_request_state " << static_cast<void *>(this) << ": deletion, cnt: " << ref_cnt);

    assert(_state() == DONE || _state() == READY2DIE);

    // if(_state() != DONE && _state()!= READY2DIE )
    //	STXXL_ERRMSG("WARNING: serious stxxl error request being deleted while I/O did not finish "<<
    //		"! Please report it to the stxxl author(s) <dementiev@mpi-sb.mpg.de>");

    // _state.wait_for (READY2DIE); // does not make sense ?
}

void request_state_impl_basic::wait(bool measure_time)
{
    STXXL_VERBOSE3("ufs_request_base : " << static_cast<void *>(this) << " wait ");

    stats::scoped_wait_timer wait_timer(measure_time);

    _state.wait_for(READY2DIE);

    check_errors();
}

bool request_state_impl_basic::poll()
{
    const request_state s = _state();

    check_errors();

    return s == DONE || s == READY2DIE;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
