/***************************************************************************
 *  tests/containers/test_map.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2005, 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/map>
#include <stxxl/stats>

#include <algorithm>
#include <cmath>

using key_type = unsigned int;
using data_type = unsigned int;

struct cmp : public std::less<key_type>
{
    key_type min_value()
    {
        return std::numeric_limits<key_type>::min();
    }
    key_type max_value()
    {
        return std::numeric_limits<key_type>::max();
    }
};

#define BLOCK_SIZE (32 * 1024)
#define CACHE_SIZE (2 * 1024 * 1024 / BLOCK_SIZE)

#define CACHE_ELEMENTS (BLOCK_SIZE * CACHE_SIZE / (sizeof(key_type) + sizeof(data_type)))

using map_type = stxxl::map<key_type, data_type, cmp, BLOCK_SIZE, BLOCK_SIZE>;

// forced instantiation
template class stxxl::map<key_type, data_type, cmp, BLOCK_SIZE, BLOCK_SIZE>;

int main(int argc, char** argv)
{
    stxxl::stats_data stats_begin(*stxxl::stats::get_instance());
    stxxl::stats_data stats_elapsed;
    STXXL_MSG(stats_begin);

    STXXL_MSG("Block size " << BLOCK_SIZE / 1024 << " KiB");
    STXXL_MSG("Cache size " << (CACHE_SIZE * BLOCK_SIZE) / 1024 << " KiB");
    int max_mult = (argc > 1) ? atoi(argv[1]) : 256;
    for (int mult = 1; mult < max_mult; mult *= 2)
    {
        stats_begin = *stxxl::stats::get_instance();
        const size_t el = mult * (CACHE_ELEMENTS / 8);
        STXXL_MSG("Elements to insert " << el << " volume =" <<
                  (el * (sizeof(key_type) + sizeof(data_type))) / 1024 << " KiB");

        // allocate map and insert elements

        map_type* DMap = new map_type(CACHE_SIZE * BLOCK_SIZE / 2, CACHE_SIZE * BLOCK_SIZE / 2);
        map_type& Map = *DMap;

        for (unsigned i = 0; i < el; ++i)
        {
            Map[i] = i + 1;
        }
        STXXL_CHECK(Map.size() == el);
        stats_elapsed = stxxl::stats_data(*stxxl::stats::get_instance()) - stats_begin;
        double writes = double(stats_elapsed.get_write_count()) / double(el);
        double logel = log(double(el)) / log(double(BLOCK_SIZE));
        STXXL_MSG("Logs: writes " << writes << " logel " << logel << " writes/logel " << (writes / logel));
        STXXL_MSG(stats_elapsed);

        // search for keys

        stats_begin = *stxxl::stats::get_instance();
        STXXL_MSG("Doing search");
        size_t queries = el / 16;
        const map_type& ConstMap = Map;
        stxxl::random_number32 myrandom;
        for (unsigned i = 0; i < queries; ++i)
        {
            key_type key = (key_type)(myrandom() % el);
            map_type::const_iterator result = ConstMap.find(key);
            STXXL_CHECK((*result).second == key + 1);
            STXXL_CHECK(result->second == key + 1);
        }
        stats_elapsed = stxxl::stats_data(*stxxl::stats::get_instance()) - stats_begin;
        double reads = double(stats_elapsed.get_read_count()) / logel;
        double readsperq = double(stats_elapsed.get_read_count()) / (double)queries;
        STXXL_MSG("reads/logel " << reads << " readsperq " << readsperq);
        STXXL_MSG(stats_elapsed);

        delete DMap;
    }

    return 0;
}
