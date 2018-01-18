/***************************************************************************
 *  tests/containers/hash_map/test_hash_map.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007 Markus Westphal <marwes@users.sourceforge.net>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl.h>
#include <stxxl/bits/common/rand.h>
#include <stxxl/bits/common/seed.h>

struct rand_pairs
{
    stxxl::random_number32& rand_;

    explicit rand_pairs(stxxl::random_number32& rand)
        : rand_(rand)
    { }

    std::pair<int, int> operator () ()
    {
        int v = (int)rand_();
        return std::pair<int, int>(v, v);
    }
};

struct hash_int
{
    size_t operator () (int key) const
    {
        // a simple integer hash function
        return (size_t)(key * 2654435761u);
    }
};

struct cmp : public std::less<int>
{
    int min_value() const { return std::numeric_limits<int>::min(); }
    int max_value() const { return std::numeric_limits<int>::max(); }
};

// forced instantiation
template class stxxl::unordered_map<int, int, hash_int, cmp, 4* 1024, 4>;

struct structA
{
    int x, y;

    structA() { }
    structA(int _x, int _y) : x(_x), y(_y) { }
};

struct structB
{
    double u, v;
};

struct hash_structA
{
    size_t operator () (const structA& key) const
    {
        // a simple integer hash function
        return (size_t)((key.x + key.y) * 2654435761u);
    }
};

struct cmp_structA
{
    bool operator () (const structA& a, const structA& b) const
    {
        if (a.x == b.x) return a.y < b.y;
        return a.x < b.x;
    }

    structA min_value() const
    {
        return structA(std::numeric_limits<int>::min(),
                       std::numeric_limits<int>::min());
    }
    structA max_value() const
    {
        return structA(std::numeric_limits<int>::max(),
                       std::numeric_limits<int>::max());
    }
};

// forced instantiation of a struct
template class stxxl::unordered_map<
        structA, structB, hash_structA, cmp_structA, 4* 1024, 4
        >;

void basic_test()
{
    using value_type = std::pair<int, int>;
    const size_t value_size = sizeof(value_type);

    const size_t n_values = 6000;
    const size_t n_tests = 3000;

    // make sure all changes will be buffered (*)
    const size_t buffer_size = n_values * (value_size + sizeof(int*));

    const size_t mem_to_sort = 32 * 1024 * 1024;

    const size_t subblock_raw_size = 4 * 1024;
    const size_t block_size = 4;
    using unordered_map = stxxl::unordered_map<int, int, hash_int, cmp,
                                               subblock_raw_size, block_size>;
    using iterator = unordered_map::iterator;
    using const_iterator = unordered_map::const_iterator;

    foxxll::stats_data stats_begin;

    unordered_map map;
    map.max_buffer_size(buffer_size);
    const unordered_map& cmap = map;

    // generate random values
    stxxl::random_number32 rand32;

    std::vector<value_type> values1(n_values);
    std::vector<value_type> values2(n_values);
    std::vector<value_type> values3(n_values / 2);
    std::generate(values1.begin(), values1.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);
    std::generate(values2.begin(), values2.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);
    std::generate(values3.begin(), values3.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);

    // --- initial import
    std::cout << "Initial import...";
    stats_begin = *foxxll::stats::get_instance();

    die_unless(map.begin() == map.end());
    map.insert(values1.begin(), values1.end(), mem_to_sort);
    die_unless(map.begin() != map.end());
    die_unless(map.size() == n_values);

    std::cout << "passed" << std::endl;
    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    // (*) all these values are stored in external memory; the remaining
    // changes will be buffered in internal memory

    // --- insert: new (from values2) and existing (from values1) values, with
    // --- and without checking
    std::cout << "Insert...";
    stats_begin = *foxxll::stats::get_instance();

    for (size_t i = 0; i < n_values / 2; i++) {
        // new without checking
        map.insert_oblivious(values2[2 * i]);
        // new with checking
        std::pair<iterator, bool> res = map.insert(values2[2 * i + 1]);
        die_unless(res.second && (*(res.first)).first == values2[2 * i + 1].first);
        // existing without checking
        map.insert_oblivious(values1[2 * i]);
        // exiting with checking
        res = map.insert(values1[2 * i + 1]);
        die_unless(!res.second && (*(res.first)).first == values1[2 * i + 1].first);
    }

    die_unless(map.size() == 2 * n_values);
    std::cout << "passed" << std::endl;
    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    // "old" values are stored in external memory, "new" values are stored in
    // internal memory

    // --- find: existing (from external and internal memory) and non-existing
    // --- values
    std::cout << "Find...";
    stats_begin = *foxxll::stats::get_instance();

    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (size_t i = 0; i < n_tests; i++) {
        die_unless(cmap.find(values1[i].first) != cmap.end());
        die_unless(cmap.find(values2[i].first) != cmap.end());
        die_unless(cmap.find(values3[i].first) == cmap.end());
    }
    std::cout << "passed" << std::endl;
    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    // --- insert with overwriting
    std::cout << "Insert with overwriting...";
    stats_begin = *foxxll::stats::get_instance();

    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (size_t i = 0; i < n_tests; i++) {
        value_type value1 = values1[i];         // in external memory
        value1.second++;
        map.insert_oblivious(value1);

        value_type value2 = values2[i];         // in internal memory
        value2.second++;
        map.insert_oblivious(value2);
    }
    // now check
    die_unless(map.size() == 2 * n_values);         // nothing added, nothing removed
    for (size_t i = 0; i < n_tests; i++) {
        const_iterator it1 = cmap.find(values1[i].first);
        const_iterator it2 = cmap.find(values2[i].first);

        die_unless((*it1).second == values1[i].second + 1);
        die_unless((*it2).second == values2[i].second + 1);
    }
    std::cout << "passed" << std::endl;
    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    // --- erase: existing and non-existing values, with and without checking
    std::cout << "Erase...";
    stats_begin = *foxxll::stats::get_instance();

    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    std::random_shuffle(values3.begin(), values3.end());
    for (size_t i = 0; i < n_tests / 2; i++) {        // external
        // existing without checking
        map.erase_oblivious(values1[2 * i].first);
        // existing with checking
        die_unless(map.erase(values1[2 * i + 1].first) == 1);
    }
    for (size_t i = 0; i < n_tests / 2; i++) {        // internal
        // existing without checking
        map.erase_oblivious(values2[2 * i].first);
        // existing with checking
        die_unless(map.erase(values2[2 * i + 1].first) == 1);
        // non-existing without checking
        map.erase_oblivious(values3[i].first);
        // non-existing with checking
    }
    die_unless(map.size() == 2 * n_values - 2 * n_tests);
    std::cout << "passed" << std::endl;
    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    map.clear();
    die_unless(map.size() == 0);

    // --- find and manipulate values by []-operator

    // make sure there are some values in our unordered_map: externally
    // [0..n/2) and internally [n/2..n) from values1
    std::cout << "[ ]-operator...";
    stats_begin = *foxxll::stats::get_instance();

    map.insert(values1.begin(), values1.begin() + n_values / 2, mem_to_sort);
    for (size_t i = n_values / 2; i < n_values; i++) {
        map.insert_oblivious(values1[i]);
    }
    // lookup of existing values
    die_unless(map[values1[5].first] == values1[5].second);                               // external
    die_unless(map[values1[n_values / 2 + 5].first] == values1[n_values / 2 + 5].second); // internal
    // manipulate existing values
    ++(map[values1[7].first]);
    ++(map[values1[n_values / 2 + 7].first]);
    {
        const_iterator cit1 = cmap.find(values1[7].first);
        die_unless((*cit1).second == (*cit1).first + 1);
        const_iterator cit2 = cmap.find(values1[n_values / 2 + 7].first);
        die_unless((*cit2).second == (*cit2).first + 1);
    }
    // lookup of non-existing values
    die_unless(map[values2[5].first] == unordered_map::mapped_type());
    // assignment of non-existing values
    map[values2[7].first] = values2[7].second;
    {
        const_iterator cit = cmap.find(values2[7].first);
        die_unless((*cit).first == values2[7].second);
    }
    die_unless(map.size() == n_values + 2);
    std::cout << "passed" << std::endl;
    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    map.clear();
    die_unless(map.size() == 0);

    // --- additional bulk insert test
    std::cout << "additional bulk-insert...";
    stats_begin = *foxxll::stats::get_instance();

    map.insert(values1.begin(), values1.begin() + n_values / 2, mem_to_sort);
    map.insert(values1.begin() + n_values / 2, values1.end(), mem_to_sort);
    die_unless(map.size() == n_values);
    // lookup some random values
    std::random_shuffle(values1.begin(), values1.end());
    for (size_t i = 0; i < n_tests; i++)
        die_unless(cmap.find(values1[i].first) != cmap.end());
    std::cout << "passed" << std::endl;
    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;

    // --- test equality predicate
    unordered_map::key_equal key_eq = map.key_eq();
    die_unless(key_eq(42, 42));
    die_unless(!key_eq(42, 6 * 9));

    std::cout << "\nAll tests passed" << std::endl;

    map.buffer_size();
}

int main()
{
    basic_test();

    return 0;
}
