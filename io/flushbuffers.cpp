/***************************************************************************
 *  io/flushbuffers.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <vector>

#include <stxxl/io>
#include <stxxl/aligned_alloc>

#ifndef BOOST_MSVC
 #include <unistd.h>
#endif


using stxxl::timestamp;
using stxxl::file;
using stxxl::request_ptr;


#ifdef BLOCK_ALIGN
 #undef BLOCK_ALIGN
#endif

#define BLOCK_ALIGN  4096

#define NOREAD

//#define DO_ONLY_READ

#define POLL_DELAY 1000

#define RAW_ACCESS

//#define WATCH_TIMES


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

int main(int argc, char * argv[])
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " length_in_GiB diskfile..." << std::endl;
        return -1;
    }

    stxxl::int64 offset = 0;
    stxxl::int64 end_offset = stxxl::int64(GB) * stxxl::int64(atoi(argv[1]));
    std::vector<std::string> disks_arr;

    for (int ii = 2; ii < argc; ++ii)
    {
        std::cout << "# Add disk: " << argv[ii] << std::endl;
        disks_arr.push_back(argv[ii]);
    }

    const unsigned ndisks = disks_arr.size();
    const unsigned buffer_size = 1024 * 1024 * 64;
    const unsigned buffer_size_int = buffer_size / sizeof(int);
    const unsigned chunks = 32;
    request_ptr * reqs = new request_ptr[ndisks * chunks];
    file ** disks = new file *[ndisks];
    int * buffer = (int *)stxxl::aligned_alloc<BLOCK_ALIGN>(buffer_size * ndisks);
#ifdef WATCH_TIMES
    double * r_finish_times = new double[ndisks];
    double * w_finish_times = new double[ndisks];
#endif

    int count = (end_offset - offset) / buffer_size;

    unsigned i = 0, j = 0;

    for (i = 0; i < ndisks * buffer_size_int; i++)
        buffer[i] = i;

    for (i = 0; i < ndisks; i++)
    {
        disks[i] = new stxxl::syscall_file(disks_arr[i],
                                           file::CREAT | file::RDWR | file::DIRECT, i);
    }

    while (count--)
    {
        std::cout << "Disk offset " << offset / MB << " MiB ";

        double begin, end;

        begin = timestamp();

        for (i = 0; i < ndisks; i++)
        {
            for (j = 0; j < chunks; j++)
                reqs[i * chunks + j] =
                    disks[i]->aread(buffer + buffer_size_int * i + buffer_size_int * j / chunks,
                                    offset + buffer_size * j / chunks,
                                    buffer_size / chunks,
                                    stxxl::default_completion_handler());
        }

#ifdef WATCH_TIMES
        watch_times(reqs, ndisks, r_finish_times);
#else
        wait_all(reqs, ndisks * chunks);
#endif

        end = timestamp();

        std::cout << int(double(buffer_size) / MB / (end - begin)) << " MiB/s" << std::endl;

#ifdef WATCH_TIMES
        out_stat(begin, end, r_finish_times, ndisks, disks_arr);
#endif

        offset += /* 4*stxxl::int64(GB); */ buffer_size;
    }

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
