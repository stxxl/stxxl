#include "vector"
#include <iostream>
#include <algorithm>

//! \example containers/test.cpp 
//! This is an example of use of \c stxxl::vector and 
//! \c stxxl::VECTOR_GENERATOR. Vector type is configured
//! to store 64-bit integers and have 2 pages each of 1 block

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


int main()
{
  typedef stxxl::VECTOR_GENERATOR<int64,2,1>::result vector_type;
	vector_type v(4*int64(1024*1024));
	vector_type::iterator it = v.begin();
	vector_type::const_iterator c_it = v.begin();
	STXXL_MSG(v.end().bid() - c_it.bid() )
	STXXL_MSG(c_it.block_offset() )
	STXXL_MSG(v.end().block_offset() )
	
	stxxl::random_number32 rnd;
	int offset = rnd();
	
	STXXL_MSG("write "<<v.size())

	stxxl::ran32State = 0xdeadbeef;
	int64 i;
	
	// fill the vector with increasing sequence of integer numbers
	for(i=0; i < v.size();i++)
	{
		v[i] = i + offset;
		assert(v[i] == i + offset);
	}
	
  
	// fill the vector with random numbers
	std::generate(v.begin(),v.end(),stxxl::random_number32());	
	v.flush();
	
	STXXL_MSG("seq read"<<v.size())
		
	stxxl::ran32State = 0xdeadbeef;

	
	for(i=0;i<v.size();i++)
	{
		assert(v[i] == rnd() );
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
