/***************************************************************************
 *  tools/mlock.cpp
 *
 *  Allocate some memory and mlock() it to consume physical memory.
 *  Needs to run as root to block more than 64 KiB in default settings.
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <stxxl/cmdline>

int do_mlock(int argc, char * argv[])
{
    // parse command line
    stxxl::cmdline_parser cp;

    cp.set_description("Allocate some memory and mlock() it to consume physical memory. "
                       "Needs to run as root to block more than 64 KiB in default settings."
        );
    cp.set_author("Andreas Beckmann <beckmann@cs.uni-frankfurt.de>");

    stxxl::uint64 M;
    cp.add_param_bytes("size", "Amount of memory to allocate (e.g. 4GiB)", M);

    if (!cp.process(argc,argv))
        return -1;

    // allocate and fill
    char * c = (char *)malloc(M);
    memset(c, 42, M);

    if (mlock(c, M) == 0)
    {
        std::cout << "mlock(" << (void*)c << ", " << M << ") successful, press Ctrl-C to exit." << std::endl;
        while (1)
            sleep(86400);
    }
    else {
        std::cerr << "mlock(" << (void*)c << ", " << M << ") failed: " << strerror(errno) << std::endl;
        return 1;
    }
}
// vim: et:ts=4:sw=4
