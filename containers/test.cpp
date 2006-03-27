#include "vector"
#include "../algo/scan.h"
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

template <class my_vec_type>
void test_const_iterator(const my_vec_type &x)
{
    typename my_vec_type::const_iterator i = x.begin();
	i = x.end();
	i.touch();
	i.flush();
	i++;
	++i;
	--i;
	i--;
	*i;
}


int main()
{
  typedef stxxl::VECTOR_GENERATOR<int64,2,1>::result vector_type;
	vector_type v(4*int64(1024*1024));
	
	// test assignment const_iterator = iterator
	vector_type::const_iterator c_it = v.begin();
	
	test_const_iterator(v);
	
	stxxl::random_number32 rnd;
	int offset = rnd();
	
	STXXL_MSG("write "<<v.size()<<" elements")

	stxxl::ran32State = 0xdeadbeef;
	int64 i;
	
	// fill the vector with increasing sequence of integer numbers
	for(i=0; i < v.size();++i)
	{
		v[i] = i + offset;
		assert(v[i] == i + offset);
	}
	
  
	// fill the vector with random numbers
	stxxl::generate(v.begin(),v.end(),stxxl::random_number32(),4);	
	v.flush();
	
	STXXL_MSG("seq read of "<<v.size()<<" elements")
		
	stxxl::ran32State = 0xdeadbeef;

	// testing swap
	vector_type a;
	std::swap(v,a);
	std::swap(v,a); 
	
	for(i=0;i<v.size();i++)
	{
		assert(v[i] == rnd() );
	}
	
	// check again
	STXXL_MSG("clear")
	
	v.clear();
		
	stxxl::ran32State = 0xdeadbeef + 10;
	
	v.resize(4*int64(1024*1024));
	
	STXXL_MSG("write "<<v.size()<<" elements")
	stxxl::generate(v.begin(),v.end(),stxxl::random_number32(),4);
	
	stxxl::ran32State = 0xdeadbeef + 10;

	STXXL_MSG("seq read of "<<v.size()<<" elements")
	
	for(i=0;i<v.size();i++)
	{
		assert(v[i] == rnd() );
	}
	
	return 0;
};
