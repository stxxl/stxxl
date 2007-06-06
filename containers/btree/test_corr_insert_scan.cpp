/***************************************************************************
 *            test_corr_insert_scan.cpp
 *
 *  Tue Feb 21 11:52:14 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/


#include <iostream>

#include "btree.h"


struct comp_type : public std::less<int>
{
    static int max_value()
    {
        return std::numeric_limits<int>::max();
    }
    static int min_value()
    {
        return std::numeric_limits<int>::min();
    }
};

typedef stxxl::btree::btree < int, double, comp_type, 4096, 4096, stxxl::SR > btree_type;
//typedef stxxl::btree::btree<int,double,comp_type,10,11,stxxl::SR> btree_type;

std::ostream & operator << (std::ostream & o, const std::pair<int, double> & obj)
{
    o << obj.first << " " << obj.second;
    return o;
}


struct rnd_gen
{
    stxxl::random_number32 rnd;
    int operator ()  ()
    {
        return (rnd() >> 2);
    }
};

bool operator == (const std::pair<int, double> & a, const std::pair<int, double> & b)
{
    return a.first == b.first;
}

int main(int argc, char * argv [])
{
    if (argc < 2)
    {
        STXXL_MSG("Usage: " << argv[0] << " #log_ins")
        return 1;
    }

    btree_type BTree(1024 * 128, 1024 * 128);

    const stxxl::uint64 nins = (1ULL << (unsigned long long)atoi(argv[1]));

    stxxl::ran32State = time(NULL);

    stxxl::vector<int> Values(nins);
    STXXL_MSG("Generating " << nins << " random values")
    stxxl::generate(Values.begin(), Values.end(), rnd_gen(), 4);

    stxxl::vector<int>::const_iterator it = Values.begin();
    STXXL_MSG("Inserting " << nins << " random values into btree")
    for ( ; it != Values.end(); ++it)
        BTree.insert(std::pair < int, double > (*it, double (*it) + 1.0));


    STXXL_MSG("Sorting the random values")
    stxxl::sort(Values.begin(), Values.end(), comp_type(), 128 * 1024 * 1024);

    STXXL_MSG("Deleting unique values")
    stxxl::vector<int>::iterator NewEnd = std::unique(Values.begin(), Values.end());
    Values.resize(NewEnd - Values.begin());

    assert(BTree.size() == Values.size());
    STXXL_MSG("Size without duplicates: " << Values.size())

    STXXL_MSG("Comparing content")

    stxxl::vector<int>::const_iterator vIt = Values.begin();
    btree_type::iterator bIt = BTree.begin();

    for ( ; vIt != Values.end(); ++vIt, ++bIt)
    {
        assert(*vIt == bIt->first);
        assert(double (bIt->first) + 1.0 == bIt->second);
        assert(bIt != BTree.end());
    }

    assert(bIt == BTree.end());

    STXXL_MSG("Test passed.")

    return 0;
}
