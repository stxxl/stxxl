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
	if(argc <2)
	{
		STXXL_MSG("Usage: "<<argv[0]<<" #ins")	
		return 1;
	}
	
	btree_type BTree1(1024*16*4,1024*16*8);
		
	const unsigned nins = atoi(argv[1]);
	
	stxxl::random_number32 rnd;
	
	// .begin() .end() test
	BTree1[10] = 100.;
	btree_type::iterator begin = BTree1.begin();
	btree_type::iterator end = BTree1.end();
	assert(begin == BTree1.begin());
	BTree1[5] = 50.;
	btree_type::iterator nbegin = BTree1.begin();
	btree_type::iterator nend = BTree1.end();
	assert(nbegin == BTree1.begin());
	assert(begin!=nbegin);
	assert(end == nend);
	assert(begin != end);
	assert(nbegin != end);
	assert(begin->first == 10);
	assert(begin->second == 100);
	assert(nbegin->first == 5);
	assert(nbegin->second == 50);
	
	BTree1[10] = 200.;
	assert(begin->second == 200.);

	btree_type::iterator it = BTree1.find(5);
	assert(it != BTree1.end());
	assert(it->first == 5);
	assert(it->second == 50.);
	it = BTree1.find(6);
	assert(it==BTree1.end());
	it = BTree1.find(1000);
	assert(it==BTree1.end());
	
	
	bool f = BTree1.erase(10);
	assert(f);
	f = BTree1.erase(5);
	assert(f);
	f = BTree1.erase(6);
	assert(!f);
	f = BTree1.erase(5);
	assert(!f);
	
	for(int i= 0;i<nins;++i)
	{
		//btree_type::iterator i1 = BTree1.insert(std::pair<int,double>(rnd(nins),10.1)).first;
		//btree_type::iterator i1 = BTree1.insert(std::pair<int,double>(rnd()%nins,10.1)).first;
		BTree1[rnd()%nins] = 10.1;
		//STXXL_MSG(*i1)
		//btree_type::iterator i2 = BTree1.insert(std::pair<int,double>(20,20.1)).first;
		//STXXL_MSG(*i2)
		//btree_type::iterator i3 = BTree1.insert(std::pair<int,double>(5,5.1)).first;
		//STXXL_MSG(*i3	)
	}
	STXXL_MSG("Size of map: "<<BTree1.size())

	
	return 0;
}
