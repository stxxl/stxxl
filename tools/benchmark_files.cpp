/***************************************************************************
 *  io/benchmark_disks.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
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
#include <stxxl/bits/common/cmdline.h>


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

// returns throughput in MiB/s
static inline double throughput(double bytes, double seconds)
{
    if (seconds == 0.0)
        return 0.0;
    return bytes / (1024 * 1024) / seconds;
}

int benchmark_files(int argc, char * argv[])
{
    stxxl::uint64 offset, length;

    bool no_direct_io = false;
    bool sync_io = false;
    bool resize_after_open = false;
    std::string file_type = default_file_type;
    stxxl::uint64 block_size = 0;
    unsigned int batch_size = 1;
    std::string opstr;

    std::vector<std::string> disks_arr;

    stxxl::cmdline_parser cp;

    cp.add_param_bytes("offset", "Starting offset to write in file.", offset);
    cp.add_param_bytes("length", "Length to write in file.", length);
    cp.add_param_stringlist("filename", "File path to run benchmark on.", disks_arr);

#ifdef RAW_ACCESS
    cp.add_flag(0, "no-direct", "", "open files without O_DIRECT", no_direct_io);
#endif
    cp.add_flag(0, "sync", "", "open files with O_SYNC|O_DSYNC|O_RSYNC", sync_io);
    cp.add_flag(0, "resize", "", "resize the file size after opening, needed e.g. for creating mmap files", resize_after_open);

    cp.add_bytes(0, "block_size", "", "block size for operations (default 8 MiB)", block_size);
    cp.add_uint(0, "batch_size", "", "increase (default 1) to submit several I/Os at once and report average rate", batch_size);

    cp.add_string('f', "file-type", "",
                  "Method to open file (syscall|mmap|wincall|boostfd|...) default: " + file_type, file_type);

    cp.add_string(0, "operations", "",
                  "ops: write, reread and check (default); (R)ead only w/o verification; read only with (V)erification; (W)rite only", opstr);

    cp.set_description("Open a file using one of STXXL's file abstractions and perform write/read/verify tests on the file. "
                       "Block sizes and batch size can be adjusted via command line. "
                       "If length == 0 , then operation will continue till end of space (please ignore the write error). "
                       "Memory consumption: block_size * batch_size * num_disks");

    if (!cp.process(argc,argv))
        return -1;

    stxxl::uint64 endpos = offset + length;

    if (block_size == 0)
        block_size = 8 * MB;

    if (batch_size == 0)
        batch_size = 1;

    bool do_read = true, do_write = true, do_verify = true;

    // deprecated, use --no-direct instead
    if (opstr.size() && (opstr.find("nd") != std::string::npos || opstr.find("ND") != std::string::npos)) {
        no_direct_io = true;
    }

    if (opstr.size() && (opstr.find('r') != std::string::npos || opstr.find('R') != std::string::npos)) {
        do_write = false;
        do_verify = false;
    } else if (opstr.size() && (opstr.find('v') != std::string::npos || opstr.find('V') != std::string::npos)) {
        do_write = false;
    } else if (opstr.size() && (opstr.find('w') != std::string::npos || opstr.find('W') != std::string::npos)) {
        do_read = false;
    }

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

    for (size_t ii = 0; ii < disks_arr.size(); ii++)
    {
        std::cout << "# Add disk: " << disks_arr[ii] << std::endl;
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
        if (!no_direct_io) {
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
              << " O_DIRECT=" << (no_direct_io ? "no" : "yes")
              << " O_SYNC=" << (sync_io ? "yes" : "no")
              << std::endl;
    stxxl::timer t_total(true);
    try {
        while (offset + stxxl::uint64(step_size) <= endpos || length == 0)
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
