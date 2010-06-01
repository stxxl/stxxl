/***************************************************************************
 *  utils/mlock.cpp
 *
 *  Allocate some memory and mlock() it to consume physical memory.
 *  Needs to run as root to block more than 64 KiB in default settings.
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/mman.h>

int main(int argc, char ** argv)
{
    if (argc == 2) {
        long M = atol(argv[1]);
        if (M > 0) {
            char * c = (char *)malloc(M);
            for (long i = 0; i < M; ++i)
                c[i] = 42;
            if (mlock(c, M) == 0) {
                std::cout << "mlock(, " << M << ") successful, press Ctrl-C to finish" << std::endl;
                while (1)
                    sleep(86400);
            } else {
                std::cerr << "mlock(, " << M << ") failed!" << std::endl;
                return 1;
            }
        }
    } else {
        std::cout << "Usage: " << argv[0] << " <bytes>" << std::endl;
    }
}

// vim: et:ts=4:sw=4
