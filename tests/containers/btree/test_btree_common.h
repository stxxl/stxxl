/***************************************************************************
 *  tests/containers/btree/test_btree_common.h
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

#ifndef STXXL_TESTS_CONTAINERS_BTREE_TEST_BTREE_COMMON_HEADER
#define STXXL_TESTS_CONTAINERS_BTREE_TEST_BTREE_COMMON_HEADER

#include <iostream>
#include <random>

#include <tlx/die.hpp>

#include <foxxll/common/die_with_message.hpp>

#include <stxxl/comparator>
#include <stxxl/bits/containers/btree/btree.h>
#include <stxxl/random_shuffle>
#include <stxxl/scan>
#include <stxxl/sort>

#include <test_helpers.h>

using key_type = int;
using payload_type = double;
using comp_type = stxxl::comparator<key_type>;
using pair = std::pair<key_type, payload_type>;
using btree_type = stxxl::btree::btree<key_type, payload_type, comp_type, 4096, 4096, foxxll::simple_random>;

std::ostream& operator << (std::ostream& o, const pair& obj)
{
    o << obj.first << " " << obj.second;
    return o;
}

bool operator == (const pair& a, const pair& b)
{
    return a.first == b.first;
}

#endif // STXXL_TESTS_CONTAINERS_BTREE_TEST_BTREE_COMMON_HEADER
