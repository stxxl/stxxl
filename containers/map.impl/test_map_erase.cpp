/***************************************************************************
                          test_map_erase.cpp  -  description
                             -------------------
    begin                : Mit Okt 13 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file test_map_erase.cpp
//! \brief File for testing erasse functions of stxxl::map.

#include <mng/mng.h>
#include <algo/ksort.h>
#include <containers/vector>
#include "map_async.h"
#include "my_type.h"
#include "test_handlers.h"

//! \example containers/map/test_map_erase.cpp
//! This is an example of use of \c stxxl::map.
using namespace stxxl;
using namespace stxxl::map_test;

typedef std::map< key_type,data_type,cmp2 >						std_map;
typedef stxxl::map< key_type,data_type,cmp2,4096,0 >	xxl_map;

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

#define LOWER 1
#define UPPER 200

void statistic()
{
	typedef stxxl::map_internal::btnode<key_type,data_type,cmp2, 4096, true,0 >     leaf_type;
	typedef stxxl::map_internal::btnode<key_type,BID< 4096 >,cmp2, 4096,true,0 >		node_type;

#ifdef STXXL_BTREE_STATISTIC

	STXXL_MSG( "**************************STATISTIC**************************" );
	STXXL_MSG( "NODES temp = " << node_type::_temp_node_stat() );
	STXXL_MSG( "NODES alle = " << node_type::_all_node_stat() );
	STXXL_MSG( "NODES read = " << node_type::_reads_stat() );
	STXXL_MSG( "NODES write= " << node_type::_writes_stat() );
	STXXL_MSG( "LEAFS temp = " << leaf_type::_temp_node_stat() );
	STXXL_MSG( "LEAFS alle = " << leaf_type::_all_node_stat() );
	STXXL_MSG( "LEAFS read = " << leaf_type::_reads_stat() );
	STXXL_MSG( "LEAFS write= " << leaf_type::_writes_stat() );
	STXXL_MSG( "**************************STATISTIC**************************" );
#endif
}


// **************************************************************************
// This function tests the erase function for a key
// stlmap is a help object from STL of type std::map
// xxlmap is a object from STXXL of type stxxl::map
// We use the same functions for both objects.
// **************************************************************************

void erase_key_impl( std_map& stlmap, xxl_map& xxlmap, int lower, int upper )
{
	// Curent maps contains keys interval [ LOWER - UPPER ).
	// We call erase(key) for all keys from interval [ lower - upper ).
	
	for( int i = lower; i < upper; i++ ) assert( stlmap.erase( i ) == 1 );
	for( int i = lower; i < upper; i++ ) assert( xxlmap.erase( i ) == 1 );

	// Now we can test if the keys are there in map or not.
	// Only the keys from interval (lower - upper ) should be deleted.

	// this nodes have to be there [ LOWER - lower )
	for( int i = LOWER; i < lower; i++ ) assert( there( stlmap, i, 2*i ));
	for( int i = LOWER; i < lower; i++ ) assert( there( xxlmap, i, 2*i ));

	// this nodes haven't to be there [ lower - upper )
	for( int i = lower; i < upper; i++ ) assert( not_there( stlmap, i ));
	for( int i = lower; i < upper; i++ ) assert( not_there( xxlmap, i ));

	// this nodes have to be there [ upper - UPPER )
	for( int i = upper; i < UPPER; i++ ) assert( there( stlmap, i, 2*i ));
	for( int i = upper; i < UPPER; i++ ) assert( there( xxlmap, i, 2*i ));

	// Now we clear hole maps.
	stlmap.clear();
	xxlmap.clear();

	// Both maps have to be empty
	assert(is_empty(stlmap));
	assert(is_empty(xxlmap));
}


// **************************************************************************
// This function tests the erase function map for an iterator
// stlmap is a help object from STL of type std::map
// xxlmap is a object from STXXL of type stxxl::map
// We use the same functions for both objects.
// **************************************************************************

void erase_iterator_impl( std_map& stlmap, xxl_map& xxlmap, int lower, int upper )
{
	// Curent maps contains keys interval [ LOWER - UPPER ).
	// We call erase(iter), where iter is an iterator to the entries for all keys from interval [ lower - upper ).

	for( int i = lower; i < upper; i++ ) stlmap.erase( stlmap.find(i) );
	for( int i = lower; i < upper; i++ ) xxlmap.erase( xxlmap.find(i) );

	// Now we can test if the keys are there in map or not.
	// Only the keys from interval (lower - upper ) should be deleted.

	// this nodes have to be there [ LOWER - lower )
	for( int i = LOWER; i < lower; i++ ) assert( there( stlmap, i, 2*i ));
	for( int i = LOWER; i < lower; i++ ) assert( there( xxlmap, i, 2*i ));

	// this nodes haven't to be there [ lower - upper )
	for( int i = lower; i < upper; i++ ) assert( not_there( stlmap, i ));
	for( int i = lower; i < upper; i++ ) assert( not_there( xxlmap, i ));

	// this nodes have to be there [ upper - UPPER )
	for( int i = upper; i < UPPER; i++ ) assert( there( stlmap, i, 2*i ));
	for( int i = upper; i < UPPER; i++ ) assert( there( xxlmap, i, 2*i ));

	// Now we clear both maps.
	stlmap.clear();
	xxlmap.clear();

	// Both maps have to be empty
	assert(is_empty(stlmap));
	assert(is_empty(xxlmap));
}

// **************************************************************************
// This function tests the erase function map for an iterator
// stlmap is a help object from STL of type std::map
// xxlmap is a object from STXXL of type stxxl::map
// We use the same functions for both objects.
// **************************************************************************

void erase_range_impl( std_map& stlmap, xxl_map& xxlmap, int lower, int upper )
{
	// Curent maps contains keys interval [ LOWER - UPPER ).
	// We call erase(iter), where iter is an iterator to the entries for all keys from interval [ lower - upper ).

	stlmap.erase( stlmap.find( lower ), stlmap.find( upper ) );
	xxlmap.erase( xxlmap.find( lower ), xxlmap.find( upper ) );

	// Now we can test if the keys are there in map or not.
	// Only the keys from interval (lower - upper ) should be deleted.

	// this nodes have to be there [ LOWER - lower )
	for( int i = LOWER; i < lower; i++ ) assert( there( stlmap, i, 2*i ));
	for( int i = LOWER; i < lower; i++ ) assert( there( xxlmap, i, 2*i ));

	// this nodes haven't to be there [ lower - upper )
	for( int i = lower; i < upper; i++ ) assert( not_there( stlmap, i ));
	for( int i = lower; i < upper; i++ ) assert( not_there( xxlmap, i ));

	// this nodes have to be there [ upper - UPPER )
	for( int i = upper; i < UPPER; i++ ) assert( there( stlmap, i, 2*i ));
	for( int i = upper; i < UPPER; i++ ) assert( there( xxlmap, i, 2*i ));

	// Now we clear both maps.
	stlmap.clear();
	xxlmap.clear();

	// Both maps have to be empty
	assert(is_empty(stlmap));
	assert(is_empty(xxlmap));
}

// This function generate a input where all nodes are full
// Thten the function erase_key_impl will be calling.
void erase_key_with_bulk_insert()
{
		MSG_TEST_START

		// A vector with entriews for the map will be generated

		typedef stxxl::VECTOR_GENERATOR<my_type>::result vector_type;
		vector_type v( UPPER - LOWER );
		for( unsigned j=0; j < UPPER - LOWER; j++)
		{
			v[j].first = LOWER +j;
			v[j].second = 2*v[j].first;
		}

		int lower = LOWER ;
		int upper;

		// Now test all posible pair for lower und upper

		while( lower <= UPPER )
		{
			upper = lower;
			while( upper <= UPPER )
			{
				statistic();
				STXXL_MSG( "*********************************************************" );
				STXXL_MSG( "ERASE KEY: " << lower << " - " << upper << "(bulk insert)" );
				STXXL_MSG( "*********************************************************" );

				std_map stlmap( v.begin(), v.end() );
				xxl_map xxlmap( v.begin(), v.end() );
				erase_key_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}

		MSG_TEST_FINISH
}

// This function generate a input where all nodes are halffull
// Thten the function erase_key_impl will be calling.
void erase_key_with_key_insert_sequentiel()
{
		MSG_TEST_START
		std_map stlmap;
		xxl_map xxlmap;
		typedef xxl_map::value_type value_type;

		int lower = LOWER ;
		int upper;

		while( lower <= UPPER )
		{
			upper = lower;

			while( upper <= UPPER )
			{
				statistic();
				STXXL_MSG( "***************************************************************" );
				STXXL_MSG( "ERASE KEY: " << lower << " - " << upper << "(seqeuntiel insert)" );
				STXXL_MSG( "***************************************************************" );

				for( int i = LOWER; i < UPPER; i++ ) stlmap.insert(value_type( i,2*i));
				for( int i = LOWER; i < UPPER; i++ ) xxlmap.insert(value_type( i,2*i));

				erase_key_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

// This function generate a random input.
// Thten the function erase_key_impl will be calling.
void erase_key_with_key_insert_random()
{
		MSG_TEST_START
		std_map stlmap;
		xxl_map xxlmap;
		typedef stxxl::VECTOR_GENERATOR<my_type>::result vector_type;
		vector_type v( UPPER - LOWER );
		for( unsigned j=0; j < UPPER - LOWER; j++)
		{
			v[j].first = LOWER +j;
			v[j].second = 2*v[j].first;
		}

		random_number32 rnd;
		stxxl::ran32State = 0xdeadbeef;
		int lower = LOWER ;
		int upper;

		while( lower <= UPPER )
		{
			upper = lower;

			while( upper <= UPPER )
			{
				for( int ix = 0; ix < UPPER- LOWER; ix ++ )
				{
					int ix1 = rnd() % (UPPER- LOWER);
					int ix2 = rnd() % (UPPER- LOWER);
					my_type help = v[ix1];
					v[ix1] = v[ix2];
					v[ix2] = help;
				}

				statistic();
				STXXL_MSG( "***********************************************************" );
				STXXL_MSG( "ERASE KEY: " << lower << " - " << upper << "(random insert)" );
				STXXL_MSG( "***********************************************************" );

				for( int i = 0; i < UPPER - LOWER; i++ ) stlmap.insert(v[i]);
				for( int i = 0; i < UPPER - LOWER; i++ ) xxlmap.insert(v[i]);

				erase_key_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

// This function generate a input where all nodes are full
// Then the function erase_iterator_impl will be calling.
void erase_iterator_with_bulk_insert()
{
		MSG_TEST_START

		// A vector with entriews for the map will be generated

		typedef stxxl::VECTOR_GENERATOR<my_type>::result vector_type;
		vector_type v( UPPER - LOWER );
		for( unsigned j=0; j < UPPER - LOWER; j++)
		{
			v[j].first = LOWER +j;
			v[j].second = 2*v[j].first;
		}

		int lower = LOWER ;
		int upper;

		// Now test all posible pair for lower und upper

		while( lower <= UPPER )
		{
			upper = lower;
			while( upper <= UPPER )
			{
				statistic();
				STXXL_MSG( "*********************************************************" );
				STXXL_MSG( "ERASE ITERATOR: " << lower << " - " << upper << "(bulk insert)" );
				STXXL_MSG( "*********************************************************" );

				std_map stlmap( v.begin(), v.end() );
				xxl_map xxlmap( v.begin(), v.end() );
				erase_iterator_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}

		MSG_TEST_FINISH
}

// This function generate a input where all nodes are halffull
// Thten the function erase_iterator_impl will be calling.
void erase_iterator_with_key_insert_sequentiel()
{
		MSG_TEST_START
		std_map stlmap;
		xxl_map xxlmap;
		typedef xxl_map::value_type value_type;

		int lower = LOWER ;
		int upper;

		while( lower <= UPPER )
		{
			upper = lower;

			while( upper <= UPPER )
			{
				statistic();
				STXXL_MSG( "***************************************************************" );
				STXXL_MSG( "ERASE ITERATOR: " << lower << " - " << upper << "(seqeuntiel insert)" );
				STXXL_MSG( "***************************************************************" );

				for( int i = LOWER; i < UPPER; i++ ) stlmap.insert(value_type( i,2*i));
				for( int i = LOWER; i < UPPER; i++ ) xxlmap.insert(value_type( i,2*i));

				erase_iterator_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

// This function generate a random input.
// Thten the function erase_iterator_impl will be calling.
void erase_iterator_with_key_insert_random()
{
		MSG_TEST_START
		std_map stlmap;
		xxl_map xxlmap;
		typedef stxxl::VECTOR_GENERATOR<my_type>::result vector_type;
		vector_type v( UPPER - LOWER );
		for( unsigned j=0; j < UPPER - LOWER; j++)
		{
			v[j].first = LOWER +j;
			v[j].second = 2*v[j].first;
		}

		random_number32 rnd;
		stxxl::ran32State = 0xdeadbeef;
		int lower = LOWER ;
		int upper;

		while( lower <= UPPER )
		{
			upper = lower;

			while( upper <= UPPER )
			{
				for( int ix = 0; ix < UPPER- LOWER; ix ++ )
				{
					int ix1 = rnd() % (UPPER- LOWER);
					int ix2 = rnd() % (UPPER- LOWER);
					my_type help = v[ix1];
					v[ix1] = v[ix2];
					v[ix2] = help;
				}

				statistic();
				STXXL_MSG( "***********************************************************" );
				STXXL_MSG( "ERASE ITERATOR: " << lower << " - " << upper << "(random insert)" );
				STXXL_MSG( "***********************************************************" );

				for( int i = 0; i < UPPER - LOWER; i++ ) stlmap.insert(v[i]);
				for( int i = 0; i < UPPER - LOWER; i++ ) xxlmap.insert(v[i]);

				erase_iterator_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

// This function generate a input where all nodes are full
// Thten the function erase_range_impl will be calling.
void erase_range_with_bulk_insert()
{
		MSG_TEST_START
		typedef stxxl::VECTOR_GENERATOR<my_type>::result vector_type;
		vector_type v( UPPER - LOWER );
		for( unsigned j=0; j < UPPER - LOWER; j++)
		{
			v[j].first = LOWER +j;
			v[j].second = 2*v[j].first;
		}

		int lower = LOWER ;
		int upper;

		while( lower <= UPPER )
		{
			upper = lower;

			while( upper <= UPPER )
			{
				statistic();
				STXXL_MSG( "***************************************************************" );
				STXXL_MSG( "ERASE INTERVAL: " << lower << " - " << upper << " (bulk insert)" );
				STXXL_MSG( "***************************************************************" );

				std_map stlmap( v.begin(), v.end() );
				xxl_map xxlmap( v.begin(), v.end() );
				erase_range_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

// This function generate a input where all nodes are halffull
// Thten the function erase_range_impl will be calling.
void erase_range_with_key_insert_sequentiel()
{
		MSG_TEST_START
		std_map stlmap;
		xxl_map xxlmap;
		typedef xxl_map::value_type value_type;
		
		// --------------------------------------------------------------------------

		int lower = LOWER ;
		int upper;

		while ( lower <= UPPER )
		{
			upper = lower;

			while( upper <= UPPER )
			{
				statistic();
				STXXL_MSG( "********************************************************************" );
				STXXL_MSG( "ERASE INTERVAL: " << lower << " - " << upper << "(seqeuntiel insert)" );
				STXXL_MSG( "********************************************************************" );

				for( int i = LOWER; i < UPPER; i++ ) stlmap.insert(value_type( i,2*i));
				for( int i = LOWER; i < UPPER; i++ ) xxlmap.insert(value_type( i,2*i));
				erase_range_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

// This function generate a random input.
// Thten the function erase_range_impl will be calling.
void erase_range_with_key_insert_random()
{
		MSG_TEST_START
		std_map stlmap;
		xxl_map xxlmap;
		typedef stxxl::VECTOR_GENERATOR<my_type>::result vector_type;
		vector_type v( UPPER - LOWER );
		for( unsigned j=0; j < UPPER - LOWER; j++)
		{
			v[j].first = LOWER +j;
			v[j].second = 2*v[j].first;
		}

		random_number32 rnd;
		stxxl::ran32State = 0xdeadbeef;
			int lower = LOWER ;
		int upper;

		while( lower <= UPPER )
		{
			upper = lower;

			while( upper <= UPPER )
			{
				for( int ix = 0; ix < UPPER- LOWER; ix ++ )
				{
					int ix1 = rnd() % (UPPER- LOWER);
					int ix2 = rnd() % (UPPER- LOWER);
					my_type help = v[ix1];
					v[ix1] = v[ix2];
					v[ix2] = help;
				}

				statistic();
				STXXL_MSG( "************************************************************" );
				STXXL_MSG( "ERASE INTERVAL: " << lower << " - " << upper << "(random insert)" );
				STXXL_MSG( "************************************************************" );

				for( int i = 0; i < UPPER - LOWER; i++ ) stlmap.insert(v[i]);
				for( int i = 0; i < UPPER - LOWER; i++ ) xxlmap.insert(v[i]);
				erase_range_impl( stlmap, xxlmap, lower, upper );
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

//! \test Test for ease function for a key of containers map.
//! The test generte a map by diferent inserting strategies.
//! For delete a key erase function will be call.
void erase_key()
{
	erase_key_with_bulk_insert();
	erase_key_with_key_insert_sequentiel();
	erase_key_with_key_insert_random();
}

//! \test Test for ease function for an interval of containers map.
//! The test generte a map by diferent inserting strategies.
//! For delete an interval erase function will be call.
void erase_range()
{
	erase_range_with_bulk_insert();
	erase_range_with_key_insert_sequentiel();
	erase_range_with_key_insert_random();
}

//! \test Test for \c stxxl::map::ease function for an iterator of containers map.
//! The test generte a map by diferent inserting strategies.
//! For delete an entry erase function for an iterator will be call.
void erase_iterator()
{
	erase_iterator_with_bulk_insert();
	erase_iterator_with_key_insert_sequentiel();
	erase_iterator_with_key_insert_random();
}

int main()
{
	erase_key();
	assert( 0 );
	erase_range();
	erase_iterator();
	return 0;
}

