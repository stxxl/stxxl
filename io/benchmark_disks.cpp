/***************************************************************************
 *  io/benchmark_disks.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

/*
   example gnuplot command for the output of this program:
   (x-axis: disk offset in GB, y-axis: bandwidth in MB/s)

   plot \
        "disk.log" using ($3/1024):($14) w l title "read", \
        "disk.log" using ($3/1024):($7)  w l title "write"
 */

#include <iomanip>
#include <vector>

#include <stxxl/io>
#include <stxxl/aligned_alloc>

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

//#define NOREAD

//#define DO_ONLY_READ

#define POLL_DELAY 1000

#define RAW_ACCESS

//#define WATCH_TIMES

#define CHECK_AFTER_READ 1


#ifdef WATCH_TIMES
void watch_times(request_ptr reqs[], unsigned n, double * out)
{
    bool * finished = new bool[n];
    unsigned count = 0;
    unsigned i = 0;
    for (i = 0; i < n; i++)
        finished[i] = false;

    while (count != n)
    {
        usleep(POLL_DELAY);
        i = 0;
        for (i = 0; i < n; i++)
        {
            if (!finished[i])
                if (reqs[i]->poll())
                {
                    finished[i] = true;
                    out[i] = timestamp();
                    count++;
                }
        }
    }
    delete[] finished;
}


void out_stat(double start, double end, double * times, unsigned n, const std::vector<std::string> & names)
{
    for (unsigned i = 0; i < n; i++)
    {
        std::cout << i << " " << names[i] << " took " <<
        100. * (times[i] - start) / (end - start) << " %" << std::endl;
    }
}
#endif

#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)

void usage(const char * argv0)
{
    std::cout << "Usage: " << argv0 << " offset length [r|w] diskfile..." << std::endl;
    std::cout << "    starting 'offset' and 'length' are given in GB" << std::endl;
    std::cout << "    length == 0 implies till end of space (please ignore the write error)" << std::endl;
    exit(-1);
}

int main(int argc, char * argv[])
{
    if (argc < 4)
        usage(argv[0]);

    stxxl::int64 offset = stxxl::int64(GB) * stxxl::int64(atoi(argv[1]));
    stxxl::int64 length = stxxl::int64(GB) * stxxl::int64(atoi(argv[2]));
    stxxl::int64 endpos = offset + length;

    bool do_read = true, do_write = true;
    int first_disk_arg = 3;

    if (strcasecmp("r", argv[3]) == 0) {
        do_write = false;
        ++first_disk_arg;
    }

    if (strcasecmp("w", argv[3]) == 0) {
        do_read = false;
        ++first_disk_arg;
    }

    std::vector<std::string> disks_arr;

    if (!(first_disk_arg < argc))
        usage(argv[0]);

    for (int ii = first_disk_arg; ii < argc; ii++)
    {
        std::cout << "# Add disk: " << argv[ii] << std::endl;
        disks_arr.push_back(argv[ii]);
    }

    const unsigned ndisks = disks_arr.size();
    //const stxxl::int64 max_mem = 8LL * 1024 * MB;
    const stxxl::int64 max_mem = 900 * MB;
    //stxxl::int64 buffer_size = 1024 * MB;
    stxxl::int64 buffer_size = 256 * MB;
    while (buffer_size * ndisks > max_mem)
        buffer_size >>= 1;
    const stxxl::int64 buffer_size_int = buffer_size / sizeof(int);

    const unsigned min_chunk = 8 * MB;
    unsigned chunks = 32;
    while (buffer_size / chunks < min_chunk)
        chunks >>= 1;
    const unsigned chunk_size = buffer_size / chunks;
    const unsigned chunk_size_int = chunk_size / sizeof(int);

    unsigned i, j;

    request_ptr * reqs = new request_ptr[ndisks * chunks];
    file ** disks = new file *[ndisks];
    unsigned * buffer = (unsigned *)stxxl::aligned_alloc<BLOCK_ALIGN>(buffer_size * ndisks);
#ifdef WATCH_TIMES
    double * r_finish_times = new double[ndisks];
    double * w_finish_times = new double[ndisks];
#endif
    double totaltimeread = 0, totaltimewrite = 0;
    stxxl::int64 totalsizeread = 0, totalsizewrite = 0;

    for (i = 0; i < ndisks * buffer_size_int; i++)
        buffer[i] = i;

    for (i = 0; i < ndisks; i++)
    {
#ifdef BOOST_MSVC
 #ifdef RAW_ACCESS
        disks[i] = new stxxl::wincall_file(disks_arr[i],
                                           file::CREAT | file::RDWR | file::DIRECT, i);
 #else
        disks[i] = new stxxl::wincall_file(disks_arr[i],
                                           file::CREAT | file::RDWR, i);
 #endif
#else
 #ifdef RAW_ACCESS
        disks[i] = new stxxl::syscall_file(disks_arr[i],
                                           file::CREAT | file::RDWR | file::DIRECT, i);
 #else
        disks[i] = new stxxl::syscall_file(disks_arr[i],
                                           file::CREAT | file::RDWR, i);
 #endif
#endif
    }

    std::cout << "# Buffer size: "
              << buffer_size << " bytes per disk ("
              << chunks << " chunks of "
              << (buffer_size / chunks) << " bytes)" << std::endl;
    try {
        while (!length || offset < endpos)
        {
            const stxxl::int64 current_block_size = length ? std::min<stxxl::int64>(buffer_size, endpos - offset) : buffer_size;
            const unsigned current_chunk_size = current_block_size / chunks;

            std::cout << "Disk offset " << std::setw(7) << offset / MB << " MB: " << std::fixed;

            double begin = timestamp(), end, elapsed;

#ifndef DO_ONLY_READ
            if (do_write) {
            for (i = 0; i < ndisks; i++)
            {
                for (j = 0; j < chunks; j++)
                    reqs[i * chunks + j] =
                        disks[i]->awrite(buffer + buffer_size_int * i + j * chunk_size_int,
                                         offset + j * current_chunk_size,
                                         current_chunk_size,
                                         stxxl::default_completion_handler());
            }

 #ifdef WATCH_TIMES
            watch_times(reqs, ndisks, w_finish_times);
 #else
            wait_all(reqs, ndisks * chunks);
 #endif

            end = timestamp();
            elapsed = end - begin;
            totalsizewrite += buffer_size;
            totaltimewrite += elapsed;
            } else {
                elapsed = 0.0;
            }

/*
   std::cout << "WRITE\nDisks: " << ndisks
        <<" \nElapsed time: "<< end-begin
        << " \nThroughput: "<< int(1e-6*(buffer_size*ndisks)/(end-begin))
        << " Mb/s \nPer one disk:"
        << int(1e-6*(buffer_size)/(end-begin)) << " Mb/s"
        << std::endl;
*/

 #ifdef WATCH_TIMES
            out_stat(begin, end, w_finish_times, ndisks, disks_arr);
 #endif
            std::cout << std::setw(2) << ndisks << " * "
                      << std::setw(7) << std::setprecision(3) << (1e-6 * (current_block_size) / elapsed) << " = "
                      << std::setw(7) << std::setprecision(3) << (1e-6 * (current_block_size * ndisks) / elapsed) << " MB/s write,";
#endif


#ifndef NOREAD
            begin = timestamp();

            if (do_read) {
            for (i = 0; i < ndisks; i++)
            {
                for (j = 0; j < chunks; j++)
                    reqs[i * chunks + j] = disks[i]->aread(buffer + buffer_size_int * i + j * chunk_size_int,
                                                           offset + j * current_chunk_size,
                                                           current_chunk_size,
                                                           stxxl::default_completion_handler());
            }

 #ifdef WATCH_TIMES
            watch_times(reqs, ndisks, r_finish_times);
 #else
            wait_all(reqs, ndisks * chunks);
 #endif

            end = timestamp();
            elapsed = end - begin;
            totalsizeread += buffer_size;
            totaltimeread += elapsed;
            } else {
                elapsed = 0.0;
            }

/*
   std::cout << "READ\nDisks: " << ndisks
        <<" \nElapsed time: "<< end-begin
        << " \nThroughput: "<< int(1e-6*(buffer_size*ndisks)/(end-begin))
        << " Mb/s \nPer one disk:"
        << int(1e-6*(buffer_size)/(end-begin)) << " Mb/s"
            << std::endl;
*/

            std::cout << std::setw(2) << ndisks << " * "
                      << std::setw(7) << std::setprecision(3) << (1e-6 * (current_block_size) / elapsed) << " = "
                      << std::setw(7) << std::setprecision(3) << (1e-6 * (current_block_size * ndisks) / elapsed) << " MB/s read" << std::endl;

#ifdef WATCH_TIMES
            out_stat(begin, end, r_finish_times, ndisks, disks_arr);
#endif

            if (CHECK_AFTER_READ) {
                for (i = 0; i < ndisks * buffer_size_int; i++)
                {
                    if (buffer[i] != i)
                    {
                        int ibuf = i / buffer_size_int;
                        int pos = i % buffer_size_int;

                        std::cout << "Error on disk " << ibuf << " position " << std::hex << std::setw(8) << offset + pos * sizeof(int)
                                  << "  got: " << std::hex << std::setw(8) << buffer[i] << " wanted: " << std::hex << std::setw(8) << i
                                  << std::dec << std::endl;

                        i = (ibuf + 1) * buffer_size_int; // jump to next
                    }
                }
            }
#else
            std::cout << std::endl;
#endif

            offset += current_block_size;
        }
    }
    catch (const std::exception & ex)
    {
        std::cout << std::endl;
        STXXL_ERRMSG(ex.what());
    }

    std::cout << "# Average over " << std::setw(7) << totalsizewrite / MB << " MB: ";
    std::cout << std::setw(2) << ndisks << " * "
              << std::setw(7) << std::setprecision(3) << (1e-6 * (totalsizewrite) / totaltimewrite) << " = "
              << std::setw(7) << std::setprecision(3) << (1e-6 * (totalsizewrite * ndisks) / totaltimewrite) << " MB/s write,";
    std::cout << std::setw(2) << ndisks << " * "
              << std::setw(7) << std::setprecision(3) << (1e-6 * (totalsizeread) / totaltimeread) << " = "
              << std::setw(7) << std::setprecision(3) << (1e-6 * (totalsizeread * ndisks) / totaltimeread) << " MB/s read" << std::endl;

    for (i = 0; i < ndisks; i++)
        delete disks[i];

    delete[] reqs;
    delete[] disks;
    stxxl::aligned_dealloc<BLOCK_ALIGN>(buffer);

#ifdef WATCH_TIMES
    delete[] r_finish_times;
    delete[] w_finish_times;
#endif

    return 0;
}

// vim: et:ts=4:sw=4
