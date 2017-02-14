/***************************************************************************
 *  include/stxxl/bits/common/types.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_TYPES_HEADER
#define STXXL_COMMON_TYPES_HEADER

#include <stxxl/bits/config.h>
#include <stxxl/bits/namespace.h>
#include <cstdint>
#include <type_traits>
#include <cstddef>

STXXL_BEGIN_NAMESPACE

// will be deprecated soon
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

// integer types declarations
enum { my_pointer_size = sizeof(void*) };

template <int PtrSize>
struct choose_int_types
{ };

template <>
struct choose_int_types<4>  // for 32-bit processors/compilers
{
    typedef int32 int_type;
    typedef uint32 unsigned_type;
};

template <>
struct choose_int_types<8>  // for 64-bit processors/compilers
{
    typedef int64 int_type;
    typedef uint64 unsigned_type;
};

typedef choose_int_types<my_pointer_size>::int_type int_type;
typedef choose_int_types<my_pointer_size>::unsigned_type unsigned_type;

typedef unsigned_type internal_size_type;  // fits in internal memory

typedef uint64_t external_size_type;         // may require external memory
typedef int64_t  external_diff_type;         // may require external memory

//! Return the given value casted to the corresponding unsigned type
template <typename Integral>
typename std::make_unsigned<Integral>::type as_unsigned(Integral value) {
    return static_cast<typename std::make_unsigned<Integral>::type>(value);
}

//! Return the given value casted to the corresponding signed type
template <typename Integral>
typename std::make_signed<Integral>::type as_signed(Integral value) {
    return static_cast<typename std::make_signed<Integral>::type>(value);
}

STXXL_END_NAMESPACE

#endif // !STXXL_COMMON_TYPES_HEADER
