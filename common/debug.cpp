/***************************************************************************
 *  common/debug.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2004 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cassert>
#include <stxxl/bits/common/debug.h>
#include <stxxl/bits/verbose.h>


__STXXL_BEGIN_NAMESPACE

#ifdef STXXL_DEBUGMON
void debugmon::block_allocated(void * ptr, void * end, size_t size)
{
    scoped_mutex_lock Lock(mutex1);

// checks are here
    STXXL_VERBOSE1("debugmon: block " << ptr << " allocated");
    assert(tags.find(ptr) == tags.end());             // not allocated
    tag t;
    t.ongoing = false;
    t.end = end;
    t.size = size;
    tags[ptr] = t;
}

void debugmon::block_deallocated(void * ptr)
{
    scoped_mutex_lock Lock(mutex1);

    STXXL_VERBOSE1("debugmon: block_deallocated from " << ptr);
    assert(tags.find(ptr) != tags.end());       // allocated
    tag t = tags[ptr];
    assert(t.ongoing == false);                 // not ongoing
    tags.erase(ptr);
    size_t size = t.size;
    STXXL_VERBOSE1("debugmon: block_deallocated to " << t.end);
    char * endptr = (char *)t.end;
    char * ptr1 = (char *)ptr;
    ptr1 += size;
    while (ptr1 < endptr)
    {
        STXXL_VERBOSE1("debugmon: block_deallocated next " << ptr1);
        assert(tags.find(ptr1) != tags.end());  // allocated
        tag t = tags[ptr1];
        assert(t.ongoing == false);             // not ongoing
        assert(t.size == size);                 // chunk size
        assert(t.end == endptr);                // array end address
        tags.erase(ptr1);
        ptr1 += size;
    }
}

void debugmon::io_started(void * ptr)
{
    scoped_mutex_lock Lock(mutex1);

    STXXL_VERBOSE1("debugmon: I/O on block " << ptr << " started");
    assert(tags.find(ptr) != tags.end());       // allocated
    tag t = tags[ptr];
    //assert(t.ongoing == false); // not ongoing
    if (t.ongoing == true)
        STXXL_ERRMSG("debugmon: I/O on block " << ptr << " started, but block is already busy");
    t.ongoing = true;

    tags[ptr] = t;
}

void debugmon::io_finished(void * ptr)
{
    scoped_mutex_lock Lock(mutex1);

    STXXL_VERBOSE1("debugmon: I/O on block " << ptr << " finished");
    assert(tags.find(ptr) != tags.end());       // allocated
    tag t = tags[ptr];
    //assert(t.ongoing == true); // ongoing
    if (t.ongoing == false)
        STXXL_ERRMSG("debugmon: I/O on block " << ptr << " finished, but block was not busy");
    t.ongoing = false;

    tags[ptr] = t;
}
#endif

__STXXL_END_NAMESPACE
