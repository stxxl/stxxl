/***************************************************************************
 *  include/stxxl/bits/common/is_sorted.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IS_SORTED_HEADER
#define STXXL_IS_SORTED_HEADER

#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

template <class _ForwardIter>
bool is_sorted_helper(_ForwardIter __first, _ForwardIter __last)
{
    if (__first == __last)
        return true;

    _ForwardIter __next = __first;
    for (++__next; __next != __last; __first = __next, ++__next) {
        if (*__next < *__first)
            return false;
    }

    return true;
}

template <class _ForwardIter, class _StrictWeakOrdering>
bool is_sorted_helper(_ForwardIter __first, _ForwardIter __last,
                      _StrictWeakOrdering __comp)
{
    if (__first == __last)
        return true;

    _ForwardIter __next = __first;
    for (++__next; __next != __last; __first = __next, ++__next) {
        if (__comp(*__next, *__first))
            return false;
    }

    return true;
}

template <class _ForwardIter>
bool is_sorted(_ForwardIter __first, _ForwardIter __last)
{
    return is_sorted_helper(__first, __last);
}

template <class _ForwardIter, class _StrictWeakOrdering>
bool is_sorted(_ForwardIter __first, _ForwardIter __last,
               _StrictWeakOrdering __comp)
{
    return is_sorted_helper(__first, __last, __comp);
}

__STXXL_END_NAMESPACE

#endif // !STXXL_IS_SORTED_HEADER
