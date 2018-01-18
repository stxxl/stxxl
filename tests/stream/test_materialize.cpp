/***************************************************************************
 *  tests/stream/test_materialize.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#define STXXL_DEFAULT_BLOCK_SIZE(T) 4096

#include <vector>

#include <tlx/die.hpp>

#include <stxxl/algorithm>
#include <stxxl/bits/defines.h>
#include <stxxl/stream>
#include <stxxl/vector>

struct forty_two
{
    using value_type = int;

    unsigned counter, length;

    explicit forty_two(unsigned l) : counter(0), length(l) { }

    bool empty() const { return !(counter < length); }

    unsigned len() const { return length; }

    value_type operator * ()
    {
        die_unless(!empty());
        return counter;
    }

    forty_two& operator ++ ()
    {
        die_unless(!empty());
        ++counter;
        return *this;
    }

    forty_two & reset()
    {
        counter = 0;
        return *this;
    }
};

/*
    template <typename OutputIterator, typename StreamAlgorithm>
    OutputIterator
    materialize(StreamAlgorithm& in, OutputIterator out);

    template <typename OutputIterator, typename StreamAlgorithm>
    OutputIterator
    materialize(StreamAlgorithm& in, OutputIterator outbegin, OutputIterator outend);

    template <typename StreamAlgorithm, typename VectorConfig>
    stxxl::vector_iterator<VectorConfig> materialize(
        StreamAlgorithm& in,
        stxxl::vector_iterator<VectorConfig> outbegin,
        stxxl::vector_iterator<VectorConfig> outend,
        size_t nbuffers = 0);

    template <typename StreamAlgorithm, typename VectorConfig>
    stxxl::vector_iterator<VectorConfig> materialize(
        StreamAlgorithm& in,
        stxxl::vector_iterator<VectorConfig> out,
        size_t nbuffers = 0);
*/

int generate_0()
{
    return 0;
}

template <typename VectorType>
void check_42_fill(VectorType& v, typename VectorType::size_type length)
{
    using value_type = typename VectorType::value_type;
    using size_type = typename VectorType::size_type;

    auto ci = v.cbegin();

    for (size_type i = 0; i < length; ++i)
    {
        die_unless(*ci == value_type(i));
        ++ci;
    }

    for (size_type i = length; i < v.size(); ++i)
    {
        die_unless(*ci == value_type(0));
        ++ci;
    }

    std::fill(v.begin(), v.end(), value_type(0));
}

int main()
{
    foxxll::config::get_instance();

    {
        forty_two _42(42);

        // materialize into std vector
        std::vector<int> v(1000);
        std::generate(v.begin(), v.end(), generate_0);

        stxxl::stream::materialize(_42.reset(), v.begin());
        check_42_fill(v, _42.len());

        stxxl::stream::materialize(_42.reset(), v.begin(), v.end());
        check_42_fill(v, _42.len());
    }
    {
        forty_two _42(42);

        // materialize into stxxl vector
        stxxl::vector<int> v(1000);
        stxxl::generate(v.begin(), v.end(), generate_0, 42);

        stxxl::stream::materialize(_42.reset(), v.begin());
        check_42_fill(v, _42.len());

        stxxl::stream::materialize(_42.reset(), v.begin(), 42);
        check_42_fill(v, _42.len());

        stxxl::stream::materialize(_42.reset(), v.begin(), v.end());
        check_42_fill(v, _42.len());

        stxxl::stream::materialize(_42.reset(), v.begin(), v.end(), 42);
        check_42_fill(v, _42.len());
    }
    {
        forty_two _42mill(42 * 10000);

        // materialize into larger stxxl vector (to cross block boundaries)
        stxxl::vector<int> v(60 * 10000);
        stxxl::generate(v.begin(), v.end(), generate_0, 42);

        stxxl::stream::materialize(_42mill.reset(), v.begin());
        check_42_fill(v, _42mill.len());

        stxxl::stream::materialize(_42mill.reset(), v.begin(), 42);
        check_42_fill(v, _42mill.len());

        stxxl::stream::materialize(_42mill.reset(), v.begin(), v.end());
        check_42_fill(v, _42mill.len());

        stxxl::stream::materialize(_42mill.reset(), v.begin(), v.end(), 42);
        check_42_fill(v, _42mill.len());
    }
}
