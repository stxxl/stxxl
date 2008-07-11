/***************************************************************************
 *  include/stxxl/bits/io/iostats.h
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

#ifndef STXXL_IOSTATS_HEADER
#define STXXL_IOSTATS_HEADER

#include <iostream>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/singleton.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//!
//! \{

#ifdef COUNT_WAIT_TIME
extern double wait_time_counter;
#endif

//! \brief Collects various I/O statistics
//! \remarks is a singleton
class stats : public singleton<stats>
{
    friend class singleton<stats>;

    unsigned reads, writes;                     // number of operations
    int64 volume_read, volume_written;          // number of bytes read/written
    double t_reads, t_writes;                   //  seconds spent in operations
    double p_reads, p_writes;                   // seconds spent in parallel operations
    double p_begin_read, p_begin_write;         // start time of parallel operation
    double p_ios;                               // seconds spent in all parallel I/O operations (read and write)
    double p_begin_io;
    int acc_ios;
    int acc_reads, acc_writes;                  // number of requests, participating in parallel operation
    double last_reset;
#ifdef STXXL_BOOST_THREADS
    boost::mutex read_mutex, write_mutex, io_mutex;
#else
    mutex read_mutex, write_mutex, io_mutex;
#endif

    stats();

public:
    //! \brief Returns total number of reads
    //! \return total number of reads
    unsigned get_reads() const
    {
        return reads;
    }

    //! \brief Returns total number of writes
    //! \return total number of writes
    unsigned get_writes() const
    {
        return writes;
    }

    //! \brief Returns number of bytes read from disks
    //! \return number of bytes read
    int64 get_read_volume() const
    {
        return volume_read;
    }

    //! \brief Returns number of bytes written to the disks
    //! \return number of bytes written
    int64 get_written_volume() const
    {
        return volume_written;
    }

    //! \brief Time that would be spent in read syscalls if all parallel reads were serialized.
    //! \return seconds spent in reading
    double get_read_time() const
    {
        return t_reads;
    }

    //! \brief Time that would be spent in write syscalls if all parallel writes were serialized.
    //! \return seconds spent in writing
    double get_write_time() const
    {
        return t_writes;
    }

    //! \brief Period of time when at least one I/O thread was executing a read.
    //! \return seconds spent in reading
    double get_pread_time() const
    {
        return p_reads;
    }

    //! \brief Period of time when at least one I/O thread was executing a write.
    //! \return seconds spent in writing
    double get_pwrite_time() const
    {
        return p_writes;
    }

    //! \brief Period of time when at least one I/O thread was executing a read or a write.
    //! \return seconds spent in I/O
    double get_pio_time() const
    {
        return p_ios;
    }

    //! \brief Return time of the last reset
    //! \return seconds passed from the last reset()
    double get_last_reset_time() const
    {
        return last_reset;
    }

    //! \brief Resets I/O time counters (including I/O wait counter)
    void reset();

    //! \brief Resets I/O wait time counter
    void _reset_io_wait_time();
    //! \brief I/O wait time counter
    //! \return number of seconds spent in I/O waiting functions
    //!  \link request::wait request::wait \endlink,
    //!  \c wait_any and
    //!  \c wait_all
    //! \return number of seconds
    double get_io_wait_time() const;
    //! \brief Increments I/O wait time counter
    //! \param val increment value in seconds
    //! \return new value of I/O wait time counter in seconds
    double increment_io_wait_time(double val);

    // for library use
    void write_started(unsigned size_);
    void write_finished();
    void read_started(unsigned size_);
    void read_finished();
};


class stats_data
{
    unsigned reads, writes;                    // number of operations
    int64 volume_read, volume_written;         // number of bytes read/written
    double t_reads, t_writes;                  // seconds spent in operations
    double p_reads, p_writes;                  // seconds spent in parallel operations
    double p_ios;                              // seconds spent in all parallel I/O operations (read and write)
    double t_wait;
    double elapsed;

public:
    stats_data() :
        reads(0),
        writes(0),
        volume_read(0),
        volume_written(0),
        t_reads(0.0),
        t_writes(0.0),
        p_reads(0.0),
        p_writes(0.0),
        p_ios(0.0),
        t_wait(0.0),
        elapsed(0.0)
    { }

    stats_data(const stats & s) :
        reads(s.get_reads()),
        writes(s.get_writes()),
        volume_read(s.get_read_volume()),
        volume_written(s.get_written_volume()),
        t_reads(s.get_read_time()),
        t_writes(s.get_write_time()),
        p_reads(s.get_pread_time()),
        p_writes(s.get_pwrite_time()),
        p_ios(s.get_pio_time()),
        t_wait(s.get_io_wait_time()),
        elapsed(stxxl_timestamp() - s.get_last_reset_time())
    { }

    stats_data operator + (const stats_data & a) const
    {
        stats_data s;
        s.reads = reads + a.reads;
        s.writes = writes + a.writes;
        s.volume_read = volume_read + a.volume_read;
        s.volume_written = volume_written + a.volume_written;
        s.t_reads = t_reads + a.t_reads;
        s.t_writes = t_writes + a.t_writes;
        s.p_reads = p_reads + a.p_reads;
        s.p_writes = p_writes + a.p_writes;
        s.p_ios = p_ios + a.p_ios;
        s.t_wait = t_wait + a.t_wait;
        s.elapsed = elapsed + a.elapsed;
        return s;
    }

    stats_data operator - (const stats_data & a) const
    {
        stats_data s;
        s.reads = reads - a.reads;
        s.writes = writes - a.writes;
        s.volume_read = volume_read - a.volume_read;
        s.volume_written = volume_written - a.volume_written;
        s.t_reads = t_reads - a.t_reads;
        s.t_writes = t_writes - a.t_writes;
        s.p_reads = p_reads - a.p_reads;
        s.p_writes = p_writes - a.p_writes;
        s.p_ios = p_ios - a.p_ios;
        s.t_wait = t_wait - a.t_wait;
        s.elapsed = elapsed - a.elapsed;
        return s;
    }

    unsigned get_reads() const
    {
        return reads;
    }

    unsigned get_writes() const
    {
        return writes;
    }

    int64 get_read_volume() const
    {
        return volume_read;
    }

    int64 get_written_volume() const
    {
        return volume_written;
    }

    double get_read_time() const
    {
        return t_reads;
    }

    double get_write_time() const
    {
        return t_writes;
    }

    double get_pread_time() const
    {
        return p_reads;
    }

    double get_pwrite_time() const
    {
        return p_writes;
    }

    double get_pio_time() const
    {
        return p_ios;
    }

    double get_elapsed_time() const
    {
        return elapsed;
    }

    double get_io_wait_time() const
    {
        return t_wait;
    }
};

std::ostream & operator << (std::ostream & o, const stats_data & s);

inline std::ostream & operator << (std::ostream & o, const stats & s)
{
    o << stxxl::stats_data(s);
    return o;
}

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IOSTATS_HEADER
// vim: et:ts=4:sw=4
