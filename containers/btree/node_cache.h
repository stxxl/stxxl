/***************************************************************************
 *            node_cache.h
 *
 *  Tue Feb 14 20:30:15 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _NODE_CACHE_H
#define _NODE_CACHE_H

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	template <class NodeType>
	class node_cache
	{
		public:
			typedef NodeType node_type;
			typedef typename node_type::bid_type bid_type;
	};
	
}

__STXXL_END_NAMESPACE

#endif /* _NODE_CACHE_H */
