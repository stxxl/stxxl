/***************************************************************************
 *  include/stxxl/bits/containers/pq_int_merger.h
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

#ifndef STXXL_CONTAINERS_PQ_INT_MERGER_HEADER
#define STXXL_CONTAINERS_PQ_INT_MERGER_HEADER

#include <stxxl/bits/containers/pq_helpers.h>

STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local {

//////////////////////////////////////////////////////////////////////
// The data structure from Knuth, "Sorting and Searching", Section 5.4.1
/*!
 * Loser tree from Knuth, "Sorting and Searching", Section 5.4.1
 * \param  MaxArity  maximum arity of loser tree, has to be a power of two
 */
template <class ValueType, class CompareType, unsigned MaxArity>
class int_arrays : private noncopyable
{
public:
    typedef ValueType value_type;
    typedef CompareType comparator_type;
    enum { max_arity = MaxArity };

protected:
#if STXXL_PQ_INTERNAL_LOSER_TREE
    struct Entry
    {
        value_type key;          // Key of Loser element (winner for 0)
        unsigned_type index;     // number of losing segment
    };
#endif //STXXL_PQ_INTERNAL_LOSER_TREE

    comparator_type cmp;
    // stack of free segment indices
    internal_bounded_stack<unsigned_type, MaxArity> free_slots;

    unsigned_type m_size;     // total number of elements stored
    unsigned_type logK;      // log of current tree size
    unsigned_type k;         // invariant (k == 1 << logK), always a power of two

    value_type sentinel;        // target of free segment pointers

#if STXXL_PQ_INTERNAL_LOSER_TREE
    // upper levels of loser trees
    // entry[0] contains the winner info
    Entry entry[MaxArity];
#endif  //STXXL_PQ_INTERNAL_LOSER_TREE

    // leaf information
    // note that Knuth uses indices k..k-1
    // while we use 0..k-1
    value_type* current[MaxArity];               // pointer to current element
    value_type* current_end[MaxArity];           // pointer to end of block for current element
    value_type* segment[MaxArity];               // start of Segments
    unsigned_type segment_size[MaxArity];     // just to count the internal memory consumption, in bytes

    unsigned_type mem_cons_;

    // private member functions
    unsigned_type init_winner(unsigned_type root);
    void deallocate_segment(unsigned_type slot);
    void rebuild_loser_tree();

    //! is this array invalid: empty and prefixed with sentinel?
    bool is_array_empty(unsigned_type slot) const
    {
        return is_sentinel(*(current[slot]));
    }

    //! is this array's backing memory still allocated or does the current
    //! point to sentinel?
    bool is_array_allocated(unsigned_type slot) const
    {
        return current[slot] != &sentinel;
    }

    void swap_arrays(unsigned_type a, unsigned_type b)
    {
        std::swap(current[a], current[b]);
        std::swap(current_end[a], current_end[b]);
        std::swap(segment[a], segment[b]);
        std::swap(segment_size[a], segment_size[b]);
    }

    void make_array_sentinel(unsigned_type slot)
    {
        assert(is_array_empty(slot));
        current[slot] = &sentinel;
        current_end[slot] = &sentinel;
        segment[slot] = NULL;
    }

public:
    bool is_sentinel(const value_type& a) const
    {
        return !(cmp(cmp.min_value(), a));
    }
    bool not_sentinel(const value_type& a) const
    {
        return cmp(cmp.min_value(), a);
    }

    typedef value_type* SequenceType;

    SequenceType* get_sequences()
    {
        return current;
    }

public:
    int_arrays()
        : m_size(0), logK(0), k(1), mem_cons_(0)
    {
        free_slots.push(0);
        segment[0] = NULL;
        current[0] = &sentinel;
        current_end[0] = &sentinel;

        // entry and sentinel are initialized by init since they need the value
        // of supremum
        init();
    }

    void init()
    {
        // verify strict weak ordering
        assert(!cmp(cmp.min_value(), cmp.min_value()));

        sentinel = cmp.min_value();
        rebuild_loser_tree();
#if STXXL_PQ_INTERNAL_LOSER_TREE
        assert(current[entry[0].index] == &sentinel);
#endif  //STXXL_PQ_INTERNAL_LOSER_TREE
    }

    ~int_arrays()
    {
        STXXL_VERBOSE1("int_arrays::~int_arrays()");
        for (unsigned_type i = 0; i < k; ++i)
        {
            if (segment[i])
            {
                STXXL_VERBOSE2("int_arrays::~int_arrays() deleting segment " << i);
                delete[] segment[i];
                mem_cons_ -= segment_size[i];
            }
        }
        // check whether we have not lost any memory
        assert(mem_cons_ == 0);
    }

    void swap(int_arrays& obj)
    {
        std::swap(cmp, obj.cmp);
        std::swap(free_slots, obj.free_slots);
        std::swap(m_size, obj.m_size);
        std::swap(logK, obj.logK);
        std::swap(k, obj.k);
        std::swap(sentinel, obj.sentinel);
#if STXXL_PQ_INTERNAL_LOSER_TREE
        swap_1D_arrays(entry, obj.entry, MaxArity);
#endif      //STXXL_PQ_INTERNAL_LOSER_TREE
        swap_1D_arrays(current, obj.current, MaxArity);
        swap_1D_arrays(current_end, obj.current_end, MaxArity);
        swap_1D_arrays(segment, obj.segment, MaxArity);
        swap_1D_arrays(segment_size, obj.segment_size, MaxArity);
        std::swap(mem_cons_, obj.mem_cons_);
    }

public:
    unsigned_type mem_cons() const { return mem_cons_; }

    bool is_space_available() const     // for new segment
    {
        return (k < MaxArity) || !free_slots.empty();
    }

    //! insert array to merger at index, takes ownership of the array.
    //! requires: is_space_available() == 1
    void insert_array(unsigned_type index, value_type* target, unsigned_type length)
    {
        assert(current[index] == &sentinel);

        // link new segment
        current[index] = segment[index] = target;
        current_end[index] = target + length;
        segment_size[index] = (length + 1) * sizeof(value_type);
        mem_cons_ += (length + 1) * sizeof(value_type);
        m_size += length;
    }

    unsigned_type size() const { return m_size; }
};

///////////////////////// LoserTree ///////////////////////////////////

// rebuild loser tree information from the values in current
template <class ValueType, class CompareType, unsigned MaxArity>
void int_arrays<ValueType, CompareType, MaxArity>::rebuild_loser_tree()
{
#if STXXL_PQ_INTERNAL_LOSER_TREE
    // MaxArity needs to be a power of two
    assert(LOG2<MaxArity>::floor == LOG2<MaxArity>::ceil);
    unsigned_type winner = init_winner(1);
    entry[0].index = winner;
    entry[0].key = *(current[winner]);
#endif  //STXXL_PQ_INTERNAL_LOSER_TREE
}

#if STXXL_PQ_INTERNAL_LOSER_TREE
// given any values in the leaves this
// routing recomputes upper levels of the tree
// from scratch in linear time
// initialize entry[root].index and the subtree rooted there
// return winner index
template <class ValueType, class CompareType, unsigned MaxArity>
unsigned_type int_arrays<ValueType, CompareType, MaxArity>::init_winner(unsigned_type root)
{
    if (root >= k) {     // leaf reached
        return root - k;
    }
    else {
        unsigned_type left = init_winner(2 * root);
        unsigned_type right = init_winner(2 * root + 1);
        value_type lk = *(current[left]);
        value_type rk = *(current[right]);
        if (!(cmp(lk, rk))) {     // right subtree loses
            entry[root].index = right;
            entry[root].key = rk;
            return left;
        } else {
            entry[root].index = left;
            entry[root].key = lk;
            return right;
        }
    }
}

#endif //STXXL_PQ_INTERNAL_LOSER_TREE

// free an empty segment .
template <class ValueType, class CompareType, unsigned MaxArity>
void int_arrays<ValueType, CompareType, MaxArity>::
deallocate_segment(unsigned_type slot)
{
    // reroute current pointer to some empty sentinel segment
    // with a sentinel key
    STXXL_VERBOSE2("int_arrays::deallocate_segment() deleting segment " <<
                   slot << " address: " << segment[slot] << " size: " << (segment_size[slot] / sizeof(value_type)) - 1);
    current[slot] = &sentinel;
    current_end[slot] = &sentinel;

    // free memory
    delete[] segment[slot];
    segment[slot] = NULL;
    mem_cons_ -= segment_size[slot];

    // push on the stack of free segment indices
    free_slots.push(slot);
}

template <class ValueType, class CompareType, unsigned MaxArity>
class int_merger : public loser_tree<
    int_arrays<ValueType, CompareType, MaxArity>,
    CompareType, MaxArity
    >
{
public:
    typedef ValueType value_type;

    typedef int_arrays<ValueType, CompareType, MaxArity> ArraysType;
    typedef loser_tree<ArraysType, CompareType, MaxArity> super_type;

    typedef loser_tree<ArraysType, CompareType, MaxArity> tree_type;
    typedef ArraysType arrays_type;

    void multi_merge(value_type* begin, value_type* end)
    {
        multi_merge(begin, end - begin);
    }

    bool is_array_empty(unsigned_type slot) const
    {
        return super_type::is_array_empty(slot);
    }

    bool is_array_allocated(unsigned_type slot) const
    {
        return super_type::is_array_allocated(slot);
    }

    //! append array to merger, takes ownership of the array.
    //! requires: is_space_available() == 1
    void append_array(value_type* target, unsigned_type length)
    {
        unsigned_type& k = this->k;

        STXXL_VERBOSE2("int_arrays::insert_segment(" << target << "," << length << ")");
        //std::copy(target,target + length,std::ostream_iterator<ValueType>(std::cout, "\n"));

        if (length == 0)
        {
            // immediately deallocate this is not only an optimization but also
            // needed to keep free segments from clogging up the tree
            delete[] target;
            return;
        }

        assert(not_sentinel(target[0]));
        assert(not_sentinel(target[length - 1]));
        assert(is_sentinel(target[length]));

        // allocate a new player slot
        int index = tree_type::new_player();

        // insert array into internal memory array list
        insert_array(index, target, length);

#if STXXL_PQ_INTERNAL_LOSER_TREE
        // propagate new information up the tree
        tree_type::update_on_insert((index + k) >> 1, *target, index);
#endif // STXXL_PQ_INTERNAL_LOSER_TREE
    }

    void deallocate_segment(unsigned_type slot)
    {
        return super_type::deallocate_segment(slot);
    }

    // delete the length smallest elements and write them to target
    // empty segments are deallocated
    // require:
    // - there are at least length elements
    // - segments are ended by sentinels
    void multi_merge(value_type* target, unsigned_type length)
    {
        unsigned_type& k = this->k;
        unsigned_type& logK = this->logK;
        unsigned_type& m_size = this->m_size;
        CompareType& cmp = this->cmp;
        typename super_type::Entry* entry = this->entry;
        internal_bounded_stack<unsigned_type, MaxArity>& free_slots = this->free_slots;
        value_type** current = this->current;

        STXXL_VERBOSE3("int_arrays::multi_merge(target=" << target << ", len=" << length << ") k=" << k);

        if (length == 0)
            return;

        assert(k > 0);
        assert(length <= m_size);

        //This is the place to make statistics about internal multi_merge calls.

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
        invert_order<CompareType, value_type, value_type> inv_cmp(cmp);
#endif
        switch (logK) {
        case 0:
            assert(k == 1);
#if STXXL_PQ_INTERNAL_LOSER_TREE
            assert(entry[0].index == 0);
#endif      //STXXL_PQ_INTERNAL_LOSER_TREE
            assert(free_slots.empty());
            memcpy(target, current[0], length * sizeof(value_type));
            //std::copy(current[0], current[0] + length, target);
            current[0] += length;
#if STXXL_PQ_INTERNAL_LOSER_TREE
            entry[0].key = **current;
#endif      //STXXL_PQ_INTERNAL_LOSER_TREE

            if (is_array_empty(0) && is_array_allocated(0))
                deallocate_segment(0);

            break;
        case 1:
            assert(k == 2);
#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
            {
                std::pair<value_type*, value_type*> seqs[2] =
                    {
                        std::make_pair(current[0], current_end[0]),
                        std::make_pair(current[1], current_end[1])
                    };
                parallel::multiway_merge_sentinel(seqs, seqs + 2, target, inv_cmp, length);
                current[0] = seqs[0].first;
                current[1] = seqs[1].first;
            }
#else
            merge_iterator(current[0], current[1], target, length, cmp);
            this->rebuild_loser_tree();
#endif
            if (is_array_empty(0) && is_array_allocated(0))
                deallocate_segment(0);

            if (is_array_empty(1) && is_array_allocated(1))
                deallocate_segment(1);

            break;
        case 2:
            assert(k == 4);
#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
            {
                std::pair<value_type*, value_type*> seqs[4] =
                    {
                        std::make_pair(current[0], current_end[0]),
                        std::make_pair(current[1], current_end[1]),
                        std::make_pair(current[2], current_end[2]),
                        std::make_pair(current[3], current_end[3])
                    };
                parallel::multiway_merge_sentinel(seqs, seqs + 4, target, inv_cmp, length);
                current[0] = seqs[0].first;
                current[1] = seqs[1].first;
                current[2] = seqs[2].first;
                current[3] = seqs[3].first;
            }
#else
            if (is_array_empty(3))
                merge3_iterator(current[0], current[1], current[2], target, length, cmp);
            else
                merge4_iterator(current[0], current[1], current[2], current[3], target, length, cmp);

            this->rebuild_loser_tree();
#endif
            if (is_array_empty(0) && is_array_allocated(0))
                deallocate_segment(0);

            if (is_array_empty(1) && is_array_allocated(1))
                deallocate_segment(1);

            if (is_array_empty(2) && is_array_allocated(2))
                deallocate_segment(2);

            if (is_array_empty(3) && is_array_allocated(3))
                deallocate_segment(3);

            break;
#if !(STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL)
        case  3: this->template multi_merge_f<3>(target, target + length);
            break;
        case  4: this->template multi_merge_f<4>(target, target + length);
            break;
        case  5: this->template multi_merge_f<5>(target, target + length);
            break;
        case  6: this->template multi_merge_f<6>(target, target + length);
            break;
        case  7: this->template multi_merge_f<7>(target, target + length);
            break;
        case  8: this->template multi_merge_f<8>(target, target + length);
            break;
        case  9: this->template multi_merge_f<9>(target, target + length);
            break;
        case 10: this->template multi_merge_f<10>(target, target + length);
            break;
#endif
        default:
#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
        {
            std::vector<std::pair<value_type*, value_type*> > seqs;
            std::vector<int_type> orig_seq_index;
            for (unsigned int i = 0; i < k; ++i)
            {
                if (current[i] != current_end[i] && !is_sentinel(*current[i]))
                {
                    seqs.push_back(std::make_pair(current[i], current_end[i]));
                    orig_seq_index.push_back(i);
                }
            }

            parallel::multiway_merge_sentinel(seqs.begin(), seqs.end(), target, inv_cmp, length);

            for (unsigned int i = 0; i < seqs.size(); ++i)
            {
                int_type seg = orig_seq_index[i];
                current[seg] = seqs[i].first;
            }

            for (unsigned int i = 0; i < k; ++i)
            {
                if (is_array_empty(i) && is_array_allocated(i))
                {
                    STXXL_VERBOSE3("deallocated " << i);
                    deallocate_segment(i);
                }
            }
        }
#else
        multi_merge_k(target, target + length);
#endif
        break;
        }

        m_size -= length;

        // compact tree if it got considerably smaller
        {
            const unsigned_type num_segments_used = k - free_slots.size();
            const unsigned_type num_segments_trigger = k - (3 * k / 5);
            // using k/2 would be worst case inefficient (for large k)
            // for k \in {2, 4, 8} the trigger is k/2 which is good
            // because we have special mergers for k \in {1, 2, 4}
            // there is also a special 3-way-merger, that will be
            // triggered if k == 4 && is_array_atsentinel(3)
            STXXL_VERBOSE3("int_arrays  compact? k=" << k << " #used=" << num_segments_used
                           << " <= #trigger=" << num_segments_trigger << " ==> "
                           << ((k > 1 && num_segments_used <= num_segments_trigger) ? "yes" : "no ")
                           << " || "
                           << ((k == 4 && !free_slots.empty() && !is_array_empty(3)) ? "yes" : "no ")
                           << " #free=" << free_slots.size());
            if (k > 1 &&
                ((num_segments_used <= num_segments_trigger) ||
                 (k == 4 && !free_slots.empty() && !is_array_empty(3))))
            {
                this->compact_tree();
            }
        }
        //std::copy(target,target + length,std::ostream_iterator<ValueType>(std::cout, "\n"));
    }
};

} // namespace priority_queue_local

//! \}

STXXL_END_NAMESPACE

#endif // !STXXL_CONTAINERS_PQ_INT_MERGER_HEADER
// vim: et:ts=4:sw=4
