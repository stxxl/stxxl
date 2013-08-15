/***************************************************************************
 *  io/benchmark_disks.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
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
#include <stxxl/timer>
#include <stxxl/bits/version.h>


using stxxl::request_ptr;
using stxxl::file;
using stxxl::timer;
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


#ifdef BOOST_MSVC
const char * default_file_type = "wincall";
#else
const char * default_file_type = "syscall";
#endif

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
    std::cout << "Usage: " << argv0 << " [options] offset length [block_size [batch_size]] [r|v|w] [--] diskfile..." << std::endl;
    std::cout <<
    "Options:\n"
#ifdef RAW_ACCESS
    "    --no-direct             open files without O_DIRECT\n"
#endif
    "    --sync                  open files with O_SYNC|O_DSYNC|O_RSYNC\n"
    "    --file-type=syscall|mmap|wincall|boostfd|...    default: " << default_file_type << "\n"
    "    --resize                resize the file size after opening,\n"
    "                            needed e.g. for creating mmap files\n"
    << std::endl;
    std::cout << "    starting 'offset' and 'length' are given in GiB," << std::endl;
    std::cout << "    'block_size' (default 8) in MiB (in B if it has a suffix B)," << std::endl;
    std::cout << "    increase 'batch_size' (default 1)" << std::endl;
    std::cout << "    to submit several I/Os at once and report average rate" << std::endl;
    std::cout << "    ops: write, reread and check (default); (R)ead only w/o verification;" << std::endl;
    std::cout << "         read only with (V)erification; (W)rite only" << std::endl;
    std::cout << "    length == 0 implies till end of space (please ignore the write error)" << std::endl;
    std::cout << "    Memory consumption: block_size * batch_size * num_disks" << std::endl;
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
    bool direct_io = true;
    bool sync_io = false;
    bool resize_after_open = false;
    const char * file_type = default_file_type;

    int arg_curr = 1;

    // FIXME: use something like getopt() instead of reinventing the wheel,
    // but there is some portability problem ...
    while (arg_curr < argc) {
        char * arg = argv[arg_curr];
        char * arg_opt = strchr(argv[arg_curr], '=');
        if (arg_opt) {
            *arg_opt = 0;
            ++arg_opt;  // skip '='
        }
        if (strcmp(arg, "--no-direct") == 0) {
            direct_io = false;
        } else if (strcmp(arg, "--sync") == 0) {
            sync_io = true;
        } else if (strcmp(arg, "--file-type") == 0) {
            if (!arg_opt) {
                ++arg_curr;
                if (arg_curr < argc)
                    arg_opt = argv[arg_curr];
                else
                    throw std::invalid_argument(std::string("missing argument for ") + arg);
            }
            file_type = arg_opt;
        } else if (strcmp(arg, "--resize") == 0) {
            resize_after_open = true;
        } else if (strncmp(arg, "--", 2) == 0) {
            throw std::invalid_argument(std::string("unknown option ") + arg);
        } else {
            // remaining arguments are "old style" command line
            break;
        }
        ++arg_curr;
    }

    if (argc < arg_curr + 3)
        usage(argv[0]);

    stxxl::int64 offset = stxxl::int64(GB) * stxxl::int64(atoi(argv[arg_curr]));
    stxxl::int64 length = stxxl::int64(GB) * stxxl::int64(atoi(argv[arg_curr + 1]));
    stxxl::int64 endpos = offset + length;
    stxxl::int64 block_size = 0;
    stxxl::int64 batch_size = 0;

    bool do_read = true, do_write = true, do_verify = true;
    int first_disk_arg = arg_curr + 2;

    if (first_disk_arg < argc)
        block_size = atoi(argv[first_disk_arg]);
    if (block_size > 0) {
        int l = strlen(argv[first_disk_arg]);
        if (argv[first_disk_arg][l - 1] == 'B' || argv[first_disk_arg][l - 1] == 'b') {
            // suffix B means exact size
        } else {
            block_size *= MB;
        }
        ++first_disk_arg;
    } else {
        block_size = 8 * MB;
    }

    if (first_disk_arg < argc)
        batch_size = atoi(argv[first_disk_arg]);
    if (batch_size > 0) {
        ++first_disk_arg;
    } else {
        batch_size = 1;
    }

    // deprecated, use --no-direct instead
    if (first_disk_arg < argc && (strcmp("nd", argv[first_disk_arg]) == 0 || strcmp("ND", argv[first_disk_arg]) == 0)) {
        direct_io = false;
        ++first_disk_arg;
    }

    if (first_disk_arg < argc && (strcmp("r", argv[first_disk_arg]) == 0 || strcmp("R", argv[first_disk_arg]) == 0)) {
        do_write = false;
        do_verify = false;
        ++first_disk_arg;
    } else if (first_disk_arg < argc && (strcmp("v", argv[first_disk_arg]) == 0 || strcmp("V", argv[first_disk_arg]) == 0)) {
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

    const char * myrev = "$Revision$";
    const char * myself = strrchr(argv[0], '/');
    if (!myself || !*(++myself))
        myself = argv[0];
    std::cout << "# " << myself << " " << myrev;
#ifdef STXXL_DIRECT_IO_OFF
    std::cout << " STXXL_DIRECT_IO_OFF";
#endif
    std::cout << std::endl;
    std::cout << "# " << stxxl::get_version_string() << std::endl;

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
        int openmode = file::CREAT | file::RDWR;
        if (direct_io) {
#ifdef RAW_ACCESS
            openmode |= file::DIRECT;
#endif
        }
        if (sync_io) {
            openmode |= file::SYNC;
        }

        disks[i] = stxxl::create_file(file_type, disks_arr[i], openmode, i);
        if (resize_after_open)
            disks[i]->set_size(endpos);
    }

#ifdef DO_ONLY_READ
    do_write = false;
#endif
#ifdef NOREAD
    do_read = false;
#endif

    std::cout << "# Step size: "
              << step_size << " bytes per disk ("
              << batch_size << " block" << (batch_size == 1 ? "" : "s") << " of "
              << block_size << " bytes)"
              << " file_type=" << file_type
              << " O_DIRECT=" << (direct_io ? "yes" : "no")
              << " O_SYNC=" << (sync_io ? "yes" : "no")
              << std::endl;
    timer t_total(true);
    try {
        while (offset + stxxl::int64(step_size) <= endpos || length == 0)
        {
            const stxxl::int64 current_step_size = (length == 0) ? stxxl::int64(step_size) : std::min<stxxl::int64>(step_size, endpos - offset);
            const stxxl::int64 current_step_size_int = current_step_size / sizeof(int);
            const unsigned current_num_blocks = stxxl::div_ceil(current_step_size, block_size);

            std::cout << "Disk offset    " << std::setw(8) << offset / MB << " MiB: " << std::fixed;

            double begin = timestamp(), end, elapsed;

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
                      << " \nElapsed time: " << end - begin
                      << " \nThroughput: " << int(double(current_step_size * ndisks) / MB / (end - begin))
                      << " MiB/s \nPer one disk:"
                      << int(double(current_step_size) / MB / (end - begin)) << " MiB/s"
                      << std::endl;
#endif

 #ifdef WATCH_TIMES
            out_stat(begin, end, w_finish_times, ndisks, disks_arr);
 #endif
            std::cout << std::setw(2) << ndisks << " * "
                      << std::setw(8) << std::setprecision(3) << (throughput(current_step_size, elapsed)) << " = "
                      << std::setw(8) << std::setprecision(3) << (throughput(current_step_size, elapsed) * ndisks) << " MiB/s write,";


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
                      << " \nElapsed time: " << end - begin
                      << " \nThroughput: " << int(double(current_step_size * ndisks) / MB / (end - begin))
                      << " MiB/s \nPer one disk:"
                      << int(double(current_step_size) / MB / (end - begin)) << " MiB/s"
                      << std::endl;
#endif

            std::cout << std::setw(2) << ndisks << " * "
                      << std::setw(8) << std::setprecision(3) << (throughput(current_step_size, elapsed)) << " = "
                      << std::setw(8) << std::setprecision(3) << (throughput(current_step_size, elapsed) * ndisks) << " MiB/s read";

#ifdef WATCH_TIMES
            out_stat(begin, end, r_finish_times, ndisks, disks_arr);
#endif

            if (do_read && do_verify) {
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
            std::cout << std::endl;

            offset += current_step_size;
        }
    }
    catch (const std::exception & ex)
    {
        std::cout << std::endl;
        STXXL_ERRMSG(ex.what());
    }
    t_total.stop();

    std::cout << "=============================================================================================" << std::endl;
    // the following line of output is parsed by misc/diskbench-avgplot.sh
    std::cout << "# Average over " << std::setw(8) << stxxl::STXXL_MAX(totalsizewrite, totalsizeread) / MB << " MiB: ";
    std::cout << std::setw(2) << ndisks << " * "
              << std::setw(8) << std::setprecision(3) << (throughput(totalsizewrite, totaltimewrite)) << " = "
              << std::setw(8) << std::setprecision(3) << (throughput(totalsizewrite, totaltimewrite) * ndisks) << " MiB/s write,";
    std::cout << std::setw(2) << ndisks << " * "
              << std::setw(8) << std::setprecision(3) << (throughput(totalsizeread, totaltimeread)) << " = "
              << std::setw(8) << std::setprecision(3) << (throughput(totalsizeread, totaltimeread) * ndisks) << " MiB/s read"
              << std::endl;
    if (totaltimewrite != 0.0)
        std::cout << "# Write time   " << std::setw(8) << std::setprecision(3) << totaltimewrite << " s" << std::endl;
    if (totaltimeread != 0.0)
        std::cout << "# Read time    " << std::setw(8) << std::setprecision(3) << totaltimeread << " s" << std::endl;
    std::cout << "# Non-I/O time " << std::setw(8) << std::setprecision(3) << (t_total.seconds() - totaltimewrite - totaltimeread) << " s, average throughput "
              << std::setw(8) << std::setprecision(3) << (throughput(totalsizewrite + totalsizeread, t_total.seconds() - totaltimewrite - totaltimeread) * ndisks) << " MiB/s"
              << std::endl;
    std::cout << "# Total time   " << std::setw(8) << std::setprecision(3) << t_total.seconds() << " s, average throughput "
              << std::setw(8) << std::setprecision(3) << (throughput(totalsizewrite + totalsizeread, t_total.seconds()) * ndisks) << " MiB/s"
              << std::endl;

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
