#include "vector"
#include <iostream>
#include <algorithm>

using namespace stxxl;

struct counter
{
	int value;
	counter(int v):value(v) {};
	int operator() ()
	{
		int old_val = value;
		value++;
		return old_val;
	};	
};

void my_assert(int value)
{
	assert(value);
};	

int main()
{
	stxxl::vector<int64> v(int64(1024)*int64(1024*1024));
	stxxl::vector<int64>::iterator it = v.begin();
	stxxl::vector<int64>::const_iterator c_it = v.begin();
	STXXL_MSG(v.end().bid() - c_it.bid() )
	STXXL_MSG( c_it.block_offset() )
	STXXL_MSG( v.end().block_offset() )
	
	stxxl::random_number rnd;
	int offset = rnd(1024);
	
	STXXL_MSG("write "<<v.size())

	stxxl::ran32State = 0xdeadbeef;
	
	std::generate(v.begin(),v.end(),stxxl::random_number32());	
	int64 i;
	//for(i=0; i < v.size();i++)
	//{
	//	v[i] = stxxl::random_number32()();//i + offset;
	//	assert(v[i] == i + offset);
	//}
	
	v.flush();
	
	STXXL_MSG("seq read"<<v.size())
		
	stxxl::ran32State = 0xdeadbeef;

	
	for(i=0;i<v.size();i++)
	{
		assert(v[i] == rand32() );
	}
	
	/*
	STXXL_MSG("rnd read")
	i = v.size();
	for(; i > 0 ;i--)
	{
		int64 pos = rnd(v.size());
		assert(v[pos] == pos + offset);
	}*/
	
	
	return 0;
};
