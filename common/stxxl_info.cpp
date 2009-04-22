/***************************************************************************
 *  common/stxxl_info.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright © 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/mng>

int main(int argc, char **)
{
    stxxl::config::get_instance();
    stxxl::block_manager::get_instance();
    stxxl::stats::get_instance();
    stxxl::disk_queues::get_instance();
#ifdef _GLIBCXX_PARALLEL
    STXXL_MSG("_GLIBCXX_PARALLEL, max threads = " << omp_get_max_threads());
#endif
#ifdef __MCSTL__
    STXXL_MSG("__MCSTL__, max threads = " << omp_get_max_threads());
#endif
    STXXL_MSG("sizeof(unsigned int)   = " << sizeof(unsigned int));
    STXXL_MSG("sizeof(unsigned_type)  = " << sizeof(stxxl::unsigned_type));
    STXXL_MSG("sizeof(uint64)         = " << sizeof(stxxl::uint64));
    STXXL_MSG("sizeof(void*)          = " << sizeof(void*));
    return argc == 1 ? 0 : -1;
}
