/***************************************************************************
 *  include/stxxl/bits/common/types.h
 *
 *  Part of the STXXL. See http://stxxl.org
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

#include <cstddef>
#include <cstdint>
#include <stxxl/bits/config.h>
#include <type_traits>

namespace stxxl {

using int_type = std::make_signed<size_t>::type;

typedef uint64_t external_size_type;       // may require external memory
typedef int64_t external_diff_type;        // may require external memory

//! Return the given value casted to the corresponding unsigned type
template <typename Integral>
typename std::make_unsigned<Integral>::type as_unsigned(Integral value)
{
    return static_cast<typename std::make_unsigned<Integral>::type>(value);
}

//! Return the given value casted to the corresponding signed type
template <typename Integral>
typename std::make_signed<Integral>::type as_signed(Integral value)
{
    return static_cast<typename std::make_signed<Integral>::type>(value);
}

} // namespace stxxl

#endif // !STXXL_COMMON_TYPES_HEADER
