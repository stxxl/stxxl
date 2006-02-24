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
						
typedef stxxl::btree::btree<int,double,comp_type,2,3,stxxl::SR> btree_type;

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
	
	btree_type BTree1(1024*16,1024*16);
		
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
	
	
	int f = BTree1.erase(5);
	assert(f==1);
	f = BTree1.erase(6);
	assert(f==0);
	f = BTree1.erase(5);
	assert(f==0);
	
	assert(BTree1.count(10) == 1);
	assert(BTree1.count(5) == 0);
	
	it = BTree1.insert(BTree1.begin(),std::pair<int,double>(7,70.));
	assert(it->second == 70.);
	assert(BTree1.size() == 2);
	it = BTree1.insert(BTree1.begin(),std::pair<int,double>(10,300.));
	assert(it->second == 200.);
	assert(BTree1.size() == 2);
	
	it = BTree1.lower_bound(6);
	assert(it != BTree1.end());
	assert(it->first == 7);
	
	it = BTree1.lower_bound(7);
	assert(it != BTree1.end());
	assert(it->first == 7);
	
	it = BTree1.lower_bound(8);
	assert(it != BTree1.end());
	assert(it->first == 10);
	
	it = BTree1.lower_bound(11);
	assert(it == BTree1.end());
	
	BTree1.erase(it);
	assert(BTree1.size() == 1);
	
	BTree1.clear();
	assert(BTree1.size() == 0);
	
	for(int i= 0;i<nins/2;++i)
	{
		BTree1[rnd()%nins] = 10.1;
	}
	STXXL_MSG("Size of map: "<<BTree1.size())

	
	BTree1.clear();
	
	for(int i= 0;i<nins/2;++i)
	{
		BTree1[rnd()%nins] = 10.1;
	}
	
	STXXL_MSG("Size of map: "<<BTree1.size())
	
	btree_type BTree2(comp_type(),1024*16,1024*16);
	
	STXXL_MSG("Constuction of BTree3 from BTree1 that has "<< BTree1.size()<<" elements")
	btree_type BTree3(BTree1.begin(),BTree1.end(),comp_type(),1024*16,1024*16);
	
	assert(BTree3 == BTree1);
	
	STXXL_MSG("Bulk constuction of BTree4 from BTree1 that has "<< BTree1.size()<<" elements")
	btree_type BTree4(BTree1.begin(),BTree1.end(),comp_type(),1024*16,1024*16,true);
	
	STXXL_MSG("Size of BTree1: "<<BTree1.size())
	STXXL_MSG("Size of BTree4: "<<BTree4.size())
	
	assert(BTree4 == BTree1);
	assert(BTree3 == BTree4);
	
	BTree4.begin()->second = 0;
	assert(BTree3 != BTree4);
	assert(BTree4 < BTree3);
	
	BTree4.begin()->second = 1000;
	assert(BTree4 > BTree3);
	
	assert(BTree3 != BTree4);
	
	it = BTree4.begin();
	++it;
	STXXL_MSG("Size of Btree4 before erase: "<<BTree4.size());
	BTree4.erase(it,BTree4.end());
	STXXL_MSG("Size of Btree4 after erase: "<<BTree4.size());
	assert(BTree4.size()==1);
	
	
	STXXL_MSG("Size of Btree1 before erase: "<<BTree1.size());
	BTree1.erase(BTree1.begin(),BTree1.end());
	STXXL_MSG("Size of Btree1 after erase: "<<BTree1.size());
	assert(BTree1.empty());
	
	
	return 0;
}
