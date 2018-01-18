/***************************************************************************
 *  tools/stxxl_tool.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007, 2009-2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <algorithm>
#include <iostream>

#include <tlx/logger.hpp>

#include <foxxll/common/utils.hpp>
#include <foxxll/io.hpp>
#include <foxxll/mng.hpp>

#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/version.h>

int stxxl_info(int, char**)
{
    foxxll::config::get_instance();
    foxxll::block_manager::get_instance();
    foxxll::stats::get_instance();
    foxxll::disk_queues::get_instance();

#if STXXL_PARALLEL
    LOG1 << "STXXL_PARALLEL, max threads = " << omp_get_max_threads();
#endif
    LOG1 << "sizeof(unsigned int)   = " << sizeof(unsigned int);
    LOG1 << "sizeof(uint64_t)       = " << sizeof(uint64_t);
    LOG1 << "sizeof(long)           = " << sizeof(long);
    LOG1 << "sizeof(size_t)         = " << sizeof(size_t);
    LOG1 << "sizeof(off_t)          = " << sizeof(off_t);
    LOG1 << "sizeof(void*)          = " << sizeof(void*);

#if defined(STXXL_HAVE_LINUXAIO_FILE)
    LOG1 << "STXXL_HAVE_LINUXAIO_FILE = " << STXXL_HAVE_LINUXAIO_FILE;
#endif

    return 0;
}

extern int benchmark_sort(int argc, char* argv[]);
extern int benchmark_pqueue(int argc, char* argv[]);
extern int do_mlock(int argc, char* argv[]);
extern int do_mallinfo(int argc, char* argv[]);

struct SubTool
{
    const char* name;
    int (* func)(int argc, char* argv[]);
    bool shortline;
    const char* description;
};

struct SubTool subtools[] = {
    {
        "info", &stxxl_info, false,
        "Print out information about the build system and which optional "
        "modules where compiled into STXXL."
    },
    {
        "benchmark_sort", &benchmark_sort, false,
        "Run benchmark tests of different sorting methods in STXXL"
    },
    {
        "benchmark_pqueue", &benchmark_pqueue, false,
        "Benchmark priority queue implementation using sequence of operations."
    },
    {
        "mlock", &do_mlock, true,
        "Lock physical memory."
    },
    {
        "mallinfo", &do_mallinfo, true,
        "Show mallinfo statistics."
    },
    { nullptr, nullptr, false, nullptr }
};

int main_usage(const char* arg0)
{
    LOG1 << stxxl::get_version_string_long();

    std::cout << "Usage: " << arg0 << " <subtool> ..." << std::endl
              << "Available subtools: " << std::endl;

    int shortlen = 0;

    for (unsigned int i = 0; subtools[i].name; ++i)
    {
        if (!subtools[i].shortline) continue;
        shortlen = std::max(shortlen, (int)strlen(subtools[i].name));
    }

    for (unsigned int i = 0; subtools[i].name; ++i)
    {
        if (subtools[i].shortline) continue;
        std::cout << "  " << subtools[i].name << std::endl;
        stxxl::cmdline_parser::output_wrap(
            std::cout, subtools[i].description, 80, 6, 6);
        std::cout << std::endl;
    }

    for (unsigned int i = 0; subtools[i].name; ++i)
    {
        if (!subtools[i].shortline) continue;
        std::cout << "  " << std::left << std::setw(shortlen + 2)
                  << subtools[i].name << subtools[i].description << std::endl;
    }
    std::cout << std::endl;

    return 0;
}

int main(int argc, char** argv)
{
    char progsub[256];

    if (stxxl::check_library_version() != 0)
        LOG1 << "version mismatch between headers and library";

    if (argc > 1)
    {
        for (unsigned int i = 0; subtools[i].name; ++i)
        {
            if (strcmp(subtools[i].name, argv[1]) == 0)
            {
                // replace argv[1] with call string of subtool.
                snprintf(progsub, sizeof(progsub), "%s %s", argv[0], argv[1]);
                argv[1] = progsub;
                return subtools[i].func(argc - 1, argv + 1);
            }
        }
        std::cout << "Unknown subtool '" << argv[1] << "'" << std::endl;
    }

    return main_usage(argv[0]);
}
