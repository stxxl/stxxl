/***************************************************************************
 *  include/stxxl/bits/common/semaphore.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_SEMAPHORE_HEADER
#define STXXL_COMMON_SEMAPHORE_HEADER

#include <condition_variable>
#include <mutex>

namespace stxxl {

class semaphore
{
public:
    //! construct semaphore
    explicit semaphore(size_t init_value = 1)
        : value_(init_value)
    { }

    //! non-copyable: delete copy-constructor
    semaphore(const semaphore&) = delete;
    //! non-copyable: delete assignment operator
    semaphore& operator = (const semaphore&) = delete;
    //! move-constructor: just move the value
    semaphore(semaphore&& s) : value_(s.value_) { }

    //! function increments the semaphore and signals any threads that are
    //! blocked waiting a change in the semaphore
    size_t signal()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        size_t res = ++value_;
        cv_.notify_one();
        return res;
    }

    //! function increments the semaphore and signals any threads that are
    //! blocked waiting a change in the semaphore
    size_t signal(size_t delta)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        size_t res = (value_ += delta);
        cv_.notify_all();
        return res;
    }

    //! function decrements the semaphore and blocks if the semaphore is <= 0
    //! until another thread signals a change
    size_t wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (value_ <= 0)
            cv_.wait(lock);
        return --value_;
    }

    //! return the current value -- should only be used for debugging.
    size_t value() const { return value_; }

private:
    //! value of the semaphore
    size_t value_;

    //! mutex for condition variable
    std::mutex mutex_;

    //! condition variable
    std::condition_variable cv_;
};

} // namespace stxxl

#endif // !STXXL_COMMON_SEMAPHORE_HEADER
