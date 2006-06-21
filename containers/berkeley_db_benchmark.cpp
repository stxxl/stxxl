/***************************************************************************
 *            berkeley_db_benchmark.cpp
 *
 *  Fri Mar  3 16:33:04 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#include <stxxl>

#define KEY_SIZE 		8
#define DATA_SIZE 	32

#define NODE_BLOCK_SIZE 	(16*1024)
#define LEAF_BLOCK_SIZE 	(32*1024)

//#define TOTAL_CACHE_SIZE 	(5*128*1024*1024/4)
#define TOTAL_CACHE_SIZE    (96*1024*1024)
#define NODE_CACHE_SIZE 	(1*(TOTAL_CACHE_SIZE/5))
#define LEAF_CACHE_SIZE 	(4*(TOTAL_CACHE_SIZE/5))

struct my_key
{
	char keybuf[KEY_SIZE];
};

std::ostream & operator << (std::ostream & o, my_key & obj)
{
	for(int i = 0;i<KEY_SIZE;++i)
		o << obj.keybuf[i];
	return o;
}

bool operator == (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) == 0;
}

bool operator != (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) != 0;
}

bool operator < (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) < 0;
}
/*
bool operator > (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) > 0;
}

bool operator <= (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) <= 0;
}

bool operator >= (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) >= 0;
}
*/

struct my_data
{
	char databuf[DATA_SIZE];
};

struct comp_type
{
	bool operator () (const my_key & a, const my_key & b) const
	{
		return strncmp(a.keybuf,b.keybuf,KEY_SIZE) < 0;
	}
	static my_key max_value()
	{ 
		my_key obj;
		memset(obj.keybuf, (std::numeric_limits<char>::max)() , KEY_SIZE);
		return obj;
	}
};



typedef stxxl::map<my_key,my_data,comp_type,NODE_BLOCK_SIZE,LEAF_BLOCK_SIZE> map_type;

int main(int argc, char * argv[])
{
	if(argc < 2)
	{
		STXXL_MSG("Usage: "<<argv[0]<<" #ops")
		return 0;
	}

	stxxl::int64 ops = atoll(argv[1]);
	
	map_type Map(NODE_CACHE_SIZE,LEAF_CACHE_SIZE);
	
	char *letters = "abcdefghijklmnopqrstuvwxuz";
	std::pair<my_key,my_data> element;
    
	memset(element.first.keybuf, 'a', KEY_SIZE);
	memset(element.second.databuf, 'b', DATA_SIZE);

	
	stxxl::timer Timer;
	stxxl::int64 n_inserts = ops, n_locates=ops, n_range_queries=ops, n_deletes = ops;
	stxxl::int64 i;
	comp_type cmp_;
	
	stxxl::ran32State = 0xdeadbeef;
	
	stxxl::random_number32 myrand;
	
	STXXL_MSG("Records in map: "<<Map.size())
	
	Timer.start();
	            
	for (i = 0; i < n_inserts; ++n_inserts)
	{
		element.first.keybuf[(i % KEY_SIZE)] = letters[(myrand() % 26)];
		Map.insert(element);
	}

	Timer.stop();
	STXXL_MSG("Records in map: "<<Map.size())
	STXXL_MSG("Insertions elapsed time: "<<(Timer.mseconds()/1000.)<<
				" seconds : "<< (double(n_inserts)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")

	Timer.reset();

	map_type & CMap(Map); // const map reference
	
	Timer.start();

	for (i = 0; i < n_locates; ++n_locates)
	{
		element.first.keybuf[(i % KEY_SIZE)] = letters[(myrand() % 26)];
		CMap.lower_bound(element.first);
	}

	Timer.stop();
	STXXL_MSG("Locates elapsed time: "<<(Timer.mseconds()/1000.)<<
		" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
	
	Timer.reset();
	
	Timer.start();

	for (i = 0; i < n_range_queries; ++n_range_queries)
	{
		element.first.keybuf[(i % KEY_SIZE)] = letters[(myrand() % 26)];
		my_key begin_key = element.first;
		map_type::const_iterator begin=CMap.lower_bound(element.first);
		element.first.keybuf[(i % KEY_SIZE)] = letters[(myrand() % 26)];
		map_type::const_iterator beyond=CMap.lower_bound(element.first);
		if(element.first<begin_key)
			std::swap(begin,beyond);
		
	}

	Timer.stop();
	STXXL_MSG("Locates elapsed time: "<<(Timer.mseconds()/1000.)<<
		" seconds : "<< (double(n_range_queries)/(Timer.mseconds()/1000.))<<
		" key/data pairs per sec, #scanned elements: ")
	
	
}
