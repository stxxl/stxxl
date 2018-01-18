/***************************************************************************
 *  tests/containers/btree/test_btree_insert_find.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <ctime>
#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/containers/btree/btree.h>
#include <stxxl/scan>

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
using btree_type = stxxl::btree::btree<
          int, double, comp_type, 4096, 4096, foxxll::simple_random>;

std::ostream& operator << (std::ostream& o, const std::pair<int, double>& obj)
{
    o << obj.first << " " << obj.second;
    return o;
}

struct rnd_gen
{
    stxxl::random_number32 rnd;
    int operator () ()
    {
        return (rnd() >> 2) * 3;
    }
};

bool operator == (const std::pair<int, double>& a, const std::pair<int, double>& b)
{
    return a.first == b.first;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        LOG1 << "Usage: " << argv[0] << " #log_ins";
        return -1;
    }

    const int log_nins = atoi(argv[1]);
    if (log_nins > 31) {
        LOG1 << "This test can't do more than 2^31 operations, "
            "you requested 2^" << log_nins;
        return -1;
    }

    btree_type BTree(1024 * 128, 1024 * 128);

    const size_t nins = 1ULL << log_nins;

    stxxl::ran32State = (unsigned int)time(nullptr);

    stxxl::vector<int> Values(nins);
    LOG1 << "Generating " << nins << " random values";
    stxxl::generate(Values.begin(), Values.end(), rnd_gen(), 4);

    stxxl::vector<int>::const_iterator it = Values.begin();
    LOG1 << "Inserting " << nins << " random values into btree";
    for ( ; it != Values.end(); ++it)
        BTree.insert(std::pair<int, double>(*it, double(*it) + 1.0));

    LOG1 << "Number of elements in btree: " << BTree.size();

    LOG1 << "Searching " << nins << " existing elements";
    stxxl::vector<int>::const_iterator vIt = Values.begin();

    for ( ; vIt != Values.end(); ++vIt)
    {
        btree_type::iterator bIt = BTree.find(*vIt);
        die_unless(bIt != BTree.end());
        die_unless(bIt->first == *vIt);
    }

    LOG1 << "Searching " << nins << " non-existing elements";
    stxxl::vector<int>::const_iterator vIt1 = Values.begin();

    for ( ; vIt1 != Values.end(); ++vIt1)
    {
        btree_type::iterator bIt = BTree.find((*vIt1) + 1);
        die_unless(bIt == BTree.end());
    }

    LOG1 << "Test passed.";

    return 0;
}
