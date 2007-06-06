/***************************************************************************
 *            disktest.cpp
 *
 *  Sat Aug 24 23:52:15 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "io.h"
#include "stdio.h"
#include <vector>

#ifndef BOOST_MSVC
 #include <unistd.h>
#endif

#include "../common/aligned_alloc.h"

using namespace stxxl;
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
                    out[i] = stxxl_timestamp();
                    count++;
                }
        }
    }
    delete [] finished;
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

int main(int argc, char * argv[])
{
    unsigned ndisks = 8;
    unsigned buffer_size = 1024 * 1024 * 64;
    unsigned buffer_size_int = buffer_size / sizeof(int);

    unsigned i = 0, j = 0;

#ifndef RAW_ACCESS
/*
   char * disk_names_dev[] =
   {
   "/dev/hde",
   "/dev/hdg",
   "/dev/hdi",
   "/dev/hdk",
   "/dev/hdm",
   "/dev/hdo",
   "/dev/hdq",
   "/dev/hds"
   };
 */


#else
/*
   const char * disk_names_dev[] =
   {
   "/dev/raw/raw1",
   "/dev/raw/raw2",
   "/dev/raw/raw3",
   "/dev/raw/raw4",
   "/dev/raw/raw5",
   "/dev/raw/raw6",
   "/dev/raw/raw7",
   "/dev/raw/raw8"
   };
 */

    const char * disk_names_dev[] =
    {
        "/mnt/hdc/stxxl",
        "/mnt/hde/stxxl",
        "/mnt/hdg/stxxl",
        "/mnt/hdi/stxxl",
        "/mnt/hdk/stxxl",
        "/mnt/hdm/stxxl",
        "/mnt/hdq/stxxl",
        "/mnt/hds/stxxl",
        "/mnt/hdu/stxxl",
        "/mnt/hdw/stxxl",
        "/mnt/raid/stxxl"
    };
#endif


#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)

    stxxl::int64 offset = 0;
    stxxl::int64 end_offset = stxxl::int64(GB) * stxxl::int64(atoi(argv[1]));
    std::vector<std::string> disks_arr;

    for (i = 1; i < unsigned (argc - 1); i++)
    {
        std::cout << "Add disk: " << disk_names_dev[atoi(argv[i + 1])]
                  << std::endl;
        disks_arr.push_back(disk_names_dev[atoi(argv[i + 1])]);
    }
    ndisks = argc - 2;

    unsigned chunks = 32;
    request_ptr * reqs = new request_ptr [ndisks * chunks];
    file * * disks = new file *[ndisks];
    int * buffer = (int *)aligned_alloc<BLOCK_ALIGN>(buffer_size * ndisks);
#ifdef WATCH_TIMES
    double * r_finish_times = new double[ndisks];
    double * w_finish_times = new double[ndisks];
#endif

    int count  = (end_offset - offset) / buffer_size;


    for (i = 0; i < ndisks * buffer_size_int; i++)
        buffer[i] = i;


    for (i = 0; i < ndisks; i++)
    {
        disks[i] = new syscall_file(disks_arr[i],
                                    file::CREAT | file::RDWR | file::DIRECT, i);
    }

    while (count--)
    {
        std::cout << "Disk offset " << offset / MB << " MB ";


        double begin = stxxl_timestamp(), end;


        begin = stxxl_timestamp();

        for (i = 0; i < ndisks; i++)
        {
            for (j = 0; j < chunks; j++)
                reqs[i * chunks + j] =
                    disks[i]->aread(        buffer + buffer_size_int * i + buffer_size_int * j / chunks,
                                            offset + buffer_size * j / chunks,
                                            buffer_size / chunks,
                                            stxxl::default_completion_handler() );

        }

#ifdef WATCH_TIMES
        watch_times(reqs, ndisks, r_finish_times);
#else
        wait_all( reqs, ndisks * chunks );
#endif

        end = stxxl_timestamp();

        std::cout << int (1e-6 * (buffer_size) / (end - begin)) << " MB/s" << std::endl;

#ifdef WATCH_TIMES
        out_stat(begin, end, r_finish_times, ndisks, disks_arr);
#endif

        offset += /* 4*stxxl::int64(GB); */ buffer_size;
    }

    delete [] reqs;
    delete [] disks;
    aligned_dealloc<BLOCK_ALIGN>(buffer);

#ifdef WATCH_TIMES
    delete [] r_finish_times;
    delete [] w_finish_times;
#endif

    return 0;
}
