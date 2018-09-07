/***************************************************************************
 *  include/stxxl/bits/parallel.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2015 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_HEADER
#define STXXL_PARALLEL_HEADER

#include <stxxl/bits/config.h>

#include <cassert>

#if STXXL_PARALLEL
 #include <omp.h>
#endif

#include <stxxl/bits/common/settings.h>

#if defined(_GLIBCXX_PARALLEL)
//use _STXXL_FORCE_SEQUENTIAL to tag calls which are not worthwhile parallelizing
#define _STXXL_FORCE_SEQUENTIAL , __gnu_parallel::sequential_tag()
#else
#define _STXXL_FORCE_SEQUENTIAL
#endif

#if 0
// sorting triggers is done sequentially
#define _STXXL_SORT_TRIGGER_FORCE_SEQUENTIAL _STXXL_FORCE_SEQUENTIAL
#else
// sorting triggers may be parallelized
#define _STXXL_SORT_TRIGGER_FORCE_SEQUENTIAL
#endif

#if !STXXL_PARALLEL
#undef STXXL_PARALLEL_MULTIWAY_MERGE
#define STXXL_PARALLEL_MULTIWAY_MERGE 0
#endif

#if STXXL_PARALLEL && !defined(STXXL_PARALLEL_MULTIWAY_MERGE)
#define STXXL_PARALLEL_MULTIWAY_MERGE 1
#endif

#if !defined(STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD)
#define STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD 0
#endif

#include <tlx/algorithm/multiway_merge.hpp>
#include <tlx/algorithm/parallel_multiway_merge.hpp>

namespace stxxl {

inline unsigned sort_memory_usage_factor()
{
#if STXXL_PARALLEL && !STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD
    return (omp_get_max_threads() > 1) ? 2 : 1;   //memory overhead for multiway mergesort
#else
    return 1;                                     //no overhead
#endif
}

inline void check_sort_settings()
{
#if STXXL_PARALLEL && !defined(STXXL_NO_WARN_OMP_NESTED)
    // nothing to check, maybe in future?
#else
    // nothing to check
#endif
}

inline bool do_parallel_merge()
{
#if STXXL_PARALLEL_MULTIWAY_MERGE
    return !stxxl::SETTINGS::native_merge && omp_get_max_threads() >= 1;
#else
    return false;
#endif
}

//! this namespace provides parallel or sequential algorithms depending on the
//! compilation settings. it should be used by all components, where
//! parallelism is optional.
namespace potentially_parallel {

using std::sort;
using std::random_shuffle;

/*! Multi-way merging dispatcher.
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \return End iterator of output sequence.
 */
template <
    typename RandomAccessIteratorPairIterator,
    typename RandomAccessIterator3,
    typename Comparator>
RandomAccessIterator3 multiway_merge(
    RandomAccessIteratorPairIterator seqs_begin,
    RandomAccessIteratorPairIterator seqs_end,
    RandomAccessIterator3 target,
    typename std::iterator_traits<typename std::iterator_traits<
                                      RandomAccessIteratorPairIterator>::value_type::first_type>::
    difference_type length,
    Comparator comp)
{
#if STXXL_PARALLEL
    return tlx::parallel_multiway_merge(
        seqs_begin, seqs_end, target, length, comp);
#else
    return tlx::multiway_merge(
        seqs_begin, seqs_end, target, length, comp);
#endif
}

/*! Multi-way merging dispatcher.
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \return End iterator of output sequence.
 */
template <
    typename RandomAccessIteratorPairIterator,
    typename RandomAccessIterator3,
    typename Comparator>
RandomAccessIterator3 multiway_merge_stable(
    RandomAccessIteratorPairIterator seqs_begin,
    RandomAccessIteratorPairIterator seqs_end,
    RandomAccessIterator3 target,
    typename std::iterator_traits<typename std::iterator_traits<
                                      RandomAccessIteratorPairIterator>::value_type::first_type>::
    difference_type length,
    Comparator comp)
{
#if STXXL_PARALLEL
    return tlx::stable_parallel_multiway_merge(
        seqs_begin, seqs_end, target, length, comp);
#else
    return tlx::stable_multiway_merge(
        seqs_begin, seqs_end, target, length, comp);
#endif
}

/*! Multi-way merging front-end.
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \return End iterator of output sequence.
 * \pre For each \c i, \c seqs_begin[i].second must be the end marker of the sequence, but also reference the one more sentinel element.
 */
template <
    typename RandomAccessIteratorPairIterator,
    typename RandomAccessIterator3,
    typename Comparator>
RandomAccessIterator3 multiway_merge_sentinels(
    RandomAccessIteratorPairIterator seqs_begin,
    RandomAccessIteratorPairIterator seqs_end,
    RandomAccessIterator3 target,
    typename std::iterator_traits<typename std::iterator_traits<
                                      RandomAccessIteratorPairIterator>::value_type::first_type>::
    difference_type length,
    Comparator comp)
{
#if STXXL_PARALLEL
    return tlx::parallel_multiway_merge_sentinels(
        seqs_begin, seqs_end, target, length, comp);
#else
    return tlx::multiway_merge_sentinels(
        seqs_begin, seqs_end, target, length, comp);
#endif
}

/*! Multi-way merging front-end.
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \return End iterator of output sequence.
 * \pre For each \c i, \c seqs_begin[i].second must be the end marker of the sequence, but also reference the one more sentinel element.
 */
template <
    typename RandomAccessIteratorPairIterator,
    typename RandomAccessIterator3,
    typename Comparator>
RandomAccessIterator3 multiway_merge_stable_sentinels(
    RandomAccessIteratorPairIterator seqs_begin,
    RandomAccessIteratorPairIterator seqs_end,
    RandomAccessIterator3 target,
    typename std::iterator_traits<typename std::iterator_traits<
                                      RandomAccessIteratorPairIterator>::value_type::first_type>::
    difference_type length,
    Comparator comp)
{
#if STXXL_PARALLEL
    return tlx::stable_parallel_multiway_merge_sentinels(
        seqs_begin, seqs_end, target, length, comp);
#else
    return tlx::stable_multiway_merge_sentinels(
        seqs_begin, seqs_end, target, length, comp);
#endif
}

} // namespace potentially_parallel
} // namespace stxxl

#endif // !STXXL_PARALLEL_HEADER
