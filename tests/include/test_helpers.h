/***************************************************************************
 *  tests/include/test_helpers.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2018 Manuel Penschuck <manuel@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_TESTS_TEST_HELPERS_HEADER
#define STXXL_TESTS_TEST_HELPERS_HEADER

#include <algorithm>
#include <random>

#include <foxxll/io/iostats.hpp>
#include <foxxll/common/timer.hpp>

#include <stxxl/scan>
#include <stxxl/vector>

template <typename Vector, typename Mapper, typename ValueType = typename Vector::value_type>
static void random_fill_vector(Vector& vector,
                               Mapper map,
                               uint64_t seed = 0)
{
    foxxll::scoped_print_iostats stats("Generation", vector.size() * sizeof(ValueType));

    std::mt19937_64 randgen(seed);
    std::uniform_int_distribution<uint64_t> distr;

    stxxl::generate(vector.begin(), vector.end(),
                    [&map, &randgen, &distr]() -> ValueType {
                        return map(distr(randgen));
                    });
}

template <typename ValueType, typename Mapper>
static void random_fill_vector(std::vector<ValueType>& vector,
                               Mapper map,
                               uint64_t seed = 0)
{
    foxxll::scoped_print_timer stats("Generation", vector.size() * sizeof(ValueType));

    std::mt19937_64 randgen(seed);
    std::uniform_int_distribution<uint64_t> distr;

    std::generate(vector.begin(), vector.end(),
                    [&map, &randgen, &distr]() -> ValueType {
                        return map(distr(randgen));
                    });
}

template <typename Vector, typename ValueType = typename Vector::value_type>
static void random_fill_vector(Vector& vector, uint64_t seed = 0) {
    random_fill_vector(vector, [](uint64_t x) { return ValueType(x); }, seed);
}

#endif //
