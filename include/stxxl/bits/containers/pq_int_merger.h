/***************************************************************************
 *  include/stxxl/bits/containers/pq_int_merger.h
 *
 *  Part of the STXXL. See http://stxxl.org
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

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

#include <tlx/die.hpp>

#include <stxxl/bits/containers/pq_mergers.h>

namespace stxxl {

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local {

template <class ValueType, class CompareType, unsigned MaxArity>
class int_merger
{
    static constexpr bool debug = false;

public:
    //! type of values in merger
    using value_type = ValueType;
    //! comparator object type
    using compare_type = CompareType;

    enum { max_arity = MaxArity };

    //! our type
    using self_type = int_merger<ValueType, CompareType, MaxArity>;

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
    //! type of embedded adapter to parallel multiway_merge
    using tree_type = parallel_merger_adapter<self_type, CompareType, MaxArity>;
#else
    //! type of embedded loser tree
    using tree_type = loser_tree<self_type, CompareType, MaxArity>;
#endif

    //! type of sequences in which the values are stored: memory arrays
    using sequence_type = value_type *;

    //! size type of total number of item in merger
    using size_type = size_t;

protected:
    //! loser tree instance
    tree_type tree;

    //! target of free segment pointers
    value_type sentinel;

    // leaf information.  note that Knuth uses indices k..k-1, while we use
    // 0..k-1

    //! pointer to current element
    value_type* current[MaxArity];
    //! pointer to end of block for current element
    value_type* current_end[MaxArity];
    //! start of Segments
    value_type* segment[MaxArity];
    //! just to count the internal memory consumption, in bytes
    size_t segment_size[MaxArity];

    size_t mem_cons_;

    //! total number of elements stored
    size_type m_size;

public:
    //! \name Interface for loser_tree
    //! \{

    //! is this array invalid? here: empty and prefixed with sentinel?
    bool is_array_empty(const size_t slot) const
    {
        assert(slot < MaxArity);
        return tree.is_sentinel(*(current[slot]));
    }

    //! is this array's backing memory still allocated or does the current
    //! point to sentinel?
    bool is_array_allocated(const size_t slot) const
    {
        return current[slot] != &sentinel;
    }

    //! Return the item sequence of the given slot
    sequence_type & get_array(const size_t slot)
    {
        return current[slot];
    }

    //! Swap contents of arrays a and b
    void swap_arrays(const size_t a, const size_t b)
    {
        std::swap(current[a], current[b]);
        std::swap(current_end[a], current_end[b]);
        std::swap(segment[a], segment[b]);
        std::swap(segment_size[a], segment_size[b]);
    }

    //! Set a usually empty array to the sentinel
    void make_array_sentinel(size_t slot)
    {
        current[slot] = &sentinel;
        current_end[slot] = &sentinel;
        segment[slot] = nullptr;
    }

    //! free an empty segment .
    void free_array(size_t slot)
    {
        // reroute current pointer to some empty sentinel segment
        // with a sentinel key
        LOG << "int_merger::free_array() deleting array " <<
            slot << " address: " << segment[slot] << " size: " << (segment_size[slot] / sizeof(value_type)) - 1;
        current[slot] = &sentinel;
        current_end[slot] = &sentinel;

        // free memory
        delete[] segment[slot];
        segment[slot] = nullptr;
        mem_cons_ -= segment_size[slot];

        // free player in loser tree
        tree.free_player(slot);
    }

    //! Hint (prefetch) first non-internal (actually second) block of each
    //! sequence. No-operation for internal arrays.
    void prefetch_arrays()
    { }

    //! \}

public:
    explicit int_merger(const compare_type& c = compare_type())
        : tree(c, *this),
          sentinel(c.min_value()),
          mem_cons_(0),
          m_size(0)
    {
        segment[0] = nullptr;
        current[0] = &sentinel;
        current_end[0] = &sentinel;

        // entry and sentinel are initialized by init since they need the value
        // of supremum
        tree.initialize();
    }

    //! non-copyable: delete copy-constructor
    int_merger(const int_merger&) = delete;
    //! non-copyable: delete assignment operator
    int_merger& operator = (const int_merger&) = delete;

    ~int_merger()
    {
        LOG << "int_merger::~int_merger()";
        for (size_t i = 0; i < tree.k; ++i)
        {
            if (segment[i])
            {
                LOG << "int_merger::~int_merger() deleting segment " << i;
                delete[] segment[i];
                mem_cons_ -= segment_size[i];
            }
        }
        // check whether we have not lost any memory
        assert(mem_cons_ == 0);
    }

    void swap(int_merger& obj)
    {
        std::swap(sentinel, obj.sentinel);
        foxxll::swap_1D_arrays(current, obj.current, MaxArity);
        foxxll::swap_1D_arrays(current_end, obj.current_end, MaxArity);
        foxxll::swap_1D_arrays(segment, obj.segment, MaxArity);
        foxxll::swap_1D_arrays(segment_size, obj.segment_size, MaxArity);
        std::swap(mem_cons_, obj.mem_cons_);
    }

    size_t mem_cons() const { return mem_cons_; }

    //! Whether there is still space for new array
    bool is_space_available() const
    {
        return tree.is_space_available();
    }

    //! True if a is the sentinel value
    bool is_sentinel(const value_type& a) const
    {
        return tree.is_sentinel(a);
    }

    //! append array to merger, takes ownership of the array.
    //! requires: is_space_available() == 1
    void append_array(value_type* target, const size_t length)
    {
        LOG << "int_merger::insert_segment(" << target << "," << length << ")";
        //std::copy(target,target + length,std::ostream_iterator<ValueType>(std::cout, "\n"));

        if (length == 0)
        {
            // immediately deallocate this is not only an optimization but also
            // needed to keep free segments from clogging up the tree
            delete[] target;
            return;
        }

        assert(!tree.is_sentinel(target[0]));
        assert(!tree.is_sentinel(target[length - 1]));
        assert(tree.is_sentinel(target[length]));

        // allocate a new player slot
        const size_t index = tree.new_player();

        assert(current[index] == &sentinel);

        // link new segment
        current[index] = segment[index] = target;
        current_end[index] = target + length;
        segment_size[index] = (length + 1) * sizeof(value_type);
        mem_cons_ += (length + 1) * sizeof(value_type);
        m_size += length;

        // propagate new information up the tree
        tree.update_on_insert((index + tree.k) >> 1, *target, index);
    }

    //! Return the number of items in the arrays
    size_type size() const
    {
        return m_size;
    }

    //! extract the (length = end - begin) smallest elements and write them to
    //! [begin..end) empty segments are deallocated. Requires: there are at
    //! least length elements and segments are ended by sentinels.
    void multi_merge(value_type* begin, value_type* end)
    {
        assert(begin + m_size >= end);

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
        multi_merge_parallel(begin, end - begin);
#else
        tree.multi_merge(begin, end);
#endif

        m_size -= end - begin;
    }

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL

protected:
    //! extract the (length = end - begin) smallest elements using parallel
    //! multiway_merge.
    void multi_merge_parallel(value_type* target, const size_t length)
    {
        const size_t& k = tree.k;
        const size_t& logK = tree.logK;
        compare_type& cmp = tree.cmp;

        LOG << "int_merger::multi_merge_parallel(target=" << target << ", len=" << length << ") k=" << k;

        if (length == 0)
            return;

        assert(k > 0);

        die_unless(MaxArity > logK || MaxArity >= 8);

        //This is the place to make statistics about internal multi_merge calls.

        invert_order<CompareType, value_type, value_type> inv_cmp(cmp);
        switch (logK) {
        case 0: {
            assert(MaxArity >= 1);
            assert(k == 1);

            memcpy(target, current[0], length * sizeof(value_type));
            current[0] += length;

            if (is_array_empty(0) && is_array_allocated(0))
                free_array(0);

            break;
        }
        case 1: {
            assert(MaxArity >= 2);
            assert(k == 2);

            std::pair<value_type*, value_type*> seqs[2] =
            {
                std::make_pair(current[0], current_end[0]),
                std::make_pair(current[1], current_end[1])
            };

            parallel::multiway_merge_sentinels(
                seqs, seqs + 2, target, length, inv_cmp);

            current[0] = seqs[0].first;
            current[1] = seqs[1].first;

            if (is_array_empty(0) && is_array_allocated(0))
                free_array(0);

            if (is_array_empty(1) && is_array_allocated(1))
                free_array(1);

            break;
        }
        case 2: {
            assert(MaxArity >= 4);
            assert(k == 4);

            std::pair<value_type*, value_type*> seqs[4] =
            {
                std::make_pair(current[0], current_end[0]),
                std::make_pair(current[1], current_end[1]),
                std::make_pair(current[2], current_end[2]),
                std::make_pair(current[3], current_end[3])
            };

            parallel::multiway_merge_sentinels(
                seqs, seqs + 4, target, length, inv_cmp);

            current[0] = seqs[0].first;
            current[1] = seqs[1].first;
            current[2] = seqs[2].first;
            current[3] = seqs[3].first;

            if (is_array_empty(0) && is_array_allocated(0))
                free_array(0);

            if (is_array_empty(1) && is_array_allocated(1))
                free_array(1);

            if (is_array_empty(2) && is_array_allocated(2))
                free_array(2);

            if (is_array_empty(3) && is_array_allocated(3))
                free_array(3);

            break;
        }
        default: {
            assert(MaxArity >= 8);
            std::vector<std::pair<value_type*, value_type*> > seqs;
            std::vector<size_t> orig_seq_index;
            for (size_t i = 0; i < k; ++i)
            {
                if (current[i] != current_end[i] && !is_sentinel(*current[i]))
                {
                    seqs.push_back(std::make_pair(current[i], current_end[i]));
                    orig_seq_index.push_back(i);
                }
            }

            parallel::multiway_merge_sentinels(
                seqs.begin(), seqs.end(), target, length, inv_cmp);

            for (size_t i = 0; i < seqs.size(); ++i)
            {
                const size_t& seg = orig_seq_index[i];
                current[seg] = seqs[i].first;
            }

            for (size_t i = 0; i < k; ++i)
            {
                if (is_array_empty(i) && is_array_allocated(i))
                {
                    LOG << "deallocated " << i;
                    free_array(i);
                }
            }
            break;
        }
        }

        tree.maybe_compact();
    }
#endif // STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_INTERNAL
};

} // namespace priority_queue_local

//! \}

} // namespace stxxl

#endif // !STXXL_CONTAINERS_PQ_INT_MERGER_HEADER
// vim: et:ts=4:sw=4
