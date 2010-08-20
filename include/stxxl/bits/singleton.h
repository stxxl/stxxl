/***************************************************************************
 *  include/stxxl/bits/singleton.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SINGLETON_HEADER
#define STXXL_SINGLETON_HEADER

#include <cstdlib>

#include <stxxl/types>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/exithandler.h>


__STXXL_BEGIN_NAMESPACE

template <typename INSTANCE, bool destroy_on_exit = true>
class singleton : private noncopyable
{
    typedef INSTANCE instance_type;
    typedef instance_type * instance_pointer;

    static instance_pointer instance;
    static mutex instance_mutex; // prevent concurrent writes only

    static instance_pointer create_instance();
    static void destroy_instance();

public:
    inline static instance_pointer get_instance()
    {
        if (!instance)
            return create_instance();

        return instance;
    }
};

template <typename INSTANCE, bool destroy_on_exit>
typename singleton<INSTANCE, destroy_on_exit>::instance_pointer
singleton<INSTANCE, destroy_on_exit>::create_instance()
{
    scoped_mutex_lock instance_write_lock(instance_mutex);
    if (!instance) {
        instance = new instance_type();
        if (destroy_on_exit)
            register_exit_handler(destroy_instance);
    }
    return instance;
}

template <typename INSTANCE, bool destroy_on_exit>
void singleton<INSTANCE, destroy_on_exit>::destroy_instance()
{
    scoped_mutex_lock instance_write_lock(instance_mutex);
    instance_pointer inst = instance;
    //instance = NULL;
    instance = reinterpret_cast<instance_pointer>(unsigned_type(-1));     // bomb if used again
    delete inst;
}

template <typename INSTANCE, bool destroy_on_exit>
INSTANCE * singleton<INSTANCE, destroy_on_exit>::instance = NULL;

template <typename INSTANCE, bool destroy_on_exit>
mutex singleton<INSTANCE, destroy_on_exit>::instance_mutex;

__STXXL_END_NAMESPACE

#endif // !STXXL_SINGLETON_HEADER
