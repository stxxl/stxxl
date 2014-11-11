/***************************************************************************
 *  include/stxxl/bits/containers/pq_mergers.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 1999 Peter Sanders <sanders@mpi-sb.mpg.de>
 *  Copyright (C) 2003, 2004, 2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_PQ_MERGERS_HEADER
#define STXXL_CONTAINERS_PQ_MERGERS_HEADER

#include <stxxl/bits/containers/pq_helpers.h>

STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local {

/*!
 * \param Arity  maximum arity of merger, does not need to be a power of 2
 */
template <class ArraysType, class CompareType, unsigned Arity>
class loser_tree : public ArraysType
{
public:
    typedef ArraysType super_type;
    typedef CompareType comparator_type;

    // arity_bound / 2  <  arity  <=  arity_bound
    enum { arity = Arity, max_arity = 1UL << (LOG2<Arity>::ceil) };

    typedef typename super_type::value_type value_type;

    typedef typename super_type::SequenceType SequenceType;
    typedef typename super_type::Entry Entry;

    //! stack of free player indices
    internal_bounded_stack<unsigned_type, arity> free_slots;

    bool is_array_empty(unsigned_type slot) const
    {
        return super_type::is_array_empty(slot);
    }

    bool is_array_allocated(unsigned_type slot) const
    {
        return super_type::is_array_allocated(slot);
    }

    loser_tree()
        : super_type()
    {
        assert(this->k == 1);

        // initial state: one empty player slot
        free_slots.push(0);

        this->rebuild_loser_tree();

#if STXXL_PQ_INTERNAL_LOSER_TREE
        assert(is_array_empty(0) && !is_array_allocated(0));
#endif // STXXL_PQ_INTERNAL_LOSER_TREE
    }

    //! Allocate a free slot for a new player.
    unsigned_type new_player()
    {
        // get a free slot
        if (free_slots.empty()) {
            // tree is too small, attempt to enlarge
            double_k();
        }

        assert(!free_slots.empty());
        unsigned_type index = free_slots.top();
        free_slots.pop();

        return index;
    }

    //! Free a finished player's slot
    void free_player(unsigned_type slot)
    {
        // push on the stack of free segment indices
        free_slots.push(slot);
    }

    //! Whether there is still space for new array
    bool is_space_available() const
    {
        return (this->k < arity) || !free_slots.empty();
    }

    /*!
     * Update loser tree on insert or decrement of player index first go up the
     * tree all the way to the root hand down old winner for the respective
     * subtree based on new value, and old winner and loser update each node on
     * the path to the root top down.  This is implemented recursively
     */
    void update_on_insert(unsigned_type node,
                          const value_type& newKey, unsigned_type newIndex,
                          value_type* winner_key,
                          unsigned_type* winner_index,   // old winner
                          unsigned_type* mask)           // 1 << (ceil(log KNK) - dist-from-root)
    {
        unsigned_type& logK = this->logK;
        CompareType& cmp = this->cmp;
        typename super_type::Entry* entry = this->entry;

        if (node == 0)
        {
            // winner part of root
            *mask = (unsigned_type)(1) << (logK - 1);
            *winner_key = entry[0].key;
            *winner_index = entry[0].index;
            if (cmp(entry[node].key, newKey))
            {
                entry[node].key = newKey;
                entry[node].index = newIndex;
            }
        }
        else
        {
            update_on_insert(node >> 1, newKey, newIndex, winner_key, winner_index, mask);
            value_type loserKey = entry[node].key;
            unsigned_type loserIndex = entry[node].index;
            if ((*winner_index & *mask) != (newIndex & *mask)) {     // different subtrees
                if (cmp(loserKey, newKey)) {                        // newKey will have influence here
                    if (cmp(*winner_key, newKey)) {                  // old winner loses here
                        entry[node].key = *winner_key;
                        entry[node].index = *winner_index;
                    }
                    else {                                          // new entry loses here
                        entry[node].key = newKey;
                        entry[node].index = newIndex;
                    }
                }
                *winner_key = loserKey;
                *winner_index = loserIndex;
            }
            // note that nothing needs to be done if the winner came from the
            // same subtree
            // a) newKey <= winner_key => even more reason for the other tree to lose
            // b) newKey >  winner_key => the old winner will beat the new
            //                           entry further down the tree
            // also the same old winner is handed down the tree

            *mask >>= 1;     // next level
        }
    }

    //! Initial call to recursive update_on_insert
    void update_on_insert(unsigned_type node,
                          const value_type& newKey, unsigned_type newIndex)
    {
        value_type dummyKey;
        unsigned_type dummyIndex, dummyMask;
        update_on_insert(node, newKey, newIndex,
                         &dummyKey, &dummyIndex, &dummyMask);
    }

    //! make the tree twice as wide
    void double_k()
    {
        unsigned_type& k = this->k;
        unsigned_type& logK = this->logK;
        internal_bounded_stack<unsigned_type, Arity>& free_slots = this->free_slots;

        STXXL_VERBOSE1("double_k (before) k=" << k << " logK=" << logK << " arity=" << arity << " max_arity=" << max_arity << " #free=" << free_slots.size());
        assert(k > 0);
        assert(k < arity);
        assert(free_slots.empty());                 // stack was free (probably not needed)

        // make all new entries free
        // and push them on the free stack
        for (unsigned_type i = 2 * k - 1; i >= k; i--) //backwards
        {
            this->make_array_sentinel(i);
            if (i < arity)
                free_slots.push(i);
        }

        // double the size
        k *= 2;
        logK++;

        STXXL_VERBOSE1("double_k (after)  k=" << k << " logK=" << logK << " arity=" << arity << " max_arity=" << max_arity << " #free=" << free_slots.size());
        assert(!free_slots.empty());
        assert(k <= max_arity);

        // recompute loser tree information
        this->rebuild_loser_tree();
    }

    //! compact nonempty segments in the left half of the tree
    void compact_tree()
    {
        unsigned_type& k = this->k;
        unsigned_type& logK = this->logK;
        internal_bounded_stack<unsigned_type, Arity>& free_slots = this->free_slots;

        STXXL_VERBOSE3("compact_tree (before) k=" << k << " logK=" << logK << " #free=" << free_slots.size());
        assert(logK > 0);

        // compact all nonempty segments to the left
        unsigned_type last_empty = 0;
        for (unsigned_type pos = 0; pos < k; pos++)
        {
            if (!is_array_empty(pos))
            {
                assert(is_array_allocated(pos));
                if (pos != last_empty)
                {
                    assert(!is_array_allocated(last_empty));
                    this->swap_arrays(last_empty, pos);
                }
                ++last_empty;
            }
            /*
              else
              {
              if(segment[pos])
              {
              STXXL_VERBOSE2("int_arrays::compact_tree() deleting segment "<<pos<<
              " address: "<<segment[pos]<<" size: "<<segment_size[pos]);
              delete [] segment[pos];
              segment[pos] = 0;
              mem_cons_ -= segment_size[pos];
              }
              }*/
        }

        // half degree as often as possible
        while ((k > 1) && last_empty <= (k / 2))
        {
            k /= 2;
            logK--;
        }

        // overwrite garbage and compact the stack of free segment indices
        free_slots.clear(); // none free
        for ( ; last_empty < k; last_empty++)
        {
            assert(!is_array_allocated(last_empty));
            this->make_array_sentinel(last_empty);
            if (last_empty < arity)
                free_slots.push(last_empty);
        }

        STXXL_VERBOSE3("compact_tree (after)  k=" << k << " logK=" << logK << " #free=" << free_slots.size());

        // recompute loser tree information
        this->rebuild_loser_tree();
    }

    // multi-merge for arbitrary K
    template <class OutputIterator>
    void multi_merge_k(OutputIterator begin, OutputIterator end)
    {
        SequenceType* states = this->get_sequences();
        Entry* current_pos;
        value_type current_key;
        unsigned_type current_index; // leaf pointed to by current entry
        unsigned_type kReg = this->k;
        unsigned_type winner_index = this->entry[0].index;
        value_type winner_key = this->entry[0].key;

        while (begin != end)
        {
            // write result
            *begin++ = *(states[winner_index]);

            // advance winner segment
            ++(states[winner_index]);

            winner_key = *(states[winner_index]);

            // remove winner segment if empty now
            if (is_sentinel(winner_key)) {
                this->free_array(winner_index);
                this->free_player(winner_index);
            }

            // go up the entry-tree
            for (unsigned_type i = (winner_index + kReg) >> 1; i > 0; i >>= 1)
            {
                current_pos = this->entry + i;
                current_key = current_pos->key;
                if (cmp(winner_key, current_key))
                {
                    current_index = current_pos->index;
                    current_pos->key = winner_key;
                    current_pos->index = winner_index;
                    winner_key = current_key;
                    winner_index = current_index;
                }
            }
        }
        this->entry[0].index = winner_index;
        this->entry[0].key = winner_key;
    }

    template <int LogK, typename OutputIterator>
    void multi_merge_f(OutputIterator begin, OutputIterator end)
    {
        SequenceType* reg_states = this->get_sequences();
        Entry* reg_entry = this->entry;
        unsigned_type winner_index = reg_entry[0].index;
        value_type winner_key = reg_entry[0].key;

        // TODO: reinsert assert(log_k >= LogK);
        while (begin != end)
        {
            // write result
            *begin++ = *(reg_states[winner_index]);

            // advance winner segment
            ++(reg_states[winner_index]);

            winner_key = *(reg_states[winner_index]);

            // remove winner segment if empty now
            if (is_sentinel(winner_key)) {
                this->free_array(winner_index);
                this->free_player(winner_index);
            }

            // update loser tree
#define TreeStep(L)                                                     \
    if (1 << LogK >= 1 << L) {                                          \
        int pos_shift = ((int(LogK - L) + 1) >= 0) ? ((LogK - L) + 1) : 0; \
        Entry* pos = reg_entry + ((winner_index + (1 << LogK)) >> pos_shift); \
        value_type key = pos->key;                              \
        if (cmp(winner_key, key)) {                             \
            unsigned_type index = pos->index;                   \
            pos->key = winner_key;                              \
            pos->index = winner_index;                          \
            winner_key = key;                                   \
            winner_index = index;                               \
        }                                                       \
    }
            TreeStep(10);
            TreeStep(9);
            TreeStep(8);
            TreeStep(7);
            TreeStep(6);
            TreeStep(5);
            TreeStep(4);
            TreeStep(3);
            TreeStep(2);
            TreeStep(1);
#undef TreeStep
        }
        reg_entry[0].index = winner_index;
        reg_entry[0].key = winner_key;
    }

    void swap(loser_tree& obj)
    {
        std::swap(free_slots, obj.free_slots);
    }
};

/////////////////////////////////////////////////////////////////////
// auxiliary functions

// merge length elements from the two sentinel terminated input
// sequences source0 and source1 to target
// advance source0 and source1 accordingly
// require: at least length nonsentinel elements available in source0, source1
// require: target may overwrite one of the sources as long as
//   *(sourcex + length) is before the end of sourcex
template <class InputIterator, class OutputIterator,
          class CompareType, typename SizeType>
void merge_iterator(
    InputIterator& source0,
    InputIterator& source1,
    OutputIterator target,
    SizeType length,
    CompareType& cmp)
{
    OutputIterator done = target + length;

    while (target != done)
    {
        if (cmp(*source0, *source1))
        {
            *target = *source1;
            ++source1;
        }
        else
        {
            *target = *source0;
            ++source0;
        }
        ++target;
    }
}

// merge length elements from the three sentinel terminated input
// sequences source0, source1 and source2 to target
// advance source0, source1 and source2 accordingly
// require: at least length nonsentinel elements available in source0, source1 and source2
// require: target may overwrite one of the sources as long as
//   *(sourcex + length) is before the end of sourcex
template <class InputIterator, class OutputIterator,
          class CompareType, typename SizeType>
void merge3_iterator(
    InputIterator& source0,
    InputIterator& source1,
    InputIterator& source2,
    OutputIterator target,
    SizeType length,
    CompareType& cmp)
{
    OutputIterator done = target + length;

    if (cmp(*source1, *source0)) {
        if (cmp(*source2, *source1)) {
            goto s012;
        }
        else {
            if (cmp(*source0, *source2)) {
                goto s201;
            }
            else {
                goto s021;
            }
        }
    } else {
        if (cmp(*source2, *source1)) {
            if (cmp(*source2, *source0)) {
                goto s102;
            }
            else {
                goto s120;
            }
        } else {
            goto s210;
        }
    }

#define Merge3Case(a, b, c)              \
    s ## a ## b ## c :                   \
    if (target == done)                  \
        return;                          \
    *target = *source ## a;              \
    ++target;                            \
    ++source ## a;                       \
    if (cmp(*source ## b, *source ## a)) \
        goto s ## a ## b ## c;           \
    if (cmp(*source ## c, *source ## a)) \
        goto s ## b ## a ## c;           \
    goto s ## b ## c ## a;

    // the order is chosen in such a way that
    // four of the trailing gotos can be eliminated by the optimizer
    Merge3Case(0, 1, 2);
    Merge3Case(1, 2, 0);
    Merge3Case(2, 0, 1);
    Merge3Case(1, 0, 2);
    Merge3Case(0, 2, 1);
    Merge3Case(2, 1, 0);

#undef Merge3Case
}

// merge length elements from the four sentinel terminated input
// sequences source0, source1, source2 and source3 to target
// advance source0, source1, source2 and source3 accordingly
// require: at least length nonsentinel elements available in source0, source1, source2 and source3
// require: target may overwrite one of the sources as long as
//   *(sourcex + length) is before the end of sourcex
template <class InputIterator, class OutputIterator,
          class CompareType, typename SizeType>
void merge4_iterator(
    InputIterator& source0,
    InputIterator& source1,
    InputIterator& source2,
    InputIterator& source3,
    OutputIterator target, SizeType length, CompareType& cmp)
{
    OutputIterator done = target + length;

#define StartMerge4(a, b, c, d)               \
    if ((!cmp(*source ## a, *source ## b)) && \
        (!cmp(*source ## b, *source ## c)) && \
        (!cmp(*source ## c, *source ## d)))   \
        goto s ## a ## b ## c ## d;

    // b>a c>b d>c
    // a<b b<c c<d
    // a<=b b<=c c<=d
    // !(a>b) !(b>c) !(c>d)

    StartMerge4(0, 1, 2, 3);
    StartMerge4(1, 2, 3, 0);
    StartMerge4(2, 3, 0, 1);
    StartMerge4(3, 0, 1, 2);

    StartMerge4(0, 3, 1, 2);
    StartMerge4(3, 1, 2, 0);
    StartMerge4(1, 2, 0, 3);
    StartMerge4(2, 0, 3, 1);

    StartMerge4(0, 2, 3, 1);
    StartMerge4(2, 3, 1, 0);
    StartMerge4(3, 1, 0, 2);
    StartMerge4(1, 0, 2, 3);

    StartMerge4(2, 0, 1, 3);
    StartMerge4(0, 1, 3, 2);
    StartMerge4(1, 3, 2, 0);
    StartMerge4(3, 2, 0, 1);

    StartMerge4(3, 0, 2, 1);
    StartMerge4(0, 2, 1, 3);
    StartMerge4(2, 1, 3, 0);
    StartMerge4(1, 3, 0, 2);

    StartMerge4(1, 0, 3, 2);
    StartMerge4(0, 3, 2, 1);
    StartMerge4(3, 2, 1, 0);
    StartMerge4(2, 1, 0, 3);

#define Merge4Case(a, b, c, d)               \
    s ## a ## b ## c ## d :                  \
    if (target == done)                      \
        return;                              \
    *target = *source ## a;                  \
    ++target;                                \
    ++source ## a;                           \
    if (cmp(*source ## c, *source ## a))     \
    {                                        \
        if (cmp(*source ## b, *source ## a)) \
            goto s ## a ## b ## c ## d;      \
        else                                 \
            goto s ## b ## a ## c ## d;      \
    }                                        \
    else                                     \
    {                                        \
        if (cmp(*source ## d, *source ## a)) \
            goto s ## b ## c ## a ## d;      \
        else                                 \
            goto s ## b ## c ## d ## a;      \
    }

    Merge4Case(0, 1, 2, 3);
    Merge4Case(1, 2, 3, 0);
    Merge4Case(2, 3, 0, 1);
    Merge4Case(3, 0, 1, 2);

    Merge4Case(0, 3, 1, 2);
    Merge4Case(3, 1, 2, 0);
    Merge4Case(1, 2, 0, 3);
    Merge4Case(2, 0, 3, 1);

    Merge4Case(0, 2, 3, 1);
    Merge4Case(2, 3, 1, 0);
    Merge4Case(3, 1, 0, 2);
    Merge4Case(1, 0, 2, 3);

    Merge4Case(2, 0, 1, 3);
    Merge4Case(0, 1, 3, 2);
    Merge4Case(1, 3, 2, 0);
    Merge4Case(3, 2, 0, 1);

    Merge4Case(3, 0, 2, 1);
    Merge4Case(0, 2, 1, 3);
    Merge4Case(2, 1, 3, 0);
    Merge4Case(1, 3, 0, 2);

    Merge4Case(1, 0, 3, 2);
    Merge4Case(0, 3, 2, 1);
    Merge4Case(3, 2, 1, 0);
    Merge4Case(2, 1, 0, 3);

#undef StartMerge4
#undef Merge4Case
}

} // namespace priority_queue_local

//! \}

STXXL_END_NAMESPACE

#endif // !STXXL_CONTAINERS_PQ_MERGERS_HEADER
// vim: et:ts=4:sw=4
