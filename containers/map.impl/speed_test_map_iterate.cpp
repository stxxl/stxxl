/***************************************************************************
                          speed_test_map_iterate.cpp  -  description
                             -------------------
    begin                : Mit Okt 13 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file speed_test_map_iterate.cpp
//! \brief File for testing iteration functions of stxxl::map and the test class stxxl::map_test::my_test.
/*!

*/

#include <mng/mng.h>
#include <algo/ksort.h>
#include <containers/vector>
#include "map_async.h"
#include "my_type.h"
#include "map_test.h"
#include "test_handlers.h"


//! \example containers/map/speed_test_map_iterate.cpp
//! This is an example of use of \c stxxl::map by \c stxxl::map_test for the insertion function.

	using namespace stxxl;
	using namespace stxxl::map_test;

/*! The iterate functions are tested here.\n
 This consequence of calls is applied to stxxl::map,
 with which the parameters are modified.
 Either the size for nodes is changed or the size of internal cache.
 */
	int main( int argc, char* argv[] )
	{
		STXXL_MSG( "BlkSize_\tCacheSize_\topertaion\t\tcounter\tdelay" );

		for( int test_count = 1; test_count < argc; test_count++ )
		{
			// *****************************************************************
			// The smallest size of the Nodes.
			// Nodess are removed directly from cache,
			// as soon as they are not any longer used.
			// *****************************************************************

			my_test<4096,0> test0;
			test0.test_iterator( atol( argv[test_count] ) );
			test0.test_const_iterator( atol( argv[test_count] ) );
			
			// *****************************************************************
			// The smallest size of the Nodes.
			// We keep at least 512 knots in the cache.
			// *****************************************************************

			my_test<4096,512> test512;
			test512.test_iterator( atol( argv[test_count] ) );
			test512.test_const_iterator( atol( argv[test_count] ) );

			// *****************************************************************
			// The size of the Nodes is 64 times biger as minimum.
			// We keep at least 512 knots in the cache.
			// *****************************************************************

			my_test<262144,0> test262144;
			test262144.test_iterator( atol( argv[test_count] ) );
			test262144.test_const_iterator( atol( argv[test_count] ) );
		}
	}
