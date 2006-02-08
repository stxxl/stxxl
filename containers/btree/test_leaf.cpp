/***************************************************************************
 *            test_leaf.cpp
 *
 *  Mon Feb  6 18:20:22 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#include "leaf.h"


using namespace stxxl::btree;


typedef normal_leaf<int,int,18,std::less<int> > myleaf_type;
typedef myleaf_type::value_type pair_type;

int main()
{
	std::vector<pair_type> Input;
	//int A[] = { 1, 2, 3, 3, 3, 5, 8 };
	Input.push_back(pair_type(1,10) );
	Input.push_back(pair_type(2,20) );
	Input.push_back(pair_type(3,30) );
	Input.push_back(pair_type(5,50) );
	Input.push_back(pair_type(6,60) );
	Input.push_back(pair_type(8,80) );
	
	myleaf_type Leaf(Input.begin(),Input.end(),1,200,std::less<int>());
	
	for(int i = 0;i<10;++i)
	STXXL_MSG(i<<" pos:"<<Leaf.find(i) - Leaf.begin()<< " value "<< (*(Leaf.find(i))).first
		<<","<<(*(Leaf.find(i))).second)
	
	
	
	return 1;
}
