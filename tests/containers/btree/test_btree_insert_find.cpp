/***************************************************************************
 *  tests/containers/btree/test_btree_insert_find.cpp
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

int main(int argc, char* argv[])
{
    size_t nins;
    {
        die_with_message_if(argc < 2, "Usage: " << argv[0] << " #log_ins");
        const auto log_nins = foxxll::atoi64(argv[1]);
        die_with_message_if(log_nins > 31, "This test can't do more than 2^31 operations, you requested 2^" << log_nins);
        nins = 1ULL << log_nins;
    }

    stxxl::vector<int> Values(nins);
    random_fill_vector(Values);

    btree_type BTree(1024 * 128, 1024 * 128);
    {
        LOG1 << "Inserting " << nins << " random values into btree";
        for (auto it = Values.cbegin(); it != Values.cend(); ++it)
            BTree.insert({ *it, static_cast<payload_type>(*it + 1) });
        LOG1 << "Number of elements in btree: " << BTree.size();
    }

    {
        LOG1 << "Searching " << nins << " existing elements";
        for (auto it = Values.cbegin(); it != Values.cend(); ++it) {
            btree_type::iterator bIt = BTree.find(*it);
            die_unless(bIt != BTree.end());
            die_unless(bIt->first == *it);
        }
    }

    {
        LOG1 << "Searching " << nins << " non-existing elements";
        for (auto it = Values.cbegin(); it != Values.cend(); ++it) {
            btree_type::iterator bIt = BTree.find(static_cast<payload_type>(*it + 1));
            die_unless(bIt == BTree.end());
        }
    }

    LOG1 << "Test passed.";
    return 0;
}
