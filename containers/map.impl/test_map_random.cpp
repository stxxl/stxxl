/***************************************************************************
                          test_map_random.cpp  -  description
                             -------------------
    begin                : Mon Feb 14 2005
    copyright            : (C) 2005 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file test_map_random.cpp
//! \brief File for testing functionality of stxxl::map.

#include <mng/mng.h>
#include <algo/ksort.h>
#include "vector"
#include "map_async.h"
#include "my_type.h"
#include "test_handlers.h"


//! \example containers/map/test_map_random.cpp
//! This is an example of use of \c stxxl::map container.

using namespace stxxl;
using namespace stxxl::map_test;

typedef std::map<key_type,data_type,cmp2>							std_map_type;
typedef stxxl::map<key_type,data_type,cmp2,4096,2 >		xxl_map_type;

#define PERCENT_CLEAR 1
#define PERCENT_ERASE_BULK 9
#define PERCENT_ERASE_KEY 90
#define PERCENT_ERASE_ITERATOR 100
#define PERCENT_INSERT_PAIR 100
#define PERCENT_INSERT_BULK 100
#define PERCENT_SIZING 100
#define PERCENT_LOWER 100
#define PERCENT_UPPER 200
#define PERCENT_FIND 100
#define PERCENT_ITERATOR 100


#define MAX_KEY 1000

void statistic()
{
	typedef stxxl::btnode<key_type,data_type,cmp2, 4096, true,2 >     leaf_type;
	typedef stxxl::btnode<key_type,BID< 4096 >,cmp2, 4096,true,2 >		node_type;
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

//#define MAX_STEP 0x0001000
int main( int argc, char* argv[] )
{

	typedef std::vector<my_type> vector_type;

	stxxl::random_number32 rnd;
	stxxl::ran32State = 0xdeadbeef;
	int a;
	STXXL_ASSERT(
			a = (PERCENT_CLEAR +
			PERCENT_SIZING +
			PERCENT_ERASE_BULK +
			PERCENT_ERASE_KEY +
			PERCENT_ERASE_ITERATOR +
			PERCENT_INSERT_PAIR +
			PERCENT_INSERT_BULK +
			PERCENT_LOWER +
			PERCENT_UPPER +
			PERCENT_FIND +
			PERCENT_ITERATOR) == 1000 );

  stxxl::uint64 MAX_STEP = atoi( argv[1] );
	std_map_type stdmap;
	xxl_map_type xxlmap;

	for( stxxl::uint64  i = 0; i < MAX_STEP; i++ )
	{

		// ***************************************************
		// A random nummber is createted to determine how kind
		// of operation we will call.
		// ***************************************************

		long step = rnd() % 1000;
		int percent = 0;

		if(  i % (MAX_STEP/1000) == 0 )
		{
			STXXL_MSG( "*****************************************************" );
			STXXL_MSG( "Step=" << i << " (" << (unsigned) stdmap.size() << ")" );
			statistic();
		}

		// *********************************************************
		// The clear function will be called
		// *********************************************************
		if( step < (percent += PERCENT_CLEAR) )
		{
			if( (unsigned) rand() % 1000 < stdmap.size() )
			{
				stdmap.clear();
				xxlmap.clear();

				assert( stdmap.empty() );
				assert( xxlmap.empty() );
			}
		}

		// *********************************************************
		// The size function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_SIZING) )
		{
			std_map_type::size_type size1 = stdmap.size();
			xxl_map_type::size_type size2 = xxlmap.size();

			assert( size1 == size2 );

			/* Ignore the test: not important
			std_map_type::size_type maxsize1 = stdmap.max_size();
			xxl_map_type::size_type maxsize2 = xxlmap.max_size();

			assert( maxsize1 == maxsize2 );
			*/
		}

		// *********************************************************
		// The range erase function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_ERASE_BULK) )
		{
			key_type key1 = rand() % MAX_KEY;
			key_type key2 = rand() % MAX_KEY;

			if( key1 > key2 )
			{
				key_type h = key1 ;
				key1 = key2;
				key2 = h;
			}

			stdmap.erase( stdmap.lower_bound(key1), stdmap.upper_bound(key2) );
			xxlmap.erase( xxlmap.lower_bound(key1), xxlmap.upper_bound(key2) );

			assert( stdmap.lower_bound( key1 ) == stdmap.end() || stdmap.lower_bound( key1 ) == stdmap.upper_bound(key2) );
			assert( xxlmap.lower_bound( key1 ) == xxlmap.end() || xxlmap.lower_bound( key1 ) == xxlmap.upper_bound(key2) );
		}

		// *********************************************************
		// The erase a key function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_ERASE_KEY) )
		{
			key_type key = rnd() % MAX_KEY;
			stdmap.erase( key );
			xxlmap.erase( key );

			assert( not_there( stdmap, key ));
			assert( not_there( xxlmap, key ));
		}

		// *********************************************************
		// The erase function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_ERASE_ITERATOR) )
		{
			key_type key = rnd() % MAX_KEY;

			std_map_type::iterator stditer = stdmap.find( key );
			xxl_map_type::iterator xxliter = xxlmap.find( key );

			if( stditer != stdmap.end() ) stdmap.erase( stditer );
			if( xxliter != xxlmap.end() ) xxlmap.erase( xxliter );

			assert( not_there( stdmap, key ));
			assert( not_there( xxlmap, key ));
		}

		// *********************************************************
		// The insert function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_INSERT_PAIR) )
		{
			key_type key = rnd() % MAX_KEY;
			stdmap.insert( std::pair<key_type,data_type>( key, 2*key ) );
			xxlmap.insert( std::pair<key_type,data_type>( key, 2*key ) );

			assert( there( stdmap, key, 2*key ));
			assert( there( xxlmap, key, 2*key ));
		}

		// *********************************************************
		// The bulk insert function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_INSERT_BULK) )
		{
			unsigned lower = rnd() % MAX_KEY;
			unsigned upper = rnd() % MAX_KEY;
			if( lower > upper )
			{
				unsigned h = upper;
				upper = lower ;
				lower = h;
			}
			vector_type v2( upper-lower );
			for( unsigned j = 0; j < (unsigned)(upper - lower); j++)
			{
				v2[j].first = lower + j;
				v2[j].second = 2*v2[j].first;
			}
   
			stdmap.insert( v2.begin(), v2.end() );
			xxlmap.insert( v2.begin(), v2.end() );

			for( unsigned i = lower; i < upper; i++ ) assert( there( stdmap, i, 2*i ));
			for( unsigned i = lower; i < upper; i++ ) assert( there( xxlmap, i, 2*i ));
		}

		// *********************************************************
		// The lower_bound function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_LOWER) )
		{
			key_type key1 = rand() % MAX_KEY;
			key_type key2 = rand() % MAX_KEY;
			if( key1 > key2 )
			{
				key_type h = key1 ;
				key1 = key2;
				key2 = h;
			}

			while( key1 < key2 )
			{
				std_map_type::iterator stditer = stdmap.lower_bound(key1);
				xxl_map_type::iterator xxliter = xxlmap.lower_bound(key1);

				assert(
					is_same( *(stditer), *(xxliter) ) ||
					is_end(stdmap,stditer) && is_end(xxlmap,xxliter)
				);
				
				key1++;
			}
		}

		// *********************************************************
		// The upper_bound function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_UPPER) )
		{
			key_type key1 = rand() % MAX_KEY;
			key_type key2 = rand() % MAX_KEY;
			if( key1 > key2 )
			{
				key_type h = key1 ;
				key1 = key2;
				key2 = h;
			}

			while( key1 < key2 )
			{
				std_map_type::iterator stditer = stdmap.upper_bound(key1);
				xxl_map_type::iterator xxliter = xxlmap.upper_bound(key1);

				assert(
					is_same( *(stditer), *(xxliter) ) ||
					is_end(stdmap,stditer) && is_end(xxlmap,xxliter)
				);

				key1++;
			}
		}

		// *********************************************************
		// The find function will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_FIND) )
		{
			key_type key1 = rand() % MAX_KEY;
			key_type key2 = rand() % MAX_KEY;
			if( key1 > key2 )
			{
				key_type h = key1 ;
				key1 = key2;
				key2 = h;
			}

			while( key1 < key2 )
			{
				std_map_type::iterator stditer = stdmap.find(key1);
				xxl_map_type::iterator xxliter = xxlmap.find(key1);

				assert(
					is_same( *(stditer), *(xxliter) ) ||
					is_end(stdmap,stditer) && is_end(xxlmap,xxliter)
				);

				key1++;
			}
		}

		// *********************************************************
		// The iterate functions will be called
		// *********************************************************
		else if ( step < (percent += PERCENT_ITERATOR) )
		{
			std_map_type::const_iterator siter1 = stdmap.begin();
			xxl_map_type::const_iterator xiter1 = xxlmap.begin();

			std_map_type::const_iterator siter2 = siter1;
			xxl_map_type::const_iterator xiter2 = xiter1;

			while( siter1 != stdmap.end() )
			{
				assert( !(xiter1 == xxlmap.end()) );
				assert( is_same( *(siter1++), *(xiter1++) ) );
				assert( !is_same( *siter1, *siter2 ) || siter1 == stdmap.end() );
				assert( !is_same( *xiter1, *xiter2 ) || xiter1 == xxlmap.end() );
			}
			assert( xiter1 == xxlmap.end() );
			assert( siter2 == stdmap.begin() );
			assert( xiter2 == xxlmap.begin() );

		}
	}
	return 0;
}
