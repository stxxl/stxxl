#include "../containers/vector"
#include <iostream>
#include <algorithm>
#include "scan.h"

//! \example algo/test_scan.cpp
//! This is an example of how to use \c stxxl::for_each() and \c stxxl::find() algorithms

using namespace stxxl;

template <typename type>
struct counter
{
	type value;
	counter(type v = type(0)):value(v) {}
	type operator() ()
	{
		type old_val = value;
		value++;
		return old_val;
	}
};

template <typename type>
struct square
{
	void operator()(type & arg)
	{
		arg = arg * arg ;
	}
};


int main()
{
	int64 i;
	stxxl::vector<int64> v(64*int64(1024*1024));
	//stxxl::vector<int64>::iterator it = v.begin();
	//stxxl::vector<int64>::const_iterator c_it = v.begin();
	
	
	//stxxl::random_number<> rnd;
	
	STXXL_MSG("write "<<(v.end() - v.begin()))
	
	std::generate(v.begin(),v.end(),counter<int64>());	
	
	STXXL_MSG("for_each ...")
	double b = stxxl_timestamp();
	stxxl::for_each(v.begin(),v.end(),square<int64>(),4);
	double e = stxxl_timestamp();
	STXXL_MSG("for_each time: "<<(e-b))
	
	STXXL_MSG("check")
	for(i=0;i<v.size();i++)
	{
		if(v[i] != i*i ) STXXL_MSG("Error at position "<<i)
	}

	STXXL_MSG("Pos of value    1023: "<< (stxxl::find(v.begin(),v.end(),1023,4) - v.begin()))
	STXXL_MSG("Pos of value 1048576: "<< (stxxl::find(v.begin(),v.end(),1024*1024,4) - v.begin()))
	STXXL_MSG("Pos of value    1024: "<< (stxxl::find(v.begin(),v.end(),32*32,4) - v.begin()))
	
	return 0;
};
