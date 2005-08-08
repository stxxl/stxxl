/***************************************************************************
                          test_map_insert.cpp  -  description
                             -------------------
    begin                : Mit Okt 13 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file test_map_insert.cpp
//! \brief File for testing insert functions of stxxl::map.

#include <mng/mng.h>
#include <algo/ksort.h>
#include <containers/vector>
#include "map_async.h"
#include "my_type.h"
#include "test_handlers.h"


//! \example containers/map/test_map_insert.cpp
//! This is an example of use of \c stxxl::map.
using namespace stxxl;
using namespace stxxl::map_test;

typedef std::map<key_type,data_type,cmp2>							std_map;
typedef stxxl::map<key_type,data_type,cmp2,4096,0 >		xxl_map;

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


#define LOWER 3
#define UPPER 100

void statistic()
{
	typedef stxxl::btnode<key_type,data_type,cmp2, 4096, true,0 >     leaf_type;
	typedef stxxl::btnode<key_type,BID< 4096 >,cmp2, 4096,true,0 >		node_type;
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


void insert_pair()
{
		MSG_TEST_START

		typedef xxl_map::value_type value_type;

		std_map stlmap;
		xxl_map xxlmap;

		assert(is_empty(stlmap));
		assert(is_empty(xxlmap));

		int lower = LOWER ;
		int upper;

		while( lower <= UPPER )
		{
			upper = lower;

			while( upper <= UPPER )
			{
				statistic();
				STXXL_MSG( "*********************************************" );
				STXXL_MSG( "INSERT PAIR: " << lower << " - " << upper );
				STXXL_MSG( "*********************************************" );

				for( int i = lower; i < upper; i++ ) stlmap.insert(value_type( i,2*i));
				for( int i = lower; i < upper; i++ ) xxlmap.insert(value_type( i,2*i));

				assert( lower != upper && !is_empty(stlmap) || lower == upper && is_empty(stlmap) );
				assert( lower != upper && !is_empty(xxlmap) || lower == upper && is_empty(xxlmap) );

				assert(is_size(stlmap,upper-lower));
				assert(is_size(xxlmap,upper-lower));

				for( int i = LOWER; i < lower; i++ ) assert( not_there( stlmap, i ));
				for( int i = LOWER; i < lower; i++ ) assert( not_there( xxlmap, i ));

				for( int i = lower; i < upper; i++ ) assert( there( stlmap, i, 2*i ));
				for( int i = lower; i < upper; i++ ) assert( there( xxlmap, i, 2*i ));

				for( int i = upper; i < UPPER; i++ ) assert( not_there( stlmap, i ));
				for( int i = upper; i < UPPER; i++ ) assert( not_there( xxlmap, i ));

				stlmap.clear();
				xxlmap.clear();

				assert(is_empty(stlmap));
				assert(is_empty(xxlmap));
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

void insert_interval()
{
		MSG_TEST_START

		typedef xxl_map::value_type value_type;
		typedef std::vector<my_type> vector_type;
		vector_type v( UPPER - LOWER );

		for( unsigned j=0; j < UPPER - LOWER; j++)
		{
			v[j].first =  LOWER + j;
			v[j].second = 2 * v[j].first;
		}

		std_map stlmap( v.begin(), v.end() );
		xxl_map xxlmap( v.begin(), v.end() );

		int lower = LOWER ;
		int upper;

		while( lower <= UPPER )
		{
			upper = lower;
			while( upper <= UPPER )
			{
				vector_type v2( upper - lower);
				for( unsigned j = 0; j < (unsigned)(upper - lower); j++)
				{
					v2[j].first = lower + j;
					v2[j].second = 2*v2[j].first;
				}

				statistic();
				STXXL_MSG( "*********************************************" );
				STXXL_MSG( "INSERT BULK: " << lower << " - " << upper );
				STXXL_MSG( "*********************************************" );

				stlmap.erase( stlmap.lower_bound( lower ), stlmap.lower_bound( upper ) );
				xxlmap.erase( xxlmap.lower_bound( lower ), xxlmap.lower_bound( upper ) );

				assert(is_size(stlmap,UPPER - LOWER - upper + lower));
				assert(is_size(xxlmap,UPPER - LOWER - upper + lower));

				stlmap.insert( v2.begin(), v2.end() );
				xxlmap.insert( v2.begin(), v2.end() );

				for( int i = LOWER; i < UPPER; i++ ) assert( there( stlmap, i, 2*i ));
				for( int i = LOWER; i < UPPER; i++ ) assert( there( xxlmap, i, 2*i ));

				assert(is_size(stlmap,UPPER-LOWER));
				assert(is_size(xxlmap,UPPER-LOWER));
				upper ++;
			}
			lower ++;
		}
		statistic();
		MSG_TEST_FINISH
}

int main()
{
	//insert_pair();
	insert_interval();
	return 0;
}

