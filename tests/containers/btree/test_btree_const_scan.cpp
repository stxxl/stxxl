/***************************************************************************
 *  tests/containers/btree/test_btree_const_scan.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2018 Manuel Penschuck <stxxl@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <foxxll/common/die_with_message.hpp>

#include <stxxl/comparator>
#include <stxxl/bits/containers/btree/btree.h>

#include <key_with_padding.h>
#include <test_helpers.h>

constexpr size_t NODE_BLOCK_SIZE = 4096;
constexpr size_t LEAF_BLOCK_SIZE = 128 * 1024;

using key_type = uint64_t;
using payload_type = key_with_padding<key_type, sizeof(key_type) + 24, false>;
using comp_type = stxxl::comparator<key_type>;

using btree_type = stxxl::btree::btree<key_type, payload_type, comp_type, NODE_BLOCK_SIZE, LEAF_BLOCK_SIZE, foxxll::simple_random>;

// forced instantiation
template class stxxl::btree::btree<key_type, payload_type, comp_type, NODE_BLOCK_SIZE, LEAF_BLOCK_SIZE, foxxll::simple_random>;

constexpr size_t node_cache_size = 25 * 1024 * 1024;
constexpr size_t leaf_cache_size = 6 * LEAF_BLOCK_SIZE;

template <typename Iterator>
uint64_t scan(btree_type& BTree, const std::string& test_key)
{
    foxxll::scoped_print_timer timer("Scan with " + test_key, BTree.size() * sizeof(*BTree.begin()));

    uint64_t sum = 0;
    for (Iterator it = BTree.begin(), end = BTree.end(); it != end; ++it)
        sum += it->second.key;

    return sum;
}

int main(int argc, char* argv[])
{
    die_with_message_if(argc < 2, "Usage: " << argv[0] << " #ins");

    const unsigned nins = atoi(argv[1]);

    LOG1 << "values set size  : " << nins * (sizeof(key_type) + sizeof(payload_type)) << " bytes\n"
        "Node cache size: " << node_cache_size << " bytes\n"
        "Leaf cache size: " << leaf_cache_size << " bytes";

    std::vector<std::pair<key_type, payload_type> > values(nins);

    uint64_t checksum = 0;
    for (unsigned int i = 0; i < nins; ++i)
    {
        values[i].first = i;
        values[i].second.key = i;
        checksum += i;
    }

    LOG1 << "Scan with prefetching";
    {
        btree_type BTree1(values.begin(), values.end(), comp_type(), node_cache_size, leaf_cache_size, true);
        btree_type BTree2(values.begin(), values.end(), comp_type(), node_cache_size, leaf_cache_size, true);

        die_unless(checksum == scan<btree_type::const_iterator>(BTree1, "const iterator"));
        die_unless(checksum == scan<btree_type::iterator>(BTree2, "non-const iterator"));
    }

    LOG1 << "Scan without prefetching";
    {
        btree_type BTree1(values.begin(), values.end(), comp_type(), node_cache_size, leaf_cache_size, true);
        btree_type BTree2(values.begin(), values.end(), comp_type(), node_cache_size, leaf_cache_size, true);

        BTree1.disable_prefetching();
        BTree2.disable_prefetching();

        die_unless(checksum == scan<btree_type::const_iterator>(BTree1, "const iterator"));
        die_unless(checksum == scan<btree_type::iterator>(BTree2, "non-const iterator"));
    }

    LOG1 << "All tests passed successfully";
    return 0;
}
