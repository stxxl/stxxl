/***************************************************************************
 *  include/stxxl/bits/algo/async_schedule.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

// Implements the "prudent prefetching" as described in
// D. Hutchinson, P. Sanders, J. S. Vitter: Duality between prefetching
// and queued writing on parallel disks, 2005
// DOI: 10.1137/S0097539703431573


#ifndef STXXL_ASYNC_SCHEDULE_HEADER
#define STXXL_ASYNC_SCHEDULE_HEADER

#include <stxxl/bits/common/types.h>


__STXXL_BEGIN_NAMESPACE

void compute_prefetch_schedule(
    const int_type * first,
    const int_type * last,
    int_type * out_first,
    int_type m,
    int_type D);

inline void compute_prefetch_schedule(
    int_type * first,
    int_type * last,
    int_type * out_first,
    int_type m,
    int_type D)
{
    compute_prefetch_schedule(static_cast<const int_type *>(first), last, out_first, m, D);
}

template <typename run_type>
void compute_prefetch_schedule(
    const run_type & input,
    int_type * out_first,
    int_type m,
    int_type D)
{
    const int_type L = input.size();
    int_type * disks = new int_type[L];
    for (int_type i = 0; i < L; ++i)
        disks[i] = input[i].bid.storage->get_id();
    compute_prefetch_schedule(disks, disks + L, out_first, m, D);
    delete[] disks;
}

template <typename bid_iterator_type>
void compute_prefetch_schedule(
    bid_iterator_type input_begin,
    bid_iterator_type input_end,
    int_type * out_first,
    int_type m,
    int_type D)
{
    const int_type L = input_end - input_begin;
    int_type * disks = new int_type[L];
    int_type i = 0;
    for (bid_iterator_type it = input_begin; it != input_end; ++it, ++i)
        disks[i] = it->storage->get_id();
    compute_prefetch_schedule(disks, disks + L, out_first, m, D);
    delete[] disks;
}

__STXXL_END_NAMESPACE

#endif // !STXXL_ASYNC_SCHEDULE_HEADER
// vim: et:ts=4:sw=4
