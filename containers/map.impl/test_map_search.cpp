/***************************************************************************
                          test_map_search.cpp  -  description
                             -------------------
    begin                : Mit Okt 13 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file test_map_search.cpp
//! \brief A test for search function of stxxl::map container.
//!
//!  A diferent inputs for \c stxxl::map will be created.
//!  The functions for searching will bee tested:
//!  \li stxxl::map::find
//!  \li stxxl::map::find const
//!  \li stxxl::map::lower_bound
//!  \li stxxl::map::lower_bound const
//!  \li stxxl::map::upper_bound
//!  \li stxxl::map::upper_bound const
//!  \li stxxl::map::equal_range
//!  \li stxxl::map::equal_range const \n
//!
//!  The output of this functions will be comare with the result of the same functions of map container in STL Library.
//!  All tests will be failed, if the results become different.
 
#include <algo/ksort.h>
#include "vector"
#include "map_async.h"
#include "my_type.h"
#include "test_handlers.h"


//! \example containers/map/test_map_search.cpp
//! This is an example of use of \c stxxl::map.

using namespace stxxl;
using namespace stxxl::map_test;

typedef std::map<key_type,data_type,cmp2>							std_map_type;
typedef stxxl::map<key_type,data_type,cmp2,4096,2 >		xxl_map_type;

#define LOWER 10
#define UPPER 1000

#define MSG_TEST_START STXXL_MSG( "TEST : test_map_iterator " << STXXL_PRETTY_FUNCTION_NAME << " start" );
#define MSG_TEST_FINISH STXXL_MSG( "TEST : test_map_iterator " << STXXL_PRETTY_FUNCTION_NAME << " finish" );

#define TEST_COUTER_START		unsigned test_couter = 1;
#define TEST_COUTER_STEP(x)\
		if ( test_couter++ % x == 0 ) \
		{\
			std::cout << "." ;\
			test_couter = 0;\
		}
#define TEST_COUTER_STOP


// *****************************************************************************
// Lookup for number in interval( 0 ; min( last_key, 0xFFFFFFFF) ).
// Call find for all keys.
// Assertion failed if one of the map gives diferent value for iterator.
// *****************************************************************************

void test_find( std_map_type& stdmap, xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::iterator siter1;
	xxl_map_type::iterator xiter1;
	
	TEST_COUTER_START
	unsigned cur_key = 0;
	
	while( cur_key <= last_key && cur_key < 0xFFFFFFFF )
	{
		TEST_COUTER_STEP( 10000 );

		siter1 = stdmap.find( cur_key );
		xiter1 = xxlmap.find( cur_key++ );
		assert(
			is_same( *(siter1), *(xiter1) ) ||
			is_end(stdmap,siter1) && is_end(xxlmap,xiter1)
		);
	}
	TEST_COUTER_STOP
	MSG_TEST_FINISH
}

// *****************************************************************************
// Lookup for number in interval( 0 ; min( last_key, 0xFFFFFFFF) ).
// Call find for all keys.
// Assertion failed if one of the map gives diferent value for const iterator.
// *****************************************************************************

void test_const_find( const std_map_type& stdmap, const xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::const_iterator siter1;
	xxl_map_type::const_iterator xiter1;

	TEST_COUTER_START
	unsigned cur_key = 0;

	while( cur_key <= last_key && cur_key < 0xFFFFFFFF )
	{
		TEST_COUTER_STEP( 10000 )
		siter1 = stdmap.find( cur_key );
		xiter1 = xxlmap.find( cur_key++ );
		assert(
			is_same( *(siter1), *(xiter1) ) ||
			is_end(stdmap,siter1) && is_end(xxlmap,xiter1)
		);
	}
	TEST_COUTER_STOP
	MSG_TEST_FINISH
}

// *****************************************************************************
// Lookup for number in interval( 0 ; min( last_key, 0xFFFFFFFF) ).
// Call upper_bound for all keys.
// Assertion failed if one of the map gives diferent value for iterator.
// *****************************************************************************

void test_upper( std_map_type& stdmap, xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::iterator siter1;
	xxl_map_type::iterator xiter1;

	TEST_COUTER_START
	unsigned cur_key = 0;

	while( cur_key <= last_key && cur_key < 0xFFFFFFFF )
	{
		TEST_COUTER_STEP( 10000 )
		siter1 = stdmap.upper_bound( cur_key );
		xiter1 = xxlmap.upper_bound( cur_key++ );
		assert(
			is_same( *(siter1), *(xiter1) ) ||
			is_end(stdmap,siter1) && is_end(xxlmap,xiter1)
		);
	}
	TEST_COUTER_STOP
	MSG_TEST_FINISH
}

// *****************************************************************************
// Lookup for number in interval( 0 ; min( last_key, 0xFFFFFFFF) ).
// Call upper_bound for all keys.
// Assertion failed if one of the map gives diferent value for const iterator.
// *****************************************************************************

void test_const_upper( const std_map_type& stdmap, const xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::const_iterator siter1;
	xxl_map_type::const_iterator xiter1;

	TEST_COUTER_START
	unsigned cur_key = 0;

	while( cur_key <= last_key && cur_key < 0xFFFFFFFF )
	{
		TEST_COUTER_STEP( 10000 )
		siter1 = stdmap.upper_bound( cur_key );
		xiter1 = xxlmap.upper_bound( cur_key++ );
		assert(
			is_same( *(siter1), *(xiter1) ) ||
			is_end(stdmap,siter1) && is_end(xxlmap,xiter1)
		);
	}
	TEST_COUTER_STOP
	MSG_TEST_FINISH
}


// *****************************************************************************
// Lookup for number in interval( 0 ; min( last_key, 0xFFFFFFFF) ).
// Call lower_bound for all keys.
// Assertion failed if one of the map gives diferent value for iterator.
// *****************************************************************************

void test_lower( std_map_type& stdmap, xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::iterator siter1;
	xxl_map_type::iterator xiter1;

	TEST_COUTER_START
	unsigned cur_key = 0;

	while( cur_key <= last_key && cur_key < 0xFFFFFFFF )
	{
		TEST_COUTER_STEP( 10000 )
		siter1 = stdmap.lower_bound( cur_key );
		xiter1 = xxlmap.lower_bound( cur_key++ );
		assert(
			is_same( *(siter1), *(xiter1) ) ||
			is_end(stdmap,siter1) && is_end(xxlmap,xiter1)
		);
	}
	TEST_COUTER_STOP
	MSG_TEST_FINISH
}

// *****************************************************************************
// Lookup for number in interval( 0 ; min( last_key, 0xFFFFFFFF) ).
// Call lower_bound for all keys.
// Assertion failed if one of the map gives diferent value for const iterator.
// *****************************************************************************

void test_const_lower( const std_map_type& stdmap, const xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std_map_type::const_iterator siter1;
	xxl_map_type::const_iterator xiter1;

	TEST_COUTER_START
	unsigned cur_key = 0;

	while( cur_key <= last_key && cur_key < 0xFFFFFFFF )
	{
		TEST_COUTER_STEP( 10000 )
		siter1 = stdmap.lower_bound( cur_key );
		xiter1 = xxlmap.lower_bound( cur_key++ );
		assert(
			is_same( *(siter1), *(xiter1) ) ||
			is_end(stdmap,siter1) && is_end(xxlmap,xiter1)
		);
	}
	TEST_COUTER_STOP
	MSG_TEST_FINISH
}

// *****************************************************************************
// Lookup for number in interval( 0 ; min( last_key, 0xFFFFFFFF) ).
// Call equal_range for all keys.
// Assertion failed if one of the map gives diferent value for iterator.
// *****************************************************************************

void test_equal_range( std_map_type& stdmap, xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std::pair<std_map_type::iterator,std_map_type::iterator> spair1;
	std::pair<xxl_map_type::iterator,xxl_map_type::iterator> xpair1;

	TEST_COUTER_START
	unsigned cur_key = 0;

	while( cur_key <= last_key && cur_key < 0xFFFFFFFF )
	{
		TEST_COUTER_STEP( 10000 )
		spair1 = stdmap.equal_range( cur_key );
		xpair1 = xxlmap.equal_range( cur_key++ );
		assert(
			is_same( *(spair1.first), *(xpair1.first) ) ||
			is_end(stdmap,spair1.first) && is_end(xxlmap,xpair1.first)
		);
		assert(
			is_same( *(spair1.second), *(xpair1.second) ) ||
			is_end(stdmap,spair1.second) && is_end(xxlmap,xpair1.second)
		);
	}
	TEST_COUTER_STOP
	MSG_TEST_FINISH
}

// *****************************************************************************
// Lookup for number in interval( 0 ; min( last_key, 0xFFFFFFFF) ).
// Call equal_range for all keys.
// Assertion failed if one of the map gives diferent value for const iterator.
// *****************************************************************************

void test_const_equal_range( const std_map_type& stdmap, const xxl_map_type& xxlmap, unsigned last_key )
{
	MSG_TEST_START

	std::pair<std_map_type::const_iterator,std_map_type::const_iterator> spair1;
	std::pair<xxl_map_type::const_iterator,xxl_map_type::const_iterator> xpair1;

	TEST_COUTER_START
	unsigned cur_key = 0;

	while( cur_key <= last_key && cur_key < 0xFFFFFFFF )
	{
		TEST_COUTER_STEP( 10000 )
		spair1 = stdmap.equal_range( cur_key );
		xpair1 = xxlmap.equal_range( cur_key++ );
		assert(
			is_same( *(spair1.first), *(xpair1.first) ) ||
			is_end(stdmap,spair1.first) && is_end(xxlmap,xpair1.first)
		);
		assert(
			is_same( *(spair1.second), *(xpair1.second) ) ||
			is_end(stdmap,spair1.second) && is_end(xxlmap,xpair1.second)
		);
	}
	TEST_COUTER_STOP
	MSG_TEST_FINISH
}
                                                             

// *****************************************************************************
// The main function creates two maps.
// First map is a STL map container we will use for verify our map.
// Second map is a STXXL map we will test.
// *****************************************************************************

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

	// create maps for testing
	std_map_type  stdmap( v.begin(), v.end() );
	xxl_map_type  xxlmap( v.begin(), v.end() );

	// call search test functions
	test_find				( stdmap, xxlmap, v[ v.size()-1 ].first );
	test_const_find	( stdmap, xxlmap, v[ v.size()-1 ].first );
	test_upper			( stdmap, xxlmap, v[ v.size()-1 ].first );
	test_const_upper( stdmap, xxlmap, v[ v.size()-1 ].first );

	test_lower						( stdmap, xxlmap, v[ v.size()-1 ].first );
	test_const_lower			( stdmap, xxlmap, v[ v.size()-1 ].first );
	test_equal_range			( stdmap, xxlmap, v[ v.size()-1 ].first );
	test_const_equal_range( stdmap, xxlmap, v[ v.size()-1 ].first );

	// clear maps for the next test
	stdmap.clear();
	xxlmap.clear();

	unsigned max = 0;

	// after each step a value will be insert in to the map
	for( int step = 0; step < 1000; step++ )
	{
		STXXL_MSG( "***********************************************************" );
		STXXL_MSG( "STEP = " << step );
		
		// call search test functions
		test_find							( stdmap, xxlmap, 2000 );
		test_const_find				( stdmap, xxlmap, 2000 );
		test_upper						( stdmap, xxlmap, 2000 );
		test_const_upper			( stdmap, xxlmap, 2000 );

		test_lower						( stdmap, xxlmap, 2000 );
		test_const_lower			( stdmap, xxlmap, 2000 );
		test_equal_range			( stdmap, xxlmap, 2000 );
		test_const_equal_range( stdmap, xxlmap, 2000 );

		// insert a random value into the map
		my_type val( rnd() % 2000, rnd() ) ;
		if( max < val.first ) max = val.first;
		stdmap.insert( val );
		xxlmap.insert( val );
	}
	
	MSG_TEST_FINISH
	return 0;
}

