/***************************************************************************
 *  include/stxxl/bits/algo/async_schedule.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef ASYNC_SCHEDULE_HEADER
#define ASYNC_SCHEDULE_HEADER

#include "stxxl/io"

#include <queue>
#include <algorithm>
#include <functional>

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include <stxxl/bits/compat_hash_map.h>
#include <stxxl/bits/compat_hash_set.h>


__STXXL_BEGIN_NAMESPACE

struct sim_event // only one type of event: WRITE COMPLETED
{
    int_type timestamp;
    int_type iblock;
    inline sim_event(int_type t, int_type b) : timestamp(t), iblock(b) { }
};

struct sim_event_cmp : public std::binary_function<sim_event, sim_event, bool>
{
    inline bool operator ()  (const sim_event & a, const sim_event & b) const
    {
        return a.timestamp > b.timestamp;
    }
};

int_type simulate_async_write(
    int * disks,
    const int_type L,
    const int_type m_init,
    const int_type D,
    std::pair<int_type, int_type> * o_time);

typedef std::pair<int_type, int_type> write_time_pair;
struct write_time_cmp : public std::binary_function<write_time_pair, write_time_pair, bool>
{
    inline bool operator ()  (const write_time_pair & a, const write_time_pair & b) const
    {
        return a.second > b.second;
    }
};

void compute_prefetch_schedule(
    int_type * first,
    int_type * last,
    int_type * out_first,
    int_type m,
    int_type D);

template <typename run_type>
void simulate_async_write(
    const run_type & input,
    const int_type m_init,
    const int_type D,
    std::pair<int_type, int_type> * o_time)
{
    typedef std::priority_queue<sim_event, std::vector<sim_event>, sim_event_cmp> event_queue_type;
    typedef std::queue<int_type> disk_queue_type;

    const int_type L = input.size();
    assert(L >= D);
    disk_queue_type * disk_queues = new disk_queue_type[L];
    event_queue_type event_queue;

    int_type m = m_init;
    int_type i = L - 1;
    int_type oldtime = 0;
    bool * disk_busy = new bool[D];

    while (m && (i >= 0))
    {
        int_type disk = input[i].bid.storage->get_disk_number();
        disk_queues[disk].push(i);
        i--;
        m--;
    }

    for (int_type ii = 0; ii < D; ii++)
        if (!disk_queues[ii].empty())
        {
            int_type j = disk_queues[ii].front();
            disk_queues[ii].pop();
            event_queue.push(sim_event(1, j));
        }

    while (!event_queue.empty())
    {
        sim_event cur = event_queue.top();
        event_queue.pop();
        if (oldtime != cur.timestamp)
        {
            // clear disk_busy
            for (int_type i = 0; i < D; i++)
                disk_busy[i] = false;


            oldtime = cur.timestamp;
        }
        o_time[cur.iblock] = std::pair < int_type, int_type > (cur.iblock, cur.timestamp + 1);

        m++;
        if (i >= 0)
        {
            m--;
            int_type disk = input[i].bid.storage->get_disk_number();
            if (disk_busy[disk])
            {
                disk_queues[disk].push(i);
            }
            else
            {
                event_queue.push(sim_event(cur.timestamp + 1, i));
                disk_busy[disk] = true;
            }

            i--;
        }

        // add next block to write
        int_type disk = input[cur.iblock].bid.storage->get_disk_number();
        if (!disk_busy[disk] && !disk_queues[disk].empty())
        {
            event_queue.push(sim_event(cur.timestamp + 1, disk_queues[disk].front()));
            disk_queues[disk].pop();
            disk_busy[disk] = true;
        }
    }

    delete[] disk_busy;
    delete[] disk_queues;
}


template <typename run_type>
void compute_prefetch_schedule(
    const run_type & input,
    int_type * out_first,
    int_type m,
    int_type D)
{
    typedef std::pair<int_type, int_type>  pair_type;
    const int_type L = input.size();
    if (L <= D)
    {
        for (int_type i = 0; i < L; ++i)
            out_first[i] = i;

        return;
    }
    pair_type * write_order = new pair_type[L];

    simulate_async_write(input, m, D, write_order);

    std::stable_sort(write_order, write_order + L, write_time_cmp());

    for (int_type i = 0; i < L; i++)
        out_first[i] = write_order[i].first;


    delete[] write_order;
}


template <typename bid_iterator_type>
void simulate_async_write(
    bid_iterator_type input,
    const int_type L,
    const int_type m_init,
    const int_type /*D*/,
    std::pair<int_type, int_type> * o_time)
{
    typedef std::priority_queue<sim_event, std::vector<sim_event>, sim_event_cmp> event_queue_type;
    typedef std::queue<int_type> disk_queue_type;

    //disk_queue_type * disk_queues = new disk_queue_type[L];
    typedef typename compat_hash_map<int, disk_queue_type>::result disk_queues_type;
    disk_queues_type disk_queues;
    event_queue_type event_queue;

    int_type m = m_init;
    int_type i = L - 1;
    int_type oldtime = 0;
    //bool * disk_busy = new bool [D];
    compat_hash_set<int>::result disk_busy;

    while (m && (i >= 0))
    {
        int_type disk = (*(input + i)).storage->get_disk_number();
        disk_queues[disk].push(i);
        i--;
        m--;
    }

    /*
       for(int_type i=0;i<D;i++)
            if(!disk_queues[i].empty())
            {
                    int_type j = disk_queues[i].front();
                    disk_queues[i].pop();
                    event_queue.push(sim_event(1,j));
            }
     */
    disk_queues_type::iterator it = disk_queues.begin();
    for ( ; it != disk_queues.end(); ++it)
    {
        int_type j = (*it).second.front();
        (*it).second.pop();
        event_queue.push(sim_event(1, j));
    }

    while (!event_queue.empty())
    {
        sim_event cur = event_queue.top();
        event_queue.pop();
        if (oldtime != cur.timestamp)
        {
            // clear disk_busy
            //for(int_type i=0;i<D;i++)
            //	disk_busy[i] = false;
            disk_busy.clear();

            oldtime = cur.timestamp;
        }
        o_time[cur.iblock] = std::pair < int_type, int_type > (cur.iblock, cur.timestamp + 1);

        m++;
        if (i >= 0)
        {
            m--;
            int_type disk = (*(input + i)).storage->get_disk_number();
            if (disk_busy.find(disk) != disk_busy.end())
            {
                disk_queues[disk].push(i);
            }
            else
            {
                event_queue.push(sim_event(cur.timestamp + 1, i));
                disk_busy.insert(disk);
            }

            i--;
        }

        // add next block to write
        int_type disk = (*(input + cur.iblock)).storage->get_disk_number();
        if (disk_busy.find(disk) == disk_busy.end() && !disk_queues[disk].empty())
        {
            event_queue.push(sim_event(cur.timestamp + 1, disk_queues[disk].front()));
            disk_queues[disk].pop();
            disk_busy.insert(disk);
        }
    }

    //delete [] disk_busy;
    //delete [] disk_queues;
}


template <typename bid_iterator_type>
void compute_prefetch_schedule(
    bid_iterator_type input_begin,
    bid_iterator_type input_end,
    int_type * out_first,
    int_type m,
    int_type D)
{
    typedef std::pair<int_type, int_type>  pair_type;
    const int_type L = input_end - input_begin;
    STXXL_VERBOSE1("compute_prefetch_schedule: sequence length=" << L << " disks=" << D);
    if (L <= D)
    {
        for (int_type i = 0; i < L; ++i)
            out_first[i] = i;

        return;
    }
    pair_type * write_order = new pair_type[L];

    simulate_async_write(input_begin, L, m, D, write_order);

    std::stable_sort(write_order, write_order + L, write_time_cmp());

    STXXL_VERBOSE1("Computed prefetch order for " << L << " blocks with " <<
                   D << " disks and " << m << " blocks");
    for (int_type i = 0; i < L; i++)
    {
        out_first[i] = write_order[i].first;
        STXXL_VERBOSE1(write_order[i].first);
    }

    delete[] write_order;
}


__STXXL_END_NAMESPACE

#endif
