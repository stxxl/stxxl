/***************************************************************************
 *  io/test_cancel.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/aligned_alloc>

//! \example io/test_cancel.cpp
//! This tests the request cancellation mechanisms.

using stxxl::file;


struct my_handler
{
    void operator () (stxxl::request * ptr)
    {
        STXXL_MSG("Request completed: " << ptr);
    }
};

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " tempdir" << std::endl;
        return -1;
    }

    const int size = 64 * 1024 * 1024;
    char * buffer = (char *)stxxl::aligned_alloc<4096>(size);
    memset(buffer, 0, size);

    std::string tempfilename = std::string(argv[1]) + "/test_cancel.dat";
    stxxl::syscall_file file(tempfilename, file::CREAT | file::RDWR | file::DIRECT, 1);
    stxxl::request_ptr req[16];

    //without cancellation
    stxxl::stats_data stats1(*stxxl::stats::get_instance());
    unsigned i = 0;
    for ( ; i < 16; i++)
        req[i] = file.awrite(buffer, i * size, size, my_handler());
    wait_all(req, 16);
    std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats1;

    //with cancellation
    stxxl::stats_data stats2(*stxxl::stats::get_instance());
    for (unsigned i = 0; i < 16; i++)
        req[i] = file.awrite(buffer, i * size, size, my_handler());
    //cancel first half
    unsigned num_cancelled = cancel_all(req, req + 8);
    STXXL_MSG("Cancelled " << num_cancelled << " requests.");
    //cancel every second in second half
    for (unsigned i = 8; i < 16; i += 2)
    {
        if (req[i]->cancel())
            STXXL_MSG("Cancelled request " << &(*(req[i])));
    }
    wait_all(req, 16);
    std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats2;

    stxxl::aligned_dealloc<4096>(buffer);

    unlink(tempfilename.c_str());

    return 0;
}
