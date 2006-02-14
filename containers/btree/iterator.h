/***************************************************************************
 *            iterator.h
 *
 *  Tue Feb 14 19:11:00 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _BTREEITERATOR_H
#define _BTREEITERATOR_H


#include <stxxl>

__STXXL_BEGIN_NAMESPACE

namespace btree
{

	template <class BIDType>
	class iterator_map;
	
	template <class BIDType>
	class btree_iterator
	{
		public:
			typedef	BIDType bid_type;
			typedef iterator_map<bid_type> iterator_map_type;
	};
	
	template <class BIDType>
	class btree_const_iterator
	{
		public:
			typedef	BIDType bid_type;
			typedef iterator_map<bid_type> iterator_map_type;
	};
	
}

__STXXL_END_NAMESPACE

#endif /* _ITERATOR_H */
