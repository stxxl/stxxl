/***************************************************************************
 *  lib/io/iostats.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
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

#include <stxxl/bits/common/log.h>
#include <stxxl/bits/common/timer.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/io/iostats.h>
#include <stxxl/bits/verbose.h>

#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace stxxl {

#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
void stats::wait_started(wait_op_type wait_op)
{
    double now = timestamp();
    {
        std::unique_lock<std::mutex> WaitLock(wait_mutex);

        double diff = now - p_begin_wait;
        t_waits += double(acc_waits) * diff;
        p_begin_wait = now;
        p_waits += (acc_waits++) ? diff : 0.0;

        if (wait_op == WAIT_OP_READ) {
            diff = now - p_begin_wait_read;
            t_wait_read += double(acc_wait_read) * diff;
            p_begin_wait_read = now;
            p_wait_read += (acc_wait_read++) ? diff : 0.0;
        }
        else /* if (wait_op == WAIT_OP_WRITE) */ {
            // wait_any() is only used from write_pool and buffered_writer, so account WAIT_OP_ANY for WAIT_OP_WRITE, too
            diff = now - p_begin_wait_write;
            t_wait_write += double(acc_wait_write) * diff;
            p_begin_wait_write = now;
            p_wait_write += (acc_wait_write++) ? diff : 0.0;
        }
    }
}

void stats::wait_finished(wait_op_type wait_op)
{
    double now = timestamp();
    {
        std::unique_lock<std::mutex> WaitLock(wait_mutex);

        double diff = now - p_begin_wait;
        t_waits += double(acc_waits) * diff;
        p_begin_wait = now;
        p_waits += (acc_waits--) ? diff : 0.0;

        if (wait_op == WAIT_OP_READ) {
            double diff2 = now - p_begin_wait_read;
            t_wait_read += double(acc_wait_read) * diff2;
            p_begin_wait_read = now;
            p_wait_read += (acc_wait_read--) ? diff2 : 0.0;
        }
        else /* if (wait_op == WAIT_OP_WRITE) */ {
            double diff2 = now - p_begin_wait_write;
            t_wait_write += double(acc_wait_write) * diff2;
            p_begin_wait_write = now;
            p_wait_write += (acc_wait_write--) ? diff2 : 0.0;
        }
#ifdef STXXL_WAIT_LOG_ENABLED
        std::ofstream* waitlog = stxxl::logger::get_instance()->waitlog_stream();
        if (waitlog)
            *waitlog << (now - last_reset) << "\t"
                     << ((wait_op == WAIT_OP_READ) ? diff : 0.0) << "\t"
                     << ((wait_op != WAIT_OP_READ) ? diff : 0.0) << "\t"
                     << t_wait_read << "\t" << t_wait_write << std::endl << std::flush;
#endif
    }
}
#endif

void stats::p_write_started(double now)
{
    {
        std::unique_lock<std::mutex> WriteLock(write_mutex);

        double diff = now - p_begin_write;
        p_begin_write = now;
        p_writes += (acc_writes++) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex);

        double diff = now - p_begin_io;
        p_ios += (acc_ios++) ? diff : 0.0;
        p_begin_io = now;
    }
}

void stats::p_write_finished(double now)
{
    {
        std::unique_lock<std::mutex> WriteLock(write_mutex);

        double diff = now - p_begin_write;
        p_begin_write = now;
        p_writes += (acc_writes--) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex);

        double diff = now - p_begin_io;
        p_ios += (acc_ios--) ? diff : 0.0;
        p_begin_io = now;
    }
}

void stats::p_read_started(double now)
{
    {
        std::unique_lock<std::mutex> read_lock(read_mutex);

        double diff = now - p_begin_read;
        p_begin_read = now;
        p_reads += (acc_reads++) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex);

        double diff = now - p_begin_io;
        p_ios += (acc_ios++) ? diff : 0.0;
        p_begin_io = now;
    }
}

void stats::p_read_finished(double now)
{
    {
//        std::unique_lock<std::mutex> read_lock(read_mutex);

        double diff = now - p_begin_read;
        p_begin_read = now;
        p_reads += (acc_reads--) ? diff : 0.0;
    }
    {
//        std::unique_lock<std::mutex> io_lock(io_mutex);

        double diff = now - p_begin_io;
        p_ios += (acc_ios--) ? diff : 0.0;
        p_begin_io = now;
    }
}

file_stats* stats::create_file_stats(unsigned int device_id)
{
    file_stats_list.emplace_back(device_id);
    return &file_stats_list.back();
}

std::vector<file_stats_data> stats::deepcopy_file_stats_data_list() const
{
    std::vector<file_stats_data> fsdl;
    for (auto it = file_stats_list.begin(); it != file_stats_list.end(); it++)
    {
        fsdl.push_back(file_stats_data(*it));
    }

    return fsdl;
}

file_stats::file_stats(unsigned int device_id)
    : m_device_id(device_id),
      reads(0),
      writes(0),
      volume_read(0),
      volume_written(0),
      c_reads(0),
      c_writes(0),
      c_volume_read(0),
      c_volume_written(0),
      t_reads(0.0),
      t_writes(0.0),
      p_begin_read(0.0),
      p_begin_write(0.0),
      acc_reads(0), acc_writes(0)
{ }

void file_stats::write_started(size_t size_, double now)
{
    if (now == 0.0)
        now = timestamp();

    {
        std::unique_lock<std::mutex> WriteLock(write_mutex);

        ++writes;
        volume_written += size_;
        double diff = now - p_begin_write;
        t_writes += double(acc_writes++) * diff;
        p_begin_write = now;
    }

    stats::get_instance()->p_write_started(now);
}

void file_stats::write_canceled(size_t size_)
{
    {
        std::unique_lock<std::mutex> WriteLock(write_mutex);

        --writes;
        volume_written -= size_;
    }
    write_finished();
}

void file_stats::write_finished()
{
    double now = timestamp();

    {
        std::unique_lock<std::mutex> WriteLock(write_mutex);

        double diff = now - p_begin_write;
        t_writes += double(acc_writes--) * diff;
        p_begin_write = now;
    }

    stats::get_instance()->p_write_finished(now);
}

void file_stats::write_cached(size_t size_)
{
    std::unique_lock<std::mutex> WriteLock(write_mutex);

    ++c_writes;
    c_volume_written += size_;
}

void file_stats::read_started(size_t size_, double now)
{
    if (now == 0.0)
        now = timestamp();

    {
        std::unique_lock<std::mutex> read_lock(read_mutex);

        ++reads;
        volume_read += size_;
        double diff = now - p_begin_read;
        t_reads += double(acc_reads++) * diff;
        p_begin_read = now;
    }

    stats::get_instance()->p_read_started(now);
}

void file_stats::read_canceled(size_t size_)
{
    {
        std::unique_lock<std::mutex> read_lock(read_mutex);

        --reads;
        volume_read -= size_;
    }
    read_finished();
}

void file_stats::read_finished()
{
    double now = timestamp();

    {
        std::unique_lock<std::mutex> read_lock(read_mutex);

        double diff = now - p_begin_read;
        t_reads += double(acc_reads--) * diff;
        p_begin_read = now;
    }

    stats::get_instance()->p_read_finished(now);
}

void file_stats::read_cached(size_t size_)
{
    std::unique_lock<std::mutex> read_lock(read_mutex);

    ++c_reads;
    c_volume_read += size_;
}

//! Returns the sum of all reads.
//! \return the sum of all reads
unsigned stats_data::get_reads() const
{
    return fetch_sum<unsigned>([](const file_stats_data& fsd) -> unsigned { return fsd.get_reads(); });
}

//! Retruns sum, min, max, avarage and median of all reads.
//! \return a summary of the read measurements
stats_data::measurement_summary<unsigned> stats_data::get_reads_summary() const
{
    return measurement_summary<unsigned>(m_file_stats_data_list,
                                         [](const file_stats_data& fsd) { return fsd.get_reads(); });
}

//! Returns the sum of all writes.
//! \return the sum of all writes
unsigned stats_data::get_writes() const
{
    return fetch_sum<unsigned>([](const file_stats_data& fsd) { return fsd.get_writes(); });
}

//! Returns sum, min, max, avarage and median of all writes.
//! \returns a summary of the write measurements
stats_data::measurement_summary<unsigned> stats_data::get_writes_summary() const
{
    return measurement_summary<unsigned>(m_file_stats_data_list,
                                         [](const file_stats_data& fsd) { return fsd.get_writes(); });
}

//! Returns number of bytes read from disks in total.
//! \return number of bytes read
external_size_type stats_data::get_read_volume() const
{
    return fetch_sum<external_size_type>(
        [](const file_stats_data& fsd) { return fsd.get_read_volume(); });
}

//! Returns sum, min, max, avarage and median of all read bytes.
//! \returns a summary of the write measurements
stats_data::measurement_summary<external_size_type> stats_data::get_read_volume_summary() const
{
    return measurement_summary<external_size_type>(
        m_file_stats_data_list,
        [](const file_stats_data& fsd) { return fsd.get_read_volume(); });
}

//! Returns number of bytes written to the disks in total.
//! \return number of bytes written
external_size_type stats_data::get_written_volume() const
{
    return fetch_sum<external_size_type>([](const file_stats_data& fsd) { return fsd.get_written_volume(); });
}

//! Returns sum, min, max, avarage and median of all written bytes.
//! \return a summary of the written bytes
stats_data::measurement_summary<external_size_type> stats_data::get_written_volume_summary() const
{
    return measurement_summary<external_size_type>(m_file_stats_data_list,
                                                   [](const file_stats_data& fsd) { return fsd.get_written_volume(); });
}

//! Returns total number of reads served from cache.
//! \return the sum of all cached reads
unsigned stats_data::get_cached_reads() const
{
    return fetch_sum<unsigned>([](const file_stats_data& fsd) { return fsd.get_cached_reads(); });
}

//! Returns sum, min, max, avarage and median of all cached reads.
//! \return a summary of the cached reads
stats_data::measurement_summary<unsigned> stats_data::get_cached_reads_summary() const
{
    return measurement_summary<unsigned>(m_file_stats_data_list,
                                         [](const file_stats_data& fsd) { return fsd.get_cached_reads(); });
}

//! Retruns the sum of all cached writes.
//! \return the sum of all cached writes
unsigned stats_data::get_cached_writes() const
{
    return fetch_sum<unsigned>([](const file_stats_data& fsd) { return fsd.get_cached_writes(); });
}

//! Returns sum, min, max, avarage and median of all cached writes
//! \return a summary of the cached writes
stats_data::measurement_summary<unsigned> stats_data::get_cached_writes_summary() const
{
    return measurement_summary<unsigned>(m_file_stats_data_list,
                                         [](const file_stats_data& fsd) { return fsd.get_cached_writes(); });
}

//! Returns number of bytes read from cache.
//! \return number of bytes read from cache
external_size_type stats_data::get_cached_read_volume() const
{
    return fetch_sum<external_size_type>([](const file_stats_data& fsd) { return fsd.get_cached_read_volume(); });
}

//! Returns sum, min, max, avarage and median of all bytes read from cache.
//! \return a summary of the bytes read from cache
stats_data::measurement_summary<external_size_type> stats_data::get_cached_read_volume_summary() const
{
    return measurement_summary<external_size_type>(m_file_stats_data_list,
                                                   [](const file_stats_data& fsd) { return fsd.get_cached_read_volume(); });
}

//! Returns number of bytes written to the cache.
//! \return number of bytes written to the cache
external_size_type stats_data::get_cached_written_volume() const
{
    return fetch_sum<external_size_type>([](const file_stats_data& fsd) { return fsd.get_cached_written_volume(); });
}

//! Returns sum, min, max, avarage and median of all cached written volumes
//! \return a summary of the cached written volumes
stats_data::measurement_summary<external_size_type> stats_data::get_cached_written_volume_summary() const
{
    return measurement_summary<external_size_type>(m_file_stats_data_list,
                                                   [](const file_stats_data& fsd) { return fsd.get_cached_written_volume(); });
}

//! Time that would be spent in read syscalls if all parallel reads were serialized.
//! \return seconds spent in reading
double stats_data::get_read_time() const
{
    return fetch_sum<double>([](const file_stats_data& fsd) { return fsd.get_read_time(); });
}

//! Returns sum, min, max, avarage and median of all read times
//! \return a summary of the read times
stats_data::measurement_summary<double> stats_data::get_read_time_summary() const
{
    return measurement_summary<double>(m_file_stats_data_list,
                                       [](const file_stats_data& fsd) { return fsd.get_read_time(); });
}

//! Time that would be spent in write syscalls if all parallel writes were serialized.
//! \return the sum of the write times of all files
double stats_data::get_write_time() const
{
    return fetch_sum<double>([](const file_stats_data& fsd) { return fsd.get_write_time(); });
}

//! Returns sum, min, max, avarage and median of all write times
//! \return a summary of the write times
stats_data::measurement_summary<double> stats_data::get_write_time_summary() const
{
    return measurement_summary<double>(m_file_stats_data_list,
                                       [](const file_stats_data& fsd) { return fsd.get_write_time(); });
}

//! Period of time when at least one I/O thread was executing a read.
//! \return seconds spent in reading
double stats_data::get_pread_time() const
{
    return p_reads;
}

//! Period of time when at least one I/O thread was executing a write.
//! \return seconds spent in writing
double stats_data::get_pwrite_time() const
{
    return p_writes;
}

//! Period of time when at least one I/O thread was executing a read or a write.
//! \return seconds spent in I/O
double stats_data::get_pio_time() const
{
    return p_ios;
}

stats_data::measurement_summary<double> stats_data::get_read_speed_summary() const
{
    return measurement_summary<double>(m_file_stats_data_list,
                                       [](const file_stats_data& fsd) { return (double)fsd.get_read_volume() / fsd.get_read_time(); });
}

stats_data::measurement_summary<double> stats_data::get_pread_speed_summary() const
{
    return measurement_summary<double>(m_file_stats_data_list,
                                       [this](const file_stats_data& fsd) { return (double)fsd.get_read_volume() / p_reads; });
}

stats_data::measurement_summary<double> stats_data::get_write_speed_summary() const
{
    return measurement_summary<double>(m_file_stats_data_list,
                                       [](const file_stats_data& fsd) { return (double)fsd.get_written_volume() / fsd.get_write_time(); });
}

stats_data::measurement_summary<double> stats_data::get_pwrite_speed_summary() const
{
    return measurement_summary<double>(m_file_stats_data_list,
                                       [this](const file_stats_data& fsd) { return (double)fsd.get_written_volume() / p_writes; });
}

stats_data::measurement_summary<double> stats_data::get_pio_speed_summary() const
{
    return measurement_summary<double>(m_file_stats_data_list,
                                       [this](const file_stats_data& fsd) { return (double)(fsd.get_read_volume() + fsd.get_written_volume()) / p_ios; });
}

//! I/O wait time counter.
//! \return number of seconds spent in I/O waiting functions
double stats_data::get_io_wait_time() const
{
    return t_wait;
}

double stats_data::get_wait_read_time() const
{
    return t_wait_read;
}

double stats_data::get_wait_write_time() const
{
    return t_wait_write;
}

std::string format_with_SI_IEC_unit_multiplier(uint64_t number, const char* unit, int multiplier)
{
    // may not overflow, std::numeric_limits<uint64_t>::max() == 16 EB
    static const char* endings[] = { "", "k", "M", "G", "T", "P", "E" };
    static const char* binary_endings[] = { "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei" };
    std::ostringstream out;
    out << number << ' ';
    int scale = 0;
    double number_d = (double)number;
    double multiplier_d = multiplier;
    while (number_d >= multiplier_d)
    {
        number_d /= multiplier_d;
        ++scale;
    }
    if (scale > 0)
        out << '(' << std::fixed << std::setprecision(3) << number_d << ' '
            << (multiplier == 1024 ? binary_endings[scale] : endings[scale])
            << (unit ? unit : "") << ") ";
    else if (unit && *unit)
        out << unit << ' ';
    return out.str();
}

std::ostream& operator << (std::ostream& o, const stats_data& s)
{
#define hr add_IEC_binary_multiplier
#define one_mib 1048576.0
    o << "STXXL I/O statistics" << std::endl;
    auto read_bytes_summary = s.get_read_volume_summary();
    auto written_bytes_summary = s.get_written_volume_summary();

    auto read_speed_summary = s.get_read_speed_summary();
    auto pread_speed_summary = s.get_pread_speed_summary();
    auto write_speed_summary = s.get_write_speed_summary();
    auto pwrite_speed_summary = s.get_pwrite_speed_summary();
    auto pio_speed_summary = s.get_pio_speed_summary();

    o << " total number of reads                      : " << hr(s.get_reads()) << std::endl;
    o << " average block size (read)                  : "
      << hr(s.get_reads() ? s.get_read_volume() / s.get_reads() : 0, "B") << std::endl;
    o << " number of bytes read from disks            : " << hr(s.get_read_volume(), "B") << std::endl;
    o << " time spent in serving all read requests    : " << s.get_read_time() << " s"
      << " @ " << ((double)s.get_read_volume() / one_mib / s.get_read_time()) << " MiB/s"
      << " (min: " << read_speed_summary.min / one_mib << " MiB/s, "
      << "max: " << read_speed_summary.max / one_mib << " MiB/s)"
      << std::endl;
    o << " time spent in reading (parallel read time) : " << s.get_pread_time() << " s"
      << " @ " << ((double)s.get_read_volume() / one_mib / s.get_pread_time()) << " MiB/s"
      << std::endl;
    o << "  reading speed per file                    : "
      << "min: " << pread_speed_summary.min / one_mib << " MiB/s, "
      << "med: " << pread_speed_summary.med / one_mib << " MiB/s, "
      << "max: " << pread_speed_summary.max / one_mib << " MiB/s"
      << std::endl;
    if (s.get_cached_reads()) {
        o << " total number of cached reads               : " << hr(s.get_cached_reads()) << std::endl;
        o << " average block size (cached read)           : " << hr(s.get_cached_read_volume() / s.get_cached_reads(), "B") << std::endl;
        o << " number of bytes read from cache            : " << hr(s.get_cached_read_volume(), "B") << std::endl;
    }
    if (s.get_cached_writes()) {
        o << " total number of cached writes              : " << hr(s.get_cached_writes()) << std::endl;
        o << " average block size (cached write)          : " << hr(s.get_cached_written_volume() / s.get_cached_writes(), "B") << std::endl;
        o << " number of bytes written to cache           : " << hr(s.get_cached_written_volume(), "B") << std::endl;
    }
    o << " total number of writes                     : " << hr(s.get_writes()) << std::endl;
    o << " average block size (write)                 : "
      << hr(s.get_writes() ? s.get_written_volume() / s.get_writes() : 0, "B") << std::endl;
    o << " number of bytes written to disks           : " << hr(s.get_written_volume(), "B") << std::endl;
    o << " time spent in serving all write requests   : " << s.get_write_time() << " s"
      << " @ " << ((double)s.get_written_volume() / one_mib / s.get_write_time()) << " MiB/s"
      << "(min: " << write_speed_summary.min / one_mib << " MiB/s, "
      << "max: " << write_speed_summary.max / one_mib << " MiB/s)"
      << std::endl;

    o << " time spent in writing (parallel write time): " << s.get_pwrite_time() << " s"
      << " @ " << ((double)s.get_written_volume() / one_mib / s.get_pwrite_time()) << " MiB/s"
      << std::endl;
    o << "   parallel write speed per file            : "
      << "min: " << pwrite_speed_summary.min / one_mib << " MiB/s, "
      << "med: " << pwrite_speed_summary.med / one_mib << " MiB/s, "
      << "max: " << pwrite_speed_summary.max / one_mib << " MiB/s"
      << std::endl;

    o << " time spent in I/O (parallel I/O time)      : " << s.get_pio_time() << " s"
      << " @ " << ((double)(s.get_read_volume() + s.get_written_volume()) / one_mib / s.get_pio_time()) << " MiB/s"
      << std::endl;
    o << "   parallel I/O speed per file              : "
      << "min: " << pio_speed_summary.min / one_mib << " MiB/s, "
      << "med: " << pio_speed_summary.med / one_mib << " MiB/s, "
      << "max: " << pio_speed_summary.max / one_mib << " MiB/s"
      << std::endl;
    o << " n/a" << std::endl;
#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
    o << " I/O wait time                              : " << s.get_io_wait_time() << " s" << std::endl;
    if (s.get_wait_read_time() != 0.0)
        o << " I/O wait4read time                         : " << s.get_wait_read_time() << " s" << std::endl;
    if (s.get_wait_write_time() != 0.0)
        o << " I/O wait4write time                        : " << s.get_wait_write_time() << " s" << std::endl;
#endif
    o << " Time since the last reset                  : " << s.get_elapsed_time() << " s" << std::endl;
    // WARNINGS add useful warnings here
    if (pio_speed_summary.min / pio_speed_summary.max < 0.5 ||
        pread_speed_summary.min / pread_speed_summary.max < 0.5 ||
        pwrite_speed_summary.min / pwrite_speed_summary.max < 0.5)
    {
        o << "Warning: Slow disk(s) detected. " << std::endl
          << " Reading: ";
        o << pread_speed_summary.values_per_device.front().second
          << "@ " << pread_speed_summary.values_per_device.front().first / one_mib << " MiB/s";
        for (int i = 1; pread_speed_summary.values_per_device[i].first / pread_speed_summary.values_per_device.back().first < 0.5; ++i)
        {
            o << ", " << pread_speed_summary.values_per_device[i].second
              << "@ " << pread_speed_summary.values_per_device[i].first / one_mib << " MiB/s";
        }
        o << std::endl
          << " Writing: "
          << pwrite_speed_summary.values_per_device.front().second
          << "@ " << pwrite_speed_summary.values_per_device.front().first / one_mib << " MiB/s";
        for (int i = 1; pwrite_speed_summary.values_per_device[i].first / pwrite_speed_summary.values_per_device.back().first < 0.5; ++i)
        {
            o << ", " << pwrite_speed_summary.values_per_device[i].second
              << "@ " << pwrite_speed_summary.values_per_device[i].first / one_mib << " MiB/s";
        }
        o << std::endl;
    }
    if ((double)read_bytes_summary.min / read_bytes_summary.max < 0.5 ||
        (double)written_bytes_summary.min / written_bytes_summary.max < 0.5)
    {
        o << "Warning: Bad load balancing." << std::endl
          << " Smallest read load on disk  "
          << read_bytes_summary.values_per_device.front().second
          << " @ " << hr(read_bytes_summary.values_per_device.front().first, "B")
          << std::endl
          << " Biggest read load on disk   "
          << read_bytes_summary.values_per_device.back().second
          << " @ " << hr(read_bytes_summary.values_per_device.back().first, "B")
          << std::endl
          << " Smallest write load on disk "
          << written_bytes_summary.values_per_device.front().second
          << " @ " << hr(written_bytes_summary.values_per_device.front().first, "B")
          << std::endl
          << " Biggest write load on disk  "
          << written_bytes_summary.values_per_device.back().second
          << " @ " << hr(written_bytes_summary.values_per_device.back().first, "B")
          << std::endl;
    }
    return o;
#undef hr
}

} // namespace stxxl
// vim: et:ts=4:sw=4
