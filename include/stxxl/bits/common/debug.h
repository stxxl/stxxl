/***************************************************************************
 *            debug.h
 *
 *  Thu Dec 30 14:52:00 2004
 *  Copyright  2004  Roman Dementiev
 *  Email dementiev@ira.uka.de
 ****************************************************************************/

#ifndef STXXL_DEBUG_HEADER
#define STXXL_DEBUG_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include "stxxl/bits/namespace.h"
#include "stxxl/bits/common/mutex.h"
#include <stxxl/bits/compat_hash_map.h>


__STXXL_BEGIN_NAMESPACE

class debugmon
{
    struct tag
    {
        bool ongoing;
        char * end;
        size_t size;
    };
    struct hash_fct
    {
        inline size_t operator ()  (char * arg) const
        {
            return long (arg);
        }
#ifdef BOOST_MSVC
        bool operator () (char * a, char * b) const
        {
            return (long (a) < long (b));
        }
        enum
        {               // parameters for hash table
            bucket_size = 4,                    // 0 < bucket_size
            min_buckets = 8
        };              // min_buckets = 2 ^^ N, 0 < N
#endif
    };

    compat_hash_map<char *, tag, hash_fct>::result tags;

#ifdef STXXL_BOOST_THREADS
    boost::mutex mutex1;
#else
    mutex mutex1;
#endif

    static debugmon * instance;

    inline debugmon() { }

public:
    void block_allocated(char * ptr, char * end, size_t size);
    void block_deallocated(char * ptr);
    void io_started(char * ptr);
    void io_finished(char * ptr);

    inline static debugmon * get_instance ()
    {
        if (!instance)
            instance = new debugmon ();

        return instance;
    }
};

__STXXL_END_NAMESPACE

#endif // !STXXL_DEBUG_HEADER
