/***************************************************************************
 *  common/stxxl_info.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright © 2007, 2009-2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/mng>
#include <stxxl/bits/compat/shared_ptr.h>

int main(int argc, char **)
{
    stxxl::config::get_instance();
    stxxl::block_manager::get_instance();
    stxxl::stats::get_instance();
    stxxl::disk_queues::get_instance();
#ifdef STXXL_PARALLEL_MODE
#ifdef _GLIBCXX_PARALLEL
    STXXL_MSG("_GLIBCXX_PARALLEL, max threads = " << omp_get_max_threads());
#else
    STXXL_MSG("STXXL_PARALLEL_MODE, max threads = " << omp_get_max_threads());
#endif
#elif defined(__MCSTL__)
    STXXL_MSG("__MCSTL__, max threads = " << omp_get_max_threads());
#endif
    STXXL_MSG("sizeof(unsigned int)   = " << sizeof(unsigned int));
    STXXL_MSG("sizeof(unsigned_type)  = " << sizeof(stxxl::unsigned_type));
    STXXL_MSG("sizeof(uint64)         = " << sizeof(stxxl::uint64));
    STXXL_MSG("sizeof(size_t)         = " << sizeof(size_t));
    STXXL_MSG("sizeof(off_t)          = " << sizeof(off_t));
    STXXL_MSG("sizeof(void*)          = " << sizeof(void *));

#if defined(STXXL_HAVE_AIO_FILE)
    STXXL_MSG("STXXL_HAVE_AIO_FILE    = " << STXXL_HAVE_AIO_FILE);
#endif
    STXXL_MSG("STXXL_HAVE_SHARED_PTR  = " << STXXL_HAVE_SHARED_PTR);
    STXXL_MSG("STXXL_HAVE_MAKE_SHARED = " << STXXL_HAVE_MAKE_SHARED);

    assert(argc < 3);          // give two extra arguments to check whether assertions are enabled
    return argc != 2 ? 0 : -1; // give one extra argument to get exit code -1
}
