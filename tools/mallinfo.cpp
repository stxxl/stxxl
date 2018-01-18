/***************************************************************************
 *  tools/mallinfo.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <tlx/logger.hpp>

#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/config.h>

#if STXXL_HAVE_MALLINFO_PROTO

#include <malloc.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>

#include <iostream>

void print_malloc_stats()
{
    struct mallinfo info = mallinfo();
    LOG1 << "MALLOC statistics BEGIN";
    LOG1 << "===============================================================";
    LOG1 << "non-mmapped space allocated from system (bytes): " << info.arena;
    LOG1 << "number of free chunks                          : " << info.ordblks;
    LOG1 << "number of fastbin blocks                       : " << info.smblks;
    LOG1 << "number of chunks allocated via mmap()          : " << info.hblks;
    LOG1 << "total number of bytes allocated via mmap()     : " << info.hblkhd;
    LOG1 << "maximum total allocated space (bytes)          : " << info.usmblks;
    LOG1 << "space available in freed fastbin blocks (bytes): " << info.fsmblks;
    LOG1 << "number of bytes allocated and in use           : " << info.uordblks;
    LOG1 << "number of bytes allocated but not in use       : " << info.fordblks;
    LOG1 << "top-most, releasable (via malloc_trim) space   : " << info.keepcost;
    LOG1 << "================================================================";
}

int do_mallinfo(int argc, char* argv[])
{
    // parse command line
    stxxl::cmdline_parser cp;

    cp.set_description(
        "Allocate some memory and mlock() it to consume physical memory. "
        "Needs to run as root to block more than 64 KiB in default settings.");

    size_t M;
    cp.add_param_bytes("size", M,
                       "Amount of memory to allocate (e.g. 1GiB)");

    if (!cp.process(argc, argv))
        return -1;

    sbrk(128 * 1024 * 1024);

    std::cout << "Nothing allocated" << std::endl;
    print_malloc_stats();

    char* ptr = new char[M];

    std::cout << "Allocated " << M << " bytes" << std::endl;
    print_malloc_stats();

    memset(ptr, 42, M);

    std::cout << "Filled " << M << " bytes" << std::endl;
    print_malloc_stats();

    delete[] ptr;

    std::cout << "Deallocated " << std::endl;
    print_malloc_stats();

    return 0;
}

#else // !STXXL_HAVE_MALLINFO_PROTO

int do_mallinfo(int, char*[])
{
    LOG1 << "Sorry, mallinfo() statistics are not supported on this platform.";
    return -1;
}

#endif // STXXL_HAVE_MALLINFO_PROTO
