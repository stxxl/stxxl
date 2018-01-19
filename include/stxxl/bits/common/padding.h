/***************************************************************************
 *  include/stxxl/bits/common/padding.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2018 Manuel Penschuck <manuel@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_PADDING_HEADER
#define STXXL_COMMON_PADDING_HEADER

#include <cstddef>

#include <algorithm>
#include <iostream>

namespace stxxl {

/*!
 * Helper structure to add a padding to arbitrary struts, e.g. to simulate a
 * payload for data items. Usually the dummy payload stays uninitialized; in
 * case STXXL_WITH_VALGRIND is define, the payload gets zeroed out to avoid
 * warning when using valgrind (or similar tools).
 *
 * \code
 * template<typename Key>
 * struct Item : stxxl::padding<20 - sizeof(Key)> {
 *    Key key;
 *    Item(Key key) : key(key) {}
 * };
 * static_assert(sizeof(Item) == 20, "This should be true");
 * \endcode
 *
 * \tparam Size  Has to be non-negative
 * \warning The compiler might add additional padding to achieve alignment.
 * Therefore, either consider Size as a lower bound or use the packed-attribute.
 */
// We ptrdiff_t here despite accepting only non-negative value for more
// verbose error messages in case, the computation of Size triggered an underflow
template <ptrdiff_t Size>
struct padding {
    static_assert(Size > 0, "Size has to be non-negative");

    uint8_t dummy[Size];

    padding()
    {
      #ifdef STXXL_WITH_VALGRIND
        std::fill(dummy, dummy + Size, 0);
      #endif
    }
};

template <>
struct padding<0>{
    padding() { }
};

} // namespace stxxl

#endif // !STXXL_COMMON_PADDING_HEADER
