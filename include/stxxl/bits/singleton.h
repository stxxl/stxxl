/***************************************************************************
 *  include/stxxl/bits/singleton.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SINGLETON_HEADER
#define STXXL_SINGLETON_HEADER

#include <stxxl/bits/common/exithandler.h>
#include <stxxl/types>

#include <cstdlib>
#include <mutex>

namespace stxxl {

template <typename INSTANCE, bool destroy_on_exit = true>
class singleton
{
    typedef INSTANCE instance_type;
    typedef instance_type* instance_pointer;
    typedef volatile instance_pointer volatile_instance_pointer;

    static volatile_instance_pointer instance;

    static instance_pointer create_instance();
    static void destroy_instance();

public:
    singleton() = default;

    //! non-copyable: delete copy-constructor
    singleton(const singleton&) = delete;
    //! non-copyable: delete assignment operator
    singleton& operator = (const singleton&) = delete;

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
    static std::mutex create_mutex;
    std::unique_lock<std::mutex> instance_write_lock(create_mutex);
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
    instance_pointer inst = instance;
    instance = reinterpret_cast<instance_pointer>(size_t(-1));     // bomb if used again
    delete inst;
}

template <typename INSTANCE, bool destroy_on_exit>
typename singleton<INSTANCE, destroy_on_exit>::volatile_instance_pointer
singleton<INSTANCE, destroy_on_exit>::instance = NULL;

} // namespace stxxl

#endif // !STXXL_SINGLETON_HEADER
