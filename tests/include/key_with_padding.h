/***************************************************************************
 *  tests/include/key_with_padding.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2018 Manuel Penschuck <manuel@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_TESTS_KEYWITHPADDING_HEADER
#define STXXL_TESTS_KEYWITHPADDING_HEADER

#include <cassert>
#include <iostream>
#include <limits>

#include <stxxl/bits/common/comparator.h>
#include <stxxl/bits/common/padding.h>

template <typename KeyType>
struct key_with_padding_base {
    using key_type = KeyType;

    key_type key;

    key_with_padding_base() { }
    explicit key_with_padding_base(KeyType key) : key(key) { }

    bool operator < (const key_with_padding_base& o) const { return key < o.key; }
    bool operator <= (const key_with_padding_base& o) const { return key <= o.key; }
    bool operator == (const key_with_padding_base& o) const { return key == o.key; }
    bool operator != (const key_with_padding_base& o) const { return key != o.key; }
    bool operator > (const key_with_padding_base& o) const { return key > o.key; }
    bool operator >= (const key_with_padding_base& o) const { return key >= o.key; }

    // Make sure we are printable
    friend std::ostream& operator << (std::ostream& os, const key_with_padding_base& k)
    {
        return os << k.key;
    }
};

/*!
 * A struct exposing a single key value, a key-extract mechanism for STXXL's
 * key-based sorting algorithms, comparators for STXXL's compare based structures
 * as well as overloads for ostream and numeric_limits
 *
 * \tparam KeyType the type of the key-attribute (and all derived values)
 * \tparam Size the minimal size in bytes this data structure should have
 * \tparam If true, there also is an key_copy attribute
 *
 * \warning The compiler may add additional padding to allow for aligned storage.
 * It is quite likely that sizeof(key_with_padding<uint64_t, 9>) == 16. See also
 * packed-attribute
 */
template <typename KeyType, size_t Size, bool IncludeKeyCopy = false>
struct key_with_padding;

template <typename KeyType, size_t Size>
struct key_with_padding<KeyType, Size, false>
    : stxxl::padding<Size - sizeof(KeyType)>,
      public key_with_padding_base<KeyType>
{
    key_with_padding() { }
    explicit key_with_padding(KeyType key) : key_with_padding_base<KeyType>(key) { }

    // To be used with key-based algorithms
    struct key_extract : public numeric_limits_sentinels<key_with_padding>{
        using key_type = KeyType;
        key_type operator () (const key_with_padding& k) const { return k.key; }
    };

    // To be used with comparison-based algorithms
    using compare_less = stxxl::comparator<key_with_padding, stxxl::direction::Less>;
    using compare_greater = stxxl::comparator<key_with_padding, stxxl::direction::Greater>;
};

template <typename KeyType, size_t Size>
struct key_with_padding<KeyType, Size, true>
    : stxxl::padding<Size - 2* sizeof(KeyType)>,
      public key_with_padding_base<KeyType>
{
    KeyType key_copy;

    key_with_padding() { }
    explicit key_with_padding(KeyType key) : key_with_padding_base<KeyType>(key), key_copy(key) { }

    // To be used with key-based algorithms
    struct key_extract : public numeric_limits_sentinels<key_with_padding>{
        using key_type = KeyType;
        key_type operator () (const key_with_padding& k) const { return k.key; }
    };

    // To be used with comparison-based algorithms
    using compare_less = stxxl::comparator<key_with_padding, stxxl::direction::Less>;
    using compare_greater = stxxl::comparator<key_with_padding, stxxl::direction::Greater>;
};

namespace std {

template <typename KeyType>
struct numeric_limits<key_with_padding_base<KeyType> >{
    using value_type = key_with_padding_base<KeyType>;

    static constexpr bool is_specialized = numeric_limits<KeyType>::is_specialized;

    static constexpr value_type min() { return value_type { numeric_limits<KeyType>::min() }; }
    static constexpr value_type max() { return value_type { numeric_limits<KeyType>::max() }; }
};

template <typename KeyType, size_t Size, bool IncludeKeyCopy>
struct numeric_limits<key_with_padding<KeyType, Size, IncludeKeyCopy> >{
    using value_type = key_with_padding<KeyType, Size, IncludeKeyCopy>;

    static constexpr bool is_specialized = numeric_limits<KeyType>::is_specialized;

    static constexpr value_type min() { return value_type { numeric_limits<KeyType>::min() }; }
    static constexpr value_type max() { return value_type { numeric_limits<KeyType>::max() }; }
};

} // namespace std

static_assert(sizeof(key_with_padding<uint8_t, 1, false>) == 1, "size-mismatch in key_with_padding");
static_assert(sizeof(key_with_padding<uint8_t, 2, false>) == 2, "size-mismatch in key_with_padding");
static_assert(sizeof(key_with_padding<uint8_t, 2, true>) == 2, "size-mismatch in key_with_padding");
static_assert(sizeof(key_with_padding<uint8_t, 3, false>) == 3, "size-mismatch in key_with_padding");
static_assert(sizeof(key_with_padding<uint8_t, 3, true>) == 3, "size-mismatch in key_with_padding");

#endif // STXXL_TESTS_KEYWITHPADDING_HEADER
