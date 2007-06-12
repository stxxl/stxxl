#include "stxxl/bits/common/debug.h"


__STXXL_BEGIN_NAMESPACE

debugmon * debugmon::instance = NULL;

#ifndef STXXL_DEBUGMON
void debugmon::block_allocated(char * /*ptr*/, char * /*end*/, size_t /*size*/)
{ }
void debugmon::block_deallocated(char * /*ptr*/)
{ }
void debugmon::io_started(char * /*ptr*/)
{ }
void debugmon::io_finished(char * /*ptr*/)
{ }
#else
void debugmon::block_allocated(char * ptr, char * end, size_t size)
{
 #ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(mutex1);
 #else
    mutex1.lock();
 #endif
// checks are here
STXXL_VERBOSE1("debugmon: block " << long (ptr) << " allocated")
assert(tags.find(ptr) == tags.end());                 // not allocated
tag t;
t.ongoing = false;
t.end = end;
t.size = size;
tags[ptr] = t;
 #ifndef STXXL_BOOST_THREADS
mutex1.unlock();
 #endif
}
void debugmon::block_deallocated(char * ptr)
{
 #ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(mutex1);
 #else
    mutex1.lock();
 #endif

    STXXL_VERBOSE1("debugmon: block_deallocated from " << long (ptr))
    assert(tags.find(ptr) != tags.end());             // allocated
    tag t = tags[ptr];
    assert(t.ongoing == false);             // not ongoing
    tags.erase(ptr);
    size_t size = t.size;
    STXXL_VERBOSE1("debugmon: block_deallocated to " << long (t.end))
    char * endptr = (char *) t.end;
    char * ptr1 = (char *) ptr;
    ptr1 += size;
    while (ptr1 < endptr)
    {
        STXXL_VERBOSE1("debugmon: block_deallocated next " << long (ptr1))
        assert(tags.find(ptr1) != tags.end());                 // allocated
        tag t = tags[ptr1];
        assert(t.ongoing == false);                 // not ongoing
        assert(t.size == size);                 // chunk size
        assert(t.end == endptr);                 // array end address
        tags.erase(ptr1);
        ptr1 += size;
    }
 #ifndef STXXL_BOOST_THREADS
    mutex1.unlock();
 #endif
}
void debugmon::io_started(char * ptr)
{
 #ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(mutex1);
 #else
    mutex1.lock();
 #endif

    STXXL_VERBOSE1("debugmon: I/O on block " << long (ptr) << " started")
    assert(tags.find(ptr) != tags.end());             // allocated
    tag t = tags[ptr];
    //assert(t.ongoing == false); // not ongoing
    if (t.ongoing == true)
        STXXL_ERRMSG("debugmon: I/O on block " << long (ptr) << " started, but block is already busy")
        t.ongoing = true;

    tags[ptr] = t;

 #ifndef STXXL_BOOST_THREADS
    mutex1.unlock();
 #endif
}
void debugmon::io_finished(char * ptr)
{
 #ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(mutex1);
 #else
    mutex1.lock();
 #endif

    STXXL_VERBOSE1("debugmon: I/O on block " << long (ptr) << " finished")
    assert(tags.find(ptr) != tags.end());             // allocated
    tag t = tags[ptr];
    //assert(t.ongoing == true); // ongoing
    if (t.ongoing == false)
        STXXL_ERRMSG("debugmon: I/O on block " << long (ptr) << " finished, but block was not busy")
        t.ongoing = false;

    tags[ptr] = t;

 #ifndef STXXL_BOOST_THREADS
    mutex1.unlock();
 #endif
}
#endif


__STXXL_END_NAMESPACE
