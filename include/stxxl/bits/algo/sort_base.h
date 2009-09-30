/***************************************************************************
 *  include/stxxl/bits/algo/sort_base.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SORT_BASE_HEADER
#define STXXL_SORT_BASE_HEADER

#ifndef STXXL_SORT_OPTIMAL_PREFETCHING
#define STXXL_SORT_OPTIMAL_PREFETCHING 1
#endif

#ifndef STXXL_CHECK_ORDER_IN_SORTS
#define STXXL_CHECK_ORDER_IN_SORTS 0
#endif

#ifndef STXXL_L2_SIZE
#define STXXL_L2_SIZE  (512 * 1024)
#endif

#endif // !STXXL_SORT_BASE_HEADER
// vim: et:ts=4:sw=4
