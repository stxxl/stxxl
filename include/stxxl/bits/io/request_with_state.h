/***************************************************************************
 *  include/stxxl/bits/io/request_with_state.h
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

#ifndef STXXL_IO__REQUEST_WITH_STATE_H_
#define STXXL_IO__REQUEST_WITH_STATE_H_

#include <stxxl/bits/common/state.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/request_with_waiters.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Request with completion state.
class request_with_state : public request, public request_with_waiters
{
protected:
    //! states of request
    //! OP - operating, DONE - request served, READY2DIE - can be destroyed
    enum request_state { OP = 0, DONE = 1, READY2DIE = 2 };

    state<request_state> _state;

protected:
    request_with_state(
        const completion_handler & on_cmpl,
        file * f,
        void * buf,
        offset_type off,
        size_type b,
        request_type t) :
        request(on_cmpl, f, buf, off, b, t),
        _state(OP)
    { }

public:
    virtual ~request_with_state();
    void wait(bool measure_time = true);
    bool poll();
    bool cancel();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO__REQUEST_WITH_STATE_H_
// vim: et:ts=4:sw=4
