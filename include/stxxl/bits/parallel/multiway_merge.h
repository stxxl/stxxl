/***************************************************************************
 *  include/stxxl/bits/parallel/multiway_merge.h
 *
 *  Implementation of sequential and parallel multiway merge.
 *  Extracted from MCSTL - http://algo2.iti.uni-karlsruhe.de/singler/mcstl/
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_MULTIWAY_MERGE_HEADER
#define STXXL_PARALLEL_MULTIWAY_MERGE_HEADER

#include <vector>
#include <iterator>
#include <algorithm>

#include <stxxl/bits/verbose.h>
#include <stxxl/bits/parallel/features.h>
#include <stxxl/bits/parallel/merge.h>
#include <stxxl/bits/parallel/losertree.h>
#include <stxxl/bits/parallel/settings.h>
#include <stxxl/bits/parallel/equally_split.h>
#include <stxxl/bits/parallel/multiseq_selection.h>
#include <stxxl/bits/parallel/timing.h>
#include <stxxl/bits/parallel/tags.h>

/*! Length of a sequence described by a pair of iterators. */
#define LENGTH(s) ((s).second - (s).first)

STXXL_BEGIN_NAMESPACE

namespace parallel {

/*!
 * Iterator wrapper supporting an implicit supremum at the end of the sequence,
 * dominating all comparisons.  Deriving from RandomAccessIterator is not
 * possible since RandomAccessIterator need not be a class.
*/
template <typename RandomAccessIterator, typename Comparator>
class guarded_iterator
{
public:
    //! Our own type
    typedef guarded_iterator<RandomAccessIterator, Comparator> self_type;

    //! Value type of the iterator
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;

private:
    //! Current iterator position.
    RandomAccessIterator current;
    //! End iterator of the sequence.
    RandomAccessIterator end;
    //! Comparator.
    Comparator& comp;

public:
    /*!
     * Constructor. Sets iterator to beginning of sequence.
     * \param begin Begin iterator of sequence.
     * \param end End iterator of sequence.
     * \param comp Comparator provided for associated overloaded compare operators.
     */
    guarded_iterator(RandomAccessIterator begin, RandomAccessIterator end,
                     Comparator& comp)
        : current(begin), end(end), comp(comp)
    { }

    /*!
     * Pre-increment operator.
     * \return This.
     */
    self_type& operator ++ ()
    {
        ++current;
        return *this;
    }

    /*!
     * Dereference operator.
     * \return Referenced element.
     */
    value_type& operator * ()
    {
        return *current;
    }

    /*!
     * Convert to wrapped iterator.
     * \return Wrapped iterator.
     */
    operator RandomAccessIterator ()
    {
        return current;
    }

    /*!
     * Compare two elements referenced by guarded iterators.
     * \param bi1 First iterator.
     * \param bi2 Second iterator.
     * \return \c True if less.
     */
    friend bool operator < (self_type& bi1, self_type& bi2)
    {
        if (bi1.current == bi1.end)             // bi1 is sup
            return bi2.current == bi2.end;      // bi2 is not sup
        if (bi2.current == bi2.end)             // bi2 is sup
            return true;
        return bi1.comp(*bi1, *bi2);            // normal compare
    }

    /*!
     * Compare two elements referenced by guarded iterators.
     * \param bi1 First iterator.
     * \param bi2 Second iterator.
     * \return \c True if less equal.
     */
    friend bool operator <= (self_type& bi1, self_type& bi2)
    {
        if (bi2.current == bi2.end)         //bi1 is sup
            return bi1.current != bi1.end;  //bi2 is not sup
        if (bi1.current == bi1.end)         //bi2 is sup
            return false;
        return !bi1.comp(*bi2, *bi1);       //normal compare
    }
};

template <typename RandomAccessIterator, typename Comparator>
class unguarded_iterator
{
public:
    //! Our own type
    typedef unguarded_iterator<RandomAccessIterator, Comparator> self_type;

    //! Value type of the iterator
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;

protected:
    //! Current iterator position.
    RandomAccessIterator& current;
    //! Comparator.
    Comparator& comp;

public:
    /*!
     * Constructor. Sets iterator to beginning of sequence.
     * \param begin Begin iterator of sequence.
     * \param end Unused, only for compatibility.
     * \param comp Unused, only for compatibility.
     */
    unguarded_iterator(RandomAccessIterator begin, RandomAccessIterator /* end */,
                       Comparator& comp)
        : current(begin), comp(comp)
    { }

    /*!
     * Pre-increment operator.
     * \return This.
     */
    self_type& operator ++ ()
    {
        current++;
        return *this;
    }

    /*!
     * Dereference operator.
     * \return Referenced element.
     */
    value_type& operator * ()
    {
        return *current;
    }

    /*!
     * Convert to wrapped iterator.
     * \return Wrapped iterator.
     */
    operator RandomAccessIterator ()
    {
        return current;
    }

    /*!
     * Compare two elements referenced by unguarded iterators.
     * \param bi1 First iterator.
     * \param bi2 Second iterator.
     * \return \c True if less.
     */
    friend bool operator < (self_type& bi1, self_type& bi2)
    {
        return bi1.comp(*bi1, *bi2);    // normal compare, unguarded
    }

    /*!
     * Compare two elements referenced by unguarded iterators.
     * \param bi1 First iterator.
     * \param bi2 Second iterator.
     * \return \c True if less equal.
     */
    friend bool operator <= (self_type& bi1, self_type& bi2)
    {
        return !bi1.comp(*bi2, *bi1);   // normal compare, unguarded
    }
};

/** Prepare a set of sequences to be merged without a (end) guard
 *  \param seqs_begin
 *  \param seqs_end
 *  \param comp
 *  \param min_sequence
 *  \param stable
 *  \pre (seqs_end - seqs_begin > 0) */
template <typename RandomAccessIteratorIterator, typename Comparator>
typename std::iterator_traits<
    typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
    >::difference_type
prepare_unguarded(RandomAccessIteratorIterator seqs_begin,
                  RandomAccessIteratorIterator seqs_end,
                  Comparator comp,
                  int& min_sequence,
                  bool stable
                  )
{
    MCSTL_CALL(seqs_end - seqs_begin)

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;
    typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
        ValueType;
    typedef typename std::iterator_traits<RandomAccessIterator1>::difference_type
        DiffType;

    if ((*seqs_begin).first == (*seqs_begin).second)
    {
        // empty sequence found, it's the first one
        min_sequence = 0;
        return -1;
    }

    // last element in sequence
    ValueType min = *((*seqs_begin).second - 1);
    min_sequence = 0;
    for (RandomAccessIteratorIterator s = seqs_begin + 1; s != seqs_end; s++)
    {
        if ((*s).first == (*s).second)
        {
            // empty sequence found
            min_sequence = static_cast<int>(s - seqs_begin);
            return -1;
        }
        const ValueType& v = *((*s).second - 1);
        if (comp(v, min))
        {
            // last element in sequence is strictly smaller
            min = v;
            min_sequence = static_cast<int>(s - seqs_begin);
        }
    }

    DiffType overhang_size = 0;

    int s = 0;
    for (s = 0; s <= min_sequence; s++)
    {
        RandomAccessIterator1 split;
        if (stable)
            split = std::upper_bound(seqs_begin[s].first, seqs_begin[s].second, min, comp);
        else
            split = std::lower_bound(seqs_begin[s].first, seqs_begin[s].second, min, comp);

        overhang_size += seqs_begin[s].second - split;
    }

    for ( ; s < (seqs_end - seqs_begin); s++)
    {
        RandomAccessIterator1 split = std::lower_bound(seqs_begin[s].first, seqs_begin[s].second, min, comp);
        overhang_size += seqs_begin[s].second - split;
    }

    return overhang_size;       // so many elements will be left over afterwards
}

/*!
 * Prepare a set of sequences to be merged with a (end) guard (sentinel)
 * \param seqs_begin
 * \param seqs_end
 * \param comp
 */
template <typename RandomAccessIteratorIterator, typename Comparator>
typename std::iterator_traits<
    typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
    >::difference_type
prepare_unguarded_sentinel(RandomAccessIteratorIterator seqs_begin,
                           RandomAccessIteratorIterator seqs_end,
                           Comparator comp
                           )
{
    MCSTL_CALL(seqs_end - seqs_begin)

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;
    typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
        ValueType;
    typedef typename std::iterator_traits<RandomAccessIterator1>::difference_type
        DiffType;

    ValueType max;      //last element in sequence
    bool max_found = false;
    for (RandomAccessIteratorIterator s = seqs_begin; s != seqs_end; s++)
    {
        if ((*s).first == (*s).second)
            continue;
        ValueType& v = *((*s).second - 1);      //last element in sequence
        if (!max_found || comp(max, v))         //strictly greater
            max = v;
        max_found = true;
    }

    DiffType overhang_size = 0;

    for (RandomAccessIteratorIterator s = seqs_begin; s != seqs_end; s++)
    {
        RandomAccessIterator1 split = std::lower_bound((*s).first, (*s).second, max, comp);
        overhang_size += (*s).second - split;
        *((*s).second) = max;   //set sentinel
    }

    return overhang_size;       // so many elements will be left over afterwards
}

/*!
 * Highly efficient 3-way merging procedure.
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \param stable Unused, stable anyway.
 * \return End iterator of output sequence.
 */
template <template <typename RAI, typename C> class iterator,
          typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_3_variant(RandomAccessIteratorIterator seqs_begin,
                         RandomAccessIteratorIterator seqs_end,
                         RandomAccessIterator3 target,
                         Comparator comp,
                         DiffType length)
{
    MCSTL_CALL(length);
    STXXL_ASSERT(seqs_end - seqs_begin == 3);

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;

    if (length == 0)
        return target;

    iterator<RandomAccessIterator1, Comparator>
    seq0(seqs_begin[0].first, seqs_begin[0].second, comp),
    seq1(seqs_begin[1].first, seqs_begin[1].second, comp),
    seq2(seqs_begin[2].first, seqs_begin[2].second, comp);

    if (seq0 <= seq1)
    {
        if (seq1 <= seq2)
            goto s012;
        else if (seq2 < seq0)
            goto s201;
        else
            goto s021;
    }
    else
    {
        if (seq1 <= seq2)
        {
            if (seq0 <= seq2)
                goto s102;
            else
                goto s120;
        }
        else
            goto s210;
    }

#define STXXL_MERGE3CASE(a, b, c, c0, c1)             \
    s ## a ## b ## c :                                \
    *target = *seq ## a;                              \
    ++target;                                         \
    length--;                                         \
    ++seq ## a;                                       \
    if (length == 0) goto finish;                     \
    if (seq ## a c0 seq ## b) goto s ## a ## b ## c;  \
    if (seq ## a c1 seq ## c) goto s ## b ## a ## c;  \
    goto s ## b ## c ## a;

    STXXL_MERGE3CASE(0, 1, 2, <=, <=);
    STXXL_MERGE3CASE(1, 2, 0, <=, <);
    STXXL_MERGE3CASE(2, 0, 1, <, <);
    STXXL_MERGE3CASE(1, 0, 2, <, <=);
    STXXL_MERGE3CASE(0, 2, 1, <=, <=);
    STXXL_MERGE3CASE(2, 1, 0, <, <);

#undef STXXL_MERGE3CASE

finish:
    ;

    seqs_begin[0].first = seq0;
    seqs_begin[1].first = seq1;
    seqs_begin[2].first = seq2;

    return target;
}

template <typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_3_combined(RandomAccessIteratorIterator seqs_begin,
                          RandomAccessIteratorIterator seqs_end,
                          RandomAccessIterator3 target,
                          Comparator comp,
                          DiffType length)
{
    MCSTL_CALL(length);
    STXXL_ASSERT(seqs_end - seqs_begin == 3);

    int min_seq;
    RandomAccessIterator3 target_end;
    DiffType overhang = prepare_unguarded(seqs_begin, seqs_end, comp, min_seq, true);           //stable anyway

    DiffType total_length = 0;
    for (RandomAccessIteratorIterator s = seqs_begin; s != seqs_end; ++s)
        total_length += LENGTH(*s);

    if (overhang != (DiffType) - 1)
    {
        DiffType unguarded_length = std::min(length, total_length - overhang);
        target_end = multiway_merge_3_variant<unguarded_iterator>
                         (seqs_begin, seqs_end, target, comp, unguarded_length);
        overhang = length - unguarded_length;
    }
    else
    {           //empty sequence found
        overhang = length;
        target_end = target;
    }

#if MCSTL_ASSERTIONS
    assert(target_end == target + length - overhang);
    assert(is_sorted(target, target_end, comp));
#endif

    switch (min_seq)
    {
    case 0:
        target_end = merge_advance(             //iterators will be advanced accordingly
            seqs_begin[1].first, seqs_begin[1].second,
            seqs_begin[2].first, seqs_begin[2].second,
            target_end, overhang, comp);
        break;
    case 1:
        target_end = merge_advance(
            seqs_begin[0].first, seqs_begin[0].second,
            seqs_begin[2].first, seqs_begin[2].second,
            target_end, overhang, comp);
        break;
    case 2:
        target_end = merge_advance(
            seqs_begin[0].first, seqs_begin[0].second,
            seqs_begin[1].first, seqs_begin[1].second,
            target_end, overhang, comp);
        break;
    default:
        assert(false);
    }

#if MCSTL_ASSERTIONS
    assert(target_end == target + length);
    assert(is_sorted(target, target_end, comp));
#endif

    return target_end;
}

/*!
 * Highly efficient 4-way merging procedure.
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \param stable Unused, stable anyway.
 * \return End iterator of output sequence.
 */
template <template <typename RAI, typename C> class iterator,
          typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_4_variant(RandomAccessIteratorIterator seqs_begin,
                         RandomAccessIteratorIterator seqs_end,
                         RandomAccessIterator3 target,
                         Comparator comp, DiffType length)
{
    MCSTL_CALL(length);
    STXXL_ASSERT(seqs_end - seqs_begin == 4);

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;

    iterator<RandomAccessIterator1, Comparator>
    seq0(seqs_begin[0].first, seqs_begin[0].second, comp),
    seq1(seqs_begin[1].first, seqs_begin[1].second, comp),
    seq2(seqs_begin[2].first, seqs_begin[2].second, comp),
    seq3(seqs_begin[3].first, seqs_begin[3].second, comp);

#define STXXL_DECISION(a, b, c, d) do {                       \
        if (seq ## d < seq ## a) goto s ## d ## a ## b ## c;  \
        if (seq ## d < seq ## b) goto s ## a ## d ## b ## c;  \
        if (seq ## d < seq ## c) goto s ## a ## b ## d ## c;  \
        goto s ## a ## b ## c ## d;                           \
}                                                             \
    while (0)

    if (seq0 <= seq1)
    {
        if (seq1 <= seq2)
            STXXL_DECISION(0, 1, 2, 3);
        else if (seq2 < seq0)
            STXXL_DECISION(2, 0, 1, 3);
        else
            STXXL_DECISION(0, 2, 1, 3);
    }
    else
    {
        if (seq1 <= seq2)
        {
            if (seq0 <= seq2)
                STXXL_DECISION(1, 0, 2, 3);
            else
                STXXL_DECISION(1, 2, 0, 3);
        }
        else
            STXXL_DECISION(2, 1, 0, 3);
    }

#define STXXL_MERGE4CASE(a, b, c, d, c0, c1, c2)           \
    s ## a ## b ## c ## d :                                \
    if (length == 0) goto finish;                          \
    *target = *seq ## a;                                   \
    ++target;                                              \
    length--;                                              \
    ++seq ## a;                                            \
    if (seq ## a c0 seq ## b) goto s ## a ## b ## c ## d;  \
    if (seq ## a c1 seq ## c) goto s ## b ## a ## c ## d;  \
    if (seq ## a c2 seq ## d) goto s ## b ## c ## a ## d;  \
    goto s ## b ## c ## d ## a;

    STXXL_MERGE4CASE(0, 1, 2, 3, <=, <=, <=);
    STXXL_MERGE4CASE(0, 1, 3, 2, <=, <=, <=);
    STXXL_MERGE4CASE(0, 2, 1, 3, <=, <=, <=);
    STXXL_MERGE4CASE(0, 2, 3, 1, <=, <=, <=);
    STXXL_MERGE4CASE(0, 3, 1, 2, <=, <=, <=);
    STXXL_MERGE4CASE(0, 3, 2, 1, <=, <=, <=);
    STXXL_MERGE4CASE(1, 0, 2, 3, <, <=, <=);
    STXXL_MERGE4CASE(1, 0, 3, 2, <, <=, <=);
    STXXL_MERGE4CASE(1, 2, 0, 3, <=, <, <=);
    STXXL_MERGE4CASE(1, 2, 3, 0, <=, <=, <);
    STXXL_MERGE4CASE(1, 3, 0, 2, <=, <, <=);
    STXXL_MERGE4CASE(1, 3, 2, 0, <=, <=, <);
    STXXL_MERGE4CASE(2, 0, 1, 3, <, <, <=);
    STXXL_MERGE4CASE(2, 0, 3, 1, <, <=, <);
    STXXL_MERGE4CASE(2, 1, 0, 3, <, <, <=);
    STXXL_MERGE4CASE(2, 1, 3, 0, <, <=, <);
    STXXL_MERGE4CASE(2, 3, 0, 1, <=, <, <);
    STXXL_MERGE4CASE(2, 3, 1, 0, <=, <, <);
    STXXL_MERGE4CASE(3, 0, 1, 2, <, <, <);
    STXXL_MERGE4CASE(3, 0, 2, 1, <, <, <);
    STXXL_MERGE4CASE(3, 1, 0, 2, <, <, <);
    STXXL_MERGE4CASE(3, 1, 2, 0, <, <, <);
    STXXL_MERGE4CASE(3, 2, 0, 1, <, <, <);
    STXXL_MERGE4CASE(3, 2, 1, 0, <, <, <);

#undef STXXL_MERGE4CASE
#undef STXXL_DECISION

finish:
    ;

    seqs_begin[0].first = seq0;
    seqs_begin[1].first = seq1;
    seqs_begin[2].first = seq2;
    seqs_begin[3].first = seq3;

    return target;
}

template <typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_4_combined(RandomAccessIteratorIterator seqs_begin,
                          RandomAccessIteratorIterator seqs_end,
                          RandomAccessIterator3 target,
                          Comparator comp, DiffType length)
{
    MCSTL_CALL(length);
    STXXL_ASSERT(seqs_end - seqs_begin == 4);

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;

    int min_seq;
    RandomAccessIterator3 target_end;
    DiffType overhang = prepare_unguarded(seqs_begin, seqs_end, comp, min_seq, true);                         //stable anyway

    DiffType total_length = 0;
    for (RandomAccessIteratorIterator s = seqs_begin; s != seqs_end; ++s)
        total_length += LENGTH(*s);

    if (overhang != (DiffType) - 1)
    {
        DiffType unguarded_length = std::min(length, total_length - overhang);
        target_end = multiway_merge_4_variant<unguarded_iterator>
                         (seqs_begin, seqs_end, target, comp, unguarded_length);
        overhang = length - unguarded_length;
    }
    else
    {                               //empty sequence found
        overhang = length;
        target_end = target;
    }

#if MCSTL_ASSERTIONS
    assert(target_end == target + length - overhang);
    assert(is_sorted(target, target_end, comp));
#endif

    std::vector<std::pair<RandomAccessIterator1, RandomAccessIterator1> > one_missing(seqs_begin, seqs_end);
    one_missing.erase(one_missing.begin() + min_seq);                                               //remove

    target_end = multiway_merge_3_variant<guarded_iterator>(one_missing.begin(), one_missing.end(), target_end, comp, overhang);

    one_missing.insert(one_missing.begin() + min_seq, seqs_begin[min_seq]);                         //insert back again
    copy(one_missing.begin(), one_missing.end(), seqs_begin);                                       //write back modified iterators

#if MCSTL_ASSERTIONS
    assert(target_end == target + length);
    assert(is_sorted(target, target_end, comp));
#endif

    return target_end;
}

/*!
 * Basic multi-way merging procedure.
 *
 * The head elements are kept in a sorted array, new heads are inserted linearly.
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \param stable Stable merging incurs a performance penalty.
 * \return End iterator of output sequence.
 */
template <typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_bubble(RandomAccessIteratorIterator seqs_begin,
                      RandomAccessIteratorIterator seqs_end,
                      RandomAccessIterator3 target,
                      Comparator comp, DiffType length,
                      bool stable)
{
    MCSTL_CALL(length)

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;
    typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
        ValueType;

    // num remaining pieces
    int k = static_cast<int>(seqs_end - seqs_begin), nrp;

    ValueType* pl = new ValueType[k];
    int* source = new int[k];
    DiffType total_length = 0;

#define POS(i) seqs_begin[(i)].first
#define STOPS(i) seqs_begin[(i)].second

    //write entries into queue
    nrp = 0;
    for (int pi = 0; pi < k; pi++)
    {
        if (STOPS(pi) != POS(pi))
        {
            pl[nrp] = *(POS(pi));
            source[nrp] = pi;
            nrp++;
            total_length += LENGTH(seqs_begin[pi]);
        }
    }

    if (stable)
    {
        for (int k = 0; k < nrp - 1; k++)
            for (int pi = nrp - 1; pi > k; pi--)
                if (comp(pl[pi], pl[pi - 1]) ||
                    (!comp(pl[pi - 1], pl[pi]) && source[pi] < source[pi - 1]))
                {
                    std::swap(pl[pi - 1], pl[pi]);
                    std::swap(source[pi - 1], source[pi]);
                }
    }
    else
    {
        for (int k = 0; k < nrp - 1; k++)
            for (int pi = nrp - 1; pi > k; pi--)
                if (comp(pl[pi], pl[pi - 1]))
                {
                    std::swap(pl[pi - 1], pl[pi]);
                    std::swap(source[pi - 1], source[pi]);
                }
    }

    // iterate
    if (stable)
    {
        int j;
        while (nrp > 0 && length > 0)
        {
            if (source[0] < source[1])
            {
                // pl[0] <= pl[1] ?
                while ((nrp == 1 || !(comp(pl[1], pl[0]))) && length > 0)
                {
                    *target = pl[0];
                    ++target;
                    ++POS(source[0]);
                    length--;
                    if (POS(source[0]) == STOPS(source[0]))
                    {
                        // move everything to the left
                        for (int s = 0; s < nrp - 1; s++)
                        {
                            pl[s] = pl[s + 1];
                            source[s] = source[s + 1];
                        }
                        nrp--;
                        break;
                    }
                    else
                        pl[0] = *(POS(source[0]));
                }
            }
            else
            {
                // pl[0] < pl[1] ?
                while ((nrp == 1 || comp(pl[0], pl[1])) && length > 0)
                {
                    *target = pl[0];
                    ++target;
                    ++POS(source[0]);
                    length--;
                    if (POS(source[0]) == STOPS(source[0]))
                    {
                        for (int s = 0; s < nrp - 1; s++)
                        {
                            pl[s] = pl[s + 1];
                            source[s] = source[s + 1];
                        }
                        nrp--;
                        break;
                    }
                    else
                        pl[0] = *(POS(source[0]));
                }
            }

            //sink down
            j = 1;
            while ((j < nrp) && (comp(pl[j], pl[j - 1]) ||
                                 (!comp(pl[j - 1], pl[j]) && (source[j] < source[j - 1]))))
            {
                std::swap(pl[j - 1], pl[j]);
                std::swap(source[j - 1], source[j]);
                j++;
            }
        }
    }
    else
    {
        int j;
        while (nrp > 0 && length > 0)
        {
            // pl[0] <= pl[1] ?
            while ((nrp == 1 || !comp(pl[1], pl[0])) && length > 0)
            {
                *target = pl[0];
                ++target;
                ++POS(source[0]);
                length--;
                if (POS(source[0]) == STOPS(source[0]))
                {
                    for (int s = 0; s < (nrp - 1); s++)
                    {
                        pl[s] = pl[s + 1];
                        source[s] = source[s + 1];
                    }
                    nrp--;
                    break;
                }
                else
                    pl[0] = *(POS(source[0]));
            }

            //sink down
            j = 1;
            while ((j < nrp) && comp(pl[j], pl[j - 1]))
            {
                std::swap(pl[j - 1], pl[j]);
                std::swap(source[j - 1], source[j]);
                j++;
            }
        }
    }

    delete[] pl;
    delete[] source;

    return target;
}

/*!
 * Multi-way merging procedure for a high branching factor, guarded case.
 *
 * The head elements are kept in a loser tree.
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \param stable Stable merging incurs a performance penalty.
 * \return End iterator of output sequence.
 */
template <typename LoserTreeType,
          typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_loser_tree(RandomAccessIteratorIterator seqs_begin,
                          RandomAccessIteratorIterator seqs_end,
                          RandomAccessIterator3 target,
                          Comparator comp, DiffType length,
                          bool stable)
{
    MCSTL_CALL(length)

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;
    typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
        ValueType;

    int k = static_cast<int>(seqs_end - seqs_begin);

    LoserTreeType lt(k, comp);

    DiffType total_length = 0;

    for (int t = 0; t < k; t++)
    {
        if (stable)
        {
            if (seqs_begin[t].first == seqs_begin[t].second)
                lt.insert_start_stable(ValueType(), t, true);
            else
                lt.insert_start_stable(*seqs_begin[t].first, t, false);
        }
        else
        {
            if (seqs_begin[t].first == seqs_begin[t].second)
                lt.insert_start(ValueType(), t, true);
            else
                lt.insert_start(*seqs_begin[t].first, t, false);
        }

        total_length += LENGTH(seqs_begin[t]);
    }

    if (stable)
        lt.init_stable();
    else
        lt.init();

    total_length = std::min(total_length, length);

    int source;

    if (stable)
    {
        for (DiffType i = 0; i < total_length; i++)
        {
            //take out
            source = lt.get_min_source();

            *(target++) = *(seqs_begin[source].first++);

            //feed
            if (seqs_begin[source].first == seqs_begin[source].second)
                lt.delete_min_insert_stable(ValueType(), true);
            else
                // replace from same source
                lt.delete_min_insert_stable(*seqs_begin[source].first, false);
        }
    }
    else
    {
        for (DiffType i = 0; i < total_length; i++)
        {
            //take out
            source = lt.get_min_source();

            *(target++) = *(seqs_begin[source].first++);

            //feed
            if (seqs_begin[source].first == seqs_begin[source].second)
                lt.delete_min_insert(ValueType(), true);
            else
                //replace from same source
                lt.delete_min_insert(*seqs_begin[source].first, false);
        }
    }

    return target;
}

/*!
 * Multi-way merging procedure for a high branching factor, unguarded case.
 * The head elements are kept in a loser tree.
 *
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \param stable Stable merging incurs a performance penalty.
 * \return End iterator of output sequence.
 * \pre No input will run out of elements during the merge.
 */
template <typename LoserTreeType,
          typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType,
          typename Comparator>
RandomAccessIterator3
multiway_merge_loser_tree_unguarded(RandomAccessIteratorIterator seqs_begin,
                                    RandomAccessIteratorIterator seqs_end,
                                    RandomAccessIterator3 target,
                                    Comparator comp, DiffType length,
                                    bool stable)
{
    MCSTL_CALL(length)

    int k = seqs_end - seqs_begin;

    LoserTreeType lt(k, comp);

    DiffType total_length = 0;

    for (int t = 0; t < k; t++)
    {
#if MCSTL_ASSERTIONS
        assert(seqs_begin[t].first != seqs_begin[t].second);
#endif
        if (stable)
            lt.insert_start_stable(*seqs_begin[t].first, t, false);
        else
            lt.insert_start(*seqs_begin[t].first, t, false);

        total_length += LENGTH(seqs_begin[t]);
    }

    if (stable)
        lt.init_stable();
    else
        lt.init();

    length = std::min(total_length, length);                         //do not go past end

    int source;

#if MCSTL_ASSERTIONS
    DiffType i = 0;
#endif

    if (stable)
    {
        RandomAccessIterator3 target_end = target + length;
        while (target < target_end)
        {
            //take out
            source = lt.get_min_source();

#if MCSTL_ASSERTIONS
            assert(i == 0 || !comp(*(seqs_begin[source].first), *(target - 1)));
#endif

            *(target++) = *(seqs_begin[source].first++);

#if MCSTL_ASSERTIONS
            assert((seqs_begin[source].first != seqs_begin[source].second) || (i == length - 1));
            i++;
#endif
            //feed
            lt.delete_min_insert_stable(*seqs_begin[source].first, false);                         //replace from same source
        }
    }
    else
    {
        RandomAccessIterator3 target_end = target + length;
        while (target < target_end)
        {
            //take out
            source = lt.get_min_source();

#if MCSTL_ASSERTIONS
            if (i > 0 && comp(*(seqs_begin[source].first), *(target - 1)))
                printf("         %i %i %i\n", length, i, source);
            assert(i == 0 || !comp(*(seqs_begin[source].first), *(target - 1)));
#endif

            *(target++) = *(seqs_begin[source].first++);

#if MCSTL_ASSERTIONS
            if (!((seqs_begin[source].first != seqs_begin[source].second) || (i >= length - 1)))
                printf("         %i %i %i\n", length, i, source);
            assert((seqs_begin[source].first != seqs_begin[source].second) || (i >= length - 1));
            i++;
#endif
            //feed
            lt.delete_min_insert(*seqs_begin[source].first, false);                         //replace from same source
        }
    }

    return target;
}

template <class ValueType, class Comparator>
struct loser_tree_traits
{
public:
    typedef LoserTree /*Pointer*/<ValueType, Comparator> LT;
};

/*#define NO_POINTER(T) \
template<class Comparator> \
struct loser_tree_traits<T, Comparator> \
{ \
        typedef LoserTreePointer<T, Comparator> LT; \
};*/
//
// NO_POINTER(unsigned char)
// NO_POINTER(char)
// NO_POINTER(unsigned short)
// NO_POINTER(short)
// NO_POINTER(unsigned int)
// NO_POINTER(int)
// NO_POINTER(unsigned long)
// NO_POINTER(long)
// NO_POINTER(unsigned long long)
// NO_POINTER(long long)
//
// #undef NO_POINTER

template <class ValueType, class Comparator>
class loser_tree_traits_unguarded
{
public:
    typedef LoserTreeUnguarded<ValueType, Comparator> LT;
};

/*#define NO_POINTER_UNGUARDED(T) \
template<class Comparator> \
struct loser_tree_traits_unguarded<T, Comparator> \
{ \
        typedef LoserTreePointerUnguarded<T, Comparator> LT; \
};*/
//
// NO_POINTER_UNGUARDED(unsigned char)
// NO_POINTER_UNGUARDED(char)
// NO_POINTER_UNGUARDED(unsigned short)
// NO_POINTER_UNGUARDED(short)
// NO_POINTER_UNGUARDED(unsigned int)
// NO_POINTER_UNGUARDED(int)
// NO_POINTER_UNGUARDED(unsigned long)
// NO_POINTER_UNGUARDED(long)
// NO_POINTER_UNGUARDED(unsigned long long)
// NO_POINTER_UNGUARDED(long long)
//
// #undef NO_POINTER_UNGUARDED

template <typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_loser_tree_combined(RandomAccessIteratorIterator seqs_begin,
                                   RandomAccessIteratorIterator seqs_end,
                                   RandomAccessIterator3 target,
                                   Comparator comp, DiffType length,
                                   bool stable)
{
    MCSTL_CALL(length)

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;
    typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
        ValueType;

    int min_seq;
    RandomAccessIterator3 target_end;
    DiffType overhang = prepare_unguarded(seqs_begin, seqs_end, comp, min_seq, stable);

    DiffType total_length = 0;
    for (RandomAccessIteratorIterator s = seqs_begin; s != seqs_end; s++)
        total_length += LENGTH(*s);

    if (overhang != -1)
    {
        DiffType unguarded_length = std::min(length, total_length - overhang);
        target_end = multiway_merge_loser_tree_unguarded
                     <typename loser_tree_traits_unguarded<ValueType, Comparator>::LT>
                         (seqs_begin, seqs_end, target, comp, unguarded_length, stable);
        overhang = length - unguarded_length;
    }
    else
    {
        // empty sequence found
        overhang = length;
        target_end = target;
    }

#if MCSTL_ASSERTIONS
    assert(target_end == target + length - overhang);
    assert(is_sorted(target, target_end, comp));
#endif

    target_end = multiway_merge_loser_tree
                 <typename loser_tree_traits<ValueType, Comparator>::LT>
                     (seqs_begin, seqs_end, target_end, comp, overhang, stable);

#if MCSTL_ASSERTIONS
    assert(target_end == target + length);
    assert(is_sorted(target, target_end, comp));
#endif

    return target_end;
}

template <typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_loser_tree_sentinel(RandomAccessIteratorIterator seqs_begin,
                                   RandomAccessIteratorIterator seqs_end,
                                   RandomAccessIterator3 target,
                                   Comparator comp, DiffType length,
                                   bool stable)
{
    MCSTL_CALL(length)

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;
    typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
        ValueType;
    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;

    RandomAccessIterator3 target_end;
    DiffType overhang = prepare_unguarded_sentinel(seqs_begin, seqs_end, comp);

    DiffType total_length = 0;
    for (RandomAccessIteratorIterator s = seqs_begin; s != seqs_end; s++)
    {
        total_length += LENGTH(*s);
        (*s).second++;                         //sentinel spot
    }

    DiffType unguarded_length = std::min(length, total_length - overhang);
    target_end = multiway_merge_loser_tree_unguarded
                 <typename loser_tree_traits_unguarded<ValueType, Comparator>::LT>
                     (seqs_begin, seqs_end, target, comp, unguarded_length, stable);
    overhang = length - unguarded_length;

#if MCSTL_ASSERTIONS
    assert(target_end == target + length - overhang);
    assert(is_sorted(target, target_end, comp));
#endif

    //copy rest stable
    for (RandomAccessIteratorIterator s = seqs_begin; s != seqs_end && overhang > 0; s++)
    {
        (*s).second--;                         //restore
        DiffType local_length = std::min((DiffType)overhang, (DiffType)LENGTH(*s));
        target_end = std::copy((*s).first, (*s).first + local_length, target_end);
        (*s).first += local_length;
        overhang -= local_length;
    }

#if MCSTL_ASSERTIONS
    assert(overhang == 0);
    assert(target_end == target + length);
    assert(is_sorted(target, target_end, comp));
#endif

    return target_end;
}

/** Sequential multi-way merging switch.
 *
 *  The decision if based on the branching factor and runtime settings.
 *  \param seqs_begin Begin iterator of iterator pair input sequence.
 *  \param seqs_end End iterator of iterator pair input sequence.
 *  \param target Begin iterator out output sequence.
 *  \param comp Comparator.
 *  \param length Maximum length to merge.
 *  \param stable Stable merging incurs a performance penalty.
 *  \param sentinel The sequences have a sentinel element.
 *  \return End iterator of output sequence. */
template <typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
sequential_multiway_merge(RandomAccessIteratorIterator seqs_begin,
                          RandomAccessIteratorIterator seqs_end,
                          RandomAccessIterator3 target,
                          Comparator comp, DiffType length,
                          bool stable, bool sentinel)
{
    MCSTL_CALL(length)

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;
    typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
        ValueType;

#if MCSTL_ASSERTIONS
    for (RandomAccessIteratorIterator s = seqs_begin; s != seqs_end; s++)
        assert(is_sorted((*s).first, (*s).second, comp));
#endif

    RandomAccessIterator3 return_target = target;
    int k = static_cast<int>(seqs_end - seqs_begin);

    SETTINGS::MultiwayMergeAlgorithm mwma = SETTINGS::multiway_merge_algorithm;

    if (!sentinel && mwma == SETTINGS::LOSER_TREE_SENTINEL)
        mwma = SETTINGS::LOSER_TREE_COMBINED;

    switch (k)
    {
    case 0:
        break;
    case 1:
        return_target = std::copy(seqs_begin[0].first, seqs_begin[0].first + length, target);
        seqs_begin[0].first += length;
        break;
    case 2:
        return_target = merge_advance(seqs_begin[0].first, seqs_begin[0].second, seqs_begin[1].first, seqs_begin[1].second, target, length, comp);
        break;
    case 3:
        switch (mwma)
        {
        case SETTINGS::LOSER_TREE_COMBINED:
            return_target = multiway_merge_3_combined(seqs_begin, seqs_end, target, comp, length);
            break;
        case SETTINGS::LOSER_TREE_SENTINEL:
            return_target = multiway_merge_3_variant<unguarded_iterator>(seqs_begin, seqs_end, target, comp, length);
            break;
        default:
            return_target = multiway_merge_3_variant<guarded_iterator>(seqs_begin, seqs_end, target, comp, length);
            break;
        }
        break;
    case 4:
        switch (mwma)
        {
        case SETTINGS::LOSER_TREE_COMBINED:
            return_target = multiway_merge_4_combined(seqs_begin, seqs_end, target, comp, length);
            break;
        case SETTINGS::LOSER_TREE_SENTINEL:
            return_target = multiway_merge_4_variant<unguarded_iterator>(seqs_begin, seqs_end, target, comp, length);
            break;
        default:
            return_target = multiway_merge_4_variant<guarded_iterator>(seqs_begin, seqs_end, target, comp, length);
            break;
        }
        break;
    default:
    {
        switch (mwma)
        {
        case SETTINGS::BUBBLE:
            return_target = multiway_merge_bubble(seqs_begin, seqs_end, target, comp, length, stable);
            break;
#if MCSTL_LOSER_TREE_EXPLICIT
        case SETTINGS::LOSER_TREE_EXPLICIT:
            return_target = multiway_merge_loser_tree<LoserTreeExplicit<ValueType, Comparator> >(seqs_begin, seqs_end, target, comp, length, stable);
            break;
#endif
#if MCSTL_LOSER_TREE
        case SETTINGS::LOSER_TREE:
            return_target = multiway_merge_loser_tree<LoserTree<ValueType, Comparator> >(seqs_begin, seqs_end, target, comp, length, stable);
            break;
#endif
#if MCSTL_LOSER_TREE_COMBINED
        case SETTINGS::LOSER_TREE_COMBINED:
            return_target = multiway_merge_loser_tree_combined(seqs_begin, seqs_end, target, comp, length, stable);
            break;
#endif
#if MCSTL_LOSER_TREE_SENTINEL
        case SETTINGS::LOSER_TREE_SENTINEL:
            return_target = multiway_merge_loser_tree_sentinel(seqs_begin, seqs_end, target, comp, length, stable);
            break;
#endif
        default:
            assert(0 && "multiway_merge algorithm not implemented");
            break;
        }
    }
    }
#if MCSTL_ASSERTIONS
    assert(is_sorted(target, target + length, comp));
#endif

    return return_target;
}

/*!
 * Parallel multi-way merge routine.
 *
 * The decision if based on the branching factor and runtime settings.
 *
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \param stable Stable merging incurs a performance penalty.
 * \param sentinel Ignored.
 * \return End iterator of output sequence.
 */
template <typename RandomAccessIteratorIterator,
          typename RandomAccessIterator3,
          typename DiffType,
          typename Comparator>
RandomAccessIterator3
parallel_multiway_merge(RandomAccessIteratorIterator seqs_begin,
                        RandomAccessIteratorIterator seqs_end,
                        RandomAccessIterator3 target,
                        Comparator comp, DiffType length,
                        bool stable)
{
    MCSTL_CALL(length)

    typedef typename std::iterator_traits<RandomAccessIteratorIterator>::value_type::first_type
        RandomAccessIterator1;
    typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
        ValueType;

#if MCSTL_ASSERTIONS
    for (RandomAccessIteratorIterator rii = seqs_begin; rii != seqs_end; rii++)
        assert(is_sorted((*rii).first, (*rii).second, comp));
#endif

    int k = static_cast<int>(seqs_end - seqs_begin);                         //k sequences

    DiffType total_length = 0;
    for (RandomAccessIteratorIterator raii = seqs_begin; raii != seqs_end; raii++)
        total_length += LENGTH(*raii);

    MCSTL_CALL(total_length)

    if (total_length == 0 || k == 0)
        return target;

    thread_index_t num_threads = static_cast<thread_index_t>(std::min(static_cast<DiffType>(SETTINGS::num_threads), total_length));

    Timing<inactive_tag>* t = new Timing<inactive_tag>[num_threads];

    for (int pr = 0; pr < num_threads; pr++)
        t[pr].tic();

    bool tight = (total_length == length);

    std::vector<std::pair<DiffType, DiffType> >* pieces = new std::vector<std::pair<DiffType, DiffType> >[num_threads];                         //thread t will have to merge pieces[iam][0..k - 1]
    for (int s = 0; s < num_threads; s++)
        pieces[s].resize(k);

    DiffType num_samples = SETTINGS::merge_oversampling * num_threads;

    if (SETTINGS::multiway_merge_splitting == SETTINGS::SAMPLING)
    {
        ValueType* samples = new ValueType[k * num_samples];
        //sample
        for (int s = 0; s < k; s++)
            for (int i = 0; (DiffType)i < num_samples; i++)
            {
                DiffType sample_index = static_cast<DiffType>(LENGTH(seqs_begin[s]) * (double(i + 1) / (num_samples + 1)) * (double(length) / total_length));
                samples[s * num_samples + i] = seqs_begin[s].first[sample_index];
            }

        if (stable)
            std::stable_sort(samples, samples + (num_samples * k), comp);
        else
            std::sort(samples, samples + (num_samples * k), comp);

        for (int slab = 0; slab < num_threads; slab++)
            //for each slab / processor
            for (int seq = 0; seq < k; seq++)
            {                                                            //for each sequence
                if (slab > 0)
                    pieces[slab][seq].first = std::upper_bound(seqs_begin[seq].first, seqs_begin[seq].second, samples[num_samples * k * slab / num_threads], comp) - seqs_begin[seq].first;
                else
                    pieces[slab][seq].first = 0;                         //absolute beginning
                if ((slab + 1) < num_threads)
                    pieces[slab][seq].second = std::upper_bound(seqs_begin[seq].first, seqs_begin[seq].second, samples[num_samples * k * (slab + 1) / num_threads], comp) - seqs_begin[seq].first;
                else
                    pieces[slab][seq].second = LENGTH(seqs_begin[seq]);  //absolute ending
            }
        delete[] samples;
    }
    else                                                                 //(SETTINGS::multiway_merge_splitting == SETTINGS::EXACT)
    {
        std::vector<RandomAccessIterator1>* offsets = new std::vector<RandomAccessIterator1>[num_threads];
        std::vector<std::pair<RandomAccessIterator1, RandomAccessIterator1> > se(k);

        copy(seqs_begin, seqs_end, se.begin());

        std::vector<DiffType> borders(num_threads + 1);
        equally_split(length, num_threads, borders.begin());

        for (int s = 0; s < (num_threads - 1); s++)
        {
            offsets[s].resize(k);
            multiseq_partition(se.begin(), se.end(), borders[s + 1], offsets[s].begin(), comp);

            if (!tight)                         //last one also needed and available
            {
                offsets[num_threads - 1].resize(k);
                multiseq_partition(se.begin(), se.end(), (DiffType)length, offsets[num_threads - 1].begin(), comp);
            }
        }

        for (int slab = 0; slab < num_threads; slab++)
        {                                                                 //for each slab / processor
            for (int seq = 0; seq < k; seq++)
            {                                                             //for each sequence
                if (slab == 0)
                    pieces[slab][seq].first = 0;                          //absolute beginning
                else
                    pieces[slab][seq].first = pieces[slab - 1][seq].second;
                if (!tight || slab < (num_threads - 1))
                    pieces[slab][seq].second = offsets[slab][seq] - seqs_begin[seq].first;
                else                            /*slab == num_threads - 1*/
                    pieces[slab][seq].second = LENGTH(seqs_begin[seq]);
            }
        }
        delete[] offsets;
    }

    for (int pr = 0; pr < num_threads; pr++)
        t[pr].tic();

#pragma omp parallel num_threads(num_threads)
    {
        thread_index_t iam = omp_get_thread_num();

        t[iam].tic();

        DiffType target_position = 0;

        for (int c = 0; c < k; c++)
            target_position += pieces[iam][c].first;

        if (k > 2)
        {
            std::pair<RandomAccessIterator1, RandomAccessIterator1>* chunks = new std::pair<RandomAccessIterator1, RandomAccessIterator1>[k];

            DiffType local_length = 0;
            for (int s = 0; s < k; s++)
            {
                chunks[s] = std::make_pair(
                    seqs_begin[s].first + pieces[iam][s].first,
                    seqs_begin[s].first + pieces[iam][s].second);
                local_length += LENGTH(chunks[s]);
            }

            sequential_multiway_merge(
                chunks,
                chunks + k,
                target + target_position,
                comp,
                std::min(local_length, length - target_position),
                stable,
                false);

            delete[] chunks;
        }
        else if (k == 2)
        {
            RandomAccessIterator1 begin0 = seqs_begin[0].first + pieces[iam][0].first, begin1 = seqs_begin[1].first + pieces[iam][1].first;
            merge_advance(begin0,
                          seqs_begin[0].first + pieces[iam][0].second,
                          begin1,
                          seqs_begin[1].first + pieces[iam][1].second,
                          target + target_position,
                          (pieces[iam][0].second - pieces[iam][0].first) + (pieces[iam][1].second - pieces[iam][1].first),
                          comp
                          );
        }

        t[iam].tic();
    }

    for (int pr = 0; pr < num_threads; pr++)
        t[pr].tic();

#if MCSTL_ASSERTIONS
    assert(is_sorted(target, target + length, comp));
#endif

    //update ends of sequences
    for (int s = 0; s < k; s++)
        seqs_begin[s].first += pieces[num_threads - 1][s].second;

    delete[] pieces;

    for (int pr = 0; pr < num_threads; pr++)
        t[pr].tic();
    for (int pr = 0; pr < num_threads; pr++)
        t[pr].print();
    delete[] t;

    return target + length;
}

/*!
 * Multi-way merging front-end.
 *
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \param stable Stable merging incurs a performance penalty.
 * \return End iterator of output sequence.
 */
template <typename RandomAccessIteratorPairIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge(RandomAccessIteratorPairIterator seqs_begin,
               RandomAccessIteratorPairIterator seqs_end,
               RandomAccessIterator3 target,
               Comparator comp,
               DiffType length,
               bool stable)
{
    MCSTL_CALL(seqs_end - seqs_begin)

    if (seqs_begin == seqs_end)
        return target;

    RandomAccessIterator3 target_end;
    if (MCSTL_PARALLEL_CONDITION(((seqs_end - seqs_begin) >= SETTINGS::multiway_merge_minimal_k) && ((sequence_index_t)length >= SETTINGS::multiway_merge_minimal_n)))
        target_end = parallel_multiway_merge(seqs_begin, seqs_end, target, comp, length, stable);
    else
        target_end = sequential_multiway_merge(seqs_begin, seqs_end, target, comp, length, stable, false);

    return target_end;
}

/*!
 * Multi-way merging front-end.
 *
 * \param seqs_begin Begin iterator of iterator pair input sequence.
 * \param seqs_end End iterator of iterator pair input sequence.
 * \param target Begin iterator out output sequence.
 * \param comp Comparator.
 * \param length Maximum length to merge.
 * \param stable Stable merging incurs a performance penalty.
 * \return End iterator of output sequence.
 * \pre For each \c i, \c seqs_begin[i].second must be the end marker of the
 * sequence, but also reference the one more sentinel element.
 */
template <typename RandomAccessIteratorPairIterator,
          typename RandomAccessIterator3,
          typename DiffType, typename Comparator>
RandomAccessIterator3
multiway_merge_sentinel(RandomAccessIteratorPairIterator seqs_begin,
                        RandomAccessIteratorPairIterator seqs_end,
                        RandomAccessIterator3 target,
                        Comparator comp,
                        DiffType length,
                        bool stable)
{
    if (seqs_begin == seqs_end)
        return target;

    MCSTL_CALL(seqs_end - seqs_begin)

    if (MCSTL_PARALLEL_CONDITION(((seqs_end - seqs_begin) >= SETTINGS::multiway_merge_minimal_k) && ((sequence_index_t)length >= SETTINGS::multiway_merge_minimal_n)))
        return parallel_multiway_merge(seqs_begin, seqs_end, target, comp, (typename std::iterator_traits<RandomAccessIterator3>::difference_type)length, stable, true);
    else
        return sequtial_multiway_merge(seqs_begin, seqs_end, target, comp, length, stable, true);
}

}                     // namespace parallel

STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_MULTIWAY_MERGE_HEADER
