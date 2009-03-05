/***************************************************************************
 *  include/stxxl/bits/io/request_state_impl_basic.h
 *
 *  UNIX file system file base
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_HEADER_IO_REQUEST_STATE_IMPL_BASIC
#define STXXL_HEADER_IO_REQUEST_STATE_IMPL_BASIC

#include <stxxl/bits/common/state.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/request_waiters_impl_basic.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Basic state implemenatition for most request implementations
class request_state_impl_basic : public request, public request_waiters_impl_basic
{
protected:
    //! states of request
    //! OP - operating, DONE - request served, READY2DIE - can be destroyed
    enum request_state { OP = 0, DONE = 1, READY2DIE = 2 };

    state<request_state> _state;

protected:
    request_state_impl_basic(
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
    virtual ~request_state_impl_basic();
    void wait();
    bool poll();
    void cancel();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_HEADER_IO_REQUEST_STATE_IMPL_BASIC
// vim: et:ts=4:sw=4
