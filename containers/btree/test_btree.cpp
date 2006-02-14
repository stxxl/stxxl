/***************************************************************************
 *            test_btree.cpp
 *
 *  Tue Feb 14 20:39:53 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#include "btree.h"
						
typedef stxxl::btree::btree<int,double,std::less<int>,5,8> btree_type;

int main()
{
	btree_type BTree1;
	
	return 0;
}
