/***************************************************************************
 *  algo/async_schedule.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/algo/async_schedule.h>

__STXXL_BEGIN_NAMESPACE

namespace async_schedule_local {

int_type simulate_async_write(
    const int_type * disks,
    const int_type L,
    const int_type m_init,
    const int_type D,
    std::pair<int_type, int_type> * o_time)
{
    typedef std::priority_queue<sim_event, std::vector<sim_event>, sim_event_cmp> event_queue_type;
    typedef std::queue<int_type> disk_queue_type;
    assert(L >= D);
    disk_queue_type * disk_queues = new disk_queue_type[D];
    event_queue_type event_queue;

    int_type m = m_init;
    int_type i = L - 1;
    int_type oldtime = 0;
    bool * disk_busy = new bool[D];

    while (m && (i >= 0))
    {
        int_type disk = disks[i];
        assert(0 <= disk && disk < D);
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
            //STXXL_MSG("Block "<<j<<" scheduled");
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


        STXXL_VERBOSE1("Block " << cur.iblock << " put out, time " << cur.timestamp << " disk: " << disks[cur.iblock]);
        o_time[cur.iblock] = std::pair<int_type, int_type>(cur.iblock, cur.timestamp);

        if (i >= 0)
        {
            int_type disk = disks[i];
            assert(0 <= disk && disk < D);
            if (disk_busy[disk])
            {
                disk_queues[disk].push(i--);
            }
            else
            {
				if(!disk_queues[disk].empty())
				{
					STXXL_VERBOSE1("c Block "<<disk_queues[disk].front()<<" scheduled for time "<< cur.timestamp + 1);
					event_queue.push(sim_event(cur.timestamp + 1, disk_queues[disk].front()));
					disk_queues[disk].pop();
				} else
				{
                	STXXL_VERBOSE1("a Block "<<i<<" scheduled for time "<< cur.timestamp + 1);
                	event_queue.push(sim_event(cur.timestamp + 1, i--));
				}
                disk_busy[disk] = true;
            }
        }

        // add next block to write
        int_type disk = disks[cur.iblock];
        assert(0 <= disk && disk < D);
        if (!disk_busy[disk] && !disk_queues[disk].empty())
        {
            STXXL_VERBOSE1("b Block "<<disk_queues[disk].front()<<" scheduled for time "<< cur.timestamp + 1);
            event_queue.push(sim_event(cur.timestamp + 1, disk_queues[disk].front()));
            disk_queues[disk].pop();
            disk_busy[disk] = true;
        }
    }

    assert(i == -1);
    for (int_type i = 0; i < D; i++)
        assert(disk_queues[i].empty());


    delete[] disk_busy;
    delete[] disk_queues;

    return (oldtime - 1);
}

}  // namespace async_schedule_local


void compute_prefetch_schedule(
    const int_type * first,
    const int_type * last,
    int_type * out_first,
    int_type m,
    int_type D)
{
    typedef std::pair<int_type, int_type> pair_type;
    int_type L = last - first;
    if (L <= D)
    {
        for (int_type i = 0; i < L; ++i)
            out_first[i] = i;

        return;
    }
    pair_type * write_order = new pair_type[L];

    int_type w_steps = async_schedule_local::simulate_async_write(first, L, m, D, write_order);

    STXXL_VERBOSE1("Write steps: " << w_steps);

    for (int_type i = 0; i < L; i++)
        STXXL_VERBOSE1(first[i] << " " << write_order[i].first << " " << write_order[i].second);

    std::stable_sort(write_order, write_order + L, write_time_cmp());


    for (int_type i = 0; i < L; i++)
    {
        out_first[i] = write_order[i].first;
        //if(out_first[i] != i)
        STXXL_VERBOSE1(i << " " << out_first[i]);
    }

    delete[] write_order;
    STXXL_UNUSED(w_steps);
}


__STXXL_END_NAMESPACE
