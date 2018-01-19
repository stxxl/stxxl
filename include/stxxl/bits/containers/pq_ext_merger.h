/***************************************************************************
 *  include/stxxl/bits/containers/pq_ext_merger.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 1999 Peter Sanders <sanders@mpi-sb.mpg.de>
 *  Copyright (C) 2003, 2004, 2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2007-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_PQ_EXT_MERGER_HEADER
#define STXXL_CONTAINERS_PQ_EXT_MERGER_HEADER

#include <algorithm>
#include <deque>
#include <utility>
#include <vector>

#include <tlx/logger.hpp>

#include <stxxl/bits/containers/pq_mergers.h>
#include <stxxl/types>

namespace stxxl {

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local {

/*!
 * External merger, based on the loser tree data structure.
 * \param Arity  maximum arity of merger, does not need to be a power of 2
 */
template <class BlockType, class CompareType, unsigned Arity,
          class AllocStr = foxxll::default_alloc_strategy>
class ext_merger
{
    static constexpr bool debug = false;

public:
    //! class is parameterized by the block of the external arrays
    using block_type = BlockType;
    using compare_type = CompareType;

    // kMaxArity / 2  <  arity  <=  kMaxArity
    enum { arity = Arity, kMaxArity = 1UL << (tlx::Log2<Arity>::ceil) };

    using alloc_strategy = AllocStr;

    using bid_type = typename block_type::bid_type;
    using value_type = typename block_type::value_type;

    using bid_container_type = typename std::deque<bid_type>;

    using pool_type = foxxll::read_write_pool<block_type>;

    //! our type
    using self_type = ext_merger<BlockType, CompareType, Arity, AllocStr>;

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
    //! type of embedded adapter to parallel multiway_merge
    using tree_type = parallel_merger_adapter<self_type, CompareType, Arity>;
#else
    //! type of embedded loser tree
    using tree_type = loser_tree<self_type, CompareType, Arity>;
#endif

    //! size type of total number of item in merger
    using size_type = external_size_type;

public:
    struct sequence_state
    {
        block_type* block;       //!< current block
        size_t current;          //!< current index in current block
        bid_container_type bids; //!< list of blocks forming this sequence
        ext_merger* merger;
        compare_type cmp;
        bool allocated;

        //! \returns current element
        const value_type & current_front() const
        {
            return (*block)[current];
        }

        explicit sequence_state(const compare_type& cmp)
            : block(nullptr), current(0),
              merger(nullptr),
              cmp(cmp),
              allocated(false)
        { }

        //! non-copyable: delete copy-constructor
        sequence_state(const sequence_state&) = delete;
        //! non-copyable: delete assignment operator
        sequence_state& operator = (const sequence_state&) = delete;

        ~sequence_state()
        {
            LOG << "ext_merger sequence_state::~sequence_state()";

            foxxll::block_manager* bm = foxxll::block_manager::get_instance();
            bm->delete_blocks(bids.begin(), bids.end());
        }

        void make_inf()
        {
            current = 0;
            (*block)[0] = cmp.min_value();
        }

        bool is_sentinel(const value_type& a) const
        {
            return !(cmp(cmp.min_value(), a));
        }

        void swap(sequence_state& obj)
        {
            if (&obj != this)
            {
                std::swap(current, obj.current);
                std::swap(block, obj.block);
                std::swap(bids, obj.bids);
                assert(merger == obj.merger);
                std::swap(allocated, obj.allocated);
            }
        }

        sequence_state & advance()
        {
            assert(!is_sentinel((*block)[current]));
            assert(current < block->size);

            ++current;

            if (current == block->size)
            {
                LOG << "ext_merger sequence_state operator++ crossing block border ";
                // go to the next block
                if (bids.empty()) // if there is no next block
                {
                    LOG << "ext_merger sequence_state operator++ it was the last block in the sequence ";
                    // swap memory area and delete other object.
                    bid_container_type to_delete;
                    std::swap(to_delete, bids);
                    make_inf();
                }
                else
                {
                    LOG << "ext_merger sequence_state operator++ there is another block ";
                    bid_type bid = bids.front();
                    bids.pop_front();
                    merger->pool->hint(bid);
                    if (!(bids.empty()))
                    {
                        LOG << "ext_merger sequence_state operator++ more blocks exist in a sequence, hinting the next";
                        merger->pool->hint(bids.front());
                    }
                    merger->pool->read(block, bid)->wait();
                    //req-ostream LOG << "first element of read block " << bid << " " << *(block->begin()) << " cached in " << block;
                    if (!(bids.empty()))
                        merger->pool->hint(bids.front());  // re-hint, reading might have made a block free
                    foxxll::block_manager::get_instance()->delete_block(bid);
                    current = 0;
                }
            }
            return *this;
        }

        // legacy adapters
        sequence_state& operator ++ ()
        {
            return advance();
        }

        const value_type& operator * () const
        {
            return current_front();
        }
    };

    //! type of sequences in which the values are stored: external arrays
    using sequence_type = sequence_state;

protected:
    //! loser tree instance
    tree_type tree;

    //! sequence including current position, dereference gives current element
    sequence_state* states;

    //! read and writer block pool
    pool_type* pool;

    //! a memory block filled with sentinel values
    block_type* sentinel_block;

    //! total number of elements stored
    size_type m_size;

    compare_type cmp;

public:
    explicit ext_merger(const compare_type& cmp = compare_type()) // TODO: pass pool as parameter
        : tree(cmp, *this),
          pool(nullptr),
          m_size(0),
          cmp(cmp)
    {
        init();

        tree.initialize();
    }

    //! non-copyable: delete copy-constructor
    ext_merger(const ext_merger&) = delete;
    //! non-copyable: delete assignment operator
    ext_merger& operator = (const ext_merger&) = delete;

    virtual ~ext_merger()
    {
        LOG << "ext_merger::~ext_merger()";
        for (size_t i = 0; i < arity; ++i)
        {
            delete states[i].block;
        }
        delete sentinel_block;

        for (size_t i = 0; i < kMaxArity; ++i)
            states[i].~sequence_state();

        delete reinterpret_cast<uint8_t*>(states);
    }

    void set_pool(pool_type* pool_)
    {
        pool = pool_;
    }

public:
    //! \name Interface for loser_tree
    //! \{

    //! is this segment empty ?
    bool is_array_empty(const size_t slot) const
    {
        return is_sentinel(states[slot].current_front());
    }

    //! Is this segment allocated? Otherwise it's empty, already on the stack
    //! of free segment indices and can be reused.
    bool is_array_allocated(size_t slot) const
    {
        return states[slot].allocated;
    }

    //! Return the item sequence of the given slot
    sequence_type & get_array(const size_t slot)
    {
        return states[slot];
    }

    //! Swap contents of arrays a and b
    void swap_arrays(const size_t a, const size_t b)
    {
        states[a].swap(states[b]);
    }

    //! Set a usually empty array to the sentinel
    void make_array_sentinel(const size_t a)
    {
        states[a].make_inf();
    }

    //! free an empty segment.
    void free_array(const size_t slot)
    {
        LOG << "ext_merger::free_array() deleting array " << slot << " allocated=" << int(is_array_allocated(slot));
        assert(is_array_allocated(slot));
        states[slot].allocated = false;
        states[slot].make_inf();

        // free player in loser tree
        tree.free_player(slot);
    }

    //! Hint (prefetch) first non-internal (actually second) block of each
    //! sequence.
    void prefetch_arrays()
    {
        for (size_t i = 0; i < tree.k; ++i)
        {
            if (!states[i].bids.empty())
                pool->hint(states[i].bids.front());
        }
    }

    //! \}

protected:
    void init()
    {
        LOG << "ext_merger::init()";

        states = reinterpret_cast<sequence_state*>(new uint8_t[sizeof(sequence_state) * kMaxArity]);
        for (size_t i = 0; i < kMaxArity; ++i)
            new (&states[i])sequence_state(cmp);

        sentinel_block = nullptr;
        if (arity < kMaxArity)
        {
            sentinel_block = new block_type;
            for (size_t i = 0; i < block_type::size; ++i)
                (*sentinel_block)[i] = tree.cmp.min_value();

            // same memory consumption, but smaller merge width, better use arity = kMaxArity
            LOGC(arity + 1 == kMaxArity) << "inefficient PQ parameters for ext_merger: arity + 1 == kMaxArity";
        }

        for (size_t i = 0; i < kMaxArity; ++i)
        {
            states[i].merger = this;
            if (i < arity)
                states[i].block = new block_type;
            else
                states[i].block = sentinel_block;

            states[i].make_inf();
        }
    }

#if 0
    void swap(ext_merger& obj)
    {
        std::swap(cmp, obj.cmp);
        std::swap(k, obj.k);
        std::swap(logK, obj.logK);
        std::swap(m_size, obj.m_size);
        swap_1D_arrays(states, obj.states, kMaxArity);

        // std::swap(pool,obj.pool);
    }
#endif

public:
    size_t mem_cons() const // only rough estimation
    {
        return (std::min<size_t>(arity + 1, kMaxArity) * block_type::raw_size);
    }

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

    //! Return the number of items in the arrays
    size_type size() const
    {
        return m_size;
    }

    /*!
      \param bidlist list of blocks to insert
      \param first_block the first block of the sequence, before bidlist
      \param first_size number of elements in the first block
      \param slot slot to insert into
    */
    void insert_segment(bid_container_type& bidlist, block_type* first_block,
                        const size_t first_size, const size_t slot)
    {
        LOG << "ext_merger::insert_segment(bidlist,...) " << this << " " << bidlist.size() << " " << slot;
        assert(!is_array_allocated(slot));
        assert(first_size > 0);

        sequence_state& new_sequence = states[slot];
        new_sequence.current = block_type::size - first_size;
        std::swap(new_sequence.block, first_block);
        delete first_block;
        std::swap(new_sequence.bids, bidlist);
        new_sequence.allocated = true;
        assert(is_array_allocated(slot));
    }

    //! Merge all items from another merger and insert the resulting external
    //! array into the merger. Requires: is_space_available() == 1
    template <class Merger>
    void append_merger(Merger& another_merger, size_type segment_size)
    {
        LOG << "ext_merger::append_merger(merger,...)" << this;

        if (segment_size == 0)
        {
            // deallocate memory ?
            LOG << "Merged segment with zero size.";
            return;
        }

        // allocate a new player slot
        const size_t index = tree.new_player();

        // construct new sorted array from merger
        assert(segment_size);
        size_t nblocks = static_cast<size_t>(segment_size / block_type::size);
        //assert(nblocks); // at least one block
        LOG << "ext_merger::insert_segment nblocks=" << nblocks;
        if (nblocks == 0)
        {
            LOG << "ext_merger::insert_segment(merger,...) WARNING: inserting a segment with " << nblocks << " blocks";
            LOG << "THIS IS INEFFICIENT: TRY TO CHANGE PRIORITY QUEUE PARAMETERS";
        }
        size_t first_size = static_cast<size_t>(segment_size % block_type::size);
        if (first_size == 0)
        {
            first_size = block_type::size;
            --nblocks;
        }

        // allocate blocks
        foxxll::block_manager* bm = foxxll::block_manager::get_instance();
        bid_container_type bids(nblocks);
        bm->new_blocks(alloc_strategy(), bids.begin(), bids.end());
        block_type* first_block = new block_type;

        another_merger.multi_merge(
            first_block->begin() + (block_type::size - first_size),
            first_block->end());

        //req-ostream LOG << "last element of first block " << *(first_block->end() - 1);
        assert(!tree.cmp(*(first_block->begin() + (block_type::size - first_size)), *(first_block->end() - 1)));

        assert(pool->size_write() > 0);

        for (const auto& curbid : bids)
        {
            block_type* b = pool->steal();
            another_merger.multi_merge(b->begin(), b->end());
            //req-ostreamLOG << "first element of following block " << curbid << " " << *(b->begin());
            //req-ostreamLOG << "last element of following block " << curbid << " " << *(b->end() - 1);
            assert(!tree.cmp(*(b->begin()), *(b->end() - 1)));
            pool->write(b, curbid);
            LOG << "written to block " << curbid << " cached in " << b;
        }

        insert_segment(bids, first_block, first_size, index);

        m_size += segment_size;

        // propagate new information up the tree
        tree.update_on_insert((index + tree.k) >> 1, states[index].current_front(), index);
    }

    // delete the (length = end-begin) smallest elements and write them to [begin..end)
    // empty segments are deallocated
    // requires:
    // - there are at least length elements
    // - segments are ended by sentinels
    template <class OutputIterator>
    void multi_merge(OutputIterator begin, OutputIterator end)
    {
        assert((size_type)(end - begin) <= m_size);

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
        multi_merge_parallel(begin, end);
#else           // STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
        tree.multi_merge(begin, end);
        m_size -= end - begin;
#endif
    }

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL

protected:
    //! extract the (length = end - begin) smallest elements using parallel
    //! multiway_merge.

    template <class OutputIterator>
    void multi_merge_parallel(OutputIterator begin, OutputIterator end)
    {
        const size_t& k = tree.k;

        if (begin == end)
            return;

        using diff_type = external_diff_type;

        using sequence = std::pair<typename block_type::iterator, typename block_type::iterator>;

        std::vector<sequence> seqs;
        std::vector<size_t> orig_seq_index;

        invert_order<compare_type, value_type, value_type> inv_cmp(tree.cmp);

        for (size_t i = 0; i < k; ++i) //initialize sequences
        {
            if (states[i].current == states[i].block->size || is_sentinel(states[i].current_front()))
                continue;

            seqs.push_back(std::make_pair(states[i].block->begin() + states[i].current, states[i].block->end()));
            orig_seq_index.push_back(i);

#if STXXL_CHECK_ORDER_IN_SORTS
            if (!is_sentinel(*seqs.back().first, cmp) && !stxxl::is_sorted(seqs.back().first, seqs.back().second, inv_cmp))
            {
                LOG << "length " << i << " " << (seqs.back().second - seqs.back().first);
                for (value_type* v = seqs.back().first + 1; v < seqs.back().second; ++v)
                {
                    if (inv_cmp(*v, *(v - 1)))
                    {
                        LOG << "Error at position " << i << "/" << (v - seqs.back().first - 1) << "/" << (v - seqs.back().first) << "   " << *(v - 1) << " " << *v;
                    }
                    if (is_sentinel(*v, cmp))
                    {
                        LOG << "Wrong sentinel at position " << (v - seqs.back().first);
                    }
                }
                assert(false);
            }
#endif

            // Hint first non-internal (actually second) block of this sequence.
            if (!states[i].bids.empty())
                pool->hint(states[i].bids.front());
        }

        assert(seqs.size() > 0);

#if STXXL_CHECK_ORDER_IN_SORTS
        value_type last_elem;
#endif

        // elements still to merge for this output block
        diff_type rest = end - begin;

        while (rest > 0)
        {
            // minimum of the sequences' last elements
            value_type min_last = tree.cmp.min_value();

            diff_type total_size = 0;

            for (size_t i = 0; i < seqs.size(); ++i)
            {
                diff_type seq_i_size = seqs[i].second - seqs[i].first;
                if (seq_i_size > 0)
                {
                    total_size += seq_i_size;
                    if (inv_cmp(*(seqs[i].second - 1), min_last))
                        min_last = *(seqs[i].second - 1);

                    LOG << "front block of seq " << i << ": len=" << seq_i_size;
                }
                else {
                    LOG << "front block of seq " << i << ": empty";
                }
            }

            assert(total_size > 0);
            assert(!is_sentinel(min_last));

            LOG << " total size " << total_size << " num_seq " << seqs.size();

            diff_type less_equal_than_min_last = 0;
            //locate this element in all sequences
            for (size_t i = 0; i < seqs.size(); ++i)
            {
                //assert(seqs[i].first < seqs[i].second);

                typename block_type::iterator position =
                    std::upper_bound(seqs[i].first, seqs[i].second, min_last, inv_cmp);

                //no element larger than min_last is merged

                //req-ostream LOG << "seq " << i << ": " << (position - seqs[i].first) << " greater equal than " << min_last;

                less_equal_than_min_last += (position - seqs[i].first);
            }

            // at most rest elements
            diff_type output_size = std::min(less_equal_than_min_last, rest);

            LOG << "output_size=" << output_size << " = min(leq_t_ml=" << less_equal_than_min_last << ", rest=" << rest << ")";

            assert(output_size > 0);

            //main call

            // sequence iterators are progressed appropriately:
            begin = parallel::multiway_merge(
                seqs.begin(), seqs.end(), begin, output_size, inv_cmp);

            rest -= output_size;
            m_size -= output_size;

            for (size_t i = 0; i < seqs.size(); ++i)
            {
                sequence_state& state = states[orig_seq_index[i]];

                state.current = seqs[i].first - state.block->begin();

                assert(seqs[i].first <= seqs[i].second);

                if (seqs[i].first == seqs[i].second)
                {
                    // has run empty?

                    assert(state.current == state.block->size);
                    if (state.bids.empty())
                    {
                        // if there is no next block
                        LOG << "seq " << i << ": ext_merger::multi_merge(...) it was the last block in the sequence ";
                        state.make_inf();
                    }
                    else
                    {
#if STXXL_CHECK_ORDER_IN_SORTS
                        last_elem = *(seqs[i].second - 1);
#endif
                        LOG << "seq " << i << ": ext_merger::multi_merge(...) there is another block ";
                        bid_type bid = state.bids.front();
                        state.bids.pop_front();
                        pool->hint(bid);
                        if (!(state.bids.empty()))
                        {
                            LOG << "seq " << i << ": ext_merger::multi_merge(...) more blocks exist, hinting the next";
                            pool->hint(state.bids.front());
                        }
                        pool->read(state.block, bid)->wait();
                        LOG << "seq " << i << ": first element of read block " << bid << " cached in " << state.block;
                        if (!(state.bids.empty()))
                            pool->hint(state.bids.front());  // re-hint, reading might have made a block free
                        state.current = 0;
                        seqs[i] = std::make_pair(state.block->begin() + state.current, state.block->end());
                        foxxll::block_manager::get_instance()->delete_block(bid);

#if STXXL_CHECK_ORDER_IN_SORTS
                        LOG << "before " << last_elem << " after " << *seqs[i].first << " newly loaded block " << bid;
                        if (!stxxl::is_sorted(seqs[i].first, seqs[i].second, inv_cmp))
                        {
                            LOG << "length " << i << " " << (seqs[i].second - seqs[i].first);
                            for (value_type* v = seqs[i].first + 1; v < seqs[i].second; ++v)
                            {
                                if (inv_cmp(*v, *(v - 1)))
                                {
                                    LOG << "Error at position " << i << "/" << (v - seqs[i].first - 1) << "/" << (v - seqs[i].first) << "   " << *(v - 1) << " " << *v;
                                }
                                if (is_sentinel(*v, cmp))
                                {
                                    LOG << "Wrong sentinel at position " << (v - seqs[i].first);
                                }
                            }
                            assert(false);
                        }
#endif
                    }
                }
            }
        }       // while (rest > 1)

        for (size_t i = 0; i < seqs.size(); ++i)
        {
            size_t seg = orig_seq_index[i];
            if (is_array_empty(seg))
            {
                LOG << "deallocated " << seg;
                free_array(seg);
            }
        }

        tree.maybe_compact();
    }
#endif // STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
};     // class ext_merger

} // namespace priority_queue_local

//! \}

} // namespace stxxl

#endif // !STXXL_CONTAINERS_PQ_EXT_MERGER_HEADER
// vim: et:ts=4:sw=4
