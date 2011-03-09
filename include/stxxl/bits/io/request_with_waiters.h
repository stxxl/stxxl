/***************************************************************************
 *  include/stxxl/bits/io/request_with_waiters.h
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

#ifndef STXXL_IO__REQUEST_WITH_WAITERS_H_
#define STXXL_IO__REQUEST_WITH_WAITERS_H_

#include <set>

#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/switch.h>
#include <stxxl/bits/io/request_interface.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Request that is aware of threads waiting for it to complete.
class request_with_waiters : virtual public request_interface
{
    mutex waiters_mutex;
    std::set<onoff_switch *> waiters;

protected:
    bool add_waiter(onoff_switch * sw);
    void delete_waiter(onoff_switch * sw);
    void notify_waiters();
    /*
    int nwaiters();             // returns number of waiters
    */
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO__REQUEST_WITH_WAITERS_H_
// vim: et:ts=4:sw=4
