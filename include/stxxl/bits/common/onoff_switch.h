/***************************************************************************
 *  include/stxxl/bits/common/onoff_switch.h
 *
 *  Kind of binary semaphore: initially OFF, then multiple waiters can attach
 *  to the switch, which get notified one-by-one when switched ON.
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_ONOFF_SWITCH_HEADER
#define STXXL_COMMON_ONOFF_SWITCH_HEADER

#include <condition_variable>
#include <mutex>

namespace stxxl {

class onoff_switch
{
    //! mutex for condition variable
    std::mutex mutex_;

    //! condition variable
    std::condition_variable cond_;

    //! the switch's state
    bool on_;

public:
    //! construct switch
    explicit onoff_switch(bool flag = false)
        : on_(flag)
    { }

    //! non-copyable: delete copy-constructor
    onoff_switch(const onoff_switch&) = delete;
    //! non-copyable: delete assignment operator
    onoff_switch& operator = (const onoff_switch&) = delete;

    //! turn switch ON and notify one waiter
    void on()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        on_ = true;
        cond_.notify_one();
    }

    //! turn switch OFF and notify one waiter
    void off()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        on_ = false;
        cond_.notify_one();
    }

    //! wait for switch to turn ON
    void wait_for_on()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!on_)
            cond_.wait(lock);
    }

    //! wait for switch to turn OFF
    void wait_for_off()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (on_)
            cond_.wait(lock);
    }

    //! return true if switch is ON
    bool is_on()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return on_;
    }
};

} // namespace stxxl

#endif // !STXXL_COMMON_ONOFF_SWITCH_HEADER
