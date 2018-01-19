/***************************************************************************
 *  tests/containers/test_dependency.cpp
 *
 *  tests/containers/test_ext_merger.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2017 Manuel Penschuck <manuel@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/deque>
#include <stxxl/map>
#include <stxxl/priority_queue>
#include <stxxl/queue>
#include <stxxl/sequence>
#include <stxxl/sorter>
#include <stxxl/unordered_map>
#include <stxxl/vector>

#include <iostream>

struct MinimalType {
    size_t a;

    explicit MinimalType(size_t a = 0) : a(a) { }
};

struct MinimalTypeWithEq : public MinimalType {
    size_t a;

    explicit MinimalTypeWithEq(size_t a = 0) : a(a) { }

    bool operator == (const MinimalType o) const
    {
        return a == o.a;
    }
};

template <typename T>
struct CompareLessWithMin {
    bool operator () (const T& a, const T& b) const
    {
        return a.a < b.a;
    }
    T min_value() const { return T { 0 }; }
};

template <typename T>
struct CompareLessWithMax {
    bool operator () (const T& a, const T& b) const
    {
        return a.a < b.a;
    }
    T max_value() const { return T { std::numeric_limits<size_t>::max() }; }
};

template <typename T>
struct CompareLessWithMinMax {
    bool operator () (const T& a, const T& b) const
    {
        return a.a < b.a;
    }

    T min_value() const { return T { 0 }; }
    T max_value() const { return T { std::numeric_limits<size_t>::max() }; }
};

template <typename T>
struct HashFunctor
{
    size_t operator () (const T& a) const
    {
        return a.a;
    }
};

template class stxxl::priority_queue<
        stxxl::priority_queue_config<MinimalType, CompareLessWithMin<MinimalType>, 32ul, 256ul, 16ul, 2ul, 16384ul, 16ul, 2ul, foxxll::random_cyclic>
        >;

template class stxxl::vector<MinimalType>;

template class stxxl::sorter<MinimalType, CompareLessWithMinMax<MinimalType> >;

template class stxxl::deque<MinimalType>;

template class stxxl::map<MinimalTypeWithEq, MinimalType, CompareLessWithMax<MinimalTypeWithEq>, 1024, 1024>;

template class stxxl::queue<MinimalType>;

template class stxxl::sequence<MinimalType>;

template class stxxl::unordered_map<
        MinimalType, MinimalType, HashFunctor<MinimalType>, CompareLessWithMinMax<MinimalType>, 1024, 128
        >;

int main()
{
    std::cout << "Nothing to see here" << std::endl;
    return 0;
}
