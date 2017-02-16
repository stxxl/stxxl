/***************************************************************************
 *  tests/common/test_comparator.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2017 Manuel Penschuck <manuel@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/common/comparator.h>
#include <stxxl/bits/verbose.h>

#include <cstdint>
#include <vector>
#include <algorithm>

template <typename T, stxxl::compare_mode M>
void test_singletons(std::vector<T> values)
{
    stxxl::comparator<T, M> cmp;

    STXXL_CHECK(cmp(cmp.min_value(), cmp.max_value()));

    size_t pos_compares = 0;
    size_t neg_compares = 0;

    for (auto i1 = values.cbegin(); i1 != values.cend(); ++i1) {
        STXXL_CHECK(!cmp(*i1, *i1));
        STXXL_CHECK(cmp(*i1, cmp.max_value()));
        STXXL_CHECK(cmp(cmp.min_value(), *i1));

        for (auto i2 = values.cbegin(); i2 != values.cend(); ++i2) {
            const bool expected = (M == stxxl::Less) ? (*i1 < *i2) : (*i1 > *i2);
            const bool compared = cmp(*i1, *i2);

            pos_compares += compared;
            neg_compares += !compared;

            STXXL_CHECK(expected == compared);
        }
    }

    STXXL_CHECK(pos_compares + values.size() == neg_compares);
}

template <
    typename AggrT,
    typename T1, typename T2,
    stxxl::compare_mode M1,
    stxxl::compare_mode M2
    >
void test_two_int(const std::vector<T1>& aval, const std::vector<T2>& bval)
{
    stxxl::comparator<AggrT, M1, M2> cmp;

    STXXL_CHECK(cmp(cmp.min_value(), cmp.max_value()));

    auto reference_compare = [](const AggrT& a, const AggrT& b) {
                                 if (M1 != stxxl::DontCare) {
                                     if (std::get<0>(a) < std::get<0>(b)) return (M1 == stxxl::Less);
                                     if (std::get<0>(a) > std::get<0>(b)) return (M1 != stxxl::Less);
                                 }
                                 if (M2 != stxxl::DontCare) {
                                     if (std::get<1>(a) < std::get<1>(b)) return (M2 == stxxl::Less);
                                     if (std::get<1>(a) > std::get<1>(b)) return (M2 != stxxl::Less);
                                 }

                                 return false;
                             };

    auto mode_to_str = [](stxxl::compare_mode mode) {
                           switch (mode) {
                           case stxxl::Less:    return " <  ";
                           case stxxl::Greater: return " >  ";
                           default:             return " dc ";
                           }
                       };

    size_t pos_compares = 0;
    size_t neg_compares = 0;

    for (auto a1 = aval.cbegin(); a1 != aval.cend(); ++a1) {
        for (auto b1 = bval.cbegin(); b1 != bval.cend(); ++b1) {
            const AggrT aggr1(*a1, *b1);

            STXXL_CHECK(cmp(aggr1, cmp.max_value()));
            STXXL_CHECK(cmp(cmp.min_value(), aggr1));

            for (auto a2 = aval.cbegin(); a2 != aval.cend(); ++a2) {
                for (auto b2 = bval.cbegin(); b2 != bval.cend(); ++b2) {
                    const AggrT aggr2(*a2, *b2);

                    bool expected = reference_compare(aggr1, aggr2);
                    bool compared = cmp(aggr1, aggr2);

                    pos_compares += compared;
                    neg_compares += !compared;

                    if (0)
                        std::cout << *a1 << mode_to_str(M1) << *a2 << " and "
                                  << *b1 << mode_to_str(M2) << *b2
                                  << " Exp: " << expected
                                  << " Got: " << compared
                                  << std::endl;

                    STXXL_CHECK(expected == compared);
                }
            }
        }
    }

    STXXL_CHECK(pos_compares > 0);
    STXXL_CHECK(neg_compares > 0);
}

template <
    typename T1, typename T2,
    stxxl::compare_mode M1, stxxl::compare_mode M2>
void test_two(std::vector<T1> v1, std::vector<T2> v2)
{
    test_two_int<std::pair<T1, T2>, T1, T2, M1, M2>(v1, v2);
    test_two_int<std::tuple<T1, T2>, T1, T2, M1, M2>(v1, v2);
}

int main()
{
    const std::vector<int> int_values({ -5, -1, 0, 1, 5 });
    const std::vector<char> char_values({ 'a', 'b', 'c', 'd', 'e' });

    test_singletons<int, stxxl::Less>(int_values);
    test_singletons<int, stxxl::Greater>(int_values);

    test_two<int, char, stxxl::Less, stxxl::Less>(int_values, char_values);
    test_two<int, char, stxxl::Less, stxxl::Greater>(int_values, char_values);
    test_two<int, char, stxxl::Greater, stxxl::Less>(int_values, char_values);
    test_two<int, char, stxxl::Greater, stxxl::Greater>(int_values, char_values);

    test_two<int, char, stxxl::Less, stxxl::DontCare>(int_values, char_values);
    test_two<int, char, stxxl::Greater, stxxl::DontCare>(int_values, char_values);
    test_two<int, char, stxxl::DontCare, stxxl::Less>(int_values, char_values);
    test_two<int, char, stxxl::DontCare, stxxl::Greater>(int_values, char_values);

    std::cout << "Success." << std::endl;
    return 0;
}
