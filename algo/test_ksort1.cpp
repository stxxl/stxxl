#include "../mng/mng.h"
#include "ksort.h"
#include "../containers/vector"

//! \example algo/test_ksort1.cpp 
//! This is an example of how to use \c stxxl::ksort() algorithm

using namespace stxxl;


struct my_type
{
	typedef stxxl::uint64 key_type1;
				
	key_type1 _key;
	key_type1 _key_copy;
	char _data[32 - 2*sizeof(key_type1)];
	key_type1 key() const {return _key; };
							
	my_type() {};
	my_type(key_type1 __key):_key(__key) {};
									
	//my_type min_value() const { return my_type(0); };
	//my_type max_value() const { return my_type(0xffffffff); };
};

std::ostream & operator << (std::ostream & o, const my_type & obj)
{
	o << obj._key << " "<<obj._key_copy;
	return o;
}

struct get_key
{
	typedef my_type::key_type1 key_type;
	key_type operator() (const my_type & obj)
	{
		return obj._key;
	}
	my_type min_value(){ return my_type(0); };
	my_type max_value(){ return my_type(0xffffffff); };
};


bool operator < (const my_type & a, const my_type & b)
{
	return a.key() < b.key();
}

int main()
{	
		unsigned memory_to_use = 32*1024*1024;
		typedef stxxl::VECTOR_GENERATOR<my_type,2,2>::result vector_type;
		const stxxl::int64 n_records = 3*32*stxxl::int64(1024*1024)/sizeof(my_type);
		vector_type v(n_records);
	
		random_number64 rnd;
		STXXL_MSG("Filling vector... ")
		for(stxxl::int64 i=0; i < v.size(); i++)
		{
			v[i]._key = rnd();
			v[i]._key_copy = v[i]._key;
		}
	
		//STXXL_MSG("Checking order...")
		//STXXL_MSG(((stxxl::is_sorted(v.begin(),v.end()))?"OK":"WRONG" ));
	
		STXXL_MSG("Sorting...")
		stxxl::ksort(v.begin(),v.end(),get_key(),memory_to_use);
    //stxxl::ksort(v.begin(),v.end(),memory_to_use);
	
		
		STXXL_MSG("Checking order...")
		STXXL_MSG(((stxxl::is_sorted(v.begin(),v.end()))?"OK":"WRONG" ));
		STXXL_MSG("Checking content...")
		my_type prev;
		for(stxxl::int64 i=0; i < v.size(); i++)
		{
			if( v[i]._key != v[i]._key_copy)
			{
				STXXL_MSG("Bug at position "<<i)
				abort();
			}
			if(i>0 && prev._key == v[i]._key)
			{
				STXXL_MSG("Duplicate at position "<<i)
				//abort();
			}
			prev = v[i];
		}
		STXXL_MSG("OK")
	
		return 0;
}
