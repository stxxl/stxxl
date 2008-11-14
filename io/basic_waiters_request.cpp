/***************************************************************************
 *  io/basic_waiters_request.cpp
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

#include <stxxl/bits/io/basic_waiters_request.h>
#include <stxxl/bits/parallel.h>


__STXXL_BEGIN_NAMESPACE

basic_waiters_request::basic_waiters_request(
    completion_handler on_cmpl,
    file * f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    request_type t) :
    request(on_cmpl, f, buf, off, b, t)
{ }

basic_waiters_request::~basic_waiters_request()
{ }

bool basic_waiters_request::add_waiter(onoff_switch * sw)
{
    if (poll())                     // request already finished
    {
        return true;
    }

    scoped_mutex_lock lock(waiters_mutex);
    waiters.insert(sw);

    return false;
}

void basic_waiters_request::delete_waiter(onoff_switch * sw)
{
    scoped_mutex_lock lock(waiters_mutex);
    waiters.erase(sw);
}

void basic_waiters_request::notify_waiters()
{
    scoped_mutex_lock lock(waiters_mutex);
    std::for_each(waiters.begin(),
                  waiters.end(),
                  std::mem_fun(&onoff_switch::on)
                  __STXXL_FORCE_SEQUENTIAL);
}

/*
int basic_waiters_request::nwaiters()
{
    scoped_mutex_lock lock(waiters_mutex);
    return waiters.size();
}
*/

const char * basic_waiters_request::io_type() const
{
    return "basic_waiters";
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
