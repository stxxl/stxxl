#include "../mng/mng.h"
#include "sort.h"
#include "../containers/vector"

//! \example algo/sort.cpp 
//! This is an example of how to use \c stxxl::sort() algorithm

using namespace stxxl;


struct my_type
{
	typedef unsigned key_type;
				
	key_type _key;
	char _data[128 - sizeof(key_type)];
	key_type key() const {return _key; };
							
	my_type() {};
	my_type(key_type __key):_key(__key) {};
									
	static my_type min_value(){ return my_type(0); };
	static my_type max_value(){ return my_type(0xffffffff); };
};

bool operator < (const my_type & a, const my_type & b)
{
	return a.key() < b.key();
}

bool operator != (const my_type & a, const my_type & b)
{
	return a.key() != b.key();
}

struct cmp: public std::less<my_type>
{
  my_type min_value() const { return my_type(0); };
	my_type max_value() const { return my_type(0xffffffff); };
};

int main()
{	
		unsigned memory_to_use = 64*1024*1024;
		typedef stxxl::vector<my_type> vector_type;
		const stxxl::int64 n_records = 16*32*1024*1024/sizeof(my_type) - 234;
		vector_type v(n_records);
	
    random_number32 rnd;
		STXXL_MSG("Filling vector...")
		for(stxxl::int64 i=0; i < v.size(); i++)
			v[i]._key = rnd();
	
		STXXL_MSG("Checking order...")
		STXXL_MSG( ((stxxl::is_sorted(v.begin(),v.end()))?"OK":"WRONG" ));
	
		STXXL_MSG("Sorting...")
		stxxl::sort(v.begin(),v.end(),cmp(),memory_to_use);
	
		STXXL_MSG("Checking order...")
		STXXL_MSG( ((stxxl::is_sorted(v.begin(),v.end()))?"OK":"WRONG" ));
		
	
		return 0;
}
