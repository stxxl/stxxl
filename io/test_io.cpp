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
    std::string hr(uint64, const char * = 0);
}

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " tempdir" << std::endl;
        return -1;
    }

    std::string tempfilename[2];
    tempfilename[0] = std::string(argv[1]) + "/test_io_1.dat";
    tempfilename[1] = std::string(argv[1]) + "/test_io_2.dat";

    std::cout << sizeof(void *) << std::endl;
    const int size = 1024 * 384;
    char * buffer = (char *)stxxl::aligned_alloc<4096>(size);
    memset(buffer, 0, size);

#ifndef BOOST_MSVC
    stxxl::mmap_file file1(tempfilename[0], file::CREAT | file::RDWR /* | file::DIRECT */, 0);
    file1.set_size(size * 1024);
#endif

    stxxl::syscall_file file2(tempfilename[1], file::CREAT | file::RDWR /* | file::DIRECT */, 1);

    stxxl::request_ptr req[16];
    unsigned i;
    for (i = 0; i < 16; i++)
        req[i] = file2.awrite(buffer, i * size, size, my_handler());

    wait_all(req, 16);

    stxxl::aligned_dealloc<4096>(buffer);

    std::cout << *(stxxl::stats::get_instance());

    stxxl::uint64 sz;
    for (sz = 123, i = 0; i < 20; ++i, sz *= 10)
        STXXL_MSG(">>>" << stxxl::hr(sz) << "<<<");
    for (sz = 123, i = 0; i < 20; ++i, sz *= 10)
        STXXL_MSG(">>>" << stxxl::hr(sz, "B") << "<<<");
    STXXL_MSG(">>>" << stxxl::hr((std::numeric_limits<stxxl::uint64>::max)(), "B") << "<<<");

    unlink(tempfilename[0].c_str());
    unlink(tempfilename[1].c_str());

    return 0;
}
