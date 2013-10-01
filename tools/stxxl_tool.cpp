/***************************************************************************
 *  tools/stxxl_tool.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright © 2007, 2009-2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/mng>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/common/cmdline.h>
#include <stxxl/version.h>

int stxxl_info(int, char **)
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
    STXXL_MSG("sizeof(long)           = " << sizeof(long));
    STXXL_MSG("sizeof(size_t)         = " << sizeof(size_t));
    STXXL_MSG("sizeof(off_t)          = " << sizeof(off_t));
    STXXL_MSG("sizeof(void*)          = " << sizeof(void *));

#if defined(STXXL_HAVE_AIO_FILE)
    STXXL_MSG("STXXL_HAVE_AIO_FILE    = " << STXXL_HAVE_AIO_FILE);
#endif

    assert(stxxl::version_major() == STXXL_VERSION_MAJOR);
    assert(stxxl::version_minor() == STXXL_VERSION_MINOR);
    assert(stxxl::version_patchlevel() == STXXL_VERSION_PATCHLEVEL);

    return 0;
}

extern int create_files(int argc, char * argv[]);
extern int benchmark_disks(int argc, char * argv[]);
extern int benchmark_files(int argc, char * argv[]);
extern int benchmark_sort(int argc, char * argv[]);

struct SubTool
{
    const char* name;
    int (*func)(int argc, char* argv[]);
    const char* description;
};

struct SubTool subtools[] = {
    { "info", &stxxl_info,
      "Print out information about the build system and which optional "
      "modules where compiled into STXXL." },
    { "create_files", &create_files,
      "Precreate large files to keep file system allocation time out to measurements." },
    { "benchmark_disks", &benchmark_disks,
      "This program will benchmark the disks configured by the standard "
      ".stxxl disk configuration files mechanism." },
    { "benchmark_files", &benchmark_files,
      "Benchmark different file access methods, e.g. syscall or mmap_files." },
    { "benchmark_sort", &benchmark_sort,
      "Run benchmark tests of different sorting methods in STXXL" },
    { NULL, NULL, NULL }
};

int main_usage(const char* arg0)
{
    std::cout << "Usage: " << arg0 << " <subtool> ..." << std::endl
              << "Available subtools: " <<  std::endl;

    for (unsigned int i = 0; subtools[i].name; ++i)
    {
        std::cout << "  " << subtools[i].name << std::endl;
        stxxl::cmdline_parser::output_wrap(std::cout, subtools[i].description, 80, 6, 6);
        std::cout << std::endl;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (stxxl::check_library_version() != 0)
        STXXL_ERRMSG("version mismatch between headers and library");

    if (argc > 1)
    {
        for (unsigned int i = 0; subtools[i].name; ++i)
        {
            if (strcmp(subtools[i].name, argv[1]) == 0)
            {
                // replace argv[1] with call string of subtool.
                std::string progsub = std::string(argv[0]) + " " + argv[1];
                argv[1] = (char*)progsub.c_str();
                return subtools[i].func(argc-1, argv+1);
            }
        }
        std::cout << "Unknown subtool '" << argv[1] << "'" << std::endl;
    }

    return main_usage(argv[0]);
}
