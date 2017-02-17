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

/******************************************************************************/
// file_stats

file_stats::file_stats(unsigned int device_id)
    : device_id_(device_id),
      read_count_(0), write_count_(0),
      read_bytes_(0), write_bytes_(0),
      read_time_(0.0), write_time_(0.0),
      p_begin_read_(0.0), p_begin_write_(0.0),
      acc_reads_(0), acc_writes_(0)
{ }

void file_stats::write_started(size_t size, double now)
{
    if (now == 0.0)
        now = timestamp();

    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        ++write_count_;
        write_bytes_ += size;
        double diff = now - p_begin_write_;
        write_time_ += double(acc_writes_++) * diff;
        p_begin_write_ = now;
    }

    stats::get_instance()->p_write_started(now);
}

void file_stats::write_canceled(size_t size)
{
    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        --write_count_;
        write_bytes_ -= size;
    }
    write_finished();
}

void file_stats::write_finished()
{
    double now = timestamp();

    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        double diff = now - p_begin_write_;
        write_time_ += double(acc_writes_--) * diff;
        p_begin_write_ = now;
    }

    stats::get_instance()->p_write_finished(now);
}

void file_stats::read_started(size_t size, double now)
{
    if (now == 0.0)
        now = timestamp();

    {
        std::unique_lock<std::mutex> read_lock(read_mutex_);

        ++read_count_;
        read_bytes_ += size;
        double diff = now - p_begin_read_;
        read_time_ += double(acc_reads_++) * diff;
        p_begin_read_ = now;
    }

    stats::get_instance()->p_read_started(now);
}

void file_stats::read_canceled(size_t size)
{
    {
        std::unique_lock<std::mutex> read_lock(read_mutex_);

        --read_count_;
        read_bytes_ -= size;
    }
    read_finished();
}

void file_stats::read_finished()
{
    double now = timestamp();

    {
        std::unique_lock<std::mutex> read_lock(read_mutex_);

        double diff = now - p_begin_read_;
        read_time_ += double(acc_reads_--) * diff;
        p_begin_read_ = now;
    }

    stats::get_instance()->p_read_finished(now);
}

/******************************************************************************/
// file_stats_data

file_stats_data file_stats_data::operator + (const file_stats_data& a) const
{
    STXXL_THROW_IF(device_id_ != a.device_id_, std::runtime_error,
                   "stxxl::file_stats_data objects do not belong to the same file/disk");

    file_stats_data fsd;
    fsd.device_id_ = device_id_;

    fsd.read_count_ = read_count_ + a.read_count_;
    fsd.write_count_ = write_count_ + a.write_count_;
    fsd.read_bytes_ = read_bytes_ + a.read_bytes_;
    fsd.write_bytes_ = write_bytes_ + a.write_bytes_;
    fsd.read_time_ = read_time_ + a.read_time_;
    fsd.write_time_ = write_time_ + a.write_time_;
    return fsd;
}

file_stats_data file_stats_data::operator - (const file_stats_data& a) const
{
    STXXL_THROW_IF(device_id_ != a.device_id_, std::runtime_error,
                   "stxxl::file_stats_data objects do not belong to the same file/disk");

    file_stats_data fsd;
    fsd.device_id_ = device_id_;

    fsd.read_count_ = read_count_ - a.read_count_;
    fsd.write_count_ = write_count_ - a.write_count_;
    fsd.read_bytes_ = read_bytes_ - a.read_bytes_;
    fsd.write_bytes_ = write_bytes_ - a.write_bytes_;
    fsd.read_time_ = read_time_ - a.read_time_;
    fsd.write_time_ = write_time_ - a.write_time_;
    return fsd;
}

/******************************************************************************/
// stats

stats::stats()
    : creation_time_(timestamp()),
      p_reads_(0.0), p_writes_(0.0),
      p_begin_read_(0.0), p_begin_write_(0.0),
      p_ios_(0.0),
      p_begin_io_(0.0),

      acc_reads_(0.0), acc_writes_(0.0),
      acc_ios_(0.0),

      t_waits_(0.0), p_waits_(0.0),
      p_begin_wait_(0.0),
      t_wait_read_(0.0), p_wait_read_(0.0),
      p_begin_wait_read_(0.0),
      t_wait_write_(0.0), p_wait_write_(0.0),
      p_begin_wait_write_(0.0),
      acc_waits_(0.0),
      acc_wait_read_(0.0), acc_wait_write_(0.0)
{ }

#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
void stats::wait_started(wait_op_type wait_op)
{
    double now = timestamp();
    {
        std::unique_lock<std::mutex> wait_lock(wait_mutex_);

        double diff = now - p_begin_wait_;
        t_waits_ += double(acc_waits_) * diff;
        p_begin_wait_ = now;
        p_waits_ += (acc_waits_++) ? diff : 0.0;

        if (wait_op == WAIT_OP_READ) {
            diff = now - p_begin_wait_read_;
            t_wait_read_ += double(acc_wait_read_) * diff;
            p_begin_wait_read_ = now;
            p_wait_read_ += (acc_wait_read_++) ? diff : 0.0;
        }
        else /* if (wait_op == WAIT_OP_WRITE) */ {
            // wait_any() is only used from write_pool and buffered_writer, so account WAIT_OP_ANY for WAIT_OP_WRITE, too
            diff = now - p_begin_wait_write_;
            t_wait_write_ += double(acc_wait_write_) * diff;
            p_begin_wait_write_ = now;
            p_wait_write_ += (acc_wait_write_++) ? diff : 0.0;
        }
    }
}

void stats::wait_finished(wait_op_type wait_op)
{
    double now = timestamp();
    {
        std::unique_lock<std::mutex> wait_lock(wait_mutex_);

        double diff = now - p_begin_wait_;
        t_waits_ += double(acc_waits_) * diff;
        p_begin_wait_ = now;
        p_waits_ += (acc_waits_--) ? diff : 0.0;

        if (wait_op == WAIT_OP_READ) {
            double diff2 = now - p_begin_wait_read_;
            t_wait_read_ += double(acc_wait_read_) * diff2;
            p_begin_wait_read_ = now;
            p_wait_read_ += (acc_wait_read_--) ? diff2 : 0.0;
        }
        else /* if (wait_op == WAIT_OP_WRITE) */ {
            double diff2 = now - p_begin_wait_write_;
            t_wait_write_ += double(acc_wait_write_) * diff2;
            p_begin_wait_write_ = now;
            p_wait_write_ += (acc_wait_write_--) ? diff2 : 0.0;
        }
#ifdef STXXL_WAIT_LOG_ENABLED
        std::ofstream* waitlog = stxxl::logger::get_instance()->waitlog_stream();
        if (waitlog)
            *waitlog << (now - last_reset) << "\t"
                     << ((wait_op == WAIT_OP_READ) ? diff : 0.0) << "\t"
                     << ((wait_op != WAIT_OP_READ) ? diff : 0.0) << "\t"
                     << t_wait_read_ << "\t" << t_wait_write_ << std::endl << std::flush;
#endif
    }
}
#endif

void stats::p_write_started(double now)
{
    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        double diff = now - p_begin_write_;
        p_begin_write_ = now;
        p_writes_ += (acc_writes_++) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex_);

        double diff = now - p_begin_io_;
        p_ios_ += (acc_ios_++) ? diff : 0.0;
        p_begin_io_ = now;
    }
}

void stats::p_write_finished(double now)
{
    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        double diff = now - p_begin_write_;
        p_begin_write_ = now;
        p_writes_ += (acc_writes_--) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex_);

        double diff = now - p_begin_io_;
        p_ios_ += (acc_ios_--) ? diff : 0.0;
        p_begin_io_ = now;
    }
}

void stats::p_read_started(double now)
{
    {
        std::unique_lock<std::mutex> read_lock(read_mutex_);

        double diff = now - p_begin_read_;
        p_begin_read_ = now;
        p_reads_ += (acc_reads_++) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex_);

        double diff = now - p_begin_io_;
        p_ios_ += (acc_ios_++) ? diff : 0.0;
        p_begin_io_ = now;
    }
}

void stats::p_read_finished(double now)
{
    {
//        std::unique_lock<std::mutex> read_lock(read_mutex_);

        double diff = now - p_begin_read_;
        p_begin_read_ = now;
        p_reads_ += (acc_reads_--) ? diff : 0.0;
    }
    {
//        std::unique_lock<std::mutex> io_lock(io_mutex_);

        double diff = now - p_begin_io_;
        p_ios_ += (acc_ios_--) ? diff : 0.0;
        p_begin_io_ = now;
    }
}

file_stats* stats::create_file_stats(unsigned int device_id)
{
    file_stats_list_.emplace_back(device_id);
    return &file_stats_list_.back();
}

std::vector<file_stats_data> stats::deepcopy_file_stats_data_list() const
{
    std::vector<file_stats_data> fsdl;
    for (auto it = file_stats_list_.begin(); it != file_stats_list_.end(); it++)
    {
        fsdl.push_back(file_stats_data(*it));
    }

    return fsdl;
}

/******************************************************************************/
// stats_data

template <typename T, typename Functor>
T stats_data::fetch_sum(const Functor& get_value) const
{
    T sum = 0;

    for (auto it = file_stats_data_list_.begin(); it != file_stats_data_list_.end(); it++)
        sum += get_value(*it);

    return sum;
}

template <typename T>
template <typename Functor>
stats_data::summary<T>::summary(
    const std::vector<file_stats_data>& fs, const Functor& get_value)
{
    values_per_device.reserve(fs.size());

    for (auto it = fs.begin(); it != fs.end(); it++)
    {
        values_per_device.emplace_back(get_value(*it), it->get_device_id());
    }

    std::sort(values_per_device.begin(), values_per_device.end(),
              [](std::pair<T, unsigned> a, std::pair<T, unsigned> b) {
                  return a.first < b.first;
              });

    if (values_per_device.size() != 0)
    {
        min = values_per_device.front().first;
        max = values_per_device.back().first;
        long mid = values_per_device.size() / 2;
        median =
            (values_per_device.size() % 2 == 1)
            ? values_per_device[mid].first
            : (values_per_device[mid - 1].first + values_per_device[mid].first) / 2.0;

        total = values_per_device.front().first;
        for (auto it = values_per_device.begin() + 1; it != values_per_device.end(); ++it)
            total += it->first;

        average = (double)total / values_per_device.size();
    }
    else
    {
        min = std::numeric_limits<T>::quiet_NaN();
        max = std::numeric_limits<T>::quiet_NaN();
        median = std::numeric_limits<T>::quiet_NaN();
        total = std::numeric_limits<T>::quiet_NaN();
        average = std::numeric_limits<T>::quiet_NaN();
    }
}

stats_data stats_data::operator + (const stats_data& a) const
{
    stats_data s;

    if (a.file_stats_data_list_.size() == 0)
    {
        s.file_stats_data_list_ = file_stats_data_list_;
    }
    else if (file_stats_data_list_.size() == 0)
    {
        s.file_stats_data_list_ = a.file_stats_data_list_;
    }
    else if (file_stats_data_list_.size() == a.file_stats_data_list_.size())
    {
        for (auto it1 = file_stats_data_list_.begin(),
             it2 = a.file_stats_data_list_.begin();
             it1 != file_stats_data_list_.end(); it1++, it2++)
        {
            s.file_stats_data_list_.push_back((*it1) + (*it2));
        }
    }
    else
    {
        STXXL_THROW(std::runtime_error,
                    "The number of files has changed between the snapshots.");
    }

    s.p_reads_ = p_reads_ + a.p_reads_;
    s.p_writes_ = p_writes_ + a.p_writes_;
    s.p_ios_ = p_ios_ + a.p_ios_;
    s.t_wait = t_wait + a.t_wait;
    s.t_wait_read_ = t_wait_read_ + a.t_wait_read_;
    s.t_wait_write_ = t_wait_write_ + a.t_wait_write_;
    s.elapsed_ = elapsed_ + a.elapsed_;
    return s;
}

stats_data stats_data::operator - (const stats_data& a) const
{
    stats_data s;

    if (a.file_stats_data_list_.size() == 0)
    {
        s.file_stats_data_list_ = file_stats_data_list_;
    }
    else if (file_stats_data_list_.size() == a.file_stats_data_list_.size())
    {
        for (auto it1 = file_stats_data_list_.begin(),
             it2 = a.file_stats_data_list_.begin();
             it1 != file_stats_data_list_.end(); it1++, it2++)
        {
            s.file_stats_data_list_.push_back((*it1) - (*it2));
        }
    }
    else
    {
        STXXL_THROW(std::runtime_error,
                    "The number of files has changed between the snapshots.");
    }

    s.p_reads_ = p_reads_ - a.p_reads_;
    s.p_writes_ = p_writes_ - a.p_writes_;
    s.p_ios_ = p_ios_ - a.p_ios_;
    s.t_wait = t_wait - a.t_wait;
    s.t_wait_read_ = t_wait_read_ - a.t_wait_read_;
    s.t_wait_write_ = t_wait_write_ - a.t_wait_write_;
    s.elapsed_ = elapsed_ - a.elapsed_;
    return s;
}

unsigned stats_data::get_read_count() const
{
    return fetch_sum<unsigned>(
        [](const file_stats_data& fsd) { return fsd.get_read_count(); });
}

stats_data::summary<unsigned> stats_data::get_read_count_summary() const
{
    return summary<unsigned>(
        file_stats_data_list_,
        [](const file_stats_data& fsd) { return fsd.get_read_count(); });
}

unsigned stats_data::get_write_count() const
{
    return fetch_sum<unsigned>(
        [](const file_stats_data& fsd) { return fsd.get_write_count(); });
}

stats_data::summary<unsigned> stats_data::get_write_count_summary() const
{
    return summary<unsigned>(
        file_stats_data_list_,
        [](const file_stats_data& fsd) { return fsd.get_write_count(); });
}

external_size_type stats_data::get_read_bytes() const
{
    return fetch_sum<external_size_type>(
        [](const file_stats_data& fsd) { return fsd.get_read_bytes(); });
}

stats_data::summary<external_size_type>
stats_data::get_read_bytes_summary() const
{
    return summary<external_size_type>(
        file_stats_data_list_,
        [](const file_stats_data& fsd) { return fsd.get_read_bytes(); });
}

external_size_type stats_data::get_write_bytes() const
{
    return fetch_sum<external_size_type>(
        [](const file_stats_data& fsd) { return fsd.get_write_bytes(); });
}

stats_data::summary<external_size_type>
stats_data::get_write_bytes_summary() const
{
    return summary<external_size_type>(
        file_stats_data_list_,
        [](const file_stats_data& fsd) { return fsd.get_write_bytes(); });
}

double stats_data::get_read_time() const
{
    return fetch_sum<double>(
        [](const file_stats_data& fsd) { return fsd.get_read_time(); });
}

stats_data::summary<double> stats_data::get_read_time_summary() const
{
    return summary<double>(
        file_stats_data_list_,
        [](const file_stats_data& fsd) { return fsd.get_read_time(); });
}

double stats_data::get_write_time() const
{
    return fetch_sum<double>(
        [](const file_stats_data& fsd) { return fsd.get_write_time(); });
}

stats_data::summary<double> stats_data::get_write_time_summary() const
{
    return summary<double>(
        file_stats_data_list_,
        [](const file_stats_data& fsd) { return fsd.get_write_time(); });
}

double stats_data::get_pread_time() const
{
    return p_reads_;
}

double stats_data::get_pwrite_time() const
{
    return p_writes_;
}

double stats_data::get_pio_time() const
{
    return p_ios_;
}

stats_data::summary<double> stats_data::get_read_speed_summary() const
{
    return summary<double>(
        file_stats_data_list_,
        [](const file_stats_data& fsd) {
            return (double)fsd.get_read_bytes() / fsd.get_read_time();
        });
}

stats_data::summary<double> stats_data::get_pread_speed_summary() const
{
    return summary<double>(
        file_stats_data_list_,
        [this](const file_stats_data& fsd) {
            return (double)fsd.get_read_bytes() / p_reads_;
        });
}

stats_data::summary<double> stats_data::get_write_speed_summary() const
{
    return summary<double>(
        file_stats_data_list_,
        [](const file_stats_data& fsd) {
            return (double)fsd.get_write_bytes() / fsd.get_write_time();
        });
}

stats_data::summary<double> stats_data::get_pwrite_speed_summary() const
{
    return summary<double>(
        file_stats_data_list_,
        [this](const file_stats_data& fsd) {
            return (double)fsd.get_write_bytes() / p_writes_;
        });
}

stats_data::summary<double> stats_data::get_pio_speed_summary() const
{
    return summary<double>(
        file_stats_data_list_,
        [this](const file_stats_data& fsd) {
            return (double)(fsd.get_read_bytes() + fsd.get_write_bytes()) / p_ios_;
        });
}

double stats_data::get_io_wait_time() const
{
    return t_wait;
}

double stats_data::get_wait_read_time() const
{
    return t_wait_read_;
}

double stats_data::get_wait_write_time() const
{
    return t_wait_write_;
}

std::string format_with_SI_IEC_unit_multiplier(
    uint64_t number, const char* unit, int multiplier)
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

    static const double one_mib = 1048576.0;

    o << "STXXL I/O statistics" << std::endl;
    auto read_bytes_summary = s.get_read_bytes_summary();
    auto write_bytes_summary = s.get_write_bytes_summary();

    auto read_speed_summary = s.get_read_speed_summary();
    auto pread_speed_summary = s.get_pread_speed_summary();
    auto write_speed_summary = s.get_write_speed_summary();
    auto pwrite_speed_summary = s.get_pwrite_speed_summary();
    auto pio_speed_summary = s.get_pio_speed_summary();

    o << " total number of reads                      : "
      << hr(s.get_read_count()) << std::endl;
    o << " average block size (read)                  : "
      << hr(s.get_read_count() ? s.get_read_bytes() / s.get_read_count() : 0, "B")
      << std::endl;
    o << " number of bytes read from disks            : "
      << hr(s.get_read_bytes(), "B") << std::endl;
    o << " time spent in serving all read requests    : "
      << s.get_read_time() << " s"
      << " @ " << ((double)s.get_read_bytes() / one_mib / s.get_read_time()) << " MiB/s"
      << " (min: " << read_speed_summary.min / one_mib << " MiB/s, "
      << "max: " << read_speed_summary.max / one_mib << " MiB/s)"
      << std::endl;
    o << " time spent in reading (parallel read time) : "
      << s.get_pread_time() << " s"
      << " @ " << ((double)s.get_read_bytes() / one_mib / s.get_pread_time()) << " MiB/s"
      << std::endl;
    o << "  reading speed per file                    : "
      << "min: " << pread_speed_summary.min / one_mib << " MiB/s, "
      << "median: " << pread_speed_summary.median / one_mib << " MiB/s, "
      << "max: " << pread_speed_summary.max / one_mib << " MiB/s"
      << std::endl;
    o << " total number of writes                     : "
      << hr(s.get_write_count()) << std::endl;
    o << " average block size (write)                 : "
      << hr(s.get_write_count() ? s.get_write_bytes() / s.get_write_count() : 0, "B") << std::endl;
    o << " number of bytes written to disks           : "
      << hr(s.get_write_bytes(), "B") << std::endl;
    o << " time spent in serving all write requests   : "
      << s.get_write_time() << " s"
      << " @ " << ((double)s.get_write_bytes() / one_mib / s.get_write_time()) << " MiB/s"
      << "(min: " << write_speed_summary.min / one_mib << " MiB/s, "
      << "max: " << write_speed_summary.max / one_mib << " MiB/s)"
      << std::endl;

    o << " time spent in writing (parallel write time): " << s.get_pwrite_time() << " s"
      << " @ " << ((double)s.get_write_bytes() / one_mib / s.get_pwrite_time()) << " MiB/s"
      << std::endl;
    o << "   parallel write speed per file            : "
      << "min: " << pwrite_speed_summary.min / one_mib << " MiB/s, "
      << "median: " << pwrite_speed_summary.median / one_mib << " MiB/s, "
      << "max: " << pwrite_speed_summary.max / one_mib << " MiB/s"
      << std::endl;

    o << " time spent in I/O (parallel I/O time)      : " << s.get_pio_time() << " s"
      << " @ " << ((double)(s.get_read_bytes() + s.get_write_bytes()) / one_mib / s.get_pio_time()) << " MiB/s"
      << std::endl;
    o << "   parallel I/O speed per file              : "
      << "min: " << pio_speed_summary.min / one_mib << " MiB/s, "
      << "median: " << pio_speed_summary.median / one_mib << " MiB/s, "
      << "max: " << pio_speed_summary.max / one_mib << " MiB/s"
      << std::endl;
#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
    o << " I/O wait time                              : "
      << s.get_io_wait_time() << " s" << std::endl;
    if (s.get_wait_read_time() != 0.0)
        o << " I/O wait4read time                         : "
          << s.get_wait_read_time() << " s" << std::endl;
    if (s.get_wait_write_time() != 0.0)
        o << " I/O wait4write time                        : "
          << s.get_wait_write_time() << " s" << std::endl;
#endif
    o << " Time since the last reset                  : "
      << s.get_elapsed_time() << " s" << std::endl;
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
        (double)write_bytes_summary.min / write_bytes_summary.max < 0.5)
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
          << write_bytes_summary.values_per_device.front().second
          << " @ " << hr(write_bytes_summary.values_per_device.front().first, "B")
          << std::endl
          << " Biggest write load on disk  "
          << write_bytes_summary.values_per_device.back().second
          << " @ " << hr(write_bytes_summary.values_per_device.back().first, "B")
          << std::endl;
    }
    return o;
#undef hr
}

} // namespace stxxl
// vim: et:ts=4:sw=4
