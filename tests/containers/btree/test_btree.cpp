/***************************************************************************
 *  tests/containers/btree/test_btree.cpp
 *
 *  A very basic test
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/containers/btree/btree.h>
#include <stxxl/timer>

struct comp_type : public std::less<int>
{
    static int max_value()
    {
        return std::numeric_limits<int>::max();
    }
};
using btree_type = stxxl::btree::btree<
          int, double, comp_type, 4096, 4096, foxxll::simple_random>;

// forced instantiation
template class stxxl::btree::btree<
        int, double, comp_type, 4096, 4096, foxxll::simple_random>;

std::ostream& operator << (std::ostream& o, const std::pair<int, double>& obj)
{
    o << obj.first << " " << obj.second;
    return o;
}

#define node_cache_size (25 * 1024 * 1024)
#define leaf_cache_size (25 * 1024 * 1024)

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        LOG1 << "Usage: " << argv[0] << " #ins";
        return -1;
    }

    btree_type BTree1(node_cache_size, leaf_cache_size);

    unsigned nins = atoi(argv[1]);
    if (nins < 100)
        nins = 100;

    stxxl::random_number32 rnd;

    // .begin() .end() test
    BTree1[10] = 100.;
    btree_type::iterator begin = BTree1.begin();
    btree_type::iterator end = BTree1.end();
    die_unless(begin == BTree1.begin());
    BTree1[5] = 50.;
    btree_type::iterator nbegin = BTree1.begin();
    btree_type::iterator nend = BTree1.end();
    die_unless(nbegin == BTree1.begin());
    die_unless(begin != nbegin);
    die_unless(end == nend);
    die_unless(begin != end);
    die_unless(nbegin != end);
    die_unless(begin->first == 10);
    die_unless(begin->second == 100);
    die_unless(nbegin->first == 5);
    die_unless(nbegin->second == 50);

    BTree1[10] = 200.;
    die_unless(begin->second == 200.);

    btree_type::iterator it = BTree1.find(5);
    die_unless(it != BTree1.end());
    die_unless(it->first == 5);
    die_unless(it->second == 50.);
    it = BTree1.find(6);
    die_unless(it == BTree1.end());
    it = BTree1.find(1000);
    die_unless(it == BTree1.end());

    btree_type::size_type f = BTree1.erase(5);
    die_unless(f == 1);
    f = BTree1.erase(6);
    die_unless(f == 0);
    f = BTree1.erase(5);
    die_unless(f == 0);

    die_unless(BTree1.count(10) == 1);
    die_unless(BTree1.count(5) == 0);

    it = BTree1.insert(BTree1.begin(), std::pair<int, double>(7, 70.));
    die_unless(it->second == 70.);
    die_unless(BTree1.size() == 2);
    it = BTree1.insert(BTree1.begin(), std::pair<int, double>(10, 300.));
    die_unless(it->second == 200.);
    die_unless(BTree1.size() == 2);

    // test lower_bound

    it = BTree1.lower_bound(6);
    die_unless(it != BTree1.end());
    die_unless(it->first == 7);

    it = BTree1.lower_bound(7);
    die_unless(it != BTree1.end());
    die_unless(it->first == 7);

    it = BTree1.lower_bound(8);
    die_unless(it != BTree1.end());
    die_unless(it->first == 10);

    it = BTree1.lower_bound(11);
    die_unless(it == BTree1.end());

    // test upper_bound

    it = BTree1.upper_bound(6);
    die_unless(it != BTree1.end());
    die_unless(it->first == 7);

    it = BTree1.upper_bound(7);
    die_unless(it != BTree1.end());
    die_unless(it->first == 10);

    it = BTree1.upper_bound(8);
    die_unless(it != BTree1.end());
    die_unless(it->first == 10);

    it = BTree1.upper_bound(10);
    die_unless(it == BTree1.end());

    it = BTree1.upper_bound(11);
    die_unless(it == BTree1.end());

    // test equal_range

    std::pair<btree_type::iterator, btree_type::iterator> it_pair = BTree1.equal_range(1);
    die_unless(BTree1.find(7) == it_pair.first);
    die_unless(BTree1.find(7) == it_pair.second);

    it_pair = BTree1.equal_range(7);
    die_unless(BTree1.find(7) == it_pair.first);
    die_unless(BTree1.find(10) == it_pair.second);

    it_pair = BTree1.equal_range(8);
    die_unless(BTree1.find(10) == it_pair.first);
    die_unless(BTree1.find(10) == it_pair.second);

    it_pair = BTree1.equal_range(10);
    die_unless(BTree1.find(10) == it_pair.first);
    die_unless(BTree1.end() == it_pair.second);

    it_pair = BTree1.equal_range(11);
    die_unless(BTree1.end() == it_pair.first);
    die_unless(BTree1.end() == it_pair.second);

    //

    it = BTree1.lower_bound(0);
    BTree1.erase(it);
    die_unless(BTree1.size() == 1);

    BTree1.clear();
    die_unless(BTree1.size() == 0);

    for (unsigned int i = 0; i < nins / 2; ++i)
    {
        BTree1[rnd() % nins] = 10.1;
    }
    LOG1 << "Size of map: " << BTree1.size();

    BTree1.clear();

    for (unsigned int i = 0; i < nins / 2; ++i)
    {
        BTree1[rnd() % nins] = 10.1;
    }

    LOG1 << "Size of map: " << BTree1.size();

    btree_type BTree2(comp_type(), node_cache_size, leaf_cache_size);

    LOG1 << "Construction of BTree3 from BTree1 that has " << BTree1.size() << " elements";
    btree_type BTree3(BTree1.begin(), BTree1.end(), comp_type(), node_cache_size, leaf_cache_size);

    die_unless(BTree3 == BTree1);

    LOG1 << "Bulk construction of BTree4 from BTree1 that has " << BTree1.size() << " elements";
    btree_type BTree4(BTree1.begin(), BTree1.end(), comp_type(), node_cache_size, leaf_cache_size, true);

    LOG1 << "Size of BTree1: " << BTree1.size();
    LOG1 << "Size of BTree4: " << BTree4.size();

    die_unless(BTree4 == BTree1);
    die_unless(BTree3 == BTree4);

    BTree4.begin()->second = 0;
    die_unless(BTree3 != BTree4);
    die_unless(BTree4 < BTree3);

    BTree4.begin()->second = 1000;
    die_unless(BTree4 > BTree3);

    die_unless(BTree3 != BTree4);

    it = BTree4.begin();
    ++it;
    LOG1 << "Size of Btree4 before erase: " << BTree4.size();
    BTree4.erase(it, BTree4.end());
    LOG1 << "Size of Btree4 after erase: " << BTree4.size();
    die_unless(BTree4.size() == 1);

    LOG1 << "Size of Btree1 before erase: " << BTree1.size();
    BTree1.erase(BTree1.begin(), BTree1.end());
    LOG1 << "Size of Btree1 after erase: " << BTree1.size();
    die_unless(BTree1.empty());

    // a copy of BTree3
    btree_type BTree5(BTree3.begin(), BTree3.end(), comp_type(), node_cache_size, leaf_cache_size, true);
    die_unless(BTree5 == BTree3);

    btree_type::iterator b3 = BTree3.begin();
    btree_type::iterator b4 = BTree4.begin();
    btree_type::iterator e3 = BTree3.end();
    btree_type::iterator e4 = BTree4.end();

    LOG1 << "Testing swapping operation (std::swap)";
    std::swap(BTree4, BTree3);
    die_unless(b3 == BTree4.begin());
    die_unless(b4 == BTree3.begin());
    die_unless(e3 == BTree4.end());
    die_unless(e4 == BTree3.end());

    die_unless(BTree5 == BTree4);
    die_unless(BTree5 != BTree3);

    btree_type::const_iterator cb = BTree3.begin();
    btree_type::const_iterator ce = BTree3.end();
    const btree_type& CBTree3 = BTree3;
    cb = CBTree3.begin();
    die_unless(!(b3 == cb));
    die_unless((b3 != cb));
    die_unless(!(cb == b3));
    die_unless((cb != b3));
    ce = CBTree3.end();
    btree_type::const_iterator cit = CBTree3.find(0);
    cit = CBTree3.lower_bound(0);
    cit = CBTree3.upper_bound(0);

    std::pair<btree_type::const_iterator, btree_type::const_iterator> cit_pair = CBTree3.equal_range(1);

    die_unless(CBTree3.max_size() >= CBTree3.size());

    CBTree3.key_comp();
    CBTree3.value_comp();

    double sum = 0.0;

    LOG1 << *foxxll::stats::get_instance();

    foxxll::timer Timer2;
    Timer2.start();
    cit = BTree5.begin();
    for ( ; cit != BTree5.end(); ++cit)
        sum += cit->second;

    Timer2.stop();
    LOG1 << "Scanning with const iterator: " << Timer2.mseconds() << " msec";

    LOG1 << *foxxll::stats::get_instance();

    foxxll::timer Timer1;
    Timer1.start();
    it = BTree5.begin();
    for ( ; it != BTree5.end(); ++it)
        sum += it->second;

    Timer1.stop();
    LOG1 << "Scanning with non const iterator: " << Timer1.mseconds() << " msec";

    LOG1 << *foxxll::stats::get_instance();

    BTree5.disable_prefetching();
    BTree5.enable_prefetching();
    BTree5.prefetching_enabled();
    die_unless(BTree5.prefetching_enabled());

    LOG1 << "All tests passed successfully";

    return 0;
}
