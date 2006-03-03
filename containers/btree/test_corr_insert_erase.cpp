/***************************************************************************
 *            test_corr_insert_erase.cpp
 *
 *  Wed Feb 22 13:41:02 2006
 *  Copyright  2006  User
 *  Email
 ****************************************************************************/


#include <iostream>

#include "btree.h"


struct comp_type : public std::less<int>
{
	static int max_value() { return std::numeric_limits<int>::max(); }
	static int min_value() { return std::numeric_limits<int>::min(); }
};
						
typedef stxxl::btree::btree<int,double,comp_type,4096,4096,stxxl::SR> btree_type;
//typedef stxxl::btree::btree<int,double,comp_type,10,11,stxxl::SR> btree_type;

std::ostream & operator << (std::ostream & o, const std::pair<int,double> & obj)
{
	o << obj.first <<" "<<obj.second ;
	return o;
}


struct rnd_gen
{
	stxxl::random_number32 rnd;
	int operator () ()
	{
		return (rnd()>>2)*3;
	}
};

bool operator == (const std::pair<int,double> & a, const std::pair<int,double> & b)
{
	return a.first == b.first;
}

int main(int argc, char * argv [])
{
	if(argc <2)
	{
		STXXL_MSG("Usage: "<<argv[0]<<" #log_ins")	
		return 1;
	}
	
	btree_type BTree(1024*128,1024*128);
		
	const stxxl::uint64 nins = (1ULL<<(unsigned long long)atoi(argv[1]));
	
	stxxl::ran32State = time(NULL);

	
	stxxl::vector<int> Values(nins);
	STXXL_MSG("Generating "<<nins<<" random values")
	stxxl::generate(Values.begin(),Values.end(),rnd_gen(),4);
	
	stxxl::vector<int>::const_iterator it = Values.begin();
	STXXL_MSG("Inserting "<<nins<<" random values into btree")
	for(;it!=Values.end();++it)
		BTree.insert(std::pair<int,double>(*it,double(*it)+1.0));

	STXXL_MSG("Number of elements in btree: "<<BTree.size())
	
	STXXL_MSG("Searching "<<nins<<" existing elements and erasing them")
	stxxl::vector<int>::const_iterator vIt = Values.begin();
	
	for(;vIt != Values.end();++vIt)
	{
		btree_type::iterator bIt = BTree.find(*vIt);
		assert(bIt != BTree.end());
		bool f = BTree.erase((*vIt)+1); // erasing non-existent element
		assert(f==0);
		f = BTree.erase(*vIt); // erasing existing element
		assert(f==1);
		bIt = BTree.find(*vIt); // checking it is not there
		assert(bIt == BTree.end());
		f = BTree.erase(*vIt); // trying to erasi it again
		assert(f==0);
	}
	
	assert(BTree.empty());
	
	STXXL_MSG("Test passed.")
	
	return 0;
}
