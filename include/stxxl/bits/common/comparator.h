/***************************************************************************
 *  include/stxxl/bits/common/comparator.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2017 Manuel Penschuck
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_COMPARATOR_HEADER
#define STXXL_COMMON_COMPARATOR_HEADER

#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stxxl {

enum compare_mode {
    Less, Greater, DontCare
};

namespace comparator_details {

template <
    typename ValueType,
    typename Enabled,                 // this flag is for enable_if statements
    compare_mode... Mode
    >
class comparator_impl;

// Helper stuff
template <std::size_t Ofst, class Tuple, std::size_t... I>
constexpr auto slice_impl(Tuple&& t, std::index_sequence<I...>)
{
    return std::forward_as_tuple(std::get<I + Ofst>(std::forward<Tuple>(t)) ...);
}

template <std::size_t I1, std::size_t I2, class Cont>
constexpr auto tuple_slice(Cont&& t)
{
    static_assert(I2 >= I1, "invalid slice");
    static_assert(std::tuple_size<std::decay_t<Cont> >::value >= I2,
                  "slice index out of bounds");

    return slice_impl<I1>(std::forward<Cont>(t), std::make_index_sequence<I2 - I1>{ });
}

template <typename T>
struct is_tuple_or_pair {
    static constexpr bool value = false;
};
template <typename... Ts>
struct is_tuple_or_pair<std::tuple<Ts...> >{
    static constexpr bool value = true;
};
template <typename T1, typename T2>
struct is_tuple_or_pair<std::pair<T1, T2> >{
    static constexpr bool value = true;
};

// Tuple
template <
    typename T1,
    typename... TTs,
    compare_mode M1,
    compare_mode... Modes
    >
class comparator_impl<std::tuple<T1, TTs...>, void, M1, Modes...>
{
public:
    using simpleType = typename std::remove_reference<T1>::type;
    using value_type = std::tuple<simpleType, TTs...>;

// compare
    bool operator () (const value_type& a, const value_type& b) const
    {
        const auto& va = std::get<0>(a);
        const auto& vb = std::get<0>(b);

        if (M1 != DontCare) {
            if (va < vb) return (M1 == Less);
            if (vb < va) return (M1 != Less);
        }

        comparator_impl<std::tuple<TTs...>, void, Modes...> cmp;
        return cmp(
            tuple_slice<1, sizeof ... (TTs)+1>(a),
            tuple_slice<1, sizeof ... (TTs)+1>(b));
    }

// sentinels
    constexpr value_type min_value() const
    {
        return limit_helper<false>();
    }

    constexpr value_type max_value() const
    {
        return limit_helper<true>();
    }

    template <bool Max>
    constexpr value_type limit_helper() const
    {
        simpleType value = ((M1 == Less) == Max)
                           ? std::numeric_limits<simpleType>::max()
                           : std::numeric_limits<simpleType>::min();

        return std::tuple_cat(
            std::tuple<simpleType>{ value },
            comparator_impl<std::tuple<TTs...>, void, Modes...>().limit_helper<Max>());
    }
};

// Default Mode to Less, if missing
template <typename T1, typename... TTs>
class comparator_impl<std::tuple<T1, TTs...>, void>
    : public comparator_impl<std::tuple<T1, TTs...>, void, Less>
{ };

// Base case of recursion
template <>
class comparator_impl<std::tuple<>, void>
{
public:
    using value_type = std::tuple<>;

    constexpr value_type min_value() const
    {
        return { };
    }
    constexpr value_type max_value() const
    {
        return { };
    }
    bool operator () (const value_type&, const value_type&) const
    {
        return false;
    }

    template <bool>
    constexpr value_type limit_helper() const
    {
        return { };
    }
};

// Pair
template <
    typename T1,
    typename T2,
    compare_mode M1,
    compare_mode M2
    >
class comparator_impl<std::pair<T1, T2>, void, M1, M2>
{
    using tuple_type = std::tuple<T1, T2>;
    using comparator_type = comparator_impl<tuple_type, void, M1, M2>;

public:
    using value_type = std::pair<T1, T2>;

    constexpr value_type min_value() const
    {
        auto val = comparator_type().min_value();
        return { std::get<0>(val), std::get<1>(val) };
    }

    constexpr value_type max_value() const
    {
        auto val = comparator_type().max_value();
        return { std::get<0>(val), std::get<1>(val) };
    }

    bool operator () (const value_type& a, const value_type& b) const
    {
        return comparator_type()(
            tuple_type { a.first, a.second },
            tuple_type { b.first, b.second });
    }
};

template <
    typename T1,
    typename T2,
    compare_mode M1
    >
class comparator_impl<std::pair<T1, T2>, void, M1>
    : public comparator_impl<std::pair<T1, T2>, void, M1, Less>
{ };

template <
    typename T1,
    typename T2
    >
class comparator_impl<std::pair<T1, T2>, void>
    : public comparator_impl<std::pair<T1, T2>, void, Less, Less>
{ };

// Singletons
template <
    typename ValueType,
    compare_mode Mode
    >
class comparator_impl<ValueType, typename std::enable_if<!is_tuple_or_pair<ValueType>::value>::type, Mode>
{
public:
    static_assert(Mode != DontCare, "DontCare is available only for collections as compare_mode");

    using value_type = ValueType;

    bool operator () (const value_type& a, const value_type& b) const
    {
        return (Mode == Less) ? (a < b) : (b < a);
    }

    value_type min_value() const
    {
        return (Mode == Less)
               ? std::numeric_limits<ValueType>::min()
               : std::numeric_limits<ValueType>::max();
    }

    value_type max_value() const
    {
        return (Mode == Less)
               ? std::numeric_limits<ValueType>::max()
               : std::numeric_limits<ValueType>::min();
    }
};

} // namespace comparator_details

template <typename ValueType, compare_mode... Modes>
class comparator : public comparator_details::comparator_impl<ValueType, void, Modes...>
{ };

} // namespace stxxl

#endif // !STXXL_COMMON_COMPARATOR_HEADER
