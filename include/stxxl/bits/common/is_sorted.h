/***************************************************************************
 *  include/stxxl/bits/common/is_sorted.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_IS_SORTED_HEADER
#define STXXL_COMMON_IS_SORTED_HEADER

#include <functional>
#include <iterator>

namespace stxxl {

/*!
 * This function checks whether the items between the iterators are sorted
 * in non decreasingly (assuming comp is less). It is functionally identical
 * to std::is_sorted, however avoids needlessly copying iterators which can
 * be prohibitively expensive with complex EM iterators.
 *
 * \note
 * You should prefer using stxxl::is_sorted over std::is_sorted for STXXL
 * containers as stxxl::is_sorted may have datastructure-specific
 * overrides to increase performance.
 */
template <class ForwardIterator, class StrictWeakOrdering>
bool is_sorted(ForwardIterator first, ForwardIterator last,
               StrictWeakOrdering comp)
{
    if (first == last)
        return true;

    ForwardIterator next = first;
    for (++next; next != last; ++first, ++next) {
        if (comp(*next, *first))
            return false;
    }

    return true;
}

/*!
 * \see stxxl::is_sorted
 */
template <class ForwardIterator>
bool is_sorted(ForwardIterator first, ForwardIterator last)
{
    return stxxl::is_sorted(
        first, last,
        std::less<typename std::iterator_traits<ForwardIterator>
                  ::value_type>());
}

} // namespace stxxl

#endif // !STXXL_COMMON_IS_SORTED_HEADER
