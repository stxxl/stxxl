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

#ifndef STXXL_IO_REQUEST_WITH_STATE_HEADER
#define STXXL_IO_REQUEST_WITH_STATE_HEADER

#include <stxxl/bits/common/shared_state.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/request_with_waiters.h>

namespace stxxl {

//! \addtogroup reqlayer
//! \{

//! Request with completion shared_state.
class request_with_state : public request_with_waiters
{
protected:
    //! states of request.
    //! OP - operating, DONE - request served, READY2DIE - can be destroyed
    enum request_state { OP = 0, DONE = 1, READY2DIE = 2 };

    shared_state<request_state> state_;

protected:
    request_with_state(
        const completion_handler& on_complete,
        file* file, void* buffer, offset_type offset, size_type bytes,
        read_or_write op)
        : request_with_waiters(on_complete, file, buffer, offset, bytes, op),
          state_(OP)
    { }

public:
    virtual ~request_with_state();
    void wait(bool measure_time = true) final;
    bool poll() final;
    bool cancel() override;

protected:
    void completed(bool canceled) override;
};

//! \}

} // namespace stxxl

#endif // !STXXL_IO_REQUEST_WITH_STATE_HEADER
// vim: et:ts=4:sw=4
