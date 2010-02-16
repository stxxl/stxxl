/***************************************************************************
 *  utils/malloc.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>
#if !defined(__APPLE__) && !defined(__FreeBSD__)
#include <malloc.h>
#endif
#include <cstdlib>
#include <stxxl/bits/verbose.h>


void print_malloc_stats()
{
#if !defined(__APPLE__) && !defined(__FreeBSD__)
    struct mallinfo info = mallinfo();
    STXXL_MSG("MALLOC statistics BEGIN");
    STXXL_MSG("===============================================================");
    STXXL_MSG("non-mmapped space allocated from system (bytes): " << info.arena);
    STXXL_MSG("number of free chunks                          : " << info.ordblks);
    STXXL_MSG("number of fastbin blocks                       : " << info.smblks);
    STXXL_MSG("number of chunks allocated via mmap()          : " << info.hblks);
    STXXL_MSG("total number of bytes allocated via mmap()     : " << info.hblkhd);
    STXXL_MSG("maximum total allocated space (bytes)          : " << info.usmblks);
    STXXL_MSG("space available in freed fastbin blocks (bytes): " << info.fsmblks);
    STXXL_MSG("number of bytes allocated and in use           : " << info.uordblks);
    STXXL_MSG("number of bytes allocated but not in use       : " << info.fordblks);
    STXXL_MSG("top-most, releasable (via malloc_trim) space   : " << info.keepcost);
    STXXL_MSG("================================================================");
#else
    STXXL_MSG("MALLOC statistics are not supported on this platform");
#endif
}

int main(int argc, char * argv[])
{
    using std::cin;
    using std::cout;
    using std::cerr;
    using std::endl;

    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " bytes_to_allocate" << endl;
        return -1;
    }
    sbrk(128 * 1024 * 1024);
    cout << "Nothing allocated" << endl;
    print_malloc_stats();
    const unsigned bytes = atoi(argv[1]);
    char * ptr = new char[bytes];
    cout << "Allocated " << bytes << " bytes" << endl;
    print_malloc_stats();
    delete[] ptr;
    cout << "Deallocated " << endl;
    print_malloc_stats();
}
