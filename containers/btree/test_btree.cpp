/***************************************************************************
 *            test_btree.cpp
 *
 *  Tue Feb 14 20:39:53 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#include <iostream>

#include "btree.h"


struct comp_type : public std::less<int>
{
	static int max_value() { return std::numeric_limits<int>::max(); }
};
						
typedef stxxl::btree::btree<int,double,comp_type,2,3,stxxl::SR> btree_type;

std::ostream & operator << (std::ostream & o, const std::pair<int,double> & obj)
{
	o << obj.first <<" "<<obj.second ;
	return o;
}


int main()
{
	btree_type BTree1(1024*16,1024*16);
	

	{
	btree_type::iterator i1 = BTree1.insert(std::pair<int,double>(10,10.1)).first;
	STXXL_MSG(*i1)
	//btree_type::iterator i2 = BTree1.insert(std::pair<int,double>(20,20.1)).first;
	//STXXL_MSG(*i2)
	btree_type::iterator i3 = BTree1.insert(std::pair<int,double>(5,5.1)).first;
	STXXL_MSG(*i3	)
	}
	
	return 0;
}
