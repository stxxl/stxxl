#ifndef ASYNC_SCHEDULE_HEADER
#define ASYNC_SCHEDULE_HEADER

/***************************************************************************
 *            async_schedule.h
 *
 *  Fri Oct 25 16:08:06 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../io/io.h"
#include "../common/utils.h"
#include <queue>
#include <algorithm>


#ifdef BOOST_MSVC
#include <hash_map>
#include <hash_set>
#else
#include <ext/hash_map>
#include <ext/hash_set>
#endif

#include <algorithm>

__STXXL_BEGIN_NAMESPACE

struct sim_event // only one type of event: WRITE COMPLETED
{
	int timestamp;
	int iblock;
	inline sim_event(int t, int b):timestamp(t),iblock(b) {};
};

struct sim_event_cmp
{
	inline bool operator () (const sim_event & a, const sim_event & b) const
	{
		return a.timestamp > b.timestamp;
	}
};

int simulate_async_write(
	int *disks,
	const int L,
	const int m_init,
	const int D,
	std::pair<int,int> * o_time);

struct write_time_cmp
{
	inline bool operator () (const std::pair<int,int> & a, const std::pair<int,int> & b)
	{
		return a.second > b.second;
	}
};

void compute_prefetch_schedule(
		int * first,
		int * last,
		int * out_first,
		int m,
		int D);

template <typename run_type>
void simulate_async_write(
											const run_type & input,
											const int m_init,
											const int D,
											std::pair<int,int> * o_time)
{
	typedef std::priority_queue<sim_event,std::vector<sim_event>,sim_event_cmp> event_queue_type;
	typedef std::queue<int> disk_queue_type;
	
	const int L = input.size();
	assert(L >= D);
	disk_queue_type * disk_queues = new disk_queue_type[L];
	event_queue_type event_queue;
	
	int m = m_init;
	int i = L - 1;
        int oldtime = 0;
        bool * disk_busy = new bool [D];
		       
	while(m && (i>=0))
	{
		int disk = input[i].bid.storage->get_disk_number();
		disk_queues[disk].push(i);
		i--;
		m--;
	}
	
	for(int ii=0;ii<D;ii++)
		if(!disk_queues[ii].empty())
		{
			int j = disk_queues[ii].front();
			disk_queues[ii].pop();
			event_queue.push(sim_event(1,j));
		} 
	
	while(! event_queue.empty())
	{
		sim_event cur = event_queue.top();
		event_queue.pop();
		if(oldtime != cur.timestamp)
    {
			// clear disk_busy
			for(int i=0;i<D;i++)
				disk_busy[i] = false;
			
			oldtime = cur.timestamp;
    }
		o_time[cur.iblock] = std::pair<int,int>(cur.iblock,cur.timestamp + 1);
		
		m++;
		if(i >= 0)
		{
			m--;
			int disk = input[i].bid.storage->get_disk_number();
			if(disk_busy[disk])
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
		int disk = input[cur.iblock].bid.storage->get_disk_number();
		if(!disk_busy[disk] && !disk_queues[disk].empty())
		{
			event_queue.push(sim_event(cur.timestamp + 1,disk_queues[disk].front()));
			disk_queues[disk].pop();
			disk_busy[disk] = true;
		}
		
	};
	
	delete [] disk_busy;
	delete [] disk_queues;
}


template <typename run_type>
void compute_prefetch_schedule(
		const run_type & input,
		int * out_first,
		int m,
		int D)
{
	typedef std::pair<int,int>  pair_type;
	const int L = input.size();
	if(L <= D)
	{
		for(int i=0;i<L;++i) out_first[i] = i;
		return;
	}
	pair_type * write_order = new pair_type[L];
	
	simulate_async_write(input,m,D,write_order);
	
	std::stable_sort(write_order,write_order + L,write_time_cmp());
	
	for(int i=0;i<L;i++)
		out_first[i] = write_order[i].first;

	delete [] write_order;
}


template <typename bid_iterator_type>
void simulate_async_write(
											bid_iterator_type input,
											const int L,
											const int m_init,
											const int D,
											std::pair<int,int> * o_time)
{
	typedef std::priority_queue<sim_event,std::vector<sim_event>,sim_event_cmp> event_queue_type;
	typedef std::queue<int> disk_queue_type;
	
	//disk_queue_type * disk_queues = new disk_queue_type[L];
	#ifndef BOOST_MSVC
	typedef __gnu_cxx::hash_map<int,disk_queue_type> disk_queues_type;
	#else
	typedef stdext::hash_map<int,disk_queue_type> disk_queues_type;
	#endif
	disk_queues_type  disk_queues;
	event_queue_type event_queue;
	
	int m = m_init;
	int i = L - 1;
    int oldtime = 0;
    //bool * disk_busy = new bool [D];
	#ifndef BOOST_MSVC
	__gnu_cxx::hash_set<int> disk_busy;
	#else
	stdext::hash_set<int> disk_busy;
	#endif
		       
	while(m && (i>=0))
	{
		int disk = (*(input + i)).storage->get_disk_number();
		disk_queues[disk].push(i);
		i--;
		m--;
	}
	
	/*
	for(int i=0;i<D;i++)
		if(!disk_queues[i].empty())
		{
			int j = disk_queues[i].front();
			disk_queues[i].pop();
			event_queue.push(sim_event(1,j));
		}
	*/
	disk_queues_type::iterator it = disk_queues.begin();
	for(;it != disk_queues.end(); ++it)
	{
		int j = (*it).second.front();
		(*it).second.pop();
		event_queue.push(sim_event(1,j));
	}
	
	while(! event_queue.empty())
	{
		sim_event cur = event_queue.top();
		event_queue.pop();
		if(oldtime != cur.timestamp)
    {
			// clear disk_busy
			//for(int i=0;i<D;i++)
			//	disk_busy[i] = false;
			disk_busy.clear();	
		
			oldtime = cur.timestamp;
    }
		o_time[cur.iblock] = std::pair<int,int>(cur.iblock,cur.timestamp + 1);
		
		m++;
		if(i >= 0)
		{
			m--;
			int disk = (*(input + i)).storage->get_disk_number();
			if(disk_busy.find(disk)!=disk_busy.end())
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
		int disk = (*(input + cur.iblock)).storage->get_disk_number();
		if(disk_busy.find(disk)==disk_busy.end() && !disk_queues[disk].empty())
		{
			event_queue.push(sim_event(cur.timestamp + 1,disk_queues[disk].front()));
			disk_queues[disk].pop();
			disk_busy.insert(disk);
		}
		
	};
	
	//delete [] disk_busy;
	//delete [] disk_queues;
}


template <typename bid_iterator_type>
void compute_prefetch_schedule(
		bid_iterator_type input_begin,
		bid_iterator_type input_end,
		int * out_first,
		int m,
		int D)
{
	
	typedef std::pair<int,int>  pair_type;
	const int L = input_end - input_begin;
	STXXL_VERBOSE1("compute_prefetch_schedule: sequence length="<<L<<" disks="<<D)
	if(L <= D)
	{
		for(int i=0;i<L;++i) out_first[i] = i;
		return;
	}
	pair_type * write_order = new pair_type[L];
	
	simulate_async_write(input_begin,L,m,D,write_order);
	
	std::stable_sort(write_order,write_order + L,write_time_cmp());
	
	for(int i=0;i<L;i++)
		out_first[i] = write_order[i].first;

	delete [] write_order;
}


__STXXL_END_NAMESPACE

#endif
