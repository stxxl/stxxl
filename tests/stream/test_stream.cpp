/***************************************************************************
 *  tests/stream/test_stream.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *  Copyright (C) 2009, 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example stream/test_stream.cpp
//! This is an example of how to use some basic algorithms from the
//! stream package. The example sorts characters of a string producing an
//! array of sorted tuples (character, index position).

#include <iostream>
#include <limits>
#include <vector>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/stream>
#include <stxxl/vector>

// comment if you want to use one 'sort' algorithm without producing
// intermediate sorted runs.
#define USE_FORMRUNS_N_MERGE

// comment if you want to use internal vectors as input/output of the algorithm
#define USE_EXTERNAL_ARRAY

#define block_size (8 * 1024)

using tuple_type = stxxl::tuple<char, int>;

namespace std {

std::ostream& operator << (std::ostream& os, const tuple_type& t)
{
    os << "<" << t.first << "," << t.second << ">";
    return os;
}

} // namespace std

#ifdef USE_EXTERNAL_ARRAY
using input_array_type = stxxl::vector<char>;
using output_array_type = stxxl::vector<tuple_type>;
#else
using input_array_type = std::vector<char>;
using output_array_type = std::vector<tuple_type>;
#endif

using stxxl::stream::streamify;
using stxxl::stream::streamify_traits;
using stxxl::stream::make_tuple;
using stxxl::tuple;

const char* phrase = "Hasta la vista, baby";

template <class Container_, class It_>
void fill_input_array(Container_& container, It_ p)
{
    while (*p)
    {
        container.push_back(*p);
        ++p;
    }
}

template <class ValTp>
struct counter
{
    using value_type = ValTp;

    value_type cnt;
    counter() : cnt(0) { }

    value_type operator () ()
    {
        value_type ret = cnt;
        ++cnt;
        return ret;
    }
};

using counter_type = counter<int>;

struct cmp_type : std::binary_function<tuple_type, tuple_type, bool>
{
    using value_type = tuple_type;
    bool operator () (const value_type& a, const value_type& b) const
    {
        if (a.first == b.first)
            return a.second < b.second;

        return a.first < b.first;
    }

    value_type min_value() const
    {
        return tuple_type::min_value();
    }
    value_type max_value() const
    {
        return tuple_type::max_value();
    }
};

struct cmp_int : std::binary_function<int, int, bool>
{
    using value_type = int;
    bool operator () (const value_type& a, const value_type& b) const
    {
        return a > b;
    }

    value_type max_value() const
    {
        return std::numeric_limits<value_type>::min();
    }
    value_type min_value() const
    {
        return std::numeric_limits<value_type>::max();
    }
};

template <typename T>
struct identity : std::unary_function<T, T>
{
    using value_type = T;

    const T& operator () (const T& t)
    {
        return t;
    }
};

int main()
{
    input_array_type input;
    output_array_type output;

    foxxll::stats* s = foxxll::stats::get_instance();

    std::cout << *s;

    fill_input_array(input, phrase);

    output.resize(input.size());

    // HERE streaming part begins (streamifying)
    // create input stream
    using input_stream_type = streamify_traits<input_array_type::iterator>::stream_type;
    input_stream_type input_stream = streamify(input.begin(), input.end());

    // create counter stream
    using counter_stream_type = stxxl::stream::generator2stream<counter_type>;
    counter_stream_type counter_stream = streamify(counter_type());

    // create tuple stream
    using tuple_stream_type = make_tuple<input_stream_type, counter_stream_type>;
    tuple_stream_type tuple_stream(input_stream, counter_stream);

    const uint64_t sorter_memory = 128 * 1024;
#ifdef USE_FORMRUNS_N_MERGE
    // sort tuples by character
    // 1. form runs
    using runs_creator_stream_type = stxxl::stream::runs_creator<tuple_stream_type, cmp_type, block_size>;
    runs_creator_stream_type runs_creator_stream(tuple_stream, cmp_type(), sorter_memory);
    // 2. merge runs
    using sorted_stream_type = stxxl::stream::runs_merger<runs_creator_stream_type::sorted_runs_type, cmp_type>;
    sorted_stream_type sorted_stream(runs_creator_stream.result(), cmp_type(), sorter_memory);
#else
    // sort tuples by character
    // (combination of the previous two steps in one algorithm: form runs and merge)
    using sorted_stream_type = stxxl::stream::sort<tuple_stream_type, cmp_type, block_size>;
    sorted_stream_type sorted_stream(tuple_stream, cmp_type(), sorter_memory);
#endif

    using transformed_stream_type = stxxl::stream::transform<identity<stxxl::tuple<char, int> >, sorted_stream_type>;
    identity<stxxl::tuple<char, int> > id;
    transformed_stream_type transformed_stream(id, sorted_stream);

    // HERE streaming part ends (materializing)
    output_array_type::iterator o = stxxl::stream::materialize(transformed_stream, output.begin(), output.end());
    // or materialize(sorted_stream,output.begin());
    die_unless(o == output.end());

    LOG1 << "input string (character,position) :";
    for (unsigned i = 0; i < input.size(); ++i)
    {
        LOG1 << "('" << input[i] << "'," << i << ")";
    }
    LOG1 << "sorted tuples (character,position):";
    for (unsigned i = 0; i < input.size(); ++i)
    {
        LOG1 << "('" << output[i].first << "'," << output[i].second << ")";
    }

    die_unless(output[0] == tuple_type(' ', 5));
    die_unless(output[1] == tuple_type(' ', 8));
    die_unless(output[2] == tuple_type(' ', 15));
    die_unless(output[3] == tuple_type(',', 14));
    die_unless(output[4] == tuple_type('H', 0));
    die_unless(output[5] == tuple_type('a', 1));
    die_unless(output[6] == tuple_type('a', 4));
    die_unless(output[7] == tuple_type('a', 7));
    die_unless(output[8] == tuple_type('a', 13));
    die_unless(output[9] == tuple_type('a', 17));
    die_unless(output[10] == tuple_type('b', 16));
    die_unless(output[11] == tuple_type('b', 18));
    die_unless(output[12] == tuple_type('i', 10));
    die_unless(output[13] == tuple_type('l', 6));
    die_unless(output[14] == tuple_type('s', 2));
    die_unless(output[15] == tuple_type('s', 11));
    die_unless(output[16] == tuple_type('t', 3));
    die_unless(output[17] == tuple_type('t', 12));
    die_unless(output[18] == tuple_type('v', 9));
    die_unless(output[19] == tuple_type('y', 19));
    die_unless(output.size() == 20);

    std::cout << *s;

    std::vector<int> InternalArray(1024 * 1024);
    std::sort(InternalArray.begin(), InternalArray.end(), cmp_int());
    //convenience function based on streaming
    stxxl::sort<1024* 1024>(InternalArray.begin(), InternalArray.end(),
                            cmp_int(), 1024 * 1024 * 31, foxxll::random_cyclic());

    return 0;
}
