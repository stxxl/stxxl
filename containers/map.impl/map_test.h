/***************************************************************************
                          map_test.h  -  description
                             -------------------
    begin                : Son Feb 20 2005
    copyright            : (C) 2005 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file map_test.h
//! \brief Implemets tests for \c stxxl::map container.
/*!
 Here the Performace the map implementation is tested.
 For this purpose different consequences of functions are called
 and the run times are measured.
 */

#ifndef __STXXL_MAP_TEST__H__
#define __STXXL_MAP_TEST__H__

#include "my_type.h"

namespace stxxl
{
namespace map_test
{

	//! \brief This class will be used to starts preformance tests.
	/*!
	This class makes us possible compares from running times,
	which result on change of the size of the knots and/or the size of the all Caches.
	*/
	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	class my_test
	{

	public:

		typedef stxxl::map< key_type, data_type, cmp2, BlkSize_, PrefCacheSize_ > map;
		typedef typename map::value_type value_type;
		typedef typename map::const_iterator const_iterator;
		typedef typename map::iterator iterator;

	private:

		void test_const_searching( const map& xxlmap, long count  );
		void test_searching( map& xxlmap, long count  );
		void test_iterator( map& xxlmap, long count  );
		void test_const_iterator( const map& xxlmap, long count  );

	public:

		// test for deleting
		//! test for deleting
		void test_erase( long count  );

		// test for deleting
		//! test for deleting
		void test_erase_iterator( long count  );

		// test for deleting
		//! test for deleting
		void test_bulk_erase( long count  );

		// test for searching
		//! test for searching
		void test_searching( long count  );

		// test for searching (const)
		//! test for searching (const)
		void test_const_searching( long count  );

		// test for iterate
		//! test for iterate
		void test_iterator( long count  );

		// test for iterate (const)
		//! test for iterate (const)
		void test_const_iterator( long count  );

		// test for map bulk constructor
		//! test for map bulk constructor
		void test_bulk_contruct( long count );

		// test for bulk insert constructor
		//! test for bulk insert constructor
		void test_bulk_insert( long count );

		// test for map::insert
		//! test for map::insert
		void test_insert( long count );
	};

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_bulk_erase( long count  )
	{
		// local variables
		double ts1;
		double ts2;
		const_iterator ci;

		// the bulk insertion will be called
		my_insert ii( 0, count );
		map xxlmap;

		ts1 = stxxl_timestamp();
		xxlmap.insert( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_constructor\t" << count << "\t" << ts2 - ts1 );

		// *****************************************************************************
		// erase
		// *****************************************************************************

		ts1 = stxxl_timestamp();
		xxlmap.erase( xxlmap.begin(), xxlmap.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_erase\t\t" << count << "\t" << ts2 - ts1 );
	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_erase( long count  )
	{
		// local variables
		double ts1;
		double ts2;
		const_iterator ci;

		// the bulk insertion will be called
		my_insert ii( 0, count );
		map xxlmap;

		ts1 = stxxl_timestamp();
		xxlmap.insert( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_constructor\t" << count << "\t" << ts2 - ts1 );

		// *****************************************************************************
		// erase
		// *****************************************************************************

		ts1 = stxxl_timestamp();
		for( int  i = 0; i < count; xxlmap.erase( i++ ) );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\terase\t\t\t" << count << "\t" << ts2 - ts1 );
	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_erase_iterator( long count  )
	{
		// local variables
		double ts1;
		double ts2;
		const_iterator ci;

		// the bulk insertion will be called
		my_insert ii( 0, count );
		map xxlmap;

		ts1 = stxxl_timestamp();
		xxlmap.insert( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_constructor\t" << count << "\t" << ts2 - ts1 );

		// *****************************************************************************
		// erase
		// *****************************************************************************

		ts1 = stxxl_timestamp();
		for( int  i = 0; i < count; xxlmap.erase( xxlmap.find(i++) ) );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\terase_iterator\t\t" << count << "\t" << ts2 - ts1 );
	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_const_searching( long count  )
	{
		// local variables
		double ts1;
		double ts2;
		const_iterator ci;

		// the bulk insertion will be called
		my_insert ii( 0, count );
		map xxlmap;

		ts1 = stxxl_timestamp();
		xxlmap.insert( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_constructor\t" << count << "\t" << ts2 - ts1 );

		// tests
		test_const_searching( xxlmap, count );
	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_const_searching( const map& xxlmap, long count  )
	{
		// local variables
		double ts1;
		double ts2;
		long i;

		// ***********************************************************************
		// find
		// ***********************************************************************

		i = 0;
		ts1 = stxxl_timestamp();
		for( ; i < count; i++ )
			if ( xxlmap.find( i ) == xxlmap.end() ) std::cerr << "error i=" << i ;
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tconst_find\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// upper_bound
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		for( i = 0; i < count - 1; i++ )
			if ( xxlmap.upper_bound( i ) == xxlmap.end() ) std::cerr << "error i=" << i ;
		if ( xxlmap.upper_bound( i ) != xxlmap.end() ) std::cerr << "error i=" << i ;
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tconst_upper_bound\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// lower_bound
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		for( i = 0; i < count; i++ )
			if ( xxlmap.lower_bound( i ) == xxlmap.end() ) std::cerr << "error i=" << i ;
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tconst_lower_bound\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// equal_range
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		for( i = 0; i < count-1; i++ )
		{
			std::pair<const_iterator,const_iterator> pair = xxlmap.equal_range( i );
			if ( pair.first == xxlmap.end() ) std::cerr << "error i=" << i ;
			if ( pair.second == xxlmap.end() ) std::cerr << "error i=" << i ;
		}
		std::pair<const_iterator,const_iterator> pair = xxlmap.equal_range( i );
		if ( pair.first == xxlmap.end() ) std::cerr << "error i=" << i ;
		if ( pair.second != xxlmap.end() ) std::cerr << "error i=" << i ;
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tconst_equal_range\t" << count << "\t" << ts2 - ts1 );

	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_searching( long count  )
	{
		// local variables
		double ts1;
		double ts2;
		const_iterator ci;

		// the bulk insertion will be called
		my_insert ii( 0, count );
		map xxlmap;

		ts1 = stxxl_timestamp();
		xxlmap.insert( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_constructor\t" << count << "\t" << ts2 - ts1 );

		// tests
		test_searching( xxlmap, count );
	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_searching( map& xxlmap, long count  )
	{
		// local variables
		double ts1;
		double ts2;
		long i;

		// ***********************************************************************
		// find
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		for( i = 0; i < count; i++ )
			if ( xxlmap.find( i ) == xxlmap.end() ) std::cerr << "error" ;
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tfind\t\t\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// upper_bound
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		for( i = 0; i < count - 1; i++ )
			if ( xxlmap.upper_bound( i ) == xxlmap.end() ) std::cerr << "error" ;
		if ( xxlmap.upper_bound( count - 1 ) != xxlmap.end() ) std::cerr << "error" ;
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tupper_bound\t\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// lower_bound
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		for( i = 0; i < count; i++ )
			if ( xxlmap.lower_bound( i ) == xxlmap.end() ) std::cerr << "error" ;
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tlower_bound\t\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// equal_range
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		for( i = 0; i < count-1; i++ )
		{
			std::pair<iterator,iterator> pair = xxlmap.equal_range( i );
			if ( pair.first == xxlmap.end() ) std::cerr << "error" ;
			if ( pair.second == xxlmap.end() ) std::cerr << "error" ;
		}	
		std::pair<iterator,iterator> pair = xxlmap.equal_range( i );
		if ( pair.first == xxlmap.end() ) std::cerr << "error" ;
		if ( pair.second != xxlmap.end() ) std::cerr << "error" ;
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\t\tequal_range\t" << count << "\t" << ts2 - ts1 );

	}


	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_const_iterator( long count  )
	{
		// local variables
		double ts1;
		double ts2;
		const_iterator ci;

		// the bulk insertion will be called
		my_insert ii( 0, count );
		map xxlmap;

		ts1 = stxxl_timestamp();
		xxlmap.insert( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_constructor\t" << count << "\t" << ts2 - ts1 );

		// tests
		test_const_iterator( xxlmap, count );
	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_const_iterator( const map& xxlmap, long count  )
	{
		// local variables
		double ts1;
		double ts2;
		const_iterator ci;
		const_iterator begin;

		// ***********************************************************************
		// iterator::operator++(int)
		// ***********************************************************************

		ci = begin = xxlmap.begin();
		ts1 = stxxl_timestamp();
		for( ; ci != xxlmap.end(); ci++ );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tconst_iterator++\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// iterator::operator++()
		// ***********************************************************************

		ci = begin = xxlmap.begin();
		ts1 = stxxl_timestamp();
		for( ; ci != xxlmap.end(); ++ci );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\t++const_iterator\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// iterator::operator--(int)
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		ci = xxlmap.find( count - 1 );
		for( ; ci != begin; ci-- );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tconst_iterator--\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// iterator::operator--()
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		ci = xxlmap.find( count - 1 );
		for( ; ci != begin; --ci );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\t--const_iterator\t" << count << "\t" << ts2 - ts1 );

	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_iterator( long count  )
	{
		// local variables
		double ts1;
		double ts2;
		iterator ci;
		iterator begin;

		// the bulk insertion will be called
		my_insert ii( 0, count );
		map xxlmap;

		ts1 = stxxl_timestamp();
		xxlmap.insert( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_constructor\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// iterator::operator++(int)
		// ***********************************************************************

		ci = begin = xxlmap.begin();
		ts1 = stxxl_timestamp();
		for( ; ci != xxlmap.end(); ci++ );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tconst_iterator++\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// iterator::operator++()
		// ***********************************************************************

		ci = begin = xxlmap.begin();
		ts1 = stxxl_timestamp();
		for( ; ci != xxlmap.end(); ++ci );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\t++const_iterator\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// iterator::operator--(int)
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		ci = xxlmap.find( count - 1 );
		for( ; ci != begin; ci-- );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tconst_iterator--\t" << count << "\t" << ts2 - ts1 );

		// ***********************************************************************
		// iterator::operator--()
		// ***********************************************************************

		ts1 = stxxl_timestamp();
		ci = xxlmap.find( count - 1 );
		for( ; ci != begin; --ci );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\t--const_iterator\t" << count << "\t" << ts2 - ts1 );

	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_bulk_contruct( long count )
	{
		// locale variables
		double ts1;
		double ts2;

		// the bulk constructor will be called
		my_insert ii( 0, count );
		ts1 = stxxl_timestamp();
		map xxlmap( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_contruct\t" << count << "\t" << ts2 - ts1 );
	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_bulk_insert( long count )
	{
		// locale variables
		double ts1;
		double ts2;

		// the bulk insertion will be called
		my_insert ii( 0, count );
		map xxlmap;

		ts1 = stxxl_timestamp();
		xxlmap.insert( ii.begin(), ii.end() );
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\tbulk_insert\t" << count << "\t" << ts2 - ts1 );
	}

	template<unsigned BlkSize_, unsigned PrefCacheSize_>
	void my_test<BlkSize_,PrefCacheSize_>::test_insert( long count )
	{
		// locale variables
		double ts1;
		double ts2;

		// the insertion will be called
		map xxlmap;
		ts1 = stxxl_timestamp();
		for( int i = 0; i < count; i++ )
		{
			xxlmap.insert( my_type(i,i) );
		}
		ts2 = stxxl_timestamp();

		// print results
		STXXL_MSG( BlkSize_ << "\t" << PrefCacheSize_ << "\t\tinsert\t" << count << "\t" << ts2 - ts1 );
	}


}// namespace map_test

}// namespace stxxl

#endif // __STXXL_MAP_TEST__H__

