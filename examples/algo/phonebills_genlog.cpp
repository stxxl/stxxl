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

#include <ctime>
#include <fstream>

#include <tlx/logger.hpp>

#include <stxxl.h>

struct LogEntry
{
    long long unsigned from; // callers number
    long long unsigned to;   // destination number
    time_t timestamp;        // time of event
    int event;               // event type 1 - call started
                             //            2 - call ended
};

struct cmp
{
    bool operator () (const LogEntry& a, const LogEntry& b) const
    {
        return a.timestamp < b.timestamp;
    }
    static LogEntry min_value()
    {
        LogEntry e;
        e.timestamp = std::numeric_limits<time_t>::min();
        return e;
    }
    static LogEntry max_value()
    {
        LogEntry e;
        e.timestamp = std::numeric_limits<time_t>::max();
        return e;
    }
};

using vector_type = stxxl::vector<LogEntry, 1, stxxl::random_pager<1> >;

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
    if (argc < 5)
    {
        LOG1 << "Usage: " << argv[0] << " ncalls avcalls main logfile";
        LOG1 << " ncalls  - number of calls";
        LOG1 << " avcalls - average number of calls per client";
        LOG1 << " main    - memory to use (in MiB)";
        LOG1 << " logfile - file name of the output";

        return 0;
    }
    size_t M = atol(argv[3]) * 1024 * 1024;
    const uint64_t ncalls = foxxll::atouint64(argv[1]);
    const long av_calls = atol(argv[2]);
    const uint64_t nclients = ncalls / av_calls;
    uint64_t calls_made = 0;

    time_t now = time(nullptr);

    vector_type log;
    log.reserve(2 * ncalls);

    stxxl::random_number64 rnd;

    for (uint64_t number = 0;
         number < nclients && calls_made < ncalls;
         ++number)
    {
        long long int serv = std::min<long long int>(rnd(av_calls * 2), (ncalls - calls_made));
        LogEntry e;
        e.from = number;

        time_t cur = now;

        while (serv-- > 0)
        {
            cur += (time_t)(1 + rnd(3600 * 24));

            e.to = rnd(nclients);
            e.timestamp = cur;

            ++calls_made;
            e.event = 1;
            log.push_back(e);

            cur += (time_t)(1 + rnd(1800));
            e.timestamp = cur;
            e.event = 2;

            log.push_back(e);
        }
    }

    // sort events in order of their appereance
    stxxl::sort(log.begin(), log.end(), cmp(), M);

    std::fstream out(argv[4], std::ios::out);

    std::copy(log.begin(), log.end(), std::ostream_iterator<LogEntry>(out));

    LOG1 << "\n" << calls_made << " calls made.";
    LOG1 << "The log is written to '" << argv[4] << "'.";

    return 0;
}
