/***************************************************************************
 *  stream/test_sorted_runs.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2010 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example stream/test_sorted_runs.cpp
//! This is an example of how to use some basic algorithms from
//! stream package. This example shows how to create
//! \c sorted_runs data structure from sorted sequences
//! using \c stream::from_sorted_sequences specialization of \c stream::runs_creator class

#include <limits>
#include <stxxl/stream>

const unsigned long long megabyte = 1024 * 1024;
const int block_size = 1 * megabyte;

typedef unsigned value_type;

struct Cmp : public std::binary_function<value_type, value_type, bool>
{
    typedef unsigned value_type;
    bool operator () (const value_type & a, const value_type & b) const
    {
        return a < b;
    }
    value_type min_value()
    {
        return (std::numeric_limits<value_type>::min)();
    }
    value_type max_value()
    {
        return (std::numeric_limits<value_type>::max)();
    }
};


int main()
{
#if STXXL_PARALLEL_MULTIWAY_MERGE
    STXXL_MSG("STXXL_PARALLEL_MULTIWAY_MERGE");
#endif
    // special parameter type
    typedef stxxl::stream::from_sorted_sequences<value_type> InputType;
    typedef stxxl::stream::runs_creator<InputType, Cmp, 4096, stxxl::RC> CreateRunsAlg;
    typedef CreateRunsAlg::sorted_runs_type SortedRunsType;

    unsigned input_size = (50 * megabyte / sizeof(value_type));

    Cmp c;
    CreateRunsAlg SortedRuns(c, 10 * megabyte);
    value_type checksum_before(0);

    stxxl::random_number32 rnd;
    stxxl::random_number<> rnd_max;
    for (unsigned cnt = input_size; cnt > 0; )
    {
        unsigned run_size = rnd_max(cnt) + 1;          // random run length
        cnt -= run_size;
        STXXL_MSG("current run size: " << run_size);

        std::vector<unsigned> tmp(run_size);           // create temp storage for current run
        std::generate(tmp.begin(), tmp.end(), rnd);     // fill with random numbers
        std::sort(tmp.begin(), tmp.end(), c);           // sort
        for (unsigned j = 0; j < run_size; ++j)
        {
            checksum_before += tmp[j];
            SortedRuns.push(tmp[j]);                    // push sorted values to the current run
        }
        SortedRuns.finish();                            // finish current run
    }


    SortedRunsType Runs = SortedRuns.result();          // get sorted_runs data structure
    assert(check_sorted_runs(Runs, Cmp()));
    // merge the runs
    stxxl::stream::runs_merger<SortedRunsType, Cmp> merger(Runs, Cmp(), 10 * megabyte);
    stxxl::vector<value_type, 4, stxxl::lru_pager<8>, block_size, STXXL_DEFAULT_ALLOC_STRATEGY> array;
    STXXL_MSG(input_size << " " << Runs.elements);
    STXXL_MSG("checksum before: " << checksum_before);
    value_type checksum_after(0);
    for (unsigned i = 0; i < input_size; ++i)
    {
        checksum_after += *merger;
        array.push_back(*merger);
        ++merger;
    }
    STXXL_MSG("checksum after:  " << checksum_after);
    assert(stxxl::is_sorted(array.begin(), array.end(), Cmp()));
    assert(checksum_before == checksum_after);
    assert(merger.empty());

    return 0;
}
