/***************************************************************************
 *            testmap.cpp
 *
 *  Tue Mar 22 10:41:38 2005
 *  Copyright  2005  Roman Dementiev
 *  Email dementiev@ira.uka.de
 ****************************************************************************/
#include "map.h"
#include <algorithm>
#include <math.h>

typedef unsigned int key_type;
typedef unsigned int data_type;

struct cmp: public std::less<key_type>
{
                static key_type min_value() { return (std::numeric_limits<key_type>::min)(); };
                static key_type max_value() { return (std::numeric_limits<key_type>::max)(); };
};

#define BLOCK_SIZE (32*1024)
#define CACHE_SIZE (2*1024*1024/BLOCK_SIZE)

#define CACHE_ELEMENTS (BLOCK_SIZE*CACHE_SIZE/(sizeof(key_type) + sizeof(data_type)))

typedef stxxl::map<key_type, data_type , cmp, BLOCK_SIZE, BLOCK_SIZE > map_type;

int main()
{

	stxxl::stats * bm = stxxl::stats::get_instance();
	STXXL_MSG(*bm)

	STXXL_MSG("Block size "<<BLOCK_SIZE/1024<<" kb")
	STXXL_MSG("Cache size "<<(CACHE_SIZE*BLOCK_SIZE)/1024<<" kb")
	for(int mult=1; mult < 256; mult *= 2 )
	{
		const unsigned el = mult*(CACHE_ELEMENTS/8);
		STXXL_MSG("Elements to insert "<< el <<" volume ="<<
			(el*(sizeof(key_type) + sizeof(data_type)))/1024<<" kb")
		map_type  * DMap = new map_type(CACHE_SIZE*BLOCK_SIZE/2,CACHE_SIZE*BLOCK_SIZE/2);
		//map_type  Map(CACHE_SIZE*BLOCK_SIZE/2,CACHE_SIZE*BLOCK_SIZE/2);
		map_type  & Map = *DMap;
		for(unsigned i = 0; i < el; ++i)
		{
			Map[i] = i+1;
		}
		double writes = double(bm->get_writes())/double(el);
		double logel = log(double(el))/log(double(BLOCK_SIZE));
		STXXL_MSG("Logs: writes "<<writes<<" logel "<<logel<<" writes/logel "<<(writes/logel) )
		STXXL_MSG(*bm)
		bm->reset();
		STXXL_MSG("Doing search")
		unsigned queries = el;
		const map_type & ConstMap = Map;
		stxxl::random_number32 myrandom;
		for(unsigned i = 0; i< queries ; ++i)
		{
			key_type key = myrandom()%el;
			map_type::const_iterator result = ConstMap.find(key);
			assert((*result).second == key + 1);
		}
		double reads = double(bm->get_reads())/logel;
	    double readsperq = double(bm->get_reads())/queries;
		STXXL_MSG("reads/logel "<<reads<<" readsperq "<<readsperq)
		STXXL_MSG(*bm)
		bm->reset();
	}
	
	
	
	return 0;	
}
