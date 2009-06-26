/***************************************************************************
 *  io/benchmark_configured_disks.cpp
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

#define POLL_DELAY 1000

#define CHECK_AFTER_READ 0


#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)

void usage(const char * argv0)
{
    std::cout << "Usage: " << argv0 << " length step [r|w]" << std::endl;
    std::cout << "    'length' is given in GiB, 'step' size in MiB" << std::endl;
    std::cout << "    length == 0 implies till end of space (please ignore the write error)" << std::endl;
    exit(-1);
}

int main(int argc, char * argv[])
{
    if (argc < 3)
        usage(argv[0]);

    stxxl::int64 offset    = 0;
    stxxl::int64 length    = stxxl::int64(GB) * stxxl::int64(atoi(argv[1]));
    stxxl::int64 step_size = stxxl::int64(MB) * stxxl::int64(atoi(argv[2]));
    stxxl::int64 endpos    = offset + length;

    bool do_read = true, do_write = true;

    if (argc == 4 && (strcmp("r", argv[3]) == 0 || strcmp("R", argv[3]) == 0))
        do_write = false;

    if (argc == 4 && (strcmp("w", argv[3]) == 0 || strcmp("W", argv[3]) == 0))
        do_read = false;

    const unsigned raw_block_size = 8 * MB;
    const unsigned block_size = raw_block_size / sizeof(int);

    typedef stxxl::typed_block<raw_block_size, unsigned> block_type;
    typedef stxxl::BID<raw_block_size> BID_type;

    unsigned num_blocks_per_step = STXXL_DIVRU(step_size, raw_block_size);
    step_size = num_blocks_per_step * raw_block_size;

    block_type* buffer = new block_type[num_blocks_per_step];
    request_ptr * reqs = new request_ptr[num_blocks_per_step];
    std::vector<BID_type> blocks;
    double totaltimeread = 0, totaltimewrite = 0;
    stxxl::int64 totalsizeread = 0, totalsizewrite = 0;

    std::cout << "# Step size: "
              << stxxl::add_IEC_binary_multiplier(step_size, "B") << " ("
              << num_blocks_per_step << " blocks of "
              << stxxl::add_IEC_binary_multiplier(raw_block_size, "B") << ")" << std::endl;

    //touch data, so it is actually allcoated
    for (unsigned j = 0; j < num_blocks_per_step; ++j)
        for (unsigned i = 0; i < block_size; ++i)
            buffer[j][i] = j * block_size + i;

    try {
        STXXL_DEFAULT_ALLOC_STRATEGY alloc;
        while (offset < endpos)
        {
            const stxxl::int64 current_step_size = std::min<stxxl::int64>(step_size, endpos - offset);
#if CHECK_AFTER_READ
            const stxxl::int64 current_step_size_int = current_step_size / sizeof(int);
#endif
            const unsigned current_num_blocks_per_step = STXXL_DIVRU(current_step_size, raw_block_size);

            std::cout << "Offset    " << std::setw(7) << offset / MB << " MiB: " << std::fixed;

            stxxl::unsigned_type num_total_blocks = blocks.size();
            blocks.resize(num_total_blocks + current_num_blocks_per_step);
            stxxl::block_manager::get_instance()->new_blocks(alloc, blocks.begin() + num_total_blocks, blocks.end());

            double begin = timestamp(), end, elapsed;

            if (do_write)
            {
                for (unsigned j = 0; j < current_num_blocks_per_step; j++)
                    reqs[j] = buffer[j].write(blocks[num_total_blocks + j]);

                wait_all(reqs, current_num_blocks_per_step);

                end = timestamp();
                elapsed = end - begin;
                totalsizewrite += current_step_size;
                totaltimewrite += elapsed;
            }
            else
                elapsed = 0.0;

            std::cout << std::setw(5) << std::setprecision(1) << (double(current_step_size) / MB / elapsed) << " MiB/s write, ";


            begin = timestamp();

            if (do_read)
            {
                for (unsigned j = 0; j < current_num_blocks_per_step; j++)
                    reqs[j] = buffer[j].read(blocks[num_total_blocks + j]);

                wait_all(reqs, current_num_blocks_per_step);

                end = timestamp();
                elapsed = end - begin;
                totalsizeread += current_step_size;
                totaltimeread += elapsed;
            }
            else
                elapsed = 0.0;

            std::cout << std::setw(5) << std::setprecision(1) << (double(current_step_size) / MB / elapsed) << " MiB/s read" << std::endl;

#if CHECK_AFTER_READ
            for (unsigned j = 0; j < current_num_blocks_per_step; j++)
            {
                for (unsigned i = 0; i < block_size; i++)
                {
                    if (buffer[j][i] != j * block_size + i)
                    {
                        int ibuf = i / current_step_size_int;
                        int pos = i % current_step_size_int;

                        std::cout << "Error on disk " << ibuf << " position " << std::hex << std::setw(8) << offset + pos * sizeof(int)
                                << "  got: " << std::hex << std::setw(8) << buffer[j][i] << " wanted: " << std::hex << std::setw(8) << (j * block_size + i)
                                << std::dec << std::endl;

                        i = (ibuf + 1) * current_step_size_int; // jump to next
                    }
                }
            }
#endif

            offset += current_step_size;
        }
    }
    catch (const std::exception & ex)
    {
        std::cout << std::endl;
        STXXL_ERRMSG(ex.what());
    }

    std::cout << "=============================================================================================" << std::endl;
    std::cout << "# Average over " << std::setw(7) << totalsizewrite / MB << " MiB: ";
    std::cout << std::setw(5) << std::setprecision(1) << (double(totalsizewrite) / MB / totaltimewrite) << " MiB/s write, ";
    std::cout << std::setw(5) << std::setprecision(1) << (double(totalsizeread) / MB / totaltimeread) << " MiB/s read" << std::endl;

    delete[] reqs;
    delete[] buffer;

    return 0;
}
