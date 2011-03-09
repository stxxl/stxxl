/***************************************************************************
 *  io/request_with_waiters.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/request_with_waiters.h>
#include <algorithm>
#include <stxxl/bits/parallel.h>


__STXXL_BEGIN_NAMESPACE

bool request_waiters_impl_basic::add_waiter(onoff_switch * sw)
{
    // this lock needs to be obtained before poll(), otherwise a race
    // condition might occur: the state might change and notify_waiters()
    // could be called between poll() and insert() resulting in waiter sw
    // never being notified
    scoped_mutex_lock lock(waiters_mutex);

    if (poll())                     // request already finished
    {
        return true;
    }

    waiters.insert(sw);

    return false;
}

void request_waiters_impl_basic::delete_waiter(onoff_switch * sw)
{
    scoped_mutex_lock lock(waiters_mutex);
    waiters.erase(sw);
}

void request_waiters_impl_basic::notify_waiters()
{
    scoped_mutex_lock lock(waiters_mutex);
    std::for_each(waiters.begin(),
                  waiters.end(),
                  std::mem_fun(&onoff_switch::on)
                  _STXXL_FORCE_SEQUENTIAL);
}

/*
int request_waiters_impl_basic::nwaiters()
{
    scoped_mutex_lock lock(waiters_mutex);
    return waiters.size();
}
*/

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
