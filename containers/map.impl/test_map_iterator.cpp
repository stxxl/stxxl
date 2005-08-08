/***************************************************************************
                          test_map_iterator.cpp  -  description
                             -------------------
    begin                : Mit Okt 13 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file test_map_iterator.cpp
//! \brief File for testing iterators of stxxl::map.


#include <mng/mng.h>
#include <algo/ksort.h>
#include "vector"
#include "map_async.h"
#include "my_type.h"
#include "test_handlers.h"


//! \example containers/map/test_map_iterator.cpp
//! This is an example of use of \c stxxl::map.
using namespace stxxl;
using namespace stxxl::map_test;

typedef std::map<key_type,data_type,cmp2>							std_map_type;
typedef stxxl::map<key_type,data_type,cmp2,4096,2 >		xxl_map_type;

#define LOWER 10
#define UPPER 1000

#define MSG_TEST_START STXXL_MSG( "TEST : test_map_iterator " << STXXL_PRETTY_FUNCTION_NAME << " start" );
#define MSG_TEST_FINISH STXXL_MSG( "TEST : test_map_iterator " << STXXL_PRETTY_FUNCTION_NAME << " finish" );


void test_next1( std_map_type& stdmap, xxl_map_type& xxlmap )
{
	MSG_TEST_START

	std_map_type::iterator siter1 = stdmap.begin();
	xxl_map_type::iterator xiter1 = xxlmap.begin();

	std_map_type::iterator siter2 = siter1;
	xxl_map_type::iterator xiter2 = xiter1;

	while( siter1 != stdmap.end() )
	{
		assert( xiter1 != xxlmap.end() );
		assert( is_same( *(siter1++), *(xiter1++) ) );
		assert( !is_same( *siter1, *siter2 ) || siter1 == stdmap.end() );
		assert( !is_same( *xiter1, *xiter2 ) || xiter1 == xxlmap.end() );
	}
	assert( xiter1 == xxlmap.end() );
	assert( siter2 == stdmap.begin() );
	assert( xiter2 == xxlmap.begin() );

	MSG_TEST_FINISH
}


void test_const_next1( const std_map_type& stdmap, const xxl_map_type& xxlmap )
{
	MSG_TEST_START

	std_map_type::const_iterator siter1 = stdmap.begin();
	xxl_map_type::const_iterator xiter1 = xxlmap.begin();

	std_map_type::const_iterator siter2 = siter1;
	xxl_map_type::const_iterator xiter2 = xiter1;

	while( siter1 != stdmap.end() )
	{
		assert( xiter1 != xxlmap.end() );
		assert( is_same( *(siter1++), *(xiter1++) ) );
		assert( !is_same( *siter1, *siter2 ) || siter1 == stdmap.end() );
		assert( !is_same( *xiter1, *xiter2 ) || xiter1 == xxlmap.end() );
	}
	assert( xiter1 == xxlmap.end() );
	assert( siter2 == stdmap.begin() );
	assert( xiter2 == xxlmap.begin() );

	MSG_TEST_FINISH
}


void test_next2( std_map_type& stdmap, xxl_map_type& xxlmap )
{
	MSG_TEST_START

	std_map_type::iterator siter1 = stdmap.begin();
	xxl_map_type::iterator xiter1 = xxlmap.begin();

	std_map_type::iterator siter2 = siter1;
	xxl_map_type::iterator xiter2 = xiter1;

	while( siter1 != stdmap.end() )
	{
		assert( xiter1 != xxlmap.end() );
		assert( is_same( *++siter1, *++xiter1 ) || siter1 == stdmap.end() && xiter1 == xxlmap.end() );
	}
	assert( xiter1 == xxlmap.end() );
	assert( siter2 == stdmap.begin() );
	assert( xiter2 == xxlmap.begin() );

	MSG_TEST_FINISH
}


void test_const_next2( const std_map_type& stdmap, const xxl_map_type& xxlmap )
{
	MSG_TEST_START

	std_map_type::const_iterator siter1 = stdmap.begin();
	xxl_map_type::const_iterator xiter1 = xxlmap.begin();

	std_map_type::const_iterator siter2 = siter1;
	xxl_map_type::const_iterator xiter2 = xiter1;

	while( siter1 != stdmap.end() )
	{
		assert( xiter1 != xxlmap.end() );
		assert( is_same( *++siter1, *++xiter1 ) || siter1 == stdmap.end() && xiter1 == xxlmap.end() );
	}
	assert( xiter1 == xxlmap.end() );
	assert( siter2 == stdmap.begin() );
	assert( xiter2 == xxlmap.begin() );

	MSG_TEST_FINISH
}


void test_prev1( std_map_type& stdmap, xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::iterator siter1 = stdmap.find( last_key );
	xxl_map_type::iterator xiter1 = xxlmap.find( last_key );

	std_map_type::iterator siter2 = siter1;
	xxl_map_type::iterator xiter2 = xiter1;

	assert( siter1 != stdmap.end() || stdmap.empty() );
	assert( xiter1 != xxlmap.end() || xxlmap.empty() );

	assert( siter1 == stdmap.end() || !stdmap.empty() );
	assert( xiter1 == xxlmap.end() || !xxlmap.empty() );

	assert( stdmap.empty() || ++siter2 == stdmap.end() );
	assert( xxlmap.empty() || ++xiter2 == xxlmap.end() );

	if( siter1 == stdmap.end() )
	{
		assert( xiter1 == xxlmap.end() );
		return;
	}

	while( siter1 != stdmap.begin() )
	{
		assert( is_same( *(--siter1), *(--xiter1) ) );
	}

	assert( xiter1 == xxlmap.begin()  );
	assert( is_same( *siter1, *xiter1 ));
	MSG_TEST_FINISH
}


void test_const_prev1( const std_map_type& stdmap, const xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::const_iterator siter1 = stdmap.find( last_key );
	xxl_map_type::const_iterator xiter1 = xxlmap.find( last_key );

	std_map_type::const_iterator siter2 = siter1;
	xxl_map_type::const_iterator xiter2 = xiter1;

	assert( siter1 != stdmap.end() || stdmap.empty() );
	assert( xiter1 != xxlmap.end() || xxlmap.empty() );

	assert( siter1 == stdmap.end() || !stdmap.empty() );
	assert( xiter1 == xxlmap.end() || !xxlmap.empty() );

	assert( stdmap.empty() || ++siter2 == stdmap.end() );
	assert( xxlmap.empty() || ++xiter2 == xxlmap.end() );

	if( siter1 == stdmap.end() )
	{
		assert( xiter1 == xxlmap.end() );
		return;
	}

	while( siter1 != stdmap.begin() )
	{
		assert( is_same( *--siter1, *--xiter1 ));
	}

	assert( xiter1 == xxlmap.begin() );
	assert( is_same( *siter1, *xiter1 ));
	MSG_TEST_FINISH
}


void test_prev2( std_map_type& stdmap, xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::iterator siter1 = stdmap.find( last_key );
	xxl_map_type::iterator xiter1 = xxlmap.find( last_key );

	assert( siter1 != stdmap.end() || stdmap.empty() );
	assert( xiter1 != xxlmap.end() || xxlmap.empty() );

	assert( siter1 == stdmap.end() || !stdmap.empty() );
	assert( xiter1 == xxlmap.end() || !xxlmap.empty() );

	if( siter1 == stdmap.end() )
	{
		assert( xiter1 == xxlmap.end() );
		return;
	}
	
	while( siter1 != stdmap.begin() )
	{
		assert( is_same( *(siter1--), *(xiter1--) ) );
	}
	assert( xiter1 == xxlmap.begin() );
	assert( is_same( *siter1, *xiter1 ) );

	MSG_TEST_FINISH
}


void test_const_prev2( const std_map_type& stdmap, const xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::const_iterator siter1 = stdmap.find( last_key );
	xxl_map_type::const_iterator xiter1 = xxlmap.find( last_key );

	std_map_type::const_iterator siter2 = siter1;
	xxl_map_type::const_iterator xiter2 = xiter1;

	assert( siter1 != stdmap.end() || stdmap.empty() );
	assert( xiter1 != xxlmap.end() || xxlmap.empty() );

	assert( siter1 == stdmap.end() || !stdmap.empty() );
	assert( xiter1 == xxlmap.end() || !xxlmap.empty() );

	if( siter1 == stdmap.end() )
	{
		assert( xiter1 == xxlmap.end() );
		return;
	}

	while( siter1 != stdmap.begin() )
	{
		assert( is_same( *(siter1--), *(xiter1--) ) );
	}
	assert( xiter1 == xxlmap.begin() );
	assert( is_same( *siter1, *xiter1 ) );

	MSG_TEST_FINISH
}
                                                             

int main()
{
	MSG_TEST_START

	typedef stxxl::VECTOR_GENERATOR<my_type>::result vector_type;
	vector_type v( int64(24*1024) );
	stxxl::random_number32 rnd;
	stxxl::ran32State = 0xdeadbeef;
	int64 i = 0;

	// fill the vector with increasing sequence of integer numbers
	for(i=0; i < v.size();i++)
	{
		v[i].first = i;
		assert(v[i].first == (unsigned) i );
	}

	std_map_type  stdmap( v.begin(), v.end() );
	xxl_map_type  xxlmap( v.begin(), v.end() );

	test_next1( stdmap, xxlmap );
	test_next2( stdmap, xxlmap );
	test_prev1( stdmap, xxlmap, v[ v.size()-1 ].first );
	test_prev2( stdmap, xxlmap, v[ v.size()-1 ].first );

	test_const_next1( stdmap, xxlmap );
	test_const_next2( stdmap, xxlmap );
	test_const_prev1( stdmap, xxlmap, v[ v.size()-1 ].first );
	test_const_prev2( stdmap, xxlmap, v[ v.size()-1 ].first ); 

	stdmap.clear();
	xxlmap.clear();

	unsigned max = 0;

	for( int step = 0; step < 1000; step++ )
	{
		STXXL_MSG( "***********************************************************" );
		STXXL_MSG( "STEP = " << step );
		test_next1( stdmap, xxlmap );
		test_next2( stdmap, xxlmap );
		test_prev1( stdmap, xxlmap, max );
		test_prev2( stdmap, xxlmap, max );

		test_const_next1( stdmap, xxlmap );
		test_const_next2( stdmap, xxlmap );
		test_const_prev1( stdmap, xxlmap, max );
		test_const_prev2( stdmap, xxlmap, max );

		my_type val( rnd() % 2000, rnd() ) ;

		if( max < val.first ) max = val.first;

		stdmap.insert( val );
		xxlmap.insert( val );
	}
	
	MSG_TEST_FINISH
	return 0;
}

