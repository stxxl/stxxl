/***************************************************************************
 *  include/stxxl/bits/parallel.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_HEADER
#define STXXL_PARALLEL_HEADER


#undef STXXL_PARALLEL
#if defined(_GLIBCXX_PARALLEL) || defined (__MCSTL__)
#define STXXL_PARALLEL 1
#else
#define STXXL_PARALLEL 0
#endif


#include <cassert>

#ifdef _GLIBCXX_PARALLEL
 #include <omp.h>
#endif

#ifdef __MCSTL__
 #include <mcstl.h>
 #include <bits/mcstl_multiway_merge.h>
#endif

#if STXXL_PARALLEL
 #include <algorithm>
#endif

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/settings.h>


#if !STXXL_PARALLEL
#undef STXXL_PARALLEL_MULTIWAY_MERGE
#endif


__STXXL_BEGIN_NAMESPACE

inline bool do_parallel_merge()
{
#if STXXL_PARALLEL && defined(STXXL_PARALLEL_MULTIWAY_MERGE) && defined(_GLIBCXX_PARALLEL)
    return !stxxl::SETTINGS::native_merge && omp_get_max_threads() >= 1;
#elif STXXL_PARALLEL && defined(STXXL_PARALLEL_MULTIWAY_MERGE) && defined(__MCSTL__)
    return !stxxl::SETTINGS::native_merge && mcstl::SETTINGS::num_threads >= 1;
#else
    return false;
#endif
}


namespace parallel
{

#if STXXL_PARALLEL

/** @brief Multi-way merging dispatcher.
 *  @param seqs_begin Begin iterator of iterator pair input sequence.
 *  @param seqs_end End iterator of iterator pair input sequence.
 *  @param target Begin iterator out output sequence.
 *  @param comp Comparator.
 *  @param length Maximum length to merge.
 *  @param stable Stable merging incurs a performance penalty.
 *  @return End iterator of output sequence. */
template <typename RandomAccessIteratorPairIterator,
          typename RandomAccessIterator3, typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge(RandomAccessIteratorPairIterator seqs_begin,
        RandomAccessIteratorPairIterator seqs_end,
        RandomAccessIterator3 target,
        Comparator comp,
        DiffType length,
        bool stable)
{
#if defined(_GLIBCXX_PARALLEL)
    return __gnu_parallel::multiway_merge(seqs_begin, seqs_end, target, comp, length, stable);
#elif defined(__MCSTL__)
    return mcstl::multiway_merge(seqs_begin, seqs_end, target, comp, length, stable);
#else
    assert(0);
#endif
}

#endif

}

__STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_HEADER

