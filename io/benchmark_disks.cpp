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
   (x-axis: disk offset in GiB, y-axis: bandwidth in MiB/s)

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
    for (unsigned i = 0; i < n; i++)
        finished[i] = false;

    while (count != n)
    {
        usleep(POLL_DELAY);
        unsigned i = 0;
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
    std::cout << "Usage: " << argv0 << " offset length [block_size [batch_size]] [r|w] [--] diskfile..." << std::endl;
    std::cout << "    starting 'offset' and 'length' are given in GiB," << std::endl;
    std::cout << "    'block_size' (default 8) in MiB, increase 'batch_size' (default 1)" << std::endl;
    std::cout << "    to submit several I/Os at once and report average rate" << std::endl;
    std::cout << "    ops: write and reread (default), (r)ead only, (w)rite only" << std::endl;
    std::cout << "    length == 0 implies till end of space (please ignore the write error)" << std::endl;
    std::cout << "    Memory consumption: block_size * batch_size * num_disks" << std::endl;
    exit(-1);
}

int main(int argc, char * argv[])
{
    if (argc < 4)
        usage(argv[0]);

    stxxl::int64 offset    = stxxl::int64(GB) * stxxl::int64(atoi(argv[1]));
    stxxl::int64 length    = stxxl::int64(GB) * stxxl::int64(atoi(argv[2]));
    stxxl::int64 endpos    = offset + length;
    stxxl::int64 block_size = 0;
    stxxl::int64 batch_size = 0;

    bool do_read = true, do_write = true;
    int first_disk_arg = 3;

    if (first_disk_arg < argc)
        block_size = atoi(argv[first_disk_arg]);
    if (block_size > 0) {
        ++first_disk_arg;
    } else {
        block_size = 8;
    }
    block_size *= MB;

    if (first_disk_arg < argc)
        batch_size = atoi(argv[first_disk_arg]);
    if (batch_size > 0) {
        ++first_disk_arg;
    } else {
        batch_size = 1;
    }

    if (first_disk_arg < argc && (strcmp("r", argv[first_disk_arg]) == 0 || strcmp("R", argv[first_disk_arg]) == 0)) {
        do_write = false;
        ++first_disk_arg;
    } else if (first_disk_arg < argc && (strcmp("w", argv[first_disk_arg]) == 0 || strcmp("W", argv[first_disk_arg]) == 0)) {
        do_read = false;
        ++first_disk_arg;
    }

    if (first_disk_arg < argc && strcmp("--", argv[first_disk_arg]) == 0) {
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

    const stxxl::unsigned_type step_size = block_size * batch_size;
    const unsigned block_size_int = block_size / sizeof(int);
    const stxxl::int64 step_size_int = step_size / sizeof(int);

    unsigned * buffer = (unsigned *)stxxl::aligned_alloc<BLOCK_ALIGN>(step_size * ndisks);
    file ** disks = new file *[ndisks];
    request_ptr * reqs = new request_ptr[ndisks * batch_size];
#ifdef WATCH_TIMES
    double * r_finish_times = new double[ndisks];
    double * w_finish_times = new double[ndisks];
#endif
    double totaltimeread = 0, totaltimewrite = 0;
    stxxl::int64 totalsizeread = 0, totalsizewrite = 0;

    for (unsigned i = 0; i < ndisks * step_size_int; i++)
        buffer[i] = i;

    for (unsigned i = 0; i < ndisks; i++)
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

    std::cout << "# Step size: "
              << step_size << " bytes per disk ("
              << batch_size << " blocks of "
              << block_size << " bytes)" << std::endl;
    try {
        while (offset < endpos)
        {
            const stxxl::int64 current_step_size = std::min<stxxl::int64>(step_size, endpos - offset);
            const stxxl::int64 current_step_size_int = current_step_size / sizeof(int);
            const unsigned current_num_blocks = STXXL_DIVRU(current_step_size, block_size);

            std::cout << "Disk offset    " << std::setw(7) << offset / MB << " MiB: " << std::fixed;

            double begin = timestamp(), end, elapsed;

#ifndef DO_ONLY_READ
            if (do_write) {
            for (unsigned i = 0; i < ndisks; i++)
            {
                for (unsigned j = 0; j < current_num_blocks; j++)
                    reqs[i * current_num_blocks + j] =
                        disks[i]->awrite(buffer + current_step_size_int * i + j * block_size_int,
                                         offset + j * block_size,
                                         block_size,
                                         stxxl::default_completion_handler());
            }

 #ifdef WATCH_TIMES
            watch_times(reqs, ndisks, w_finish_times);
 #else
            wait_all(reqs, ndisks * current_num_blocks);
 #endif

            end = timestamp();
            elapsed = end - begin;
            totalsizewrite += current_step_size;
            totaltimewrite += elapsed;
            } else {
                elapsed = 0.0;
            }

#if 0
   std::cout << "WRITE\nDisks: " << ndisks
        <<" \nElapsed time: "<< end-begin
        << " \nThroughput: "<< int(double(current_step_size*ndisks)/MB/(end-begin))
        << " MiB/s \nPer one disk:"
        << int(double(current_step_size)/MB/(end-begin)) << " MiB/s"
        << std::endl;
#endif

 #ifdef WATCH_TIMES
            out_stat(begin, end, w_finish_times, ndisks, disks_arr);
 #endif
            std::cout << std::setw(2) << ndisks << " * "
                      << std::setw(7) << std::setprecision(3) << (double(current_step_size) / MB / elapsed) << " = "
                      << std::setw(7) << std::setprecision(3) << (double(current_step_size * ndisks) / MB / elapsed) << " MiB/s write,";
#endif


#ifndef NOREAD
            begin = timestamp();

            if (do_read) {
                for (unsigned i = 0; i < ndisks; i++)
                {
                    for (unsigned j = 0; j < current_num_blocks; j++)
                        reqs[i * current_num_blocks + j] = disks[i]->aread(buffer + current_step_size_int * i + j * block_size_int,
                                                            offset + j * block_size,
                                                            block_size,
                                                            stxxl::default_completion_handler());
                }

 #ifdef WATCH_TIMES
                watch_times(reqs, ndisks, r_finish_times);
 #else
                wait_all(reqs, ndisks * current_num_blocks);
 #endif

                end = timestamp();
                elapsed = end - begin;
                totalsizeread += current_step_size;
                totaltimeread += elapsed;
            } else {
                elapsed = 0.0;
            }

#if 0
   std::cout << "READ\nDisks: " << ndisks
        <<" \nElapsed time: "<< end-begin
        << " \nThroughput: "<< int(double(current_step_size*ndisks)/MB/(end-begin))
        << " MiB/s \nPer one disk:"
        << int(double(current_step_size)/MB/(end-begin)) << " MiB/s"
            << std::endl;
#endif

            std::cout << std::setw(2) << ndisks << " * "
                      << std::setw(7) << std::setprecision(3) << (double(current_step_size) / MB / elapsed) << " = "
                      << std::setw(7) << std::setprecision(3) << (double(current_step_size * ndisks) / MB / elapsed) << " MiB/s read" << std::endl;

#ifdef WATCH_TIMES
            out_stat(begin, end, r_finish_times, ndisks, disks_arr);
#endif

            if (CHECK_AFTER_READ) {
                for (unsigned i = 0; i < ndisks * current_step_size_int; i++)
                {
                    if (buffer[i] != i)
                    {
                        int ibuf = i / current_step_size_int;
                        int pos = i % current_step_size_int;

                        std::cout << "Error on disk " << ibuf << " position " << std::hex << std::setw(8) << offset + pos * sizeof(int)
                                  << "  got: " << std::hex << std::setw(8) << buffer[i] << " wanted: " << std::hex << std::setw(8) << i
                                  << std::dec << std::endl;

                        i = (ibuf + 1) * current_step_size_int; // jump to next
                    }
                }
            }
#else
            std::cout << std::endl;
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
    std::cout << std::setw(2) << ndisks << " * "
              << std::setw(7) << std::setprecision(3) << (double(totalsizewrite) / MB / totaltimewrite) << " = "
              << std::setw(7) << std::setprecision(3) << (double(totalsizewrite * ndisks) / MB / totaltimewrite) << " MiB/s write,";
    std::cout << std::setw(2) << ndisks << " * "
              << std::setw(7) << std::setprecision(3) << (double(totalsizeread) / MB / totaltimeread) << " = "
              << std::setw(7) << std::setprecision(3) << (double(totalsizeread * ndisks) / MB / totaltimeread) << " MiB/s read" << std::endl;

#ifdef WATCH_TIMES
    delete[] r_finish_times;
    delete[] w_finish_times;
#endif
    delete[] reqs;
    for (unsigned i = 0; i < ndisks; i++)
        delete disks[i];
    delete[] disks;
    stxxl::aligned_dealloc<BLOCK_ALIGN>(buffer);

    return 0;
}

// vim: et:ts=4:sw=4
