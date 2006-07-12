/***************************************************************************
 *            stack_benchmark.cpp
 *
 *  Wed Jul 12 18:28:43 2006
 *  Copyright  2006  User Roman Dementiev
 *  Email
 ****************************************************************************/

#include <stxxl>

#define MEM_2_RESERVE    (768*1024*1024)

#ifndef BLOCK_SIZE
#define    BLOCK_SIZE (2*1024*1024)
#endif

#ifndef DISKS
#define DISKS 1
#endif

template <unsigned RECORD_SIZE>
struct my_record_
{
	char data[RECORD_SIZE];
	my_record_() {}
};

template <class my_record>
void run_stxxl_growshrink2_stack(stxxl::int64 volume)
{
	typedef typename stxxl::STACK_GENERATOR<my_record,stxxl::external,
	stxxl::grow_shrink2,DISKS,BLOCK_SIZE>::result stack_type;
	typedef typename stack_type::block_type block_type;
	
	stxxl::prefetch_pool<block_type> p_pool(DISKS*8);
  	stxxl::write_pool<block_type>    w_pool(DISKS*8);
	
	stack_type Stack(p_pool,w_pool);
	
	stxxl::int64 ops = volume/sizeof(my_record);
	
	stxxl::int64 i;
	
	my_record cur;
	
	stxxl::stats * Stats = stxxl::stats::get_instance();
	Stats->reset();
	
	stxxl::timer Timer;
	Timer.start();
	
	for(i=0;i<ops;++i)
	{
		Stack.push(cur);
	}
	
	Timer.stop();
	
	STXXL_MSG("Records in Stack: "<<Stack.size())
	if(i != Stack.size())
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
		Stack.pop();
	}
	
	Timer.stop();
	
	STXXL_MSG("Records in Stack: "<<Stack.size())
	if(!Stack.empty())
	{
		STXXL_MSG("Stack must be empty")
		abort();
	}
	
	STXXL_MSG("Deletions elapsed time: "<<(Timer.mseconds()/1000.)<<
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
		STXXL_MSG("Usage: "<<argv[0]<<" version #volume")
		STXXL_MSG("\t version = 1: grow-shrink-stack2 with 8 byte records")
		STXXL_MSG("\t version = 2: grow-shrink-stack2 with 128 byte records")
		return 0;
	}

	int version = atoi(argv[1]);
	stxxl::int64 volume = atoll(argv[2]);
	
	STXXL_MSG("Allocating array with size "<<MEM_2_RESERVE
		<<" bytes to prevent file buffering.")
	int * array = new int[MEM_2_RESERVE/sizeof(int)];
	std::fill(array,array+MEM_2_RESERVE,0);
	
	STXXL_MSG("Running version: "<<version)
	STXXL_MSG("Data volume    : "<<volume <<" bytes");
	
	switch(version)
	{
		case 1:
			run_stxxl_growshrink2_stack<my_record_<8> >(volume);
			break;
		case 2:
			run_stxxl_growshrink2_stack<my_record_<128> >(volume);
			break;
		default:
			STXXL_MSG("Unsupported version "<<version)
	}
	
	delete [] array;
	
}
