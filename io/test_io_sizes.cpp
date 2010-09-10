/***************************************************************************
 *  io/test_io_sizes.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/io>
#include <stxxl/aligned_alloc>
#include <stxxl/bits/mng/mng.h>

//! \example io/test_io_size.cpp
//! This tests the maximum chunk size that a file type can handle with a single request.

using stxxl::file;

int main(int argc, char ** argv)
{
    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << "filetype tempfile maxsize" << std::endl;
        return -1;
    }

    stxxl::uint64 max_size = stxxl::atoint64(argv[3]);
    char * buffer = (char *)stxxl::aligned_alloc<4096>(max_size);
    memset(buffer, 0, max_size);

    stxxl::file* file
	    = stxxl::FileCreator::create(argv[1], argv[2], file::CREAT | file::RDWR | file::DIRECT);
    stxxl::request_ptr req;

    stxxl::stats_data stats1(*stxxl::stats::get_instance());
    for (stxxl::uint64 size = 4096 ; size < max_size; size *= 2)
    {
        STXXL_MSG(stxxl::add_IEC_binary_multiplier(size, "B") << "are being written at once" << std::endl);
        req = file->awrite(buffer, 0, size, stxxl::default_completion_handler());
        wait_all(&req, 1);
    }
    std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats1;

    stxxl::aligned_dealloc<4096>(buffer);

    file->remove();

    delete file;

    return 0;
}
