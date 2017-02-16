/***************************************************************************
 *  tests/io/test_io_sizes.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/aligned_alloc>
#include <stxxl/bits/mng/block_manager.h>
#include <stxxl/io>

//! \example io/test_io_sizes.cpp
//! This tests the maximum chunk size that a file type can handle with a single request.

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " filetype tempfile maxsize" << std::endl;
        return -1;
    }

    size_t max_size = atoi(argv[3]);
    size_t* buffer = (size_t*)stxxl::aligned_alloc<4096>(max_size);

    try
    {
        stxxl::file_ptr file = stxxl::create_file(
            argv[1], argv[2],
            stxxl::file::CREAT | stxxl::file::RDWR | stxxl::file::DIRECT);
        file->set_size(max_size);

        stxxl::request_ptr req;
        stxxl::stats_data stats1(*stxxl::stats::get_instance());
        for (size_t size = 4096; size < max_size; size *= 2)
        {
            //generate data
            for (size_t i = 0; i < size / sizeof(size_t); ++i)
                buffer[i] = i;

            //write
            STXXL_MSG(stxxl::add_IEC_binary_multiplier(size, "B") << "are being written at once");
            req = file->awrite(buffer, 0, size);
            wait_all(&req, 1);

            //fill with wrong data
            for (size_t i = 0; i < size / sizeof(size_t); ++i)
                buffer[i] = 0xFFFFFFFFFFFFFFFFull;

            //read again
            STXXL_MSG(stxxl::add_IEC_binary_multiplier(size, "B") << "are being read at once");
            req = file->aread(buffer, 0, size);
            wait_all(&req, 1);

            //check
            bool wrong = false;
            for (size_t i = 0; i < size / sizeof(size_t); ++i)
                if (buffer[i] != i)
                {
                    STXXL_ERRMSG("Read inconsistent data at position " << i * sizeof(size_t));
                    wrong = true;
                    break;
                }

            if (wrong)
                break;
        }
        std::cout << stxxl::stats_data(*stxxl::stats::get_instance()) - stats1;

        file->close_remove();
    }
    catch (stxxl::io_error e)
    {
        std::cerr << e.what() << std::endl;
        throw;
    }

    stxxl::aligned_dealloc<4096>(buffer);

    return 0;
}
// vim: et:ts=4:sw=4
