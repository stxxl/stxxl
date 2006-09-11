/***************************************************************************
 *            pq_benchmark.cpp
 *
 *  Wed Jul 12 18:25:58 2006
 *  Copyright  2006  User Roman Dementiev
 *  Email
 ****************************************************************************/


#include <stxxl>

#define TOTAL_PQ_MEM_SIZE    (768*1024*1024)

#define PQ_MEM_SIZE					(512*1024*1024)

#define PREFETCH_POOL_SIZE 			((TOTAL_PQ_MEM_SIZE-PQ_MEM_SIZE)/2)
#define WRITE_POOL_SIZE						(PREFETCH_POOL_SIZE)


#define MAX_ELEMENTS (2000*1024*1024)


struct my_record
{
	int key;
	int data;
	my_record() {}
	my_record(int k, int d): key(k),data(d) { }
};

std::ostream & operator << (std::ostream & o, const my_record & obj)
{
	o << obj.key << " "<<obj.data;
	return o;
}

bool operator == (const my_record & a, const my_record & b)
{
	return a.key == b.key;
}

bool operator != (const my_record & a, const my_record & b)
{
	return a.key != b.key;
}

bool operator < (const my_record & a, const my_record & b)
{
	return a.key < b.key;
}

bool operator > (const my_record & a, const my_record & b)
{
	return a.key>b.key;
}



struct comp_type
{
	bool operator () (const my_record & a, const my_record & b) const
	{
		return a>b;
	}
	static my_record min_value()
	{ 
		return my_record((std::numeric_limits<int>::max)(),0);
	}
};


typedef stxxl::PRIORITY_QUEUE_GENERATOR<my_record,comp_type,
	PQ_MEM_SIZE,MAX_ELEMENTS/(1024/8)>::result pq_type;

typedef pq_type::block_type block_type;

#define    BLOCK_SIZE block_type::raw_size


#if 1
unsigned ran32State = 0xdeadbeef;
inline int myrand()
{
		return ((int)((ran32State = 1664525 * ran32State + 1013904223)>>1))-1;
}
#else // a longer pseudo random sequence
long long unsigned ran32State = 0xdeadbeef;
inline long long unsigned myrand()
{
		return (ran32State = (ran32State*0x5DEECE66DULL + 0xBULL)&0xFFFFFFFFFFFFULL );
}
#endif


void run_stxxl_insert_all_delete_all(stxxl::int64 ops)
{
	stxxl::prefetch_pool<block_type> p_pool(PREFETCH_POOL_SIZE/BLOCK_SIZE);
  	stxxl::write_pool<block_type>    w_pool(WRITE_POOL_SIZE/BLOCK_SIZE);
	
	pq_type PQ(p_pool,w_pool);
	
	stxxl::int64 i;
	
	my_record cur;
	
	stxxl::stats * Stats = stxxl::stats::get_instance();
	Stats->reset();
	
	stxxl::timer Timer;
	Timer.start();
	
	for(i=0;i<ops;++i)
	{
		cur.key = myrand();
		PQ.push(cur);
	}
	
	Timer.stop();
	
	STXXL_MSG("Records in PQ: "<<PQ.size())
	if(i != PQ.size())
	{
		STXXL_MSG("Size does not match")
		abort();
	}
	
	STXXL_MSG("Insertions elapsed time: "<<(Timer.mseconds()/1000.)<<
				" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
	
	std::cout << *Stats;
	Stats->reset();

	////////////////////////////////////////////////
	Timer.reset();
	Timer.start();
	
	for(i=0;i<ops;++i)
	{
		PQ.pop();
	}
	
	Timer.stop();
	
	STXXL_MSG("Records in PQ: "<<PQ.size())
	if(!PQ.empty())
	{
		STXXL_MSG("PQ must be empty")
		abort();
	}
	
	STXXL_MSG("Deletions elapsed time: "<<(Timer.mseconds()/1000.)<<
				" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
	
	std::cout << *Stats;
	
}


void run_stxxl_intermixed(stxxl::int64 ops)
{
	stxxl::prefetch_pool<block_type> p_pool(PREFETCH_POOL_SIZE/BLOCK_SIZE);
  	stxxl::write_pool<block_type>    w_pool(WRITE_POOL_SIZE/BLOCK_SIZE);
	
	pq_type PQ(p_pool,w_pool);
	
	stxxl::int64 i;
	
	my_record cur;
	
	stxxl::stats * Stats = stxxl::stats::get_instance();
	Stats->reset();
	
	stxxl::timer Timer;
	Timer.start();
	
	for(i=0;i<ops;++i)
	{
		cur.key = myrand();
		PQ.push(cur);
	}
	
	Timer.stop();
	
	STXXL_MSG("Records in PQ: "<<PQ.size())
	if(i != PQ.size())
	{
		STXXL_MSG("Size does not match")
		abort();
	}
	
	STXXL_MSG("Insertions elapsed time: "<<(Timer.mseconds()/1000.)<<
				" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
	
	std::cout << *Stats;
	Stats->reset();

	////////////////////////////////////////////////
	Timer.reset();
	Timer.start();
	
	for(i=0;i<ops;++i)
	{
		int o = myrand()%3;
		if(o == 0)
		{
			cur.key = myrand();
			PQ.push(cur);
		}
		else
		{
			assert(!PQ.empty());
			PQ.pop();
		}
	}
	
	Timer.stop();
	
	STXXL_MSG("Records in PQ: "<<PQ.size())
	
	STXXL_MSG("Deletions/Insertion elapsed time: "<<(Timer.mseconds()/1000.)<<
				" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
	
	std::cout << *Stats;
	
}

int main(int argc, char * argv[])
{
    STXXL_MSG("stxxl::pq lock size: "<<BLOCK_SIZE<<" bytes")
    
   	#ifdef STXXL_DIRECT_IO_OFF
	STXXL_MSG("STXXL_DIRECT_IO_OFF is defined")
	#else
	STXXL_MSG("STXXL_DIRECT_IO_OFF is NOT defined")
	#endif
	
	if(argc < 3)
	{
		STXXL_MSG("Usage: "<<argv[0]<<" version #ops")
		STXXL_MSG("\t version = 1: insert-all-delete-all stxxl pq")
		STXXL_MSG("\t version = 2: intermixed insert/delete stxxl pq")
		return 0;
	}

	int version = atoi(argv[1]);
	stxxl::int64 ops = atoll(argv[2]);
	

	STXXL_MSG("Running version      : "<<version)
	STXXL_MSG("Operations to perform: "<<ops);
	
	switch(version)
	{
		case 1:
			run_stxxl_insert_all_delete_all(ops);
			break;
		case 2:
			run_stxxl_intermixed(ops);
			break;
		default:
			STXXL_MSG("Unsupported version "<<version)
	}
	
}
