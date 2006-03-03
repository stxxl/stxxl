/***************************************************************************
 *            test_btree.cpp
 *  A very basic test
 *  Tue Feb 14 20:39:53 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#include <iostream>

#include "btree.h"


struct comp_type : public std::less<int>
{
	static int max_value() { return (std::numeric_limits<int>::max)(); }
};
						
typedef stxxl::btree::btree<int,double,comp_type,4096,4096,stxxl::SR> btree_type;

std::ostream & operator << (std::ostream & o, const std::pair<int,double> & obj)
{
	o << obj.first <<" "<<obj.second ;
	return o;
}

#define node_cache_size (25*1024*1024)
#define leaf_cache_size (1*1024*1024)

void NC(btree_type & BTree)
{
	double sum = 0;
	stxxl::timer Timer1;
	Timer1.start();
	btree_type::iterator it = BTree.begin();
	btree_type::iterator end = BTree.end();
	for(;it!=end;++it)
		sum += it->second;
	Timer1.stop();
	STXXL_MSG("Scanning with non const iterator: "<<Timer1.mseconds()<<" msec")
}

void C(btree_type & BTree)
{
	double sum = 0;
	stxxl::timer Timer1;
	Timer1.start();
	btree_type::const_iterator it = BTree.begin();
	btree_type::const_iterator end = BTree.end();
	for(;it!=end;++it)
		sum += it->second;
	Timer1.stop();
	STXXL_MSG("Scanning with const iterator: "<<Timer1.mseconds()<<" msec")
}

int main(int argc, char * argv [])
{
	
	if(argc <2)
	{
		STXXL_MSG("Usage: "<<argv[0]<<" #ins")	
		return 1;
	}
		
	const unsigned nins = atoi(argv[1]);
	
	stxxl::random_number32 rnd;
	
	std::vector<std::pair<int,double> > Data(nins);
		
	for(int i = 0;i<nins;++i)
	{
		Data[i].first = i;
	}
	
	btree_type BTree1(Data.begin(),Data.end(),comp_type(),node_cache_size,leaf_cache_size,true);
	btree_type BTree2(Data.begin(),Data.end(),comp_type(),node_cache_size,leaf_cache_size,true);
	
	
	STXXL_MSG(*stxxl::stats::get_instance());
	
	C(BTree1);
	
	
	
	STXXL_MSG(*stxxl::stats::get_instance());
	
	NC(BTree2);
	
	STXXL_MSG(*stxxl::stats::get_instance());
	
	STXXL_MSG("All tests passed successufully")
	
	return 0;
}
