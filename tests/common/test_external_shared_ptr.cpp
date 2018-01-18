/***************************************************************************
 *  tests/common/test_external_shared_ptr.cpp
 *
 *  This file has been derived from the following tests written
 *  by Roman Dementiev:
 *     - test_vector.cpp
 *     - test_map.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2011 Daniel Godas-Lopez <dgodas@gmail.com>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/common/external_shared_ptr.h>
#include <stxxl/map>
#include <stxxl/scan>

#include <stxxl/vector>

struct actual_element   // 24 bytes, not a power of 2 intentionally
{
    int64_t key;
    int64_t load0;
    int64_t load1;

    actual_element& operator = (int64_t i)
    {
        key = i;
        load0 = i + 42;
        load1 = i ^ 42;
        return *this;
    }

    bool operator == (const actual_element& e2) const
    {
        return key == e2.key && load0 == e2.load0 && load1 == e2.load1;
    }
};

using actual_element_ptr = std::shared_ptr<actual_element>;
using element = stxxl::external_shared_ptr<actual_element_ptr>;

struct counter
{
    int value;
    explicit counter(int v) : value(v) { }
    int operator () ()
    {
        int old_val = value;
        value++;
        return old_val;
    }
};

template <class my_vec_type>
void test_const_iterator(const my_vec_type& x)
{
    typename my_vec_type::const_iterator i = x.begin();
    i = x.end() - 1;
    i.block_externally_updated();
    i.flush();
    i++;
    ++i;
    --i;
    i--;
    *i;
}

void test_vector()
{
    // use non-randomized striping to avoid side effects on random generator
    using vector_type = stxxl::vector<element, 2, stxxl::lru_pager<2>, (2* 1024* 1024), foxxll::striping>;
    vector_type v(64 * 1024 * 1024 / sizeof(element));

    // test assignment const_iterator = iterator
    vector_type::const_iterator c_it = v.begin();
    tlx::unused(c_it);

    test_const_iterator(v);

    stxxl::random_number32 rnd;
    int offset = rnd();

    LOG1 << "write " << v.size() << " elements";

    stxxl::ran32State = 0xdeadbeef;
    vector_type::size_type i;

    // fill the vector with increasing sequence of integer numbers
    for (i = 0; i < v.size(); ++i)
    {
        actual_element_ptr aep(std::make_shared<actual_element>());
        aep->key = i + offset;
        element e(aep);

        v[i] = e;

        die_unless(v[i].get()->key == int64_t(i + offset));
    }

    // fill the vector with random numbers
    for (i = 0; i < v.size(); ++i)
    {
        actual_element_ptr aep(std::make_shared<actual_element>());
        aep->key = rnd();
        element e(aep);

        v[i].unwrap();
        v[i] = e;

        die_unless(v[i].get()->key == aep->key);
    }
    v.flush();

    LOG1 << "seq read of " << v.size() << " elements";

    stxxl::ran32State = 0xdeadbeef;

    // testing swap
    vector_type a;
    std::swap(v, a);
    std::swap(v, a);

    for (i = 0; i < v.size(); i++)
        die_unless(v[i].get()->key == rnd());

    // check again
    LOG1 << "clear";

    for (vector_type::iterator it = v.begin(); it != v.end(); ++it)
        it->unwrap();

    v.clear();

    stxxl::ran32State = 0xdeadbeef + 10;

    v.resize(64 * 1024 * 1024 / sizeof(element));

    LOG1 << "write " << v.size() << " elements";
    for (i = 0; i < v.size(); ++i)
    {
        actual_element_ptr aep(std::make_shared<actual_element>());
        aep->key = rnd();
        element e(aep);

        v[i] = e;

        die_unless(v[i].get()->key == aep->key);
    }

    stxxl::ran32State = 0xdeadbeef + 10;

    LOG1 << "seq read of " << v.size() << " elements";

    for (i = 0; i < v.size(); i++)
        die_unless(v[i].get()->key == rnd());

    LOG1 << "copy vector of " << v.size() << " elements";

    vector_type v_copy0(v);
    die_unless(v == v_copy0);

    vector_type v_copy1;
    v_copy1 = v;
    die_unless(v == v_copy1);

    while (v.size() != 0) {
        element e = v.back();
        v.pop_back();
        e.unwrap();
    }
}

using key_type = size_t;

struct test_data {
    unsigned char a;
    unsigned long b[3];
    unsigned int c;
};

using test_data_ptr = std::shared_ptr<test_data>;
using data_type = stxxl::external_shared_ptr<test_data_ptr>;

struct cmp : public std::less<key_type>
{
    static key_type min_value()
    {
        return (std::numeric_limits<key_type>::min)();
    }
    static key_type max_value()
    {
        return (std::numeric_limits<key_type>::max)();
    }
};

#define BLOCK_SIZE (32 * 1024)
#define CACHE_SIZE (2 * 1024 * 1024 / BLOCK_SIZE)

#define CACHE_ELEMENTS (BLOCK_SIZE * CACHE_SIZE / (sizeof(key_type) + sizeof(data_type)))

using map_type = stxxl::map<key_type, data_type, cmp, BLOCK_SIZE, BLOCK_SIZE>;

void test_map()
{
    const unsigned max_mult = 8;

    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());
    foxxll::stats_data stats_elapsed;
    LOG1 << stats_begin;

    LOG1 << "Block size " << BLOCK_SIZE / 1024 << " KiB";
    LOG1 << "Cache size " << (CACHE_SIZE * BLOCK_SIZE) / 1024 << " KiB";

    for (unsigned mult = 1; mult < max_mult; mult *= 2)
    {
        stats_begin = *foxxll::stats::get_instance();
        const size_t el = mult * (CACHE_ELEMENTS / 8);
        LOG1 << "Elements to insert " << el << " volume =" <<
        (el * (sizeof(key_type) + sizeof(data_type))) / 1024 << " KiB";
        map_type* DMap = new map_type(CACHE_SIZE * BLOCK_SIZE / 2, CACHE_SIZE * BLOCK_SIZE / 2);
        map_type& Map = *DMap;

        for (size_t i = 0; i < el; ++i)
        {
            test_data_ptr test = std::make_shared<test_data>();

            test->a = (unsigned char)(i + 1);
            for (unsigned j = 0; j < 3; j++)
                test->b[j] = (unsigned long)(i + 2);
            test->c = (unsigned int)(i + 3);

            data_type data(test);
            Map[i] = data;
        }
        stats_elapsed = foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
        double writes = double(stats_elapsed.get_write_count()) / double(el);
        double logel = log(double(el)) / log(double(BLOCK_SIZE));
        LOG1 << "Logs: writes " << writes << " logel " << logel << " writes/logel " << (writes / logel);
        LOG1 << stats_elapsed;

        stats_begin = *foxxll::stats::get_instance();
        LOG1 << "Doing search";
        size_t queries = el;
        stxxl::random_number32 myrandom;
        for (unsigned i = 0; i < queries; ++i)
        {
            key_type key = myrandom() % el;
            map_type::iterator result = Map.find(key);

            data_type data = (*result).second;
            test_data_ptr tmp = data.get();

            die_unless(tmp->a == (unsigned char)(key + 1));
            for (unsigned j = 0; j < 3; ++j)
                die_unless(tmp->b[j] == (unsigned long)(key + 2));
            die_unless(tmp->c == (unsigned int)(key + 3));
        }
        stats_elapsed = foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
        double reads = double(stats_elapsed.get_read_count()) / logel;
        double readsperq = double(stats_elapsed.get_read_count()) / (double)queries;
        LOG1 << "reads/logel " << reads << " readsperq " << readsperq;
        LOG1 << stats_elapsed;

        while (Map.size() != 0) {
            map_type::iterator it = Map.begin();
            data_type data = (*it).second;
            Map.erase(it);
            data.unwrap();
        }

        delete DMap;
    }
}

int main()
{
    test_vector();
    test_map();
    return 0;
}
