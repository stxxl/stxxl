/***************************************************************************
 *  tests/containers/hash_map/test_hash_map_iterators.cpp
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
#include <stxxl/bits/containers/hash_map/hash_map.h>

struct rand_pairs
{
    stxxl::random_number32& rand_;

    explicit rand_pairs(stxxl::random_number32& rand)
        : rand_(rand)
    { }

    std::pair<int, int> operator () ()
    {
        int v = rand_();
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

////////////////////////////////////////////////////////////////////////////////
void cmp_with_internal_map()
{
    using value_type = std::pair<int, int>;
    const size_t value_size = sizeof(value_type);

    const size_t n_values = 6000;
    const size_t n_tests = 3000;

    // make sure all changes will be buffered
    const size_t buffer_size = n_values * (value_size + sizeof(int*));
    const size_t mem_to_sort = 32 * 1024 * 1024;

    const size_t subblock_raw_size = 4 * 1024;
    const size_t block_size = 4;
    using hash_map = stxxl::hash_map::hash_map<int, int, hash_int, cmp,
                                               subblock_raw_size, block_size>;
    using const_iterator = hash_map::const_iterator;

    using int_hash_map = std::unordered_map<int, int>;

    foxxll::stats_data stats_begin = *foxxll::stats::get_instance();

    hash_map map;
    map.max_buffer_size(buffer_size);
    const hash_map& cmap = map;
    int_hash_map int_map;

    // generate random values
    stxxl::random_number32 rand32;
    std::vector<value_type> values1(n_values);
    std::vector<value_type> values2(n_values);
    std::vector<value_type> values3(n_values);
    std::generate(values1.begin(), values1.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);
    std::generate(values2.begin(), values2.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);
    std::generate(values3.begin(), values3.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);

    // --- initial import: create a nice mix of externally (values1) and
    // --- internally (values2) stored values
    std::cout << "Initial import...";

    map.insert(values1.begin(), values1.end(), mem_to_sort);
    int_map.insert(values1.begin(), values1.end());

    std::vector<value_type>::iterator val_it = values2.begin();
    for ( ; val_it != values2.end(); ++val_it) {
        map.insert_oblivious(*val_it);
        int_map.insert(*val_it);
    }

    // --- erase and overwrite some external values
    std::random_shuffle(values1.begin(), values1.end());
    val_it = values1.begin();
    for ( ; val_it != values1.begin() + n_tests; ++val_it) {
        map.erase_oblivious(val_it->first);
        int_map.erase(val_it->first);
    }
    for ( ; val_it != values1.begin() + 2 * n_tests; ++val_it) {
        map.insert_oblivious(*val_it);
        int_map.insert(*val_it);
    }

    // --- scan and compare with internal memory hash-map
    std::cout << "Compare with internal-memory map...";
    die_unless(int_map.size() == map.size());
    const_iterator cit = cmap.begin();
    for ( ; cit != cmap.end(); ++cit) {
        int key = (*cit).first;
        die_unless(int_map.find(key) != int_map.end());
    }
    std::cout << "passed" << std::endl;

    // --- another bulk insert
    std::cout << "Compare with internal-memory map after another bulk-insert...";
    map.insert(values3.begin(), values3.end(), mem_to_sort);
    int_map.insert(values3.begin(), values3.end());
    die_unless(map.size() == map.size());
    cit = cmap.begin();
    for ( ; cit != cmap.end(); ++cit) {
        int key = (*cit).first;
        die_unless(int_map.find(key) != int_map.end());
    }
    std::cout << "passed" << std::endl;
    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
}

////////////////////////////////////////////////////////////////////////////////
void basic_iterator_test()
{
    using value_type = std::pair<int, int>;
    const size_t value_size = sizeof(value_type);

    const size_t n_values = 2000;
    const size_t n_tests = 1000;

    // make sure all changes will be buffered
    const size_t buffer_size = n_values * (value_size + sizeof(int*));

    const size_t mem_to_sort = 32 * 1024 * 1024;

    const size_t subblock_raw_size = 4 * 1024;
    const size_t block_size = 4;
    using hash_map = stxxl::hash_map::hash_map<int, int, hash_int, cmp,
                                               subblock_raw_size, block_size>;
    using iterator = hash_map::iterator;
    using const_iterator = hash_map::const_iterator;

    foxxll::stats_data stats_begin = *foxxll::stats::get_instance();

    hash_map map;
    map.max_buffer_size(buffer_size);
    const hash_map& cmap = map;

    // generate random values
    stxxl::random_number32 rand32;

    std::vector<value_type> values1(n_values);
    std::vector<value_type> values2(n_values);
    std::generate(values1.begin(), values1.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);
    std::generate(values2.begin(), values2.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);

    // --- initial import: create a nice mix of externally (values1) and
    // --- internally (values2) stored values
    std::cout << "Initial import...";

    die_unless(map.begin() == map.end());
    map.insert(values1.begin(), values1.end(), mem_to_sort);
    for (std::vector<value_type>::iterator val_it = values2.begin();
         val_it != values2.end(); ++val_it)
        map.insert_oblivious(*val_it);
    die_unless(map.begin() != map.end());
    die_unless(map.size() == 2 * n_values);
    std::cout << "passed" << std::endl;

    // --- actual testing begins: modfiy random values via iterator
    std::cout << "Lookup and modify...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (size_t i = 0; i < n_tests; ++i) {
        iterator it1 = map.find(values1[i].first);
        iterator it2 = map.find(values2[i].first);
        die_unless(it1 != map.end());
        die_unless(it2 != map.end());
        (*it1).second++;
        (*it2).second++;
    }
    // check again
    for (size_t i = 0; i < n_tests; ++i) {
        const_iterator cit1 = cmap.find(values1[i].first);
        const_iterator cit2 = cmap.find(values2[i].first);
        die_unless(cit1 != map.end());
        die_unless(cit2 != map.end());
        value_type value1 = *cit1;
        value_type value2 = *cit2;
        die_unless(value1.second == value1.first + 1);
        die_unless(value2.second == value2.first + 1);
    }
    std::cout << "passed" << std::endl;

    // --- scan and modify
    std::cout << "Scan and modify...";
    {
        for (iterator it = map.begin(); it != map.end(); ++it)
            (*it).second = (*it).first + 1;

        for (const_iterator cit = cmap.begin(); cit != cmap.end(); ++cit) {
            die_unless((*cit).second == (*cit).first + 1);
        }
    }
    std::cout << "passed" << std::endl;

    // --- interator-value altered by insert_oblivious
    std::cout << "Iterator-value altered by insert_oblivious...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (size_t i = 0; i < n_tests; i++) {
        int key1 = values1[i].first;
        int key2 = values2[i].first;
        const_iterator cit1 = cmap.find(key1);
        die_unless(cit1 != cmap.end());
        const_iterator cit2 = cmap.find(key2);
        die_unless(cit2 != cmap.end());

        map.insert_oblivious(value_type(key1, key1 + 3));
        map.insert_oblivious(value_type(key2, key2 + 3));

        die_unless((*cit1).second == key1 + 3);
        die_unless((*cit2).second == key2 + 3);
    }
    std::cout << "passed" << std::endl;

    // --- iterator-value altered by other iterator
    std::cout << "Iterator-value altered by other iterator...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (size_t i = 0; i < n_tests; i++) {
        const_iterator cit1 = cmap.find(values1[i].first);
        die_unless(cit1 != cmap.end());
        const_iterator cit2 = cmap.find(values2[i].first);
        die_unless(cit2 != cmap.end());
        iterator it1 = map.find(values1[i].first);
        die_unless(it1 != map.end());
        iterator it2 = map.find(values2[i].first);
        die_unless(it2 != map.end());

        (*it1).second = (*it1).first + 5;
        (*it2).second = (*it2).first + 5;
        die_unless((*cit1).second == (*cit1).first + 5);
        die_unless((*cit2).second == (*cit2).first + 5);
    }
    std::cout << "passed" << std::endl;

    // --- erase by iterator
    std::cout << "Erase by iterator...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    for (size_t i = 0; i < n_tests; i++) {
        const_iterator cit1 = cmap.find(values1[i].first);
        die_unless(cit1 != cmap.end());
        const_iterator cit2 = cmap.find(values2[i].first);
        die_unless(cit2 != cmap.end());
        iterator it1 = map.find(values1[i].first);
        die_unless(it1 != map.end());
        iterator it2 = map.find(values2[i].first);
        die_unless(it2 != map.end());

        map.erase(it1);
        map.erase(it2);
        die_unless(cit1 == cmap.end());
        die_unless(cit2 == cmap.end());
    }
    std::cout << "passed" << std::endl;

    // --- erase by value (key)
    std::cout << "Erase by key...";
    for (size_t i = 0; i < n_tests; i++) {
        const_iterator cit1 = cmap.find(values1[i + n_tests].first);
        die_unless(cit1 != cmap.end());
        const_iterator cit2 = cmap.find(values2[i + n_tests].first);
        die_unless(cit2 != cmap.end());

        map.erase_oblivious(values1[i + n_tests].first);
        map.erase_oblivious(values2[i + n_tests].first);
        die_unless(cit1 == cmap.end());
        die_unless(cit2 == cmap.end());
    }
    std::cout << "passed" << std::endl;

    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
}

////////////////////////////////////////////////////////////////////////////////
void more_iterator_test()
{
    using value_type = std::pair<int, int>;
    const size_t value_size = sizeof(value_type);

    const size_t n_values = 6000;

    // make sure all changes will be buffered
    const size_t buffer_size = n_values * (value_size + sizeof(int*));
    const size_t mem_to_sort = 32 * 1024 * 1024;

    const size_t subblock_raw_size = 4 * 1024;
    const size_t block_size = 4;
    using hash_map = stxxl::hash_map::hash_map<int, int, hash_int, cmp,
                                               subblock_raw_size, block_size>;
    using const_iterator = hash_map::const_iterator;

    foxxll::stats_data stats_begin = *foxxll::stats::get_instance();

    hash_map map;
    map.max_buffer_size(buffer_size);
    const hash_map& cmap = map;

    // generate random values
    stxxl::random_number32 rand32;
    std::vector<value_type> values1(n_values);
    std::vector<value_type> values2(n_values);
    std::generate(values1.begin(), values1.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);
    std::generate(values2.begin(), values2.end(), rand_pairs(rand32) _STXXL_FORCE_SEQUENTIAL);

    // --- initial import
    map.insert(values1.begin(), values1.end(), mem_to_sort);
    for (std::vector<value_type>::iterator val_it = values2.begin();
         val_it != values2.end(); ++val_it)
        map.insert_oblivious(*val_it);

    // --- store some iterators, rebuild and check
    std::cout << "Rebuild test...";
    std::random_shuffle(values1.begin(), values1.end());
    std::random_shuffle(values2.begin(), values2.end());
    {
        const_iterator cit1 = cmap.find(values1[17].first);
        const_iterator cit2 = cmap.find(values2[19].first);
        *cit1;
        *cit2;
        map.rehash();
        die_unless(map.size() == 2 * n_values);
        die_unless((*cit1).first == values1[17].first);
        die_unless((*cit2).first == values2[19].first);
    }
    std::cout << "passed" << std::endl;

    // --- unusual cases while scanning
    std::cout << "Another scan-test...";
    {
        const_iterator cit1 = cmap.find(values1[n_values / 2].first);
        const_iterator cit2 = cit1;
        ++cit1;
        int key1 = (*cit1).first;
        ++cit1;
        int key2 = (*cit1).first;
        map.erase_oblivious(key1);
        map.insert_oblivious(value_type(key2, key2 + 2));

        die_unless((*cit1).second == key2 + 2);
        ++cit2;
        die_unless(cit1 == cit2);
    }
    std::cout << "passed" << std::endl;

    LOG1 << foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
}

////////////////////////////////////////////////////////////////////////////////

int main()
{
    cmp_with_internal_map();
    basic_iterator_test();
    more_iterator_test();
    return 0;
}
