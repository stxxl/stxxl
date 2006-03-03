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

#define NODE_BLOCK_SIZE 	(32*1024)
#define LEAF_BLOCK_SIZE 		(32*1024)

#define TOTAL_CACHE_SIZE 	(32*1024*1024)
#define NODE_CACHE_SIZE 	(3*TOTAL_CACHE_SIZE/4)
#define LEAF_CACHE_SIZE 		(1*TOTAL_CACHE_SIZE/4)

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
/*
bool operator != (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) != 0;
}

bool operator < (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) < 0;
}

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

int main()
{
	stxxl::int64 ops = 1000000;
	
	map_type Map(NODE_CACHE_SIZE,LEAF_CACHE_SIZE);
	
	char *letters = "abcdefghijklmnopqrstuvwxuz";
	std::pair<my_key,my_data> element;
    
	memset(element.first.keybuf, 'a', KEY_SIZE);
	memset(element.second.databuf, 'b', DATA_SIZE);

	
	stxxl::timer Timer;
	stxxl::int64 i = ops;
	comp_type cmp_;
	Timer.start();
	            
	for (; i > 0; --i)
	{
		element.first.keybuf[(i % KEY_SIZE)] = letters[(i % 26)];
		Map.insert(element);
	}

	Timer.stop();
	STXXL_MSG("Records in map: "<<Map.size())
	STXXL_MSG("elapsed time: "<<(Timer.mseconds()/1000.)<<
				" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
}
