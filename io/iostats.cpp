/***************************************************************************
 *  io/iostats.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <string>
#include <sstream>
#include <iomanip>
#include <stxxl/bits/io/iostats.h>


__STXXL_BEGIN_NAMESPACE

double wait_time_counter = 0.0;

stats::stats() :
    reads(0),
    writes(0),
    volume_read(0),
    volume_written(0),
    t_reads(0.0),
    t_writes(0.0),
    p_reads(0.0),
    p_writes(0.0),
    p_begin_read(0.0),
    p_begin_write(0.0),
    p_ios(0.0),
    p_begin_io(0),
    acc_ios(0), acc_reads(0), acc_writes(0),
    last_reset(timestamp())
{ }

void stats::reset()
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock ReadLock(read_mutex);
#else
    read_mutex.lock();
#endif
    //      assert(acc_reads == 0);
    if (acc_reads)
        STXXL_ERRMSG("Warning: " << acc_reads <<
                     " read(s) not yet finished");

    reads = 0;

    volume_read = 0;
    t_reads = 0;
    p_reads = 0.0;

#ifdef STXXL_BOOST_THREADS
    ReadLock.unlock();
    boost::mutex::scoped_lock WriteLock(write_mutex);
#else
    read_mutex.unlock();
    write_mutex.lock();
#endif

    //      assert(acc_writes == 0);
    if (acc_writes)
        STXXL_ERRMSG("Warning: " << acc_writes <<
                     " write(s) not yet finished");

    writes = 0;

    volume_written = 0;
    t_writes = 0.0;
    p_writes = 0.0;

#ifdef STXXL_BOOST_THREADS
    WriteLock.unlock();
#else
    write_mutex.unlock();
#endif

    last_reset = timestamp();

#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock IOLock(io_mutex);
#else
    io_mutex.lock();
#endif

    //      assert(acc_ios == 0);
    if (acc_ios)
        STXXL_ERRMSG("Warning: " << acc_ios <<
                     " io(s) not yet finished");

    p_ios = 0.0;


#ifdef STXXL_BOOST_THREADS
    IOLock.unlock();
#else
    io_mutex.unlock();
#endif

#ifdef COUNT_WAIT_TIME
    stxxl::wait_time_counter = 0.0;
#endif
}


void stats::_reset_io_wait_time()
{
#ifdef COUNT_WAIT_TIME
    stxxl::wait_time_counter = 0.0;
#endif
}

double stats::get_io_wait_time() const
{
#ifdef COUNT_WAIT_TIME
    return stxxl::wait_time_counter;
#else
    return -1.0;
#endif
}

double stats::increment_io_wait_time(double val)
{
#ifdef COUNT_WAIT_TIME
    return stxxl::wait_time_counter += val;
#else
    return -1.0;
#endif
}


void stats::write_started(unsigned size_)
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock WriteLock(write_mutex);
#else
    write_mutex.lock();
#endif
    double now = timestamp();
    ++writes;
    volume_written += size_;
    double diff = now - p_begin_write;
    t_writes += double(acc_writes) * diff;
    p_begin_write = now;
    p_writes += (acc_writes++) ? diff : 0.0;

#ifdef STXXL_BOOST_THREADS
    WriteLock.unlock();
    boost::mutex::scoped_lock IOLock(io_mutex);
#else
    write_mutex.unlock();
    io_mutex.lock();
#endif

    diff = now - p_begin_io;
    p_ios += (acc_ios++) ? diff : 0.0;
    p_begin_io = now;

#ifndef STXXL_BOOST_THREADS
    io_mutex.unlock();
#endif
}

void stats::write_finished()
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock WriteLock(write_mutex);
#else
    write_mutex.lock();
#endif

    double now = timestamp();
    double diff = now - p_begin_write;
    t_writes += double(acc_writes) * diff;
    p_begin_write = now;
    p_writes += (acc_writes--) ? diff : 0.0;

#ifdef STXXL_BOOST_THREADS
    WriteLock.unlock();
    boost::mutex::scoped_lock IOLock(io_mutex);
#else
    write_mutex.unlock();
    io_mutex.lock();
#endif

    diff = now - p_begin_io;
    p_ios += (acc_ios--) ? diff : 0.0;
    p_begin_io = now;

#ifndef STXXL_BOOST_THREADS
    io_mutex.unlock();
#endif
}

void stats::read_started(unsigned size_)
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock ReadLock(read_mutex);
#else
    read_mutex.lock();
#endif
    double now = timestamp();
    ++reads;
    volume_read += size_;
    double diff = now - p_begin_read;
    t_reads += double(acc_reads) * diff;
    p_begin_read = now;
    p_reads += (acc_reads++) ? diff : 0.0;

#ifdef STXXL_BOOST_THREADS
    ReadLock.unlock();
    boost::mutex::scoped_lock IOLock(io_mutex);
#else
    read_mutex.unlock();
    io_mutex.lock();
#endif

    diff = now - p_begin_io;
    p_ios += (acc_ios++) ? diff : 0.0;
    p_begin_io = now;

#ifndef STXXL_BOOST_THREADS
    io_mutex.unlock();
#endif
}

void stats::read_finished()
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock ReadLock(read_mutex);
#else
    read_mutex.lock();
#endif

    double now = timestamp();
    double diff = now - p_begin_read;
    t_reads += double(acc_reads) * diff;
    p_begin_read = now;
    p_reads += (acc_reads--) ? diff : 0.0;

#ifdef STXXL_BOOST_THREADS
    ReadLock.unlock();
    boost::mutex::scoped_lock IOLock(io_mutex);
#else
    read_mutex.unlock();
    io_mutex.lock();
#endif

    diff = now - p_begin_io;
    p_ios += (acc_ios--) ? diff : 0.0;
    p_begin_io = now;

#ifndef STXXL_BOOST_THREADS
    io_mutex.unlock();
#endif
}

std::string hr(uint64 number, const char * unit = "")
{
    // may not overflow, std::numeric_limits<uint64>::max() == 16 EB
    static const char* endings[] = {" ", "K", "M", "G", "T", "P", "E" };
    std::ostringstream out;
    out << number << ' ';
    int scale = 0;
    double number_d = number;
    while (number_d >= 1024.0)
    {
        number_d /= 1024.0;
        ++scale;
    }
    if (scale > 0)
        out << '(' << std::fixed << std::setprecision(3) << number_d << ' ' << endings[scale] << unit << ") ";
    return out.str();
}

std::ostream & operator << (std::ostream & o, const stats_data & s)
{
    o << "STXXL I/O statistics" << std::endl;
    o << " total number of reads                      : " << hr(s.get_reads()) << std::endl;
    o << " number of bytes read from disks            : " << hr(s.get_read_volume(), "B") << std::endl;
    o << " time spent in serving all read requests    : " << s.get_read_time() << " sec."
      << " @ " << (s.get_read_volume() / 1048576.0 / s.get_read_time()) << " MB/sec."
      << std::endl;
    o << " time spent in reading (parallel read time) : " << s.get_pread_time() << " sec."
      << " @ " << (s.get_read_volume() / 1048576.0 / s.get_pread_time()) << " MB/sec."
      << std::endl;
    o << " total number of writes                     : " << hr(s.get_writes()) << std::endl;
    o << " number of bytes written to disks           : " << hr(s.get_written_volume(), "B") << std::endl;
    o << " time spent in serving all write requests   : " << s.get_write_time() << " sec."
      << " @ " << (s.get_written_volume() / 1048576.0 / s.get_write_time()) << " MB/sec."
      << std::endl;
    o << " time spent in writing (parallel write time): " << s.get_pwrite_time() << " sec."
      << " @ " << (s.get_written_volume() / 1048576.0 / s.get_pwrite_time()) << " MB/sec."
      << std::endl;
    o << " time spent in I/O (parallel I/O time)      : " << s.get_pio_time() << " sec."
      << " @ " << ((s.get_read_volume() + s.get_written_volume()) / 1048576.0 / s.get_pio_time()) << " MB/sec."
      << std::endl;
    o << " I/O wait time                              : " << s.get_io_wait_time() << " sec." << std::endl;
    o << " Time since the last reset                  : " << s.get_elapsed_time() << " sec." << std::endl;
    return o;
}

__STXXL_END_NAMESPACE
