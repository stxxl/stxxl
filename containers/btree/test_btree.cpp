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
						
typedef stxxl::btree::btree<int,double,comp_type,10,11,stxxl::SR> btree_type;

std::ostream & operator << (std::ostream & o, const std::pair<int,double> & obj)
{
	o << obj.first <<" "<<obj.second ;
	return o;
}


int main(int argc, char * argv [])
{
	btree_type BTree1(1024*16*4,1024*16*8);
	
	const unsigned nins = atoi(argv[1]);
	
	stxxl::random_number32 rnd;
	
	for(int i= 0;i<nins;++i)
	{
		//btree_type::iterator i1 = BTree1.insert(std::pair<int,double>(rnd(nins),10.1)).first;
		btree_type::iterator i1 = BTree1.insert(std::pair<int,double>(rnd()%nins,10.1)).first;
		//STXXL_MSG(*i1)
		//btree_type::iterator i2 = BTree1.insert(std::pair<int,double>(20,20.1)).first;
		//STXXL_MSG(*i2)
		//btree_type::iterator i3 = BTree1.insert(std::pair<int,double>(5,5.1)).first;
		//STXXL_MSG(*i3	)
	}
	STXXL_MSG("Size of map: "<<BTree1.size())

	
	return 0;
}
