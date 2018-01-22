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

#include <algorithm>
#include <cmath>
#include <random>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/comparator>
#include <stxxl/map>

using key_type = unsigned int;
using data_type = unsigned int;
using cmp = stxxl::comparator<key_type>;

#define BLOCK_SIZE (32 * 1024)
#define CACHE_SIZE (2 * 1024 * 1024 / BLOCK_SIZE)

#define CACHE_ELEMENTS (BLOCK_SIZE * CACHE_SIZE / (sizeof(key_type) + sizeof(data_type)))

using map_type = stxxl::map<key_type, data_type, cmp, BLOCK_SIZE, BLOCK_SIZE>;

// forced instantiation
template class stxxl::map<key_type, data_type, cmp, BLOCK_SIZE, BLOCK_SIZE>;

int main(int argc, char** argv)
{
    foxxll::stats_data stats_begin(*foxxll::stats::get_instance());
    foxxll::stats_data stats_elapsed;
    LOG1 << stats_begin;

    LOG1 << "Block size " << BLOCK_SIZE / 1024 << " KiB";
    LOG1 << "Cache size " << (CACHE_SIZE * BLOCK_SIZE) / 1024 << " KiB";
    int max_mult = (argc > 1) ? atoi(argv[1]) : 256;
    for (int mult = 1; mult < max_mult; mult *= 2)
    {
        stats_begin = *foxxll::stats::get_instance();
        const size_t el = mult * (CACHE_ELEMENTS / 8);
        LOG1 << "Elements to insert " << el << " volume =" <<
        (el * (sizeof(key_type) + sizeof(data_type))) / 1024 << " KiB";

        // allocate map and insert elements

        map_type* DMap = new map_type(CACHE_SIZE * BLOCK_SIZE / 2, CACHE_SIZE * BLOCK_SIZE / 2);
        map_type& Map = *DMap;

        for (unsigned i = 0; i < el; ++i)
        {
            Map[i] = i + 1;
        }
        die_unless(Map.size() == el);
        stats_elapsed = foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
        double writes = double(stats_elapsed.get_write_count()) / double(el);
        double logel = log(double(el)) / log(double(BLOCK_SIZE));
        LOG1 << "Logs: writes " << writes << " logel " << logel << " writes/logel " << (writes / logel);
        LOG1 << stats_elapsed;

        // search for keys

        stats_begin = *foxxll::stats::get_instance();
        LOG1 << "Doing search";
        size_t queries = el / 16;
        const map_type& ConstMap = Map;

        std::mt19937_64 randgen;
        std::uniform_int_distribution<key_type> distr(0, el - 1);
        for (unsigned i = 0; i < queries; ++i)
        {
            const key_type key = distr(randgen);

            map_type::const_iterator result = ConstMap.find(key);
            die_unless((*result).second == key + 1);
            die_unless(result->second == key + 1);
        }
        stats_elapsed = foxxll::stats_data(*foxxll::stats::get_instance()) - stats_begin;
        double reads = double(stats_elapsed.get_read_count()) / logel;
        double readsperq = double(stats_elapsed.get_read_count()) / static_cast<double>(queries);
        LOG1 << "reads/logel " << reads << " readsperq " << readsperq;
        LOG1 << stats_elapsed;

        delete DMap;
    }

    return 0;
}
