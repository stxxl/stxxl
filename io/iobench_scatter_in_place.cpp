/***************************************************************************
 *  io/iobench_scatter_in_place.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iomanip>
#include <vector>
#include <cstdio>

#include <stxxl/io>
#include <stxxl/aligned_alloc>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/msvc_compatibility.h>


using stxxl::request_ptr;
using stxxl::file;
using stxxl::timestamp;
using stxxl::uint64;


#ifndef BLOCK_ALIGN
 #define BLOCK_ALIGN  4096
#endif


#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)

void usage(const char * argv0)
{
    std::cout << "Usage: " << argv0 << " num_blocks block_size file" << std::endl;
    std::cout << "    'block_size' in bytes" << std::endl;
    std::cout << "    'file' is split into 'num_blocks' blocks of size 'block_size'," << std::endl;
    std::cout << "    starting from end-of-file and truncated after each chunk was read" << std::endl;
    exit(-1);
}

// returns throughput in MiB/s
inline double throughput(double bytes, double seconds)
{
    if (seconds == 0.0)
        return 0.0;
    return bytes / (1024 * 1024) / seconds;
}

int main(int argc, char * argv[])
{
    if (argc < 4)
        usage(argv[0]);


    uint64 num_blocks = stxxl::atoint64(argv[1]);
    uint64 block_size = stxxl::atoint64(argv[2]);
    const char * filebase = argv[3];

    std::cout << "# Splitting '" << filebase << "' into " << num_blocks << " blocks of size " << block_size << std::endl;

    char * buffer = (char *)stxxl::aligned_alloc<BLOCK_ALIGN>(block_size);
    double totaltimeread = 0, totaltimewrite = 0;
    stxxl::int64 totalsizeread = 0, totalsizewrite = 0;

    typedef stxxl::syscall_file file_type;

    file_type input_file(filebase, file::RDWR | file::DIRECT, 0);

    double t_start = timestamp();
    try {
        for (stxxl::unsigned_type i = num_blocks; i-- > 0; )
        {
            double begin, end, elapsed;
            const uint64 offset = i * block_size;
            std::cout << "Input offset   " << std::setw(8) << offset / MB << " MiB: " << std::fixed;

            // read the block
            begin = timestamp();
            {
                input_file.aread(buffer, offset, block_size, stxxl::default_completion_handler())->wait();
            }
            end = timestamp();
            elapsed = end - begin;
            totalsizeread += block_size;
            totaltimeread += elapsed;
            
            std::cout << std::setw(8) << std::setprecision(3) << throughput(block_size, elapsed) << " MiB/s read, ";

            input_file.set_size(offset);

            // write the block
            begin = timestamp();
            {
                char cfn[4096]; // PATH_MAX
                snprintf(cfn, sizeof(cfn), "%s_%012llX", filebase, offset);
                file_type chunk_file(cfn, file::CREAT | file::RDWR | file::DIRECT, 0);
                chunk_file.awrite(buffer, 0, block_size, stxxl::default_completion_handler())->wait();
            }
            end = timestamp();
            elapsed = end - begin;
            totalsizewrite += block_size;
            totaltimewrite += elapsed;

            std::cout << std::setw(8) << std::setprecision(3) << throughput(block_size, elapsed) << " MiB/s write";

            std::cout << std::endl;
        }

    }
    catch (const std::exception & ex)
    {
        std::cout << std::endl;
        STXXL_ERRMSG(ex.what());
    }
    double t_total = timestamp() - t_start;

    const int ndisks = 1;

    std::cout << "=============================================================================================" << std::endl;
    std::cout << "# Average over " << std::setw(8) << stxxl::STXXL_MAX(totalsizewrite, totalsizeread) / MB << " MiB: ";
    std::cout << std::setw(8) << std::setprecision(3) << (throughput(totalsizeread, totaltimeread)) << " MiB/s read, ";
    std::cout << std::setw(8) << std::setprecision(3) << (throughput(totalsizewrite, totaltimewrite)) << " MiB/s write" << std::endl;
    if (totaltimeread != 0.0)
        std::cout << "# Read time    " << std::setw(8) << std::setprecision(3) << totaltimeread << " s" << std::endl;
    if (totaltimewrite != 0.0)
        std::cout << "# Write time   " << std::setw(8) << std::setprecision(3) << totaltimewrite << " s" << std::endl;
    std::cout << "# Non-I/O time " << std::setw(8) << std::setprecision(3) << (t_total - totaltimewrite - totaltimeread) << " s, average throughput "
              << std::setw(8) << std::setprecision(3) << (throughput(totalsizewrite + totalsizeread, t_total - totaltimewrite - totaltimeread) * ndisks) << " MiB/s"
              << std::endl;
    std::cout << "# Total time   " << std::setw(8) << std::setprecision(3) << t_total << " s, average throughput "
              << std::setw(8) << std::setprecision(3) << (throughput(totalsizewrite + totalsizeread, t_total) * ndisks) << " MiB/s"
              << std::endl;

    stxxl::aligned_dealloc<BLOCK_ALIGN>(buffer);

    return 0;
}

// vim: et:ts=4:sw=4
