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

int main()
{
    const int size = 64 * 1024 * 1024;
    char * buffer = (char *)stxxl::aligned_alloc<4096>(size);
    memset(buffer, 0, size);
#ifdef BOOST_MSVC
    const char * path = "data1";
#else
    const char * path = "/var/tmp/data1";
#endif

    stxxl::syscall_file file(path, file::CREAT | file::RDWR | file::DIRECT, 1);
    stxxl::request_ptr req[16];

    //without cancellation
    stxxl::stats::get_instance()->reset();
    unsigned i = 0;
    for ( ; i < 16; i++)
        req[i] = file.awrite(buffer, i * size, size, my_handler());
    wait_all(req, 16);
    std::cout << *(stxxl::stats::get_instance());

    //with cancellation
    stxxl::stats::get_instance()->reset();
    for (unsigned i = 0 ; i < 16; i++)
        req[i] = file.awrite(buffer, i * size, size, my_handler());
    //cancel first half
    unsigned num_cancelled = cancel_all(req, req + 8);
    STXXL_MSG("Cancelled " << num_cancelled << " requests.");
    //cancel every second in second half
    for (unsigned i = 8 ; i < 16; i += 2)
    {
        if(req[i]->cancel())
            STXXL_MSG("Cancelled request " << &(*(req[i])));
    }
    wait_all(req, 16);
    std::cout << *(stxxl::stats::get_instance());

    stxxl::aligned_dealloc<4096>(buffer);

    unlink(path);

    return 0;
}
