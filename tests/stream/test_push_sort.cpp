/***************************************************************************
 *  tests/stream/test_push_sort.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009, 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

//! \example stream/test_push_sort.cpp
//! This is an example of how to use some basic algorithms from
//! stream package. This example shows how to create
//! \c sorted_runs data structure
//! using \c stream::use_push specialization of \c stream::runs_creator class

#include <limits>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/defines.h>
#include <stxxl/stream>

const unsigned long long megabyte = 1024 * 1024;

using value_type = unsigned;

struct Cmp : public std::binary_function<value_type, value_type, bool>
{
    using value_type = unsigned;
    bool operator () (const value_type& a, const value_type& b) const
    {
        return a < b;
    }
    value_type min_value()
    {
        return std::numeric_limits<value_type>::min();
    }
    value_type max_value()
    {
        return std::numeric_limits<value_type>::max();
    }
};

// special parameter type
using InputType = stxxl::stream::use_push<value_type>;
using CreateRunsAlg = stxxl::stream::runs_creator<
          InputType, Cmp, 4096, foxxll::random_cyclic>;
using SortedRunsType = CreateRunsAlg::sorted_runs_type;

// forced instantiation
template class stxxl::stream::runs_merger<SortedRunsType, Cmp>;

int main()
{
#if STXXL_PARALLEL_MULTIWAY_MERGE
    LOG1 << "STXXL_PARALLEL_MULTIWAY_MERGE";
#endif

    unsigned input_size = (10 * megabyte / sizeof(value_type));

    Cmp c;
    CreateRunsAlg SortedRuns(c, 10 * megabyte);
    value_type checksum_before(0);

    stxxl::random_number32 rnd;

    for (unsigned cnt = input_size; cnt > 0; --cnt)
    {
        const value_type element = rnd();
        checksum_before += element;
        SortedRuns.push(element);               // push into the sorter
    }

    SortedRunsType Runs = SortedRuns.result();  // get sorted_runs data structure
    die_unless(stxxl::stream::check_sorted_runs(Runs, Cmp()));

    // merge the runs
    stxxl::stream::runs_merger<SortedRunsType, Cmp> merger(Runs, Cmp(), 10 * megabyte);
    stxxl::vector<value_type, 4, stxxl::lru_pager<8> > array;
    LOG1 << input_size << " " << Runs->elements;
    LOG1 << "checksum before: " << checksum_before;
    value_type checksum_after(0);
    for (unsigned i = 0; i < input_size; ++i)
    {
        checksum_after += *merger;
        array.push_back(*merger);
        ++merger;
    }
    LOG1 << "checksum after:  " << checksum_after;
    die_unless(stxxl::is_sorted(array.cbegin(), array.cend(), Cmp()));
    die_unless(checksum_before == checksum_after);
    die_unless(merger.empty());

    return 0;
}
