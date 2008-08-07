/***************************************************************************
 *  io/test_io.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/aligned_alloc>

//! \example io/test_io.cpp
//! This is an example of use of \c \<stxxl\> files, requests, and
//! completion tracking mechanisms, i.e. \c stxxl::file , \c stxxl::request

using stxxl::file;


struct my_handler
{
    void operator () (stxxl::request * ptr)
    {
        STXXL_MSG("Request completed: " << ptr);
    }
};

namespace stxxl
{
    std::string hr(uint64, const char *);
}

int main()
{
    std::cout << sizeof(void *) << std::endl;
    const int size = 1024 * 384;
    char * buffer = (char *)stxxl::aligned_alloc<4096>(size);
    memset(buffer, 0, size);
#ifdef BOOST_MSVC
    const char * paths[2] = { "data1", "data2" };
#else
    const char * paths[2] = { "/var/tmp/data1", "/var/tmp/data2" };
    stxxl::mmap_file file1(paths[0], file::CREAT | file::RDWR /* | file::DIRECT */, 0);
    file1.set_size(size * 1024);
#endif

    stxxl::syscall_file file2(paths[1], file::CREAT | file::RDWR /* | file::DIRECT */, 1);

    stxxl::request_ptr req[16];
    unsigned i = 0;
    for ( ; i < 16; i++)
        req[i] = file2.awrite(buffer, i * size, size, my_handler());


    wait_all(req, 16);

    stxxl::aligned_dealloc<4096>(buffer);

    std::cout << *(stxxl::stats::get_instance());

    stxxl::uint64 sz = 123;
    for (i = 0; i < 20; ++i, sz *= 10)
        STXXL_MSG(stxxl::hr(sz, "B"));
    STXXL_MSG(stxxl::hr((std::numeric_limits<stxxl::uint64>::max)(), "B"));

    return 0;
}
