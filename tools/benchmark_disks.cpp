/***************************************************************************
 *  tools/benchmark_disks.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

/*
  This programm will benchmark the disks configured via .stxxl disk
  configuration files. The block manager is used to read and write blocks using
  the different allocation strategies.
*/

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

using stxxl::timestamp;

#ifdef BLOCK_ALIGN
 #undef BLOCK_ALIGN
#endif

#define BLOCK_ALIGN  4096

#define POLL_DELAY 1000

#define CHECK_AFTER_READ 0

#define MB (1024 * 1024)

static int usage(const char * argv0)
{
    std::cout << "Usage: " << argv0 << " size [step] [r|w] [alloc]" << std::endl
              << "Arguments:" << std::endl
              << "  size    Amount of data to write/read from disks (e.g. 10GiB)" << std::endl
              << "  step    Step size between block access (default: 8 MiB)" << std::endl
              << "  r/w     Only read or write blocks (default: both write and read)" << std::endl
              << "  alloc   Block allocation strategy: RC, SR, FR, striping. (default: RC)" << std::endl
              << std::endl;

    const char* desc =
        "This program will benchmark the disks configured by the standard "
        ".stxxl disk configuration files mechanism. Blocks of 8 MiB are "
        "written and/or read in sequence using the block manager. The step "
        "size describes the spacing of the operations and alloc the block "
        "allocation strategy. If size == 0, then writing/reading operation "
        "are done until an error occurs.";

    std::cout << stxxl::string_wrap("", desc, 78);

    return 0;
}

template <typename AllocStrategy>
int benchmark_disks_alloc(int argc, char * argv[])
{
    if (argc < 2)
        return usage(argv[0]);

    // parse command line

    stxxl::uint64 offset = 0, length, step_size = 0;
    if (!stxxl::parse_SI_IEC_filesize(argv[1], length)) {
        std::cout << "Error parsing 'size' size string." << std::endl;
        return usage(argv[0]);
    }
    if (argc >= 3 && !stxxl::parse_SI_IEC_filesize(argv[2], step_size)) {
        std::cout << "Error parsing 'step' size string." << std::endl;
        return usage(argv[0]);
    }
    stxxl::uint64 endpos = offset + length;

    if (length == 0)
        endpos = std::numeric_limits<stxxl::uint64>::max();

    bool do_read = true, do_write = true;

    if (argc >= 4 && (strcmp("r", argv[3]) == 0 || strcmp("R", argv[3]) == 0))
        do_write = false;

    if (argc >= 4 && (strcmp("w", argv[3]) == 0 || strcmp("W", argv[3]) == 0))
        do_read = false;

    // construct block type

    const unsigned raw_block_size = 8 * MB;
    const unsigned block_size = raw_block_size / sizeof(int);

    typedef stxxl::typed_block<raw_block_size, unsigned> block_type;
    typedef stxxl::BID<raw_block_size> BID_type;

    if (step_size == 0) step_size = raw_block_size;

    unsigned num_blocks_per_step = stxxl::div_ceil(step_size, raw_block_size);
    step_size = num_blocks_per_step * raw_block_size;

    block_type * buffer = new block_type[num_blocks_per_step];
    stxxl::request_ptr * reqs = new stxxl::request_ptr[num_blocks_per_step];
    std::vector<BID_type> blocks;
    double totaltimeread = 0, totaltimewrite = 0;
    stxxl::int64 totalsizeread = 0, totalsizewrite = 0;

    std::cout << "# Step size: "
              << stxxl::add_IEC_binary_multiplier(step_size, "B") << " ("
              << num_blocks_per_step << " blocks of "
              << stxxl::add_IEC_binary_multiplier(raw_block_size, "B") << ")"
              << " using " << AllocStrategy().name()
              << std::endl;

    // touch data, so it is actually allcoated
    for (unsigned j = 0; j < num_blocks_per_step; ++j)
        for (unsigned i = 0; i < block_size; ++i)
            buffer[j][i] = j * block_size + i;

    try {
        AllocStrategy alloc;
        while (offset < endpos)
        {
            const stxxl::int64 current_step_size = std::min<stxxl::int64>(step_size, endpos - offset);
#if CHECK_AFTER_READ
            const stxxl::int64 current_step_size_int = current_step_size / sizeof(int);
#endif
            const unsigned current_num_blocks_per_step = stxxl::div_ceil(current_step_size, raw_block_size);

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

int benchmark_disks(int argc, char * argv[])
{
    if (argc < 2)
        return usage(argv[0]);

    if (argc >= 5)
    {
        if (strcmp(argv[4], "RC") == 0)
            return benchmark_disks_alloc<stxxl::RC>(argc, argv);
        if (strcmp(argv[4], "SR") == 0)
            return benchmark_disks_alloc<stxxl::SR>(argc, argv);
        if (strcmp(argv[4], "FR") == 0)
            return benchmark_disks_alloc<stxxl::FR>(argc, argv);
        if (strcmp(argv[4], "striping") == 0)
            return benchmark_disks_alloc<stxxl::striping>(argc, argv);

        std::cout << "Unknown allocation strategy '" << argv[4] << "'" << std::endl;
        return usage(argv[0]);
    }

    return benchmark_disks_alloc<STXXL_DEFAULT_ALLOC_STRATEGY>(argc, argv);
}
