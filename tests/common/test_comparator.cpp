/***************************************************************************
 *  tests/common/test_comparator.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2017 Manuel Penschuck <manuel@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>

#include <stxxl/bits/common/comparator.h>

using stxxl::direction;

template <typename CompareT, typename T>
void test_singletons(CompareT cmp, std::vector<T> values, direction M)
{
    std::cout << "Test singletons" << std::endl;

    die_unless(cmp(cmp.min_value(), cmp.max_value()));

    size_t pos_compares = 0;
    size_t neg_compares = 0;

    for (auto i1 = values.cbegin(); i1 != values.cend(); ++i1) {
        die_unless(!cmp(*i1, *i1));
        die_unless(cmp(*i1, cmp.max_value()));
        die_unless(cmp(cmp.min_value(), *i1));

        for (auto i2 = values.cbegin(); i2 != values.cend(); ++i2) {
            const bool expected = (M == direction::Less) ? (*i1 < *i2) : (*i1 > *i2);
            const bool compared = cmp(*i1, *i2);

            pos_compares += compared;
            neg_compares += !compared;

            die_unless(expected == compared);
        }
    }

    die_unless(pos_compares + values.size() == neg_compares);
}

template <
    typename AggrT,
    typename T1, typename T2,
    direction M1,
    direction M2
    >
void test_two_int(const std::vector<T1>& aval, const std::vector<T2>& bval)
{
    stxxl::comparator<AggrT, M1, M2> cmp;

    die_unless(cmp(cmp.min_value(), cmp.max_value()));

    auto reference_compare = [](const AggrT& a, const AggrT& b) {
                                 if (M1 != direction::DontCare) {
                                     if (std::get<0>(a) < std::get<0>(b)) return (M1 == direction::Less);
                                     if (std::get<0>(a) > std::get<0>(b)) return (M1 != direction::Less);
                                 }
                                 if (M2 != direction::DontCare) {
                                     if (std::get<1>(a) < std::get<1>(b)) return (M2 == direction::Less);
                                     if (std::get<1>(a) > std::get<1>(b)) return (M2 != direction::Less);
                                 }

                                 return false;
                             };

    auto mode_to_str = [](direction mode) {
                           switch (mode) {
                           case direction::Less:    return " <  ";
                           case direction::Greater: return " >  ";
                           default:             return " dc ";
                           }
                       };

    size_t pos_compares = 0;
    size_t neg_compares = 0;

    for (auto a1 = aval.cbegin(); a1 != aval.cend(); ++a1) {
        for (auto b1 = bval.cbegin(); b1 != bval.cend(); ++b1) {
            const AggrT aggr1(*a1, *b1);

            die_unless(cmp(aggr1, cmp.max_value()));
            die_unless(cmp(cmp.min_value(), aggr1));

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

                    die_unless(expected == compared);
                }
            }
        }
    }

    die_unless(pos_compares > 0);
    die_unless(neg_compares > 0);
}

template <
    typename T1, typename T2,
    direction M1, direction M2>
void test_two(std::vector<T1> v1, std::vector<T2> v2)
{
    std::cout << "Test pair and tuple<X,Y>" << std::endl;

    test_two_int<std::pair<T1, T2>, T1, T2, M1, M2>(v1, v2);
    test_two_int<std::tuple<T1, T2>, T1, T2, M1, M2>(v1, v2);
}

// this function tests the usage of own min/max values
void test_own_implementation()
{
    std::cout << "Test own implementation of min/max" << std::endl;

    struct my_type {
        int a;

        explicit my_type(int a = 0) : a(a) { }

        bool operator < (const my_type& o) const
        {
            return a > o.a; // THIS IS THE WRONG WAY
        }

        static const my_type min_value()
        {
            return my_type { -1000 };
        }

        static my_type max_value()
        {
            return my_type { 1000 };
        }
    };

    {
        stxxl::comparator<my_type> cmp;
        die_unless(cmp.min_value().a == -1000);
        die_unless(cmp.max_value().a == 1000);

        // compare is inverted due to own less implementation
        die_unless(cmp(cmp.max_value(), cmp.min_value()));
        die_unless(!cmp(cmp.min_value(), cmp.max_value()));
    }

    {
        stxxl::comparator<my_type, direction::Greater> cmp;
        die_unless(cmp.min_value().a == 1000);
        die_unless(cmp.max_value().a == -1000);

        // compare is inverted due to own less implementation
        die_unless(cmp(cmp.max_value(), cmp.min_value()));
        die_unless(!cmp(cmp.min_value(), cmp.max_value()));
    }
}

void test_comparator_extract()
{
    std::cout << "Test structured comparator" << std::endl;

    struct my_type {
        explicit my_type(int a = 0, double b = 0.0, float c = 0.0)
            : keyA(a), keyB(b), keyC(c)
        { }

        int keyA;
        double keyB;
        float keyC;

        double val;

        void dump()
        {
            std::cout
                << "[a: " << keyA
                << ", b: " << keyB
                << ", c: " << keyC
                << ", val: " << val
                << "]" << std::endl;
        }
    };

    const auto cmp = stxxl::make_struct_comparator<my_type, direction::Less, direction::Greater>(
        [](auto& o) { return std::tie(o.keyA, o.keyB, o.keyC); });

    std::vector<my_type> values({
                                    my_type { -1, 0.0, 0.0 },
                                    my_type { -1, 0.0, 1.0 },
                                    my_type { 0, 2.0, 0.0 },
                                    my_type { 0, -1.0, 2.0 },
                                    my_type { 1, -10.0, 0.0 }
                                });

    for (auto i1 = values.cbegin(); i1 != values.cend(); ++i1) {
        die_unless(!cmp(*i1, *i1));
        die_unless(cmp(*i1, cmp.max_value()));
        die_unless(cmp(cmp.min_value(), *i1));

        for (auto i2 = i1 + 1; i2 != values.cend(); ++i2) {
            die_unless(cmp(*i1, *i2));
            die_unless(!cmp(*i2, *i1));
        }
    }
}

int main()
{
    const std::vector<int> int_values({ -5, -1, 0, 1, 5 });
    const std::vector<char> char_values({ 'a', 'b', 'c', 'd', 'e' });

    test_singletons(stxxl::comparator<int, direction::Less>(), int_values, direction::Less);
    test_singletons(stxxl::comparator<int, direction::Greater>(), int_values, direction::Greater);

    test_two<int, char, direction::Less, direction::Less>(int_values, char_values);
    test_two<int, char, direction::Less, direction::Greater>(int_values, char_values);
    test_two<int, char, direction::Greater, direction::Less>(int_values, char_values);
    test_two<int, char, direction::Greater, direction::Greater>(int_values, char_values);

    test_two<int, char, direction::Less, direction::DontCare>(int_values, char_values);
    test_two<int, char, direction::Greater, direction::DontCare>(int_values, char_values);
    test_two<int, char, direction::DontCare, direction::Less>(int_values, char_values);
    test_two<int, char, direction::DontCare, direction::Greater>(int_values, char_values);

    test_own_implementation();
    test_comparator_extract();

    std::cout << "Success." << std::endl;
    return 0;
}
