/***************************************************************************
 *  include/stxxl/bits/containers/pq_ext_merger.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
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

#include <stxxl/bits/containers/pq_helpers.h>

STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local {

template <typename Iterator>
class short_sequence : public std::pair<Iterator, Iterator>
{
    typedef std::pair<Iterator, Iterator> pair;

public:
    typedef Iterator iterator;
    typedef const iterator const_iterator;
    typedef typename std::iterator_traits<iterator>::value_type value_type;
    typedef typename std::iterator_traits<iterator>::difference_type size_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef unsigned_type origin_type;

private:
    origin_type m_origin;

public:
    short_sequence(Iterator first, Iterator last, origin_type origin)
        : pair(first, last), m_origin(origin)
    { }

    iterator begin()
    {
        return this->first;
    }

    const_iterator begin() const
    {
        return this->first;
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    iterator end()
    {
        return this->second;
    }

    const_iterator end() const
    {
        return this->second;
    }

    const_iterator cend() const
    {
        return end();
    }

    reference front()
    {
        return *begin();
    }

    const_reference front() const
    {
        return *begin();
    }

    reference back()
    {
        return *(end() - 1);
    }

    const_reference back() const
    {
        return *(end() - 1);
    }

    size_type size() const
    {
        return end() - begin();
    }

    bool empty() const
    {
        return size() == 0;
    }

    origin_type origin() const
    {
        return m_origin;
    }
};

/*!
 * External merger, based on the loser tree data structure.
 * \param Arity_  maximum arity of merger, does not need to be a power of 2
 */
template <class BlockType, class CompareType, unsigned Arity,
          class AllocStr = STXXL_DEFAULT_ALLOC_STRATEGY>
class ext_arrays : private noncopyable
{
public:
    typedef stxxl::uint64 size_type;
    typedef BlockType block_type;
    typedef typename block_type::bid_type bid_type;
    typedef typename block_type::value_type value_type;
    typedef CompareType compare_type;
    typedef AllocStr alloc_strategy;
    typedef read_write_pool<block_type> pool_type;

    // max_arity / 2  <  arity  <=  max_arity
    enum { arity = Arity, max_arity = 1UL << (LOG2<Arity>::ceil) };

protected:
    //! the comparator object
    compare_type cmp;

    bool is_sentinel(const value_type& a) const
    {
        return !(cmp(cmp.min_value(), a)); // a <= cmp.min_value()
    }

    bool not_sentinel(const value_type& a) const
    {
        return cmp(cmp.min_value(), a); // a > cmp.min_value()
    }

    struct sequence_state : private noncopyable
    {
        block_type* block;          //current block
        unsigned_type current;      //current index in current block
        std::list<bid_type>* bids;  //list of blocks forming this sequence
        compare_type cmp;
        ext_arrays* merger;
        bool allocated;

        //! \returns current element
        const value_type& operator * () const
        {
            return (*block)[current];
        }

        sequence_state()
            : block(NULL), current(0),
              bids(NULL), merger(NULL),
              allocated(false)
        { }

        ~sequence_state()
        {
            STXXL_VERBOSE2("ext_arrays sequence_state::~sequence_state()");
            if (bids != NULL)
            {
                block_manager* bm = block_manager::get_instance();
                bm->delete_blocks(bids->begin(), bids->end());
                delete bids;
            }
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

        bool not_sentinel(const value_type& a) const
        {
            return cmp(cmp.min_value(), a);
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

        sequence_state& operator ++ ()
        {
            assert(not_sentinel((*block)[current]));
            assert(current < block->size);

            ++current;

            if (current == block->size)
            {
                STXXL_VERBOSE2("ext_arrays sequence_state operator++ crossing block border ");
                // go to the next block
                assert(bids != NULL);
                if (bids->empty()) // if there is no next block
                {
                    STXXL_VERBOSE2("ext_arrays sequence_state operator++ it was the last block in the sequence ");
                    delete bids;
                    bids = NULL;
                    make_inf();
                }
                else
                {
                    STXXL_VERBOSE2("ext_arrays sequence_state operator++ there is another block ");
                    bid_type bid = bids->front();
                    bids->pop_front();
                    merger->pool->hint(bid);
                    if (!(bids->empty()))
                    {
                        STXXL_VERBOSE2("ext_arrays sequence_state operator++ more blocks exist in a sequence, hinting the next");
                        merger->pool->hint(bids->front());
                    }
                    merger->pool->read(block, bid)->wait();
                    STXXL_VERBOSE2("first element of read block " << bid << " " << *(block->begin()) << " cached in " << block);
                    if (!(bids->empty()))
                        merger->pool->hint(bids->front());  // re-hint, reading might have made a block free
                    block_manager::get_instance()->delete_block(bid);
                    current = 0;
                }
            }
            return *this;
        }
    };

    //! current tree size, invariant (k == 1 << logK), always a power of two
    unsigned_type k;
    //! log of current tree size
    unsigned_type logK;

    //! total number of elements stored
    size_type m_size;

    // only entries 0 .. arity-1 may hold actual sequences, the other
    // entries arity .. max_arity-1 are sentinels to make the size of the tree
    // a power of 2 always

    // leaf information.  note that Knuth uses indices k..k-1, while we use
    // 0..k-1

    //! sequence including current position, dereference gives current element
    sequence_state states[max_arity];

    pool_type* pool;

    block_type* sentinel_block;

public:
    ext_arrays() // TODO: pass pool as parameter
        : k(1), logK(0), m_size(0), pool(NULL)
    {
        init();
    }

    virtual ~ext_arrays()
    {
        STXXL_VERBOSE1("ext_arrays::~ext_arrays()");
        for (unsigned_type i = 0; i < arity; ++i)
        {
            delete states[i].block;
        }
        delete sentinel_block;
    }

    void set_pool(pool_type* pool_)
    {
        pool = pool_;
    }

    typedef sequence_state SequenceType;

    SequenceType* get_sequences()
    {
        return states;
    }

    void swap_arrays(unsigned_type a, unsigned_type b)
    {
        states[a].swap(states[b]);
    }

    void make_array_sentinel(unsigned_type a)
    {
        states[a].make_inf();
    }

protected:
    void init()
    {
        STXXL_VERBOSE2("ext_arrays::init()");
        assert(!cmp(cmp.min_value(), cmp.min_value())); // verify strict weak ordering

        sentinel_block = NULL;
        if (arity < max_arity)
        {
            sentinel_block = new block_type;
            for (unsigned_type i = 0; i < block_type::size; ++i)
                (*sentinel_block)[i] = cmp.min_value();
            if (arity + 1 == max_arity) {
                // same memory consumption, but smaller merge width, better use arity = max_arity
                STXXL_ERRMSG("inefficient PQ parameters for ext_arrays: arity + 1 == max_arity");
            }
        }

        for (unsigned_type i = 0; i < max_arity; ++i)
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
    void swap(ext_arrays& obj)
    {
        std::swap(cmp, obj.cmp);
        std::swap(k, obj.k);
        std::swap(logK, obj.logK);
        std::swap(m_size, obj.m_size);
        swap_1D_arrays(states, obj.states, max_arity);

        // std::swap(pool,obj.pool);
    }
#endif

public:
    unsigned_type mem_cons() const // only rough estimation
    {
        return (STXXL_MIN<unsigned_type>(arity + 1, max_arity) * block_type::raw_size);
    }

public:

    size_type size() const { return m_size; }

protected:
    /*!
      \param bidlist list of blocks to insert
      \param first_block the first block of the sequence, before bidlist
      \param first_size number of elements in the first block
      \param slot slot to insert into
    */
    void insert_segment(std::list<bid_type>* bidlist, block_type* first_block,
                        unsigned_type first_size, unsigned_type slot)
    {
        STXXL_VERBOSE1("ext_arrays::insert_segment(bidlist,...) " << this << " " << bidlist->size() << " " << slot);
        assert(!is_array_allocated(slot));
        assert(first_size > 0);

        sequence_state& new_sequence = states[slot];
        new_sequence.current = block_type::size - first_size;
        std::swap(new_sequence.block, first_block);
        delete first_block;
        std::swap(new_sequence.bids, bidlist);
        if (bidlist) // the old list
        {
            assert(bidlist->empty());
            delete bidlist;
        }
        new_sequence.allocated = true;
        assert(is_array_allocated(slot));
    }

    // free an empty segment .
    void free_array(unsigned_type slot)
    {
        STXXL_VERBOSE1("ext_arrays::free_array() deleting array " << slot << " allocated=" << int(is_array_allocated(slot)));
        assert(is_array_allocated(slot));
        states[slot].allocated = false;
        states[slot].make_inf();
    }

    //! is this segment empty ?
    bool is_array_empty(unsigned_type slot) const
    {
        return is_sentinel(*(states[slot]));
    }

    //! Is this segment allocated? Otherwise it's empty, already on the stack
    //! of free segment indices and can be reused.
    bool is_array_allocated(unsigned_type slot) const
    {
        return states[slot].allocated;
    }

};  // class ext_arrays

template <class BlockType, class CompareType, unsigned MaxArity,
          class AllocStr = STXXL_DEFAULT_ALLOC_STRATEGY>
class ext_merger : public loser_tree<
    ext_arrays<BlockType, CompareType, MaxArity, AllocStr>,
    CompareType, MaxArity
    >
{
public:

    typedef BlockType block_type;
    typedef AllocStr alloc_strategy;

    typedef ext_arrays<BlockType, CompareType, MaxArity, AllocStr> arrays_type;
    typedef loser_tree<arrays_type, CompareType, MaxArity> super_type;

    typedef loser_tree<arrays_type, CompareType, MaxArity> tree_type;

    typedef stxxl::uint64 size_type;

    typedef typename block_type::bid_type bid_type;

    typedef typename super_type::value_type value_type;
    typedef typename super_type::sequence_state sequence_state;
    typedef typename super_type::pool_type pool_type;

    bool is_array_empty(unsigned_type slot) const
    {
        return super_type::is_array_empty(slot);
    }

    bool is_array_allocated(unsigned_type slot) const
    {
        return super_type::is_array_allocated(slot);
    }

    //! Merge all items from another merger and insert the resulting external
    //! array into the merger. Requires: is_space_available() == 1
    template <class Merger>
    void append_merger(Merger& another_merger, size_type segment_size)
    {
        STXXL_VERBOSE1("ext_merger::append_merger(merger,...)" << this);

        if (segment_size == 0)
        {
            // deallocate memory ?
            STXXL_VERBOSE1("Merged segment with zero size.");
            return;
        }

        // allocate a new player slot
        int index = tree_type::new_player();

        // construct new sorted array from merger
        assert(segment_size);
        unsigned_type nblocks = (unsigned_type)(segment_size / block_type::size);
        //assert(nblocks); // at least one block
        STXXL_VERBOSE1("ext_arrays::insert_segment nblocks=" << nblocks);
        if (nblocks == 0)
        {
            STXXL_VERBOSE1("ext_arrays::insert_segment(merger,...) WARNING: inserting a segment with " <<
                           nblocks << " blocks");
            STXXL_VERBOSE1("THIS IS INEFFICIENT: TRY TO CHANGE PRIORITY QUEUE PARAMETERS");
        }
        unsigned_type first_size = (unsigned_type)(segment_size % block_type::size);
        if (first_size == 0)
        {
            first_size = block_type::size;
            --nblocks;
        }

        // allocate blocks
        block_manager* bm = block_manager::get_instance();
        std::list<bid_type>* bids = new std::list<bid_type>(nblocks);
        bm->new_blocks(alloc_strategy(), bids->begin(), bids->end());
        block_type* first_block = new block_type;

        another_merger.multi_merge(
            first_block->begin() + (block_type::size - first_size),
            first_block->end());

        STXXL_VERBOSE1("last element of first block " << *(first_block->end() - 1));
        assert(!cmp(*(first_block->begin() + (block_type::size - first_size)), *(first_block->end() - 1)));

        pool_type* pool = this->pool;
        assert(pool->size_write() > 0);

        for (typename std::list<bid_type>::iterator curbid = bids->begin(); curbid != bids->end(); ++curbid)
        {
            block_type* b = pool->steal();
            another_merger.multi_merge(b->begin(), b->end());
            STXXL_VERBOSE1("first element of following block " << *curbid << " " << *(b->begin()));
            STXXL_VERBOSE1("last element of following block " << *curbid << " " << *(b->end() - 1));
            assert(!cmp(*(b->begin()), *(b->end() - 1)));
            pool->write(b, *curbid);
            STXXL_VERBOSE1("written to block " << *curbid << " cached in " << b);
        }

        insert_segment(bids, first_block, first_size, index);

        this->m_size += segment_size;

#if STXXL_PQ_EXTERNAL_LOSER_TREE
        tree_type::update_on_insert((index + this->k) >> 1, *(this->states[index]), index);
#endif // STXXL_PQ_EXTERNAL_LOSER_TREE
    }

    void free_array(unsigned_type slot)
    {
        // free array in array list
        super_type::free_array(slot);

        // free player in loser tree
        tree_type::free_player(slot);
    }

    // delete the (length = end-begin) smallest elements and write them to [begin..end)
    // empty segments are deallocated
    // requires:
    // - there are at least length elements
    // - segments are ended by sentinels
    template <class OutputIterator>
    void multi_merge(OutputIterator begin, OutputIterator end)
    {
        unsigned_type& k = this->k;
        unsigned_type& logK = this->logK;
        unsigned_type& m_size = this->m_size;
        CompareType& cmp = this->cmp;
        typename super_type::Entry* entry = this->entry;
        typename super_type::sequence_state* states = this->states;
        typename super_type::pool_type* pool = this->pool;
        internal_bounded_stack<unsigned_type, MaxArity>& free_slots = this->free_slots;

        int_type length = end - begin;

        STXXL_VERBOSE1("ext_arrays::multi_merge from " << k << " sequence(s),"
                       " length = " << length);

        if (length == 0)
            return;

        assert(k > 0);
        assert(length <= (int_type)m_size);

        //This is the place to make statistics about external multi_merge calls.

#if STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL
        typedef stxxl::int64 diff_type;

        typedef std::pair<typename block_type::iterator, typename block_type::iterator> sequence;

        std::vector<sequence> seqs;
        std::vector<unsigned_type> orig_seq_index;

        Cmp cmp;
        invert_order<Cmp, value_type, value_type> inv_cmp(cmp);

        for (unsigned_type i = 0; i < k; ++i) //initialize sequences
        {
            if (states[i].current == states[i].block->size || is_sentinel(*states[i]))
                continue;

            seqs.push_back(std::make_pair(states[i].block->begin() + states[i].current, states[i].block->end()));
            orig_seq_index.push_back(i);

#if STXXL_CHECK_ORDER_IN_SORTS
            if (!is_sentinel(*seqs.back().first) && !stxxl::is_sorted(seqs.back().first, seqs.back().second, inv_cmp))
            {
                STXXL_VERBOSE1("length " << i << " " << (seqs.back().second - seqs.back().first));
                for (value_type* v = seqs.back().first + 1; v < seqs.back().second; ++v)
                {
                    if (inv_cmp(*v, *(v - 1)))
                    {
                        STXXL_VERBOSE1("Error at position " << i << "/" << (v - seqs.back().first - 1) << "/" << (v - seqs.back().first) << "   " << *(v - 1) << " " << *v);
                    }
                    if (is_sentinel(*v))
                    {
                        STXXL_VERBOSE1("Wrong sentinel at position " << (v - seqs.back().first));
                    }
                }
                assert(false);
            }
#endif

            //Hint first non-internal (actually second) block of this sequence.
            if (states[i].bids != NULL && !states[i].bids->empty())
                pool->hint(states[i].bids->front());
        }

        assert(seqs.size() > 0);

#if STXXL_CHECK_ORDER_IN_SORTS
        value_type last_elem;
#endif

        diff_type rest = length;                     //elements still to merge for this output block

        while (rest > 0)
        {
            value_type min_last = cmp.min_value();   // minimum of the sequences' last elements
            diff_type total_size = 0;

            for (unsigned_type i = 0; i < seqs.size(); ++i)
            {
                diff_type seq_i_size = seqs[i].second - seqs[i].first;
                if (seq_i_size > 0)
                {
                    total_size += seq_i_size;
                    if (inv_cmp(*(seqs[i].second - 1), min_last))
                        min_last = *(seqs[i].second - 1);

                    STXXL_VERBOSE1("front block of seq " << i << ": front=" << *(seqs[i].first) << " back=" << *(seqs[i].second - 1) << " len=" << seq_i_size);
                }
                else {
                    STXXL_VERBOSE1("front block of seq " << i << ": empty");
                }
            }

            assert(total_size > 0);
            assert(!is_sentinel(min_last));

            STXXL_VERBOSE1("min_last " << min_last << " total size " << total_size << " num_seq " << seqs.size());

            diff_type less_equal_than_min_last = 0;
            //locate this element in all sequences
            for (unsigned_type i = 0; i < seqs.size(); ++i)
            {
                //assert(seqs[i].first < seqs[i].second);

                typename block_type::iterator position =
                    std::upper_bound(seqs[i].first, seqs[i].second, min_last, inv_cmp);

                //no element larger than min_last is merged

                STXXL_VERBOSE1("seq " << i << ": " << (position - seqs[i].first) << " greater equal than " << min_last);

                less_equal_than_min_last += (position - seqs[i].first);
            }

            diff_type output_size = STXXL_MIN(less_equal_than_min_last, rest);  // at most rest elements

            STXXL_VERBOSE1("output_size=" << output_size << " = min(leq_t_ml=" << less_equal_than_min_last << ", rest=" << rest << ")");

            assert(output_size > 0);

            //main call

            begin = parallel::multiway_merge(seqs.begin(), seqs.end(), begin, inv_cmp, output_size); //sequence iterators are progressed appropriately

            rest -= output_size;
            m_size -= output_size;

            for (unsigned_type i = 0; i < seqs.size(); ++i)
            {
                sequence_state& state = states[orig_seq_index[i]];

                state.current = seqs[i].first - state.block->begin();

                assert(seqs[i].first <= seqs[i].second);

                if (seqs[i].first == seqs[i].second)               //has run empty
                {
                    assert(state.current == state.block->size);
                    if (state.bids == NULL || state.bids->empty()) // if there is no next block
                    {
                        STXXL_VERBOSE1("seq " << i << ": ext_arrays::multi_merge(...) it was the last block in the sequence ");
                        state.make_inf();
                    }
                    else
                    {
#if STXXL_CHECK_ORDER_IN_SORTS
                        last_elem = *(seqs[i].second - 1);
#endif
                        STXXL_VERBOSE1("seq " << i << ": ext_arrays::multi_merge(...) there is another block ");
                        bid_type bid = state.bids->front();
                        state.bids->pop_front();
                        pool->hint(bid);
                        if (!(state.bids->empty()))
                        {
                            STXXL_VERBOSE2("seq " << i << ": ext_arrays::multi_merge(...) more blocks exist, hinting the next");
                            pool->hint(state.bids->front());
                        }
                        pool->read(state.block, bid)->wait();
                        STXXL_VERBOSE1("seq " << i << ": first element of read block " << bid << " " << *(state.block->begin()) << " cached in " << state.block);
                        if (!(state.bids->empty()))
                            pool->hint(state.bids->front());  // re-hint, reading might have made a block free
                        state.current = 0;
                        seqs[i] = std::make_pair(state.block->begin() + state.current, state.block->end());
                        block_manager::get_instance()->delete_block(bid);

#if STXXL_CHECK_ORDER_IN_SORTS
                        STXXL_VERBOSE1("before " << last_elem << " after " << *seqs[i].first << " newly loaded block " << bid);
                        if (!stxxl::is_sorted(seqs[i].first, seqs[i].second, inv_cmp))
                        {
                            STXXL_VERBOSE1("length " << i << " " << (seqs[i].second - seqs[i].first));
                            for (value_type* v = seqs[i].first + 1; v < seqs[i].second; ++v)
                            {
                                if (inv_cmp(*v, *(v - 1)))
                                {
                                    STXXL_VERBOSE1("Error at position " << i << "/" << (v - seqs[i].first - 1) << "/" << (v - seqs[i].first) << "   " << *(v - 1) << " " << *v);
                                }
                                if (is_sentinel(*v))
                                {
                                    STXXL_VERBOSE1("Wrong sentinel at position " << (v - seqs[i].first));
                                }
                            }
                            assert(false);
                        }
#endif
                    }
                }
            }
        }   //while (rest > 1)

        for (unsigned_type i = 0; i < seqs.size(); ++i)
        {
            unsigned_type seg = orig_seq_index[i];
            if (is_array_empty(seg))
            {
                STXXL_VERBOSE1("deallocated " << seg);
                free_array(seg);
            }
        }

#else       // STXXL_PARALLEL && STXXL_PARALLEL_PQ_MULTIWAY_MERGE_EXTERNAL

        //Hint first non-internal (actually second) block of each sequence.
        for (unsigned_type i = 0; i < k; ++i)
        {
            if (states[i].bids != NULL && !states[i].bids->empty())
                pool->hint(states[i].bids->front());
        }

        switch (logK) {
        case 0:
            assert(k == 1);
            assert(entry[0].index == 0);
            assert(free_slots.empty());
            //memcpy(target, states[0], length * sizeof(value_type));
            //std::copy(states[0],states[0]+length,target);
            for (int_type i = 0; i < length; ++i, ++(states[0]), ++begin)
                *begin = *(states[0]);
            entry[0].key = **states;

            if (is_array_empty(0))
                free_array(0);

            break;
        case 1:
            assert(k == 2);
            merge_iterator(states[0], states[1], begin, length, cmp);
            this->rebuild_loser_tree();

            if (is_array_empty(0) && is_array_allocated(0))
                free_array(0);

            if (is_array_empty(1) && is_array_allocated(1))
                free_array(1);

            break;
        case 2:
            assert(k == 4);
            if (is_array_empty(3))
                merge3_iterator(states[0], states[1], states[2], begin, length, cmp);
            else
                merge4_iterator(states[0], states[1], states[2], states[3], begin, length, cmp);
            this->rebuild_loser_tree();

            if (is_array_empty(0) && is_array_allocated(0))
                free_array(0);

            if (is_array_empty(1) && is_array_allocated(1))
                free_array(1);

            if (is_array_empty(2) && is_array_allocated(2))
                free_array(2);

            if (is_array_empty(3) && is_array_allocated(3))
                free_array(3);

            break;
        case  3: this->template multi_merge_f<3>(begin, end);
            break;
        case  4: this->template multi_merge_f<4>(begin, end);
            break;
        case  5: this->template multi_merge_f<5>(begin, end);
            break;
        case  6: this->template multi_merge_f<6>(begin, end);
            break;
        case  7: this->template multi_merge_f<7>(begin, end);
            break;
        case  8: this->template multi_merge_f<8>(begin, end);
            break;
        case  9: this->template multi_merge_f<9>(begin, end);
            break;
        case 10: this->template multi_merge_f<10>(begin, end);
            break;
        default: multi_merge_k(begin, end);
            break;
        }

        m_size -= length;
#endif

        // compact tree if it got considerably smaller
        {
            const unsigned_type num_segments_used = std::min<unsigned_type>(MaxArity, k) - free_slots.size();
            const unsigned_type num_segments_trigger = k - (3 * k / 5);
            // using k/2 would be worst case inefficient (for large k)
            // for k \in {2, 4, 8} the trigger is k/2 which is good
            // because we have special mergers for k \in {1, 2, 4}
            // there is also a special 3-way-merger, that will be
            // triggered if k == 4 && is_array_empty(3)
            STXXL_VERBOSE3("ext_arrays  compact? k=" << k << " #used=" << num_segments_used
                                                     << " <= #trigger=" << num_segments_trigger << " ==> "
                                                     << ((k > 1 && num_segments_used <= num_segments_trigger) ? "yes" : "no ")
                                                     << " || "
                                                     << ((k == 4 && !free_slots.empty() && !is_array_empty(3)) ? "yes" : "no ")
                                                     << " #free=" << free_slots.size());
            if (k > 1 && ((num_segments_used <= num_segments_trigger) ||
                          (k == 4 && !free_slots.empty() && !is_array_empty(3))))
            {
                this->compact_tree();
            }
        }
    }
};

} // namespace priority_queue_local

//! \}

STXXL_END_NAMESPACE

#endif // !STXXL_CONTAINERS_PQ_EXT_MERGER_HEADER
// vim: et:ts=4:sw=4
