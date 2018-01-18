/***************************************************************************
 *  include/stxxl/bits/parallel/types.h
 *
 *  Basic typedefs.
 *  Extracted from MCSTL http://algo2.iti.uni-karlsruhe.de/singler/mcstl/
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_TYPES_HEADER
#define STXXL_PARALLEL_TYPES_HEADER

#include <cstdlib>

#include <foxxll/common/types.hpp>

namespace stxxl {
namespace parallel {

/**
 * Unsigned integer to index elements.
 * The total number of elements for each algorithm must fit into this type.
 */
using sequence_index_t = uint64_t;
/**
 * Unsigned integer to index a thread number.
 * The maximum thread number must fit into this type.
 */
using thread_index_t = int;
/**
 * Longest compare-and-swappable integer type on this platform.
 */
using lcas_t = int64_t;
/**
 * Number of bits of ::lcas_t.
 */
static const size_t lcas_t_bits = sizeof(lcas_t) * 8;
/**
 * ::lcas_t with the right half of bits set to 1.
 */
static const lcas_t lcas_t_mask = (((lcas_t)1 << (lcas_t_bits / 2)) - 1);

} // namespace parallel
} // namespace stxxl

#endif // !STXXL_PARALLEL_TYPES_HEADER
