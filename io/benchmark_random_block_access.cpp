/***************************************************************************
 *  io/benchmark_random_block_access.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

/*
   example gnuplot command for the output of this program:
   (x-axis: offset in GiB, y-axis: bandwidth in MiB/s)

   plot \
        "disk.log" using ($2/1024):($7) w l title "read", \
        "disk.log" using ($2/1024):($4)  w l title "write"
 */

#include <iomanip>
#include <vector>

#include <stxxl/io>
#include <stxxl/mng>

#ifndef BOOST_MSVC
 #include <unistd.h>
#endif


using stxxl::request_ptr;
using stxxl::file;
using stxxl::timestamp;


#ifdef BLOCK_ALIGN
 #undef BLOCK_ALIGN
#endif

#define BLOCK_ALIGN  4096


#define KB (1024)
#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)

void usage(const char * argv0)
{
    std::cout << "Usage: " << argv0 << " span num_accesses [i][r][w]" << std::endl;
    std::cout << "    'span' is given in GiB" << std::endl;
    exit(-1);
}

struct print_number
{
    int n;

    print_number(int n) : n(n) { }

    void operator () (stxxl::request_ptr)
    {
        //std::cout << n << " " << std::flush;
    }
};

int main(int argc, char * argv[])
{
    const unsigned raw_block_size = 4 * KB;

    if (argc < 2)
        usage(argv[0]);

    stxxl::int64 span = stxxl::int64(MB) * stxxl::int64(atoi(argv[1]));
    stxxl::int64 num_blocks = stxxl::int64(atoi(argv[2]));

    bool do_init = false, do_read = false, do_write = false;

    if (argc == 4 && (strstr(argv[3], "i") != NULL))
        do_init = true;

    if (argc == 4 && (strstr(argv[3], "r") != NULL))
        do_read = true;

    if (argc == 4 && (strstr(argv[3], "w") != NULL))
        do_write = true;

    typedef stxxl::typed_block<raw_block_size, unsigned> block_type;
    typedef stxxl::BID<raw_block_size> BID_type;

    stxxl::int64 num_blocks_in_span = stxxl::div_ceil(span, raw_block_size);
    num_blocks = stxxl::STXXL_MIN(num_blocks, num_blocks_in_span);

    block_type * buffer = new block_type;
    request_ptr * reqs = new request_ptr[num_blocks_in_span];
    std::vector<BID_type> blocks;

    //touch data, so it is actually allocated

    try {
        STXXL_DEFAULT_ALLOC_STRATEGY alloc;

        blocks.resize(num_blocks_in_span);
        stxxl::block_manager::get_instance()->new_blocks(alloc, blocks.begin(), blocks.end());

        std::cout << "# Span size: "
                  << stxxl::add_IEC_binary_multiplier(span, "B") << " ("
                  << num_blocks_in_span << " blocks of "
                  << stxxl::add_IEC_binary_multiplier(raw_block_size, "B") << ")" << std::endl;

        double begin, end, elapsed;

        if (do_init)
        {
            begin = timestamp();
            std::cout << "First fill up space by writing sequentially..." << std::endl;
            for (unsigned j = 0; j < num_blocks_in_span; j++)
                reqs[j] = buffer->write(blocks[j]);
            wait_all(reqs, num_blocks_in_span);
            end = timestamp();
            elapsed = end - begin;
            std::cout << "Written " << num_blocks << " blocks in " << std::fixed << std::setw(5) << std::setprecision(2) << elapsed << " seconds: "
                      << std::setw(5) << std::setprecision(1) << (double(num_blocks) / elapsed) << " blocks/s "
                      << std::setw(5) << std::setprecision(1) << (double(num_blocks * raw_block_size) / MB / elapsed) << " MiB/s write " << std::endl;
        }

        std::cout << "Random block access..." << std::endl;

        srand(time(NULL));
        std::random_shuffle(blocks.begin(), blocks.end());

        begin = timestamp();
        if (do_read)
        {
            for (unsigned j = 0; j < num_blocks; j++)
                reqs[j] = buffer->read(blocks[j], print_number(j));
            wait_all(reqs, num_blocks);

            end = timestamp();
            elapsed = end - begin;
            std::cout << "Read    " << num_blocks << " blocks in " << std::fixed << std::setw(5) << std::setprecision(2) << elapsed << " seconds: "
                      << std::setw(5) << std::setprecision(1) << (double(num_blocks) / elapsed) << " blocks/s "
                      << std::setw(5) << std::setprecision(1) << (double(num_blocks * raw_block_size) / MB / elapsed) << " MiB/s read" << std::endl;
        }

        std::random_shuffle(blocks.begin(), blocks.end());

        begin = timestamp();
        if (do_write)
        {
            for (unsigned j = 0; j < num_blocks; j++)
                reqs[j] = buffer->write(blocks[j], print_number(j));
            wait_all(reqs, num_blocks);

            end = timestamp();
            elapsed = end - begin;
            std::cout << "Written " << num_blocks << " blocks in " << std::fixed << std::setw(5) << std::setprecision(2) << elapsed << " seconds: "
                      << std::setw(5) << std::setprecision(1) << (double(num_blocks) / elapsed) << " blocks/s "
                      << std::setw(5) << std::setprecision(1) << (double(num_blocks * raw_block_size) / MB / elapsed) << " MiB/s write " << std::endl;
        }
    }
    catch (const std::exception & ex)
    {
        std::cout << std::endl;
        STXXL_ERRMSG(ex.what());
    }

    //stxxl::block_manager::get_instance()->delete_blocks(blocks.begin(), blocks.end());

    delete[] reqs;
    delete buffer;

    return 0;
}
// vim: et:ts=4:sw=4
