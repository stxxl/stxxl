/***************************************************************************
 *  include/stxxl/bits/common/comparator.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2017 Manuel Penschuck <manuel@ae.cs.uni-frankfurt.de>
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

enum class direction {
    Less, Greater, DontCare
};

namespace comparator_details {

template <
    typename ValueType,
    typename Enabled,                 // this flag is for enable_if statements
    direction... Mode
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

template <typename>
struct enable_if_typed {
    using type = int;
};

// Tuple
template <
    typename T1,
    typename... TTs,
    direction M1,
    direction... Modes
    >
class comparator_impl<std::tuple<T1, TTs...>, void, M1, Modes...>
{
public:
    using simple_type = typename std::remove_reference<T1>::type;
    using value_type = std::tuple<simple_type, TTs...>;

// compare
    bool operator () (const value_type& a, const value_type& b) const
    {
        const auto& va = std::get<0>(a);
        const auto& vb = std::get<0>(b);

        if (M1 != direction::DontCare) {
            // lexicographical compare; if its equal fall trough to recursion
            if (va < vb) return (M1 == direction::Less);
            if (vb < va) return (M1 != direction::Less);
        }

        return recurse_cmp(
            tuple_slice<1, sizeof ... (TTs)+1>(a),
            tuple_slice<1, sizeof ... (TTs)+1>(b));
    }

// sentinels
    const value_type min_value() const
    {
        return limit_helper<false>();
    }

    const value_type max_value() const
    {
        return limit_helper<true>();
    }

    template <bool Max>
    const value_type limit_helper() const
    {
        static_assert(
            (M1 == direction::DontCare) ||
            (std::numeric_limits<simple_type>::is_specialized),
            "std::numeric_limits<> not specialized: Comparator cannot deduce min/max values");

        simple_type value = ((M1 == direction::Less) == Max)
                            ? std::numeric_limits<simple_type>::max()
                            : std::numeric_limits<simple_type>::min();

        return std::tuple_cat(
            std::tuple<simple_type>{ value },
            recurse_cmp.template limit_helper<Max>());
    }

private:
    comparator_impl<std::tuple<TTs...>, void, Modes...> recurse_cmp;
};

// Default Mode to direction::Less, if missing
template <typename T1, typename... TTs>
class comparator_impl<std::tuple<T1, TTs...>, void>
    : public comparator_impl<std::tuple<T1, TTs...>, void, direction::Less>
{ };

// Base case of recursion
template <>
class comparator_impl<std::tuple<>, void>
{
public:
    using value_type = std::tuple<>;

    value_type min_value() const
    {
        return { };
    }
    value_type max_value() const
    {
        return { };
    }
    bool operator () (const value_type&, const value_type&) const
    {
        return false;
    }

    template <bool Max>
    const value_type limit_helper() const
    {
        return { };
    }
};

// Pair
template <
    typename T1,
    typename T2,
    direction M1,
    direction M2
    >
class comparator_impl<std::pair<T1, T2>, void, M1, M2>
{
    using tuple_type = std::tuple<T1, T2>;
    using comparator_type = comparator_impl<tuple_type, void, M1, M2>;

public:
    using value_type = std::pair<T1, T2>;

    value_type min_value() const
    {
        auto val = comparator_type().min_value();
        return { std::get<0>(val), std::get<1>(val) };
    }

    value_type max_value() const
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
    direction M1
    >
class comparator_impl<std::pair<T1, T2>, void, M1>
    : public comparator_impl<std::pair<T1, T2>, void, M1, direction::Less>
{ };

template <
    typename T1,
    typename T2
    >
class comparator_impl<std::pair<T1, T2>, void>
    : public comparator_impl<std::pair<T1, T2>, void, direction::Less, direction::Less>
{ };

// Singletons
template <
    typename ValueType,
    direction Mode
    >
class comparator_impl<ValueType, typename std::enable_if<!is_tuple_or_pair<ValueType>::value>::type, Mode>
{
public:
    static_assert(Mode != direction::DontCare, "direction::DontCare is available only for collections as direction");

    using value_type = ValueType;

    bool operator () (const value_type& a, const value_type& b) const
    {
        return (Mode == direction::Less) ? (a < b) : (b < a);
    }

    value_type min_value() const
    {
        return (Mode != direction::Greater)
               ? own_or_derive_switch_min<ValueType>::min_value()
               : own_or_derive_switch_max<ValueType>::max_value();
    }

    value_type max_value() const
    {
        return (Mode != direction::Greater)
               ? own_or_derive_switch_max<ValueType>::max_value()
               : own_or_derive_switch_min<ValueType>::min_value();
    }

private:
    // Min Switch
    template <typename S>
    struct has_own_min_available
    {
        template <typename T>
        static auto sig_matches(T())->std::true_type;

        template <typename T>
        static auto sig_matches(const T())->std::true_type;

        template <typename T>
        static auto limit_exists(std::nullptr_t)->decltype(sig_matches<T>(&T::min_value));

        template <typename T>
        static auto limit_exists(...)->std::false_type;

        static constexpr int value = decltype(limit_exists<S>(nullptr))::value;
    };

    template <typename T, class X = void>
    struct own_or_derive_switch_min { };

    template <typename T>
    struct own_or_derive_switch_min<T, typename std::enable_if<!!has_own_min_available<T>::value>::type>{
        static value_type min_value()
        {
            return value_type::min_value();
        }
    };

    template <typename T>
    struct own_or_derive_switch_min<T, typename std::enable_if<!has_own_min_available<T>::value>::type>{
        static value_type min_value()
        {
            static_assert(std::numeric_limits<ValueType>::is_specialized,
                          "std::numeric_limits<> is not specialised for type. Provide static T T::min_value() and static T T::max_value()");

            return std::numeric_limits<ValueType>::min();
        }
    };

    // Max Switch
    template <typename S>
    struct has_own_max_available
    {
        template <typename T>
        static auto sig_matches(T())->std::true_type;

        template <typename T>
        static auto sig_matches(const T())->std::true_type;

        template <typename T>
        static auto limit_exists(std::nullptr_t)->decltype(sig_matches<T>(&T::max_value));

        template <typename T>
        static auto limit_exists(...)->std::false_type;

        static constexpr int value = decltype(limit_exists<S>(nullptr))::value;
    };

    template <typename T, class X = void>
    struct own_or_derive_switch_max { };

    template <typename T>
    struct own_or_derive_switch_max<T, typename std::enable_if<!!has_own_max_available<T>::value>::type>{
        static const value_type max_value()
        {
            return value_type::max_value();
        }
    };

    template <typename T>
    struct own_or_derive_switch_max<T, typename std::enable_if<!has_own_max_available<T>::value>::type>{
        static const value_type max_value()
        {
            static_assert(std::numeric_limits<ValueType>::is_specialized,
                          "std::numeric_limits<> is not specialised for type. Provide static T T::min_value() and static T T::max_value()");

            return std::numeric_limits<ValueType>::max();
        }
    };
};

template <typename ValueType>
class comparator_impl<ValueType, typename std::enable_if<!is_tuple_or_pair<ValueType>::value>::type>
    : public comparator_impl<ValueType, typename std::enable_if<!is_tuple_or_pair<ValueType>::value>::type, direction::Less>
{ };

} // namespace comparator_details

template <typename ValueType, direction... Modes>
class comparator
{
public:
    // use wrapper to hide defaulted template parameters used for SFINAE
    const ValueType min_value() const { return cmp.min_value(); }
    const ValueType max_value() const { return cmp.max_value(); }
    bool operator () (const ValueType& a, const ValueType& b) const
    {
        return cmp(a, b);
    }

private:
    comparator_details::comparator_impl<
        ValueType,
        void,     // this void is only for enable_if
        Modes...> cmp;
};

template <typename ValueType, typename KeyExtract, direction... Modes>
class struct_comparator
{
public:
    explicit struct_comparator(const KeyExtract keys = KeyExtract()) : keys(keys) { }

    ValueType min_value() const
    {
        ValueType result;
        keys(result) = impl.min_value();
        return result;
    }

    ValueType max_value() const
    {
        ValueType result;
        keys(result) = impl.max_value();
        return result;
    }

    bool operator () (const ValueType& a, const ValueType& b) const
    {
        return impl(keys(a), keys(b));
    }

private:
    // store key extractor
    KeyExtract keys;

    template <typename... Ts>
    using tuple_with_removed_refs = std::tuple<typename std::remove_reference<Ts>::type...>;

    template <typename... Ts>
    static tuple_with_removed_refs<Ts...> remove_ref_from_tuple_members(std::tuple<Ts...>)
    {
        return tuple_with_removed_refs<Ts...>();
    }

    using return_type = typename std::result_of<KeyExtract(ValueType&)>::type;
    using tuple_type = decltype(remove_ref_from_tuple_members(std::declval<return_type>()));

    comparator<tuple_type, Modes...> impl;

    template <typename... Ts>
    static tuple_with_removed_refs<Ts...> remove_ref_from_tuple_members(std::tuple<Ts...> const&)
    {
        return tuple_with_removed_refs<Ts...>();
    }
};

template <typename ValueType, direction... Modes, typename KeyExtract>
auto make_struct_comparator(const KeyExtract extract)
{
    return struct_comparator<ValueType, KeyExtract, Modes...>(extract);
}

} // namespace stxxl

/*!
 * Helper class to easily construct key-extractors and manual comparators
 * that can determine their min and max elements based on std::numeric_limits.
 *
 * \code
 * // example of an integer key-extractor
 * struct my_keyextract : public stxxl::numeric_limits_sentinels<uint64_t> {
 *     uint64_t operator() (const uint64_t x) {return x;}
 * };
 * \endcode
 *
 * \tparam ValueType    Type of comparator/key-extractor input
 * \tparam Less         Less <=> (min_value() < max_value())
 */
template <typename ValueType, bool Less = false>
struct numeric_limits_sentinels {
    static_assert(std::numeric_limits<ValueType>::is_specialized,
                  "Only type with a std::numeric_limits specialization are supported");

    using value_type = ValueType;

    value_type max_value() const
    {
        return Less
               ? std::numeric_limits<value_type>::max()
               : std::numeric_limits<value_type>::min();
    }

    value_type min_value() const
    {
        return Less
               ? std::numeric_limits<value_type>::min()
               : std::numeric_limits<value_type>::max();
    }
};

#endif // !STXXL_COMMON_COMPARATOR_HEADER
