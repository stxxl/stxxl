/***************************************************************************
 *            btree.h
 *
 *  Tue Feb 14 19:02:35 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _BTREE_H
#define _BTREE_H

#include <stxxl>

#include "iterator.h"
#include "iterator_map.h"
#include "leaf.h"
#include "node_cache.h"
#include "root_node.h"
#include "splitter.h"
#include "node.h"

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	template <	class KeyType, 
						class DataType, 
						class CompareType, 
						unsigned LogNodeSize,
						unsigned LogLeafSize
					>
	class btree
	{
		public:
			typedef KeyType key_type;
			typedef DataType	data_type;
			typedef CompareType key_compare;
			enum {
				log_node_size = LogNodeSize,
				log_leaf_size = LogLeafSize
			};
		typedef btree<KeyType,DataType,CompareType,LogNodeSize,LogLeafSize> SelfType;
			
		// leaf type declarations
		typedef normal_leaf<key_type,data_type,key_compare,log_leaf_size,SelfType> leaf_type;
		typedef typename leaf_type::bid_type leaf_bid_type;
		typedef node_cache<leaf_type> leaf_cache_type;
		// iterator types
		typedef btree_iterator<leaf_bid_type> iterator;
		typedef btree_const_iterator<leaf_bid_type> const_iterator;
		// iterator map type
		typedef iterator_map<leaf_bid_type> iterator_map_type;
		// node type declarations
		typedef normal_node<key_type,key_compare,log_node_size,SelfType> node_type;
		typedef typename node_type::bid_type node_bid_type;
		typedef node_cache<node_type> node_cache_type;
			
			
	};
	
}

__STXXL_END_NAMESPACE

#endif /* _BTREE_H */
