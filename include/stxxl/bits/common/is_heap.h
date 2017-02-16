/***************************************************************************
 *  include/stxxl/bits/common/is_heap.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2015 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_IS_HEAP_HEADER
#define STXXL_COMMON_IS_HEAP_HEADER

#include <functional>
#include <iterator>

namespace stxxl {

template <typename RandomAccessIterator, typename StrictWeakOrdering>
bool is_heap(
    RandomAccessIterator first, RandomAccessIterator last,
    StrictWeakOrdering comp =
        std::less<typename std::iterator_traits<RandomAccessIterator>
                  ::value_type>())
{
    typedef typename std::iterator_traits<RandomAccessIterator>
        ::difference_type diff_type;

    if (first == last) return true;

    RandomAccessIterator parent = first;

    diff_type num = 1;
    for (RandomAccessIterator child = first + 1; child != last; ++child, ++num)
    {
        if (comp(*parent, *child)) // parent < child -> max-heap violated
            return false;

        if ((num & 1) == 0)
            ++parent;
    }

    return true;
}

} // namespace stxxl

#endif // !STXXL_COMMON_IS_HEAP_HEADER
