/***************************************************************************
 *  include/stxxl/bits/io/iostats.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009, 2010 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2016 Alex Schickedanz <alex@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_IOSTATS_HEADER
#define STXXL_IO_IOSTATS_HEADER

#ifndef STXXL_IO_STATS
 #define STXXL_IO_STATS 1
#endif

#include <stxxl/bits/common/timer.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/deprecated.h>
#include <stxxl/bits/singleton.h>
#include <stxxl/bits/unused.h>

#include <iostream>
#include <mutex>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <limits>

namespace stxxl {

//! \addtogroup iolayer
//!
//! \{

class file_stats
{
    const unsigned m_device_id;

    unsigned reads, writes;                     // number of operations
    external_size_type  volume_read, volume_written;          // number of bytes read/written
    unsigned c_reads, c_writes;                 // number of cached operations
    external_size_type  c_volume_read, c_volume_written;      // number of bytes read/written from/to cache
    double t_reads, t_writes;                   // seconds spent in operations
    double p_begin_read, p_begin_write;         // start time of parallel operation

    int acc_reads, acc_writes;                  // number of requests, participating in parallel operation

    std::mutex read_mutex, write_mutex;

public:

    file_stats(unsigned int device_id);

    class scoped_read_write_timer
    {
        typedef size_t size_type;
        file_stats& file_stats_;

        bool is_write;
#if STXXL_IO_STATS
        bool running;
#endif

    public:
        explicit scoped_read_write_timer(file_stats* file_stats, size_type size, bool is_write = false)
                : file_stats_(*file_stats), is_write(is_write)
#if STXXL_IO_STATS
                , running(false)
#endif
        {
            start(size);
        }

        ~scoped_read_write_timer()
        {
            stop();
        }

        void start(size_type size)
        {
#if STXXL_IO_STATS
            if (!running) {
                running = true;
                if (is_write)
                    file_stats_.write_started(size);
                else
                    file_stats_.read_started(size);
            }
#else
            STXXL_UNUSED(size);
#endif
        }

        void stop()
        {
#if STXXL_IO_STATS
            if (running) {
                if (is_write)
                    file_stats_.write_finished();
                else
                    file_stats_.read_finished();
                running = false;
            }
#endif
        }
    };

    class scoped_write_timer
    {
        typedef size_t size_type;
        file_stats& file_stats_;

#if STXXL_IO_STATS
        bool running;
#endif

    public:
        explicit scoped_write_timer(file_stats* file_stats, size_type size)
            : file_stats_(*file_stats)

#if STXXL_IO_STATS
                , running(false)
#endif
        {
            start(size);
        }

        ~scoped_write_timer()
        {
            stop();
        }

        void start(size_type size)
        {
#if STXXL_IO_STATS
            if (!running) {
                running = true;
                file_stats_.write_started(size);
            }
#else
            STXXL_UNUSED(size);
#endif
        }

        void stop()
        {
#if STXXL_IO_STATS
            if (running) {
                file_stats_.write_finished();
                running = false;
            }
#endif
        }
    };

    class scoped_read_timer
    {
        typedef size_t size_type;
        file_stats& file_stats_;

#if STXXL_IO_STATS
        bool running;
#endif

    public:
        explicit scoped_read_timer(file_stats* file_stats, size_type size)
                : file_stats_(*file_stats)
#if STXXL_IO_STATS
                , running(false)
#endif
        {
            start(size);
        }

        ~scoped_read_timer()
        {
            stop();
        }

        void start(size_type size)
        {
#if STXXL_IO_STATS
            if (!running) {
                running = true;
                file_stats_.read_started(size);
            }
#else
            STXXL_UNUSED(size);
#endif
        }

        void stop()
        {
#if STXXL_IO_STATS
            if (running) {
                file_stats_.read_finished();
                running = false;
            }
#endif
        }
    };

public:
    //! Returns the device id.
    //! \return device id
    unsigned get_device_id() const
    {
        return m_device_id;
    }

    //! Returns total number of reads.
    //! \return total number of reads
    unsigned get_reads() const
    {
        return reads;
    }

    //! Returns total number of writes.
    //! \return total number of writes
    unsigned get_writes() const
    {
        return writes;
    }

    //! Returns number of bytes read from disks.
    //! \return number of bytes read
    external_size_type get_read_volume() const
    {
        return volume_read;
    }

    //! Returns number of bytes written to the disks.
    //! \return number of bytes written
    external_size_type get_written_volume() const
    {
        return volume_written;
    }

    //! Returns total number of reads served from cache.
    //! \return total number of cached reads
    unsigned get_cached_reads() const
    {
        return c_reads;
    }

    //! Returns total number of cached writes.
    //! \return total number of cached writes
    unsigned get_cached_writes() const
    {
        return c_writes;
    }

    //! Returns number of bytes read from cache.
    //! \return number of bytes read from cache
    external_size_type get_cached_read_volume() const
    {
        return c_volume_read;
    }

    //! Returns number of bytes written to the cache.
    //! \return number of bytes written to cache
    external_size_type get_cached_written_volume() const
    {
        return c_volume_written;
    }

    //! Time that would be spent in read syscalls if all parallel reads were serialized.
    //! \return seconds spent in reading
    double get_read_time() const
    {
        return t_reads;
    }

    //! Time that would be spent in write syscalls if all parallel writes were serialized.
    //! \return seconds spent in writing
    double get_write_time() const
    {
        return t_writes;
    }

    // for library use
    void write_started(size_t size_, double now = 0.0);
    void write_canceled(size_t size_);
    void write_finished();
    void write_cached(size_t size_);
    void read_started(size_t size_, double now = 0.0);
    void read_canceled(size_t size_);
    void read_finished();
    void read_cached(size_t size_);
};

class file_stats_data
{
    //! device id
    unsigned device_id;
    //! number of operations
    unsigned reads, writes;
    //! number of bytes read/written
    external_size_type volume_read, volume_written;
    //! number of cached operations
    unsigned c_reads, c_writes;
    //! number of bytes read/written from/to cache
    external_size_type c_volume_read, c_volume_written;
    //! seconds spent in operations
    double t_reads, t_writes;

public:
    file_stats_data()
            : device_id(std::numeric_limits<unsigned>::max()),
              reads(0),
              writes(0),
              volume_read(0),
              volume_written(0),
              c_reads(0),
              c_writes(0),
              c_volume_read(0),
              c_volume_written(0),
              t_reads(0.0),
              t_writes(0.0)
    { }

    file_stats_data(const file_stats& fs)
            : device_id(fs.get_device_id()),
              reads(fs.get_reads()),
              writes(fs.get_writes()),
              volume_read(fs.get_read_volume()),
              volume_written(fs.get_written_volume()),
              c_reads(fs.get_cached_reads()),
              c_writes(fs.get_cached_writes()),
              c_volume_read(fs.get_cached_read_volume()),
              c_volume_written(fs.get_cached_written_volume()),
              t_reads(fs.get_read_time()),
              t_writes(fs.get_write_time())
    { }

    file_stats_data operator + (const file_stats_data& a) const
    {
        STXXL_THROW_IF(device_id != a.device_id, std::runtime_error,
                       "stxxl::file_stats_data objects do not belong to the same file/disk");

        file_stats_data fsd;
        fsd.device_id = device_id;

        fsd.reads = reads + a.reads;
        fsd.writes = writes + a.writes;
        fsd.volume_read = volume_read + a.volume_read;
        fsd.volume_written = volume_written + a.volume_written;
        fsd.c_reads = c_reads + a.c_reads;
        fsd.c_writes = c_writes + a.c_writes;
        fsd.c_volume_read = c_volume_read + a.c_volume_read;
        fsd.c_volume_written = c_volume_written + a.c_volume_written;
        fsd.t_reads = t_reads + a.t_reads;
        fsd.t_writes = t_writes + a.t_writes;
        return fsd;
    }

    file_stats_data operator - (const file_stats_data& a) const
    {
        STXXL_THROW_IF(device_id != a.device_id, std::runtime_error, "stxxl::file_stats_data objects do not belong to the same file/disk");

        file_stats_data fsd;
        fsd.device_id = device_id;

        fsd.reads = reads - a.reads;
        fsd.writes = writes - a.writes;
        fsd.volume_read = volume_read - a.volume_read;
        fsd.volume_written = volume_written - a.volume_written;
        fsd.c_reads = c_reads - a.c_reads;
        fsd.c_writes = c_writes - a.c_writes;
        fsd.c_volume_read = c_volume_read - a.c_volume_read;
        fsd.c_volume_written = c_volume_written - a.c_volume_written;
        fsd.t_reads = t_reads - a.t_reads;
        fsd.t_writes = t_writes - a.t_writes;
        return fsd;
    }

    unsigned get_device_id() const
    {
        return device_id;
    }

    unsigned get_reads() const
    {
        return reads;
    }

    unsigned get_writes() const
    {
        return writes;
    }

    external_size_type get_read_volume() const
    {
        return volume_read;
    }

    external_size_type get_written_volume() const
    {
        return volume_written;
    }

    unsigned get_cached_reads() const
    {
        return c_reads;
    }

    unsigned get_cached_writes() const
    {
        return c_writes;
    }

    external_size_type get_cached_read_volume() const
    {
        return c_volume_read;
    }

    external_size_type get_cached_written_volume() const
    {
        return c_volume_written;
    }

    double get_read_time() const
    {
        return t_reads;
    }

    double get_write_time() const
    {
        return t_writes;
    }
};

//! Collects various I/O statistics.
//! \remarks is a singleton
class stats : public singleton<stats>
{
    friend class singleton<stats>;
    friend class file_stats;

    const double creation_time;

    //! need std::list here, because the io::file objects keep a pointer to the
    //! enclosed file_stats objects and this list may grow.
    std::list<file_stats> file_stats_list;

    // parallel times have to be counted globally
    double p_reads, p_writes;                   // seconds spent in parallel operations
    double p_begin_read, p_begin_write;         // start time of parallel operation
    double p_ios;                               // seconds spent in all parallel I/O operations (read and write)
    double p_begin_io;

    int acc_reads, acc_writes;                  // number of requests, participating in parallel operation
    int acc_ios;

    // waits are measured globally
    double t_waits, p_waits;                    // seconds spent waiting for completion of I/O operations
    double p_begin_wait;
    double t_wait_read, p_wait_read;
    double p_begin_wait_read;
    double t_wait_write, p_wait_write;
    double p_begin_wait_write;
    int acc_waits;
    int acc_wait_read, acc_wait_write;

    std::mutex wait_mutex, read_mutex, write_mutex, io_mutex;

    stats() : creation_time(timestamp()) { }

public:

    enum wait_op_type {
        WAIT_OP_ANY,
        WAIT_OP_READ,
        WAIT_OP_WRITE
    };

    class scoped_wait_timer
    {
#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
        bool running;
        wait_op_type wait_op;
#endif

    public:
        scoped_wait_timer(wait_op_type wait_op, bool measure_time = true)
#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
                : running(false), wait_op(wait_op)
#endif
        {
            if (measure_time)
                start();
        }

        ~scoped_wait_timer()
        {
            stop();
        }

        void start()
        {
#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
            if (!running) {
                running = true;
                stats::get_instance()->wait_started(wait_op);
            }
#endif
        }

        void stop()
        {
#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
            if (running) {
                stats::get_instance()->wait_finished(wait_op);
                running = false;
            }
#endif
        }
    };

public:
    //! return list of file's stats data (deeply copied from each file_stats)
    std::vector<file_stats_data> deepcopy_file_stats_data_list() const;

    double get_creation_time() const
    {
        return creation_time;
    }

    //! create new instance of a file_stats for an io::file to collect
    //! statistics. (for internal library use.)
    file_stats* create_file_stats(unsigned device_id);

    //! I/O wait time counter.
    //! \return number of seconds spent in I/O waiting functions \link
    //! request::wait request::wait \endlink, \c wait_any and \c wait_all
    double get_io_wait_time() const
    {
        return t_waits;
    }

    double get_wait_read_time() const
    {
        return t_wait_read;
    }

    double get_wait_write_time() const
    {
        return t_wait_write;
    }

    //! Period of time when at least one I/O thread was executing a read.
    //! \return seconds spent in reading
    double get_pread_time() const
    {
        return p_reads;
    }

    //! Period of time when at least one I/O thread was executing a write.
    //! \return seconds spent in writing
    double get_pwrite_time() const
    {
        return p_writes;
    }

    //! Period of time when at least one I/O thread was executing a read or a write.
    //! \return seconds spent in I/O
    double get_pio_time() const
    {
        return p_ios;
    }

    // for library use
private:    // only called from file_stats
    void p_write_started(double now);
    void p_write_finished(double now);
    void p_read_started(double now);
    void p_read_finished(double now);

public:
    void wait_started(wait_op_type wait_op);
    void wait_finished(wait_op_type wait_op);
};

#if !STXXL_IO_STATS
inline void stats::write_started(size_t size_, double now)
{
    STXXL_UNUSED(size_);
    STXXL_UNUSED(now);
}
inline void stats::write_cached(size_t size_)
{
    STXXL_UNUSED(size_);
}
inline void stats::write_finished() { }
inline void stats::read_started(size_t size_, double now)
{
    STXXL_UNUSED(size_);
    STXXL_UNUSED(now);
}
inline void stats::read_cached(size_t size_)
{
    STXXL_UNUSED(size_);
}
inline void stats::read_finished() { }
#endif
#ifdef STXXL_DO_NOT_COUNT_WAIT_TIME
inline void stats::wait_started(wait_op_type) { }
inline void stats::wait_finished(wait_op_type) { }
#endif

class stats_data
{
    //! seconds spent in parallel io
    double p_reads, p_writes, p_ios;

    //! seconds spent waiting for completion of I/O operations
    double t_wait;
    double t_wait_read, t_wait_write;

    double elapsed;

    //! list of individual file statistics.
    std::vector<file_stats_data> m_file_stats_data_list;

    template <typename T, typename Functor>
    T fetch_sum(const Functor& get_value) const
    {
        T sum = 0;
        for(auto it = m_file_stats_data_list.begin(); it != m_file_stats_data_list.end(); it++)
        {
            sum += get_value(*it);
        }

        return sum;
    }

public:

    template <typename T>
    struct measurement_summary
    {
        T total, min, max;
        double avg, med;
        std::vector<std::pair<T,unsigned>> values_per_device;

        template <typename Functor>
        measurement_summary(const std::vector<file_stats_data>& fs,
                            const Functor& get_value)
            : total(0)
        {
            values_per_device.reserve(fs.size());

            for(auto it = fs.begin(); it != fs.end(); it++)
            {
                values_per_device.emplace_back(get_value(*it),it->get_device_id());
            }

            std::sort(values_per_device.begin(), values_per_device.end(),[](std::pair<T,unsigned> a, std::pair<T,unsigned> b){ return a.first < b.first; });

            min = values_per_device.front().first;
            max = values_per_device.back().first;
            long middle = values_per_device.size() / 2;
            med = (values_per_device.size() % 2 == 1) ? values_per_device[middle].first : (values_per_device[middle - 1].first + values_per_device[middle].first) / 2.0;

            for(auto it = values_per_device.begin(); it != values_per_device.end(); it++)
            {
                total += it->first;
            }

            avg = (double) total / values_per_device.size();
        }

    };

public:
    stats_data()
        : p_reads(0.0),
          p_writes(0.0),
          p_ios(0.0),
          t_wait(0.0),
          t_wait_read(0.0),
          t_wait_write(0.0),
          elapsed(0.0)
    { }

    stats_data(const stats& s) // implicit conversion -- NOLINT
        : p_reads(s.get_pread_time()),
          p_writes(s.get_pwrite_time()),
          p_ios(s.get_pio_time()),
          t_wait(s.get_io_wait_time()),
          t_wait_read(s.get_wait_read_time()),
          t_wait_write(s.get_wait_write_time()),
          elapsed(timestamp() - s.get_creation_time()),
          m_file_stats_data_list(s.deepcopy_file_stats_data_list())
    { }

    stats_data operator + (const stats_data& a) const
    {
        stats_data s;

        if(a.m_file_stats_data_list.size() == 0)
        {
            s.m_file_stats_data_list = m_file_stats_data_list;
        }
        else if(m_file_stats_data_list.size() == 0)
        {
            s.m_file_stats_data_list = a.m_file_stats_data_list;
        }
        else if(m_file_stats_data_list.size() == a.m_file_stats_data_list.size())
        {
            for (auto it1 = m_file_stats_data_list.begin(), it2 = a.m_file_stats_data_list.begin();
                 it1 != m_file_stats_data_list.end(); it1++, it2++)
            {
                s.m_file_stats_data_list.push_back((*it1) + (*it2));
            }
        }
        else
        {
            STXXL_THROW(std::runtime_error, "The number of files has changed between the snapshots.");
        }

        s.p_reads = p_reads + a.p_reads;
        s.p_writes = p_writes + a.p_writes;
        s.p_ios = p_ios + a.p_ios;
        s.t_wait = t_wait + a.t_wait;
        s.t_wait_read = t_wait_read + a.t_wait_read;
        s.t_wait_write = t_wait_write + a.t_wait_write;
        s.elapsed = elapsed + a.elapsed;
        return s;
    }

    stats_data operator - (const stats_data& a) const
    {
        stats_data s;

        if(a.m_file_stats_data_list.size() == 0)
        {
            s.m_file_stats_data_list = m_file_stats_data_list;
        }
        else if(m_file_stats_data_list.size() == a.m_file_stats_data_list.size())
        {
            for (auto it1 = m_file_stats_data_list.begin(), it2 = a.m_file_stats_data_list.begin();
                 it1 != m_file_stats_data_list.end(); it1++, it2++)
            {
                s.m_file_stats_data_list.push_back((*it1) - (*it2));
            }
        }
        else
        {
            STXXL_THROW(std::runtime_error,"The number of files has changed between the snapshots.");
        }

        s.p_reads = p_reads - a.p_reads;
        s.p_writes = p_writes - a.p_writes;
        s.p_ios = p_ios - a.p_ios;
        s.t_wait = t_wait - a.t_wait;
        s.t_wait_read = t_wait_read - a.t_wait_read;
        s.t_wait_write = t_wait_write - a.t_wait_write;
        s.elapsed = elapsed - a.elapsed;
        return s;
    }

    //! Returns the sum of all reads.
    //! \return the sum of all reads
    unsigned get_reads() const;

    //! Retruns sum, min, max, avarage and median of all reads.
    //! \return a summary of the read measurements
    stats_data::measurement_summary<unsigned> get_reads_summary() const;

    //! Returns the sum of all writes.
    //! \return the sum of all writes
    unsigned get_writes() const;

    //! Returns sum, min, max, avarage and median of all writes.
    //! \returns a summary of the write measurements
    stats_data::measurement_summary<unsigned> get_writes_summary() const;

    //! Returns number of bytes read from disks in total.
    //! \return number of bytes read
    external_size_type get_read_volume() const;

    //! Returns sum, min, max, avarage and median of all read bytes.
    //! \returns a summary of the write measurements
    stats_data::measurement_summary<external_size_type> get_read_volume_summary() const;

    //! Returns number of bytes written to the disks in total.
    //! \return number of bytes written
    external_size_type get_written_volume() const;

    //! Returns sum, min, max, avarage and median of all written bytes.
    //! \return a summary of the written bytes
    stats_data::measurement_summary<external_size_type> get_written_volume_summary() const;

    //! Returns total number of reads served from cache.
    //! \return the sum of all cached reads
    unsigned get_cached_reads() const;

    //! Returns sum, min, max, avarage and median of all cached reads.
    //! \return a summary of the cached reads
    stats_data::measurement_summary<unsigned> get_cached_reads_summary() const;

    //! Retruns the sum of all cached writes.
    //! \return the sum of all cached writes
    unsigned get_cached_writes() const;

    //! Returns sum, min, max, avarage and median of all cached writes
    //! \return a summary of the cached writes
    stats_data::measurement_summary<unsigned> get_cached_writes_summary() const;

    //! Returns number of bytes read from cache.
    //! \return number of bytes read from cache
    external_size_type get_cached_read_volume() const;

    //! Returns sum, min, max, avarage and median of all bytes read from cache.
    //! \return a summary of the bytes read from cache
    stats_data::measurement_summary<external_size_type> get_cached_read_volume_summary() const;

    //! Returns number of bytes written to the cache.
    //! \return number of bytes written to the cache
    external_size_type get_cached_written_volume() const;

    //! Returns sum, min, max, avarage and median of all cached written volumes
    //! \return a summary of the cached written volumes
    stats_data::measurement_summary<external_size_type> get_cached_written_volume_summary() const;

    //! Time that would be spent in read syscalls if all parallel reads were serialized.
    //! \return seconds spent in reading
    double get_read_time() const;

    //! Returns sum, min, max, avarage and median of all read times
    //! \return a summary of the read times
    stats_data::measurement_summary<double> get_read_time_summary() const;

    //! Time that would be spent in write syscalls if all parallel writes were serialized.
    //! \return the sum of the write times of all files
    double get_write_time() const;

    //! Returns sum, min, max, avarage and median of all write times
    //! \return a summary of the write times
    stats_data::measurement_summary<double> get_write_time_summary() const;

    //! Period of time when at least one I/O thread was executing a read.
    //! \return seconds spent in reading
    double get_pread_time() const;

    //! Period of time when at least one I/O thread was executing a write.
    //! \return seconds spent in writing
    double get_pwrite_time() const;

    //! Period of time when at least one I/O thread was executing a read or a write.
    //! \return seconds spent in I/O
    double get_pio_time() const;

    stats_data::measurement_summary<double> get_read_speed_summary() const;

    stats_data::measurement_summary<double> get_pread_speed_summary() const;

    stats_data::measurement_summary<double> get_write_speed_summary() const;

    stats_data::measurement_summary<double> get_pwrite_speed_summary() const;

    stats_data::measurement_summary<double> get_pio_speed_summary() const;

    //! Retruns elapsed time
    //! \remark If stats_data is not the difference between two other stats_data
    //! objects, then this value is measures the time since the first file object
    //! was initilized.
    //! \return elapsed time
    double get_elapsed_time() const
    {
        return elapsed;
    }

    //! I/O wait time counter.
    //! \return number of seconds spent in I/O waiting functions
    double get_io_wait_time() const;

    double get_wait_read_time() const;

    double get_wait_write_time() const;

};

std::ostream& operator << (std::ostream& o, const stats_data& s);

inline std::ostream& operator << (std::ostream& o, const stats& s)
{
    o << stxxl::stats_data(s);
    return o;
}

std::string format_with_SI_IEC_unit_multiplier(external_size_type number, const char* unit = "", int multiplier = 1000);

inline std::string add_IEC_binary_multiplier(external_size_type number, const char* unit = "")
{
    return format_with_SI_IEC_unit_multiplier(number, unit, 1024);
}

inline std::string add_SI_multiplier(external_size_type number, const char* unit = "")
{
    return format_with_SI_IEC_unit_multiplier(number, unit, 1000);
}

//! \}

} // namespace stxxl

#endif // !STXXL_IO_IOSTATS_HEADER
// vim: et:ts=4:sw=4
