/***************************************************************************
 *  include/stxxl/bits/common/shared_state.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_SHARED_STATE_HEADER
#define STXXL_COMMON_SHARED_STATE_HEADER

#include <condition_variable>
#include <mutex>

namespace stxxl {

template <typename ValueType = size_t>
class shared_state
{
    typedef ValueType value_type;

    //! mutex for condition variable
    std::mutex mutex_;

    //! condition variable
    std::condition_variable cv_;

    //! current shared_state
    value_type state_;

public:
    explicit shared_state(const value_type& s)
        : state_(s) { }

    //! non-copyable: delete copy-constructor
    shared_state(const shared_state&) = delete;
    //! non-copyable: delete assignment operator
    shared_state& operator = (const shared_state&) = delete;

    void set_to(const value_type& new_state)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        state_ = new_state;
        lock.unlock();
        cv_.notify_all();
    }

    void wait_for(const value_type& needed_state)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (needed_state != state_)
            cv_.wait(lock);
    }

    value_type operator () ()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return state_;
    }
};

} // namespace stxxl

#endif // !STXXL_COMMON_SHARED_STATE_HEADER
