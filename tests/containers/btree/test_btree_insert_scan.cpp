/***************************************************************************
 *  tests/containers/btree/test_btree_insert_scan.cpp
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

#include "test_btree_common.h"

#include <tlx/logger.hpp>

int main(int argc, char* argv[])
{
    size_t nins;
    {
        die_verbose_if(argc < 2, "Usage: " << argv[0] << " #log_ins");
        const auto log_nins = foxxll::atoi64(argv[1]);
        die_verbose_if(log_nins > 31, "This test can't do more than 2^31 operations, you requested 2^" << log_nins);
        nins = 1ULL << log_nins;
    }

    btree_type BTree(1024 * 128, 1024 * 128);

    // prepare random unique keys
    stxxl::vector<key_type> values(nins);
    random_fill_vector(values);

    for (auto it = values.cbegin(); it != values.cend(); ++it)
        BTree.insert({ *it, static_cast<payload_type>(*it + 1) });

    LOG1 << "Sorting the random values";
    stxxl::sort(values.begin(), values.end(), comp_type(), 128 * 1024 * 1024);

    LOG1 << "Deleting duplicate values";
    {
        auto new_end = std::unique(values.begin(), values.end());
        values.resize(std::distance(values.begin(), new_end));
    }

    die_unless(BTree.size() == values.size());
    LOG1 << "Size without duplicates: " << values.size();

    LOG1 << "Comparing content";

    btree_type::iterator bIt = BTree.begin();
    for (auto vIt = values.begin(); vIt != values.end(); ++vIt, ++bIt)
    {
        die_unless(*vIt == bIt->first);
        die_unless(static_cast<payload_type>(bIt->first + 1) == bIt->second);
        die_unless(bIt != BTree.end());
    }

    die_unless(bIt == BTree.end());

    LOG1 << "Test passed.";
    return 0;
}
