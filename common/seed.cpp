/***************************************************************************
 *  common/seed.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cassert>
#include <ctime>
#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif
#ifdef BOOST_MSVC
 #include <io.h>
 #include <windows.h>
#else
 #include <unistd.h>
 #include <sys/time.h>
#endif

#include "stxxl/bits/common/seed.h"
#include "stxxl/bits/common/mutex.h"


__STXXL_BEGIN_NAMESPACE

inline unsigned initial_seed();

struct seed_generator_t {
    unsigned seed;
#ifdef STXXL_BOOST_THREADS
    boost::mutex mtx;
#else
    mutex mtx;
#endif

    seed_generator_t(unsigned s) : seed(s)
    { }
};

seed_generator_t & seed_generator()
{
    static seed_generator_t sg(initial_seed());
    return sg;
}

inline unsigned initial_seed()
{
    static bool initialized = false;
    assert(!initialized); // this should only be called once!

    initialized = true;
#ifdef BOOST_MSVC
    // GetTickCount():  ms since system start
    return GetTickCount() ^ GetCurrentProcessId();
#else
    struct timeval tv;
    gettimeofday(&tv, 0);

    return tv.tv_sec ^ tv.tv_usec ^ (getpid() << 16);
#endif
}

void set_seed(unsigned seed)
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(seed_generator().mtx);
    seed_generator().seed = seed;
#else
    seed_generator().mtx.lock();
    seed_generator().seed = seed;
    seed_generator().mtx.unlock();
#endif
}

unsigned get_next_seed()
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(seed_generator().mtx);
    return ++(seed_generator().seed);
#else
    seed_generator().mtx.lock();
    unsigned seed = seed_generator().seed++;
    seed_generator().mtx.unlock();
    return seed;
#endif
}

__STXXL_END_NAMESPACE
