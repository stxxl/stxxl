/***************************************************************************
 *            test_node_cache.cpp
 *
 *  Wed Feb 15 14:32:43 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#include "btree.h"

typedef stxxl::btree::btree<int,double,std::less<int>,4096,4096,stxxl::SR> btree_type;

int main()
{
	btree_type BTree1;
	typedef btree_type::node_cache_type node_cache_type;
	node_cache_type NodeCache(1024*1024*50,&BTree1,btree_type::key_compare());
	

	
	return 0;
}
