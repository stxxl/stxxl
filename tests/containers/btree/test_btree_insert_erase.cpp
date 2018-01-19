/***************************************************************************
 *  tests/containers/btree/test_btree_insert_erase.cpp
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

    // prepare random unique keys
    stxxl::vector<key_type> values(nins);
    {
        random_fill_vector(values);

        LOG1 << "Sorting the random values";
        stxxl::sort(values.begin(), values.end(), comp_type(), 128 * 1024 * 1024);

        LOG1 << "Deleting duplicate values";
        {
            auto new_end = std::unique(values.begin(), values.end());
            values.resize(std::distance(values.begin(), new_end));
        }

        LOG1 << "Randomly permute input values";
        stxxl::random_shuffle(values.begin(), values.end(), 128 * 1024 * 1024);
    }

    btree_type BTree(1024 * 128, 1024 * 128);
    {
        LOG1 << "Inserting " << values.size() << " random values into btree";
        for (auto it = values.cbegin(); it != values.cend(); ++it)
            BTree.insert({ *it, static_cast<payload_type>(*it + 1) });
        LOG1 << "Number of elements in btree: " << BTree.size();
    }

    {
        LOG1 << "Searching " << values.size() << " existing elements and erasing them";
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            auto bIt = BTree.find(*it);

            die_unless(bIt != BTree.end());
            // erasing non-existent element
            die_unless(BTree.erase((*it) + 1) == 0);
            // erasing existing element
            die_unless(BTree.erase(*it) == 1);
            // checking it is not there
            die_unless(BTree.find(*it) == BTree.end());
            // trying to erase it again
            die_unless(BTree.erase(*it) == 0);
        }
    }

    die_unless(BTree.empty());

    LOG1 << "Test passed.";

    return 0;
}
