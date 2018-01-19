/***************************************************************************
 *  examples/algo/phonebills_genlog.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <fstream>

#include <tlx/logger.hpp>

#include <foxxll/common/die_with_message.hpp>
#include <foxxll/common/utils.hpp>

#include <stxxl.h>
#include <stxxl/bits/common/comparator.h>

struct LogEntry
{
    LogEntry() = default;

    LogEntry(uint64_t from, uint64_t to, double timestamp, int event)
        : from(from), to(to), timestamp(timestamp), event(event)
    { }

    uint64_t from;    // callers number
    uint64_t to;      // destination number
    time_t timestamp; // time of event
    int event;        // event type 1 - call started
                      //            2 - call ended
};

std::ostream& operator << (std::ostream& i, const LogEntry& entry)
{
    i << entry.from << " ";
    i << entry.to << " ";
    i << entry.timestamp << " ";
    i << entry.event << "\n";
    return i;
}

int main(int argc, char* argv[])
{
    die_with_message_if(argc < 5, "Usage: " << argv[0] << " ncalls avcalls main logfile\n"
                        " ncalls  - number of calls\n"
                        " avcalls - average number of calls per client\n"
                        " main    - memory to use (in MiB)\n"
                        " logfile - file name of the output");

    const size_t M = foxxll::atouint64(argv[3]) * 1024 * 1024;
    const size_t ncalls = foxxll::atouint64(argv[1]);
    const size_t av_calls = foxxll::atouint64(argv[2]);
    const size_t nclients = ncalls / av_calls;

    size_t calls_made = 0;

    const auto now = foxxll::timestamp();

    stxxl::vector<LogEntry> log;
    log.reserve(2 * ncalls);

    std::mt19937_64 randgen;
    std::uniform_real_distribution<double> distr_timestamp(1, 24.0 * 3600);
    std::uniform_real_distribution<double> distr_duration(1, 24.0 * 3600);

    std::uniform_int_distribution<size_t> distr_clients(0, nclients - 1);
    std::uniform_int_distribution<size_t> distr_serv(0, av_calls - 1);

    for (uint64_t number = 0; number < nclients && calls_made < ncalls; ++number)
    {
        auto serv = std::min<size_t>(distr_serv(randgen), ncalls - calls_made);

        auto cur = now;
        while (serv-- > 0)
        {
            const auto to = distr_clients(randgen);

            cur += distr_timestamp(randgen);
            log.push_back({ number, to, cur, 1 });

            cur += distr_duration(randgen);
            log.push_back({ number, to, cur, 2 });

            ++calls_made;
        }
    }

    // derive a stxxl::struct_comparator by providing a lambda returning
    // a tuple of references to all relevant field of the struct (here only timestamp)
    auto cmp = stxxl::make_struct_comparator<LogEntry>(
        [](auto& l) { return std::tie(l.timestamp); });

    // sort events in order of their appereance
    stxxl::sort(log.begin(), log.end(), cmp, M);

    // Write result
    std::fstream out(argv[4], std::ios::out);
    std::copy(log.begin(), log.end(), std::ostream_iterator<LogEntry>(out));

    LOG1 << "\n" << calls_made << " calls made.";
    LOG1 << "The log is written to '" << argv[4] << "'.";

    return 0;
}
