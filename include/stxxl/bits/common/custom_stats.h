/***************************************************************************
 *  include/stxxl/bits/common/custom_stats.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_CUSTOM_STATS_HEADER
#define STXXL_COMMON_CUSTOM_STATS_HEADER

#include <string>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <stxxl/bits/common/timer.h>
#include <stxxl/bits/common/utils.h>

STXXL_BEGIN_NAMESPACE

class custom_stats
{
public:
    inline void add_counter(std::string name)
    {
        assert(m_counters.count(name) == 0);
        m_counters[name] = 0;
    }

    inline void inc_counter(std::string name)
    {
        assert(m_counters.count(name) == 1);
        ++m_counters[name];
    }

    inline void inc_counter(std::string name, long amount)
    {
        assert(m_counters.count(name) == 1);
        m_counters[name] += amount;
    }

    inline void max_counter(std::string name, long value)
    {
        assert(m_counters.count(name) == 1);
        m_counters[name] = std::max(m_counters[name], value);
    }

    inline long get_counter(std::string name)
    {
        assert(m_counters.count(name) == 1);
        return m_counters[name];
    }

    inline void set_counter(std::string name, long value)
    {
        assert(m_counters.count(name) == 1);
        m_counters[name] = value;
    }

    inline void add_mem_counter(std::string name, long mem_per_element)
    {
        assert(m_mem_counters.count(name) == 0);
        m_mem_counters[name] = std::make_pair(0, mem_per_element);
    }

    inline void inc_mem_counter(std::string name)
    {
        assert(m_mem_counters.count(name) == 1);
        ++m_mem_counters[name].first;
    }

    inline void inc_mem_counter(std::string name, long amount)
    {
        assert(m_mem_counters.count(name) == 1);
        m_mem_counters[name].first += amount;
    }

    inline void max_mem_counter(std::string name, long value)
    {
        assert(m_mem_counters.count(name) == 1);
        m_mem_counters[name].first = std::max(m_mem_counters[name].first, value);
    }

    inline void add_timer(std::string name)
    {
        assert(m_timers.count(name) == 0);
        m_timers[name] = stxxl::timer(false);
    }

    inline void start_timer(std::string name)
    {
        assert(m_timers.count(name) == 1);
        m_timers[name].start();
    }

    inline void stop_timer(std::string name)
    {
        assert(m_timers.count(name) == 1);
        m_timers[name].stop();
    }

    void print_all() const
    {
        for (auto it = m_counters.begin(); it != m_counters.end(); ++it)
            STXXL_MSG(it->first << " = " << it->second);

        for (auto it = m_mem_counters.begin(); it != m_mem_counters.end(); ++it)
            STXXL_MSG(it->first << " = " << it->second.first << " elements = " << format_IEC_size(it->second.first * it->second.second) << "B");

        for (auto it = m_timers.begin(); it != m_timers.end(); ++it)
            STXXL_MSG(it->first << " = " << it->second.seconds() << " s");
    }

protected:
    std::unordered_map<std::string, long> m_counters;
    std::unordered_map<std::string, std::pair<long, long> > m_mem_counters;
    std::unordered_map<std::string, stxxl::timer> m_timers;
};

class dummy_custom_stats
{
public:
    inline void add_counter(std::string) { }
    inline void inc_counter(std::string) { }
    inline void inc_counter(std::string, long) { }
    inline void max_counter(std::string, long) { }
    inline long get_counter(std::string) const { return 0; }
    inline void set_counter(std::string, long) { }

    inline void add_mem_counter(std::string, long) { }
    inline void inc_mem_counter(std::string) { }
    inline void inc_mem_counter(std::string, long) { }
    inline void max_mem_counter(std::string, long) { }

    inline void add_timer(std::string) { }
    inline void start_timer(std::string) { }
    inline void stop_timer(std::string) { }

    void print_all() const { }
};

STXXL_END_NAMESPACE
#endif // !STXXL_COMMON_CUSTOM_STATS_HEADER
