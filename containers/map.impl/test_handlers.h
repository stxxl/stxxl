/***************************************************************************
                          test_handlers.h  -  description
                             -------------------
    begin                : Son Okt 24 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/


//! \file test_handlers.h
//! \brief This file contains helpfully functions for testimg of stxxl::map.

#ifndef _TEST_HANDLERS_H_
#define _TEST_HANDLERS_H_

using namespace stxxl;

// ***********************************************
// THERE
// ***********************************************

template<typename MAPTYPE>
bool there( MAPTYPE& map, const typename MAPTYPE::key_type& key, typename MAPTYPE::mapped_type data )
{
	if( !((*map.find(key)).second  == data) )
	{
		typename MAPTYPE::iterator iter = map.find(key);
		STXXL_VERBOSE2( "iter=(" << (*iter).first << ":" << (*iter).second << ")" );
		STXXL_VERBOSE2( "key=" << key );
		STXXL_VERBOSE2( "data=" << data );
		return false;
	}
	return true;
};

// ***********************************************
// IS EQUAL END
// ***********************************************

template<typename MAPTYPE>
bool is_equal_end( MAPTYPE& map, typename MAPTYPE::iterator& iter )
{
	return iter == map.end();
};

// ***********************************************
// IS SAME
// ***********************************************

template<typename value_type>
bool is_same( value_type& v1, value_type& v2 )
{
	return v1.first == v2.first && v1.second == v2.second;
}

template<typename value_type>
bool is_same( const value_type& v1, const value_type& v2 )
{
	return v1.first == v2.first && v1.second == v2.second;
}

// ***********************************************
// NOT THERE
// ***********************************************

template<typename MAPTYPE>
bool not_there( MAPTYPE& map, const typename MAPTYPE::key_type& key )
{
	return map.find( key ) == map.end();
};

// ***********************************************
// IS EMPTY
// ***********************************************

template<typename MAPTYPE>
bool is_empty( MAPTYPE& map )
{
	return map.empty();
};

// ***********************************************
// IS END
// ***********************************************

template<typename MAPTYPE>
bool is_end( MAPTYPE& map, typename MAPTYPE::iterator& iter )
{
	return iter == map.end();
}

template<typename MAPTYPE>
bool is_end( const MAPTYPE& map, typename MAPTYPE::const_iterator& iter )
{
	return iter == map.end();
}

// ***********************************************
// IS SIZE
// ***********************************************

template<typename MAPTYPE>
bool is_size( MAPTYPE& map, const typename MAPTYPE::size_type& size )
{
	return map.size() == size;
};


#endif
