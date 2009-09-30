/***************************************************************************
 *  include/stxxl/bits/algo/async_schedule.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

// Implements the "prudent prefetching" as described in
// D. Hutchinson, P. Sanders, J. S. Vitter: Duality between prefetching
// and queued writing on parallel disks, 2005
// DOI: 10.1137/S0097539703431573


#ifndef STXXL_ASYNC_SCHEDULE_HEADER
#define STXXL_ASYNC_SCHEDULE_HEADER

#include <queue>
#include <algorithm>
#include <functional>
#include <cassert>

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include <stxxl/bits/common/types.h>
#include <stxxl/bits/common/utils.h>
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
    inline bool operator () (const sim_event & a, const sim_event & b) const
    {
        return a.timestamp > b.timestamp;
    }
};

typedef std::pair<int_type, int_type> write_time_pair;
struct write_time_cmp : public std::binary_function<write_time_pair, write_time_pair, bool>
{
    inline bool operator () (const write_time_pair & a, const write_time_pair & b) const
    {
        return a.second > b.second;
    }
};

void compute_prefetch_schedule(
    const int_type * first,
    const int_type * last,
    int_type * out_first,
    int_type m,
    int_type D);

inline void compute_prefetch_schedule(
    int_type * first,
    int_type * last,
    int_type * out_first,
    int_type m,
    int_type D)
{
    compute_prefetch_schedule(static_cast<const int_type *>(first), last, out_first, m, D);
}

template <typename run_type>
void compute_prefetch_schedule(
    const run_type & input,
    int_type * out_first,
    int_type m,
    int_type D)
{
    const int_type L = input.size();
    int_type * disks = new int_type[L];
    for (int_type i = 0; i < L; ++i)
        disks[i] = input[i].bid.storage->get_id();
    compute_prefetch_schedule(disks, disks + L, out_first, m, D);
    delete[] disks;
}

#if 0
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
        int_type disk = (*(input + i)).storage->get_id();
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
        o_time[cur.iblock] = std::pair<int_type, int_type>(cur.iblock, cur.timestamp + 1);

        m++;
        if (i >= 0)
        {
            m--;
            int_type disk = (*(input + i)).storage->get_id();
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
        int_type disk = (*(input + cur.iblock)).storage->get_id();
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
#endif

template <typename bid_iterator_type>
void compute_prefetch_schedule(
    bid_iterator_type input_begin,
    bid_iterator_type input_end,
    int_type * out_first,
    int_type m,
    int_type D)
{
    typedef std::pair<int_type, int_type> pair_type;
    const int_type L = input_end - input_begin;
#if 0
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
#else
    int_type * disks = new int_type[L];
    int_type i = 0;
    for (bid_iterator_type it = input_begin; it != input_end; ++it, ++i)
        disks[i] = it->storage->get_id();
    compute_prefetch_schedule(disks, disks + L, out_first, m, D);
    delete[] disks;
#endif
}

__STXXL_END_NAMESPACE

#endif // !STXXL_ASYNC_SCHEDULE_HEADER
