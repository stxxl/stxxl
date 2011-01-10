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

struct print_completion
{
    void operator () (stxxl::request * ptr)
    {
        STXXL_MSG("Request completed: " << ptr);
    }
};

int main(int argc, char ** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " filetype tempfile" << std::endl;
        return -1;
    }

    const stxxl::uint64 size = 64 * 1024 * 1024, num_blocks = 16;
    char * buffer = (char *)stxxl::aligned_alloc<4096>(size);
    memset(buffer, 0, size);

    std::auto_ptr<stxxl::file> file(
        stxxl::create_file(argv[1], argv[2], stxxl::file::CREAT | stxxl::file::RDWR | stxxl::file::DIRECT));

    file->set_size(num_blocks * size);
    stxxl::request_ptr req[num_blocks];

    //without cancellation
    stxxl::stats_data stats1(*stxxl::stats::get_instance());
    unsigned i = 0;
    for ( ; i < num_blocks; i++)
        req[i] = file->awrite(buffer, i * size, size, print_completion());
    wait_all(req, num_blocks);
    std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats1;

    //with cancellation
    stxxl::stats_data stats2(*stxxl::stats::get_instance());
    for (unsigned i = 0; i < num_blocks; i++)
        req[i] = file->awrite(buffer, i * size, size, print_completion());
    //cancel first half
    unsigned num_cancelled = cancel_all(req, req + 8);
    STXXL_MSG("Cancelled " << num_cancelled << " requests.");
    //cancel every second in second half
    for (unsigned i = num_blocks / 2; i < num_blocks; i += 2)
    {
        if (req[i]->cancel())
            STXXL_MSG("Cancelled request " << &(*(req[i])));
    }
    wait_all(req, num_blocks);
    std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats2;

    stxxl::aligned_dealloc<4096>(buffer);

    return 0;
}
