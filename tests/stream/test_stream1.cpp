/***************************************************************************
 *  tests/stream/test_stream1.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#include <iostream>
#include <limits>
#include <vector>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/defines.h>
#include <stxxl/stream>

struct Input
{
    using value_type = unsigned;
    value_type value;
    value_type rnd_value;
    stxxl::random_number32 rnd;
    value_type crc;
    explicit Input(value_type init) : value(init)
    {
        rnd_value = rnd();
        crc = rnd_value;
    }
    bool empty() const
    {
        return value == 1;
    }
    Input& operator ++ ()
    {
        --value;
        rnd_value = rnd();
        if (!empty())
            crc += rnd_value;

        return *this;
    }
    const value_type& operator * () const
    {
        return rnd_value;
    }
};

struct Cmp : std::binary_function<unsigned, unsigned, bool>
{
    using value_type = unsigned;
    bool operator () (const value_type& a, const value_type& b) const
    {
        return a < b;
    }
    value_type min_value() const
    {
        return std::numeric_limits<value_type>::min();
    }
    value_type max_value() const
    {
        return std::numeric_limits<value_type>::max();
    }
};

#define MULT (1000)

int main()
{
    using CreateRunsAlg = stxxl::stream::runs_creator<
              Input, Cmp, 4096* MULT, foxxll::random_cyclic>;
    using SortedRunsType = CreateRunsAlg::sorted_runs_type;

    foxxll::stats* s = foxxll::stats::get_instance();

    std::cout << *s;

    LOG1 << "Size of block type " << sizeof(CreateRunsAlg::block_type);
    unsigned size = MULT * 1024 * 128 / (unsigned)(sizeof(Input::value_type) * 2);
    Input in(size + 1);
    CreateRunsAlg SortedRuns(in, Cmp(), 1024 * 128 * MULT);
    SortedRunsType Runs = SortedRuns.result();
    die_unless(stxxl::stream::check_sorted_runs(Runs, Cmp()));
    // merge the runs
    stxxl::stream::runs_merger<SortedRunsType, Cmp> merger(Runs, Cmp(), MULT * 1024 * 128);
    stxxl::vector<Input::value_type> array;
    LOG1 << size << " " << Runs->elements;
    LOG1 << "CRC: " << in.crc;
    Input::value_type crc(0);
    for (unsigned i = 0; i < size; ++i)
    {
        crc += *merger;
        array.push_back(*merger);
        ++merger;
    }
    LOG1 << "CRC: " << crc;
    die_unless(stxxl::is_sorted(array.cbegin(), array.cend(), Cmp()));
    die_unless(merger.empty());

    std::cout << *s;

    return 0;
}
