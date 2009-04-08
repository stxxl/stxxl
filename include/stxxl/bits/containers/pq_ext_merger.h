/***************************************************************************
 *  include/stxxl/bits/containers/pq_ext_merger.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 1999 Peter Sanders <sanders@mpi-sb.mpg.de>
 *  Copyright (C) 2003, 2004, 2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PQ_EXT_MERGER_HEADER
#define STXXL_PQ_EXT_MERGER_HEADER

__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local
{
    /**
     *!  \brief  External merger, based on the loser tree data structure.
     *!  \param  Arity_  maximum arity of merger, does not need to be a power of two
     */
    template <class BlockType_,
              class Cmp_,
              unsigned Arity_,
              class AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY>
    class ext_merger : private noncopyable
    {
    public:
        typedef stxxl::uint64 size_type;
        typedef BlockType_ block_type;
        typedef typename block_type::bid_type bid_type;
        typedef typename block_type::value_type value_type;
        typedef Cmp_ comparator_type;
        typedef AllocStr_ alloc_strategy;
        typedef value_type Element;
        typedef typed_block<sizeof(value_type), value_type> sentinel_block_type;

        // KNKMAX / 2  <  arity  <=  KNKMAX
        enum { arity = Arity_, KNKMAX = 1UL << (LOG2<Arity_>::ceil) };

        block_type * convert_block_pointer(sentinel_block_type * arg)
        {
            return reinterpret_cast<block_type *>(arg);
        }

    protected:
        comparator_type cmp;

        bool is_sentinel(const Element & a) const
        {
            return !(cmp(cmp.min_value(), a)); // a <= cmp.min_value()
        }

        bool not_sentinel(const Element & a) const
        {
            return cmp(cmp.min_value(), a); // a > cmp.min_value()
        }

        struct sequence_state : private noncopyable
        {
            block_type * block;         //current block
            unsigned_type current;      //current index in current block
            std::list<bid_type> * bids; //list of blocks forming this sequence
            comparator_type cmp;
            ext_merger * merger;
            bool allocated;

            //! \returns current element
            const value_type & operator * () const
            {
                return (*block)[current];
            }

            sequence_state() : bids(NULL), allocated(false)
            { }

            ~sequence_state()
            {
                STXXL_VERBOSE2("ext_merger sequence_state::~sequence_state()");
                if (bids != NULL)
                {
                    block_manager * bm = block_manager::get_instance();
                    bm->delete_blocks(bids->begin(), bids->end());
                    delete bids;
                }
            }

            void make_inf()
            {
                current = 0;
                (*block)[0] = cmp.min_value();
            }

            bool is_sentinel(const Element & a) const
            {
                return !(cmp(cmp.min_value(), a));
            }

            bool not_sentinel(const Element & a) const
            {
                return cmp(cmp.min_value(), a);
            }

            void swap(sequence_state & obj)
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

            sequence_state & operator ++ ()
            {
                assert(not_sentinel((*block)[current]));
                assert(current < block->size);

                ++current;

                if (current == block->size)
                {
                    STXXL_VERBOSE2("ext_merger sequence_state operator++ crossing block border ");
                    // go to the next block
                    assert(bids != NULL);
                    if (bids->empty()) // if there is no next block
                    {
                        STXXL_VERBOSE2("ext_merger sequence_state operator++ it was the last block in the sequence ");
                        delete bids;
                        bids = NULL;
                        make_inf();
                    }
                    else
                    {
                        STXXL_VERBOSE2("ext_merger sequence_state operator++ there is another block ");
                        bid_type bid = bids->front();
                        bids->pop_front();
                        if (!(bids->empty()))
                        {
                            STXXL_VERBOSE2("ext_merger sequence_state operator++ one more block exists in a sequence: " <<
                                           "flushing this block in write cache (if not written yet) and giving hint to prefetcher");
                            bid_type next_bid = bids->front();
                            //Hint next block of sequence.
                            //This is mandatory to ensure proper synchronization between prefetch pool and write pool.
                            merger->p_pool->hint(next_bid, *(merger->w_pool));
                        }
                        merger->p_pool->read(block, bid)->wait();
                        STXXL_VERBOSE2("first element of read block " << bid << " " << *(block->begin()) << " cached in " << block);
                        block_manager::get_instance()->delete_block(bid);
                        current = 0;
                    }
                }
                return *this;
            }
        };


        //a pair consisting a value
        struct Entry
        {
            value_type key;      // key of loser element (winner for 0)
            unsigned_type index; // the number of the losing segment
        };

        size_type size_;         // total number of elements stored
        unsigned_type logK;      // log of current tree size
        unsigned_type k;         // invariant (k == 1 << logK), always a power of two
        // only entries 0 .. arity-1 may hold actual sequences, the other
        // entries arity .. KNKMAX-1 are sentinels to make the size of the tree
        // a power of two always

        // stack of empty segment indices
        internal_bounded_stack<unsigned_type, arity> free_segments;

        // upper levels of loser trees
        // entry[0] contains the winner info
        Entry entry[KNKMAX];

        // leaf information
        // note that Knuth uses indices k..k-1
        // while we use 0..k-1
        sequence_state states[KNKMAX]; // sequence including current position, dereference gives current element

        prefetch_pool<block_type> * p_pool;
        write_pool<block_type> * w_pool;

        sentinel_block_type sentinel_block;

    public:
        ext_merger() :
            size_(0), logK(0), k(1), p_pool(0), w_pool(0)
        {
            init();
        }

        ext_merger(prefetch_pool<block_type> * p_pool_,
                   write_pool<block_type> * w_pool_) :
            size_(0), logK(0), k(1),
            p_pool(p_pool_),
            w_pool(w_pool_)
        {
            init();
        }

        virtual ~ext_merger()
        {
            STXXL_VERBOSE1("ext_merger::~ext_merger()");
            for (unsigned_type i = 0; i < arity; ++i)
            {
                delete states[i].block;
            }
        }

        void set_pools(prefetch_pool<block_type> * p_pool_,
                       write_pool<block_type> * w_pool_)
        {
            p_pool = p_pool_;
            w_pool = w_pool_;
        }

    private:
        void init()
        {
            STXXL_VERBOSE2("ext_merger::init()");
            assert(!cmp(cmp.min_value(), cmp.min_value())); // verify strict weak ordering

            sentinel_block[0] = cmp.min_value();

            for (unsigned_type i = 0; i < KNKMAX; ++i)
            {
                states[i].merger = this;
                if (i < arity)
                    states[i].block = new block_type;
                else
                    states[i].block = convert_block_pointer(&(sentinel_block));

                states[i].make_inf();
            }

            assert(k == 1);
            free_segments.push(0); //total state: one free sequence

            rebuildLoserTree();
            assert(is_sentinel(*states[entry[0].index]));
        }

        // rebuild loser tree information from the values in current
        void rebuildLoserTree()
        {
            unsigned_type winner = initWinner(1);
            entry[0].index = winner;
            entry[0].key = *(states[winner]);
        }


        // given any values in the leaves this
        // routing recomputes upper levels of the tree
        // from scratch in linear time
        // initialize entry[root].index and the subtree rooted there
        // return winner index
        unsigned_type initWinner(unsigned_type root)
        {
            if (root >= k) { // leaf reached
                return root - k;
            } else {
                unsigned_type left = initWinner(2 * root);
                unsigned_type right = initWinner(2 * root + 1);
                Element lk = *(states[left]);
                Element rk = *(states[right]);
                if (!(cmp(lk, rk))) { // right subtree looses
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

        // first go up the tree all the way to the root
        // hand down old winner for the respective subtree
        // based on new value, and old winner and loser
        // update each node on the path to the root top down.
        // This is implemented recursively
        void update_on_insert(
            unsigned_type node,
            const Element & newKey,
            unsigned_type newIndex,
            Element * winnerKey,
            unsigned_type * winnerIndex,        // old winner
            unsigned_type * mask)               // 1 << (ceil(log KNK) - dist-from-root)
        {
            if (node == 0) {                    // winner part of root
                *mask = 1 << (logK - 1);
                *winnerKey = entry[0].key;
                *winnerIndex = entry[0].index;
                if (cmp(entry[node].key, newKey))
                {
                    entry[node].key = newKey;
                    entry[node].index = newIndex;
                }
            } else {
                update_on_insert(node >> 1, newKey, newIndex, winnerKey, winnerIndex, mask);
                Element loserKey = entry[node].key;
                unsigned_type loserIndex = entry[node].index;
                if ((*winnerIndex & *mask) != (newIndex & *mask)) { // different subtrees
                    if (cmp(loserKey, newKey)) {                    // newKey will have influence here
                        if (cmp(*winnerKey, newKey)) {              // old winner loses here
                            entry[node].key = *winnerKey;
                            entry[node].index = *winnerIndex;
                        } else {                                    // new entry looses here
                            entry[node].key = newKey;
                            entry[node].index = newIndex;
                        }
                    }
                    *winnerKey = loserKey;
                    *winnerIndex = loserIndex;
                }
                // note that nothing needs to be done if
                // the winner came from the same subtree
                // a) newKey <= winnerKey => even more reason for the other tree to lose
                // b) newKey >  winnerKey => the old winner will beat the new
                //                           entry further down the tree
                // also the same old winner is handed down the tree

                *mask >>= 1; // next level
            }
        }

        // make the tree two times as wide
        void doubleK()
        {
            STXXL_VERBOSE1("ext_merger::doubleK (before) k=" << k << " logK=" << logK << " KNKMAX=" << KNKMAX << " arity=" << arity << " #free=" << free_segments.size());
            assert(k > 0);
            assert(k < arity);
            assert(free_segments.empty());                 // stack was free (probably not needed)

            // make all new entries free
            // and push them on the free stack
            for (unsigned_type i = 2 * k - 1; i >= k; i--) //backwards
            {
                states[i].make_inf();
                if (i < arity)
                    free_segments.push(i);
            }

            // double the size
            k *= 2;
            logK++;

            STXXL_VERBOSE1("ext_merger::doubleK (after)  k=" << k << " logK=" << logK << " KNKMAX=" << KNKMAX << " arity=" << arity << " #free=" << free_segments.size());
            assert(!free_segments.empty());

            // recompute loser tree information
            rebuildLoserTree();
        }


        // compact nonempty segments in the left half of the tree
        void compactTree()
        {
            STXXL_VERBOSE1("ext_merger::compactTree (before) k=" << k << " logK=" << logK << " #free=" << free_segments.size());
            assert(logK > 0);

            // compact all nonempty segments to the left

            unsigned_type target = 0;
            for (unsigned_type from = 0; from < k; from++)
            {
                if (!is_segment_empty(from))
                {
                    assert(is_segment_allocated(from));
                    if (from != target) {
                        assert(!is_segment_allocated(target));
                        states[target].swap(states[from]);
                    }
                    ++target;
                }
            }

            // half degree as often as possible
            while (k > 1 && target <= (k / 2)) {
                k /= 2;
                logK--;
            }

            // overwrite garbage and compact the stack of free segment indices
            free_segments.clear(); // none free
            for ( ;  target < k;  target++) {
                assert(!is_segment_allocated(target));
                states[target].make_inf();
                if (target < arity)
                    free_segments.push(target);
            }

            STXXL_VERBOSE1("ext_merger::compactTree (after)  k=" << k << " logK=" << logK << " #free=" << free_segments.size());
            assert(k > 0);

            // recompute loser tree information
            rebuildLoserTree();
        }


#if 0
        void swap(ext_merger & obj)
        {
            std::swap(cmp, obj.cmp);
            std::swap(free_segments, obj.free_segments);
            std::swap(size_, obj.size_);
            std::swap(logK, obj.logK);
            std::swap(k, obj.k);
            swap_1D_arrays(entry, obj.entry, KNKMAX);
            swap_1D_arrays(states, obj.states, KNKMAX);

            // std::swap(p_pool,obj.p_pool);
            // std::swap(w_pool,obj.w_pool);
        }
#endif

    public:
        unsigned_type mem_cons() const // only rough estimation
        {
            return (arity * block_type::raw_size);
        }

        // delete the (length = end-begin) smallest elements and write them to "begin..end"
        // empty segments are deallocated
        // require:
        // - there are at least length elements
        // - segments are ended by sentinels
        template <class OutputIterator>
        void multi_merge(OutputIterator begin, OutputIterator end)
        {
            size_type length = end - begin;

            STXXL_VERBOSE1("ext_merger::multi_merge from " << k << " sequence(s), length = " << length);

            if (length == 0)
                return;

            assert(k > 0);
            assert(length <= size_);

            //Hint first non-internal (actually second) block of each sequence.
            //This is mandatory to ensure proper synchronization between prefetch pool and write pool.
            for (unsigned_type i = 0; i < k; ++i)
            {
                if (states[i].bids != NULL && !states[i].bids->empty())
                    p_pool->hint(states[i].bids->front(), *w_pool);
            }

            switch (logK) {
            case 0:
                assert(k == 1);
                assert(entry[0].index == 0);
                assert(free_segments.empty());
                //memcpy(target, states[0], length * sizeof(Element));
                //std::copy(states[0],states[0]+length,target);
                for (size_type i = 0; i < length; ++i, ++(states[0]), ++begin)
                    *begin = *(states[0]);

                entry[0].key = **states;
                if (is_segment_empty(0))
                    deallocate_segment(0);

                break;
            case 1:
                assert(k == 2);
                merge_iterator(states[0], states[1], begin, length, cmp);
                rebuildLoserTree();
                if (is_segment_empty(0) && is_segment_allocated(0))
                    deallocate_segment(0);

                if (is_segment_empty(1) && is_segment_allocated(1))
                    deallocate_segment(1);

                break;
            case 2:
                assert(k == 4);
                if (is_segment_empty(3))
                    merge3_iterator(states[0], states[1], states[2], begin, length, cmp);
                else
                    merge4_iterator(states[0], states[1], states[2], states[3], begin, length, cmp);
                rebuildLoserTree();
                if (is_segment_empty(0) && is_segment_allocated(0))
                    deallocate_segment(0);

                if (is_segment_empty(1) && is_segment_allocated(1))
                    deallocate_segment(1);

                if (is_segment_empty(2) && is_segment_allocated(2))
                    deallocate_segment(2);

                if (is_segment_empty(3) && is_segment_allocated(3))
                    deallocate_segment(3);

                break;
            case  3: multi_merge_f<OutputIterator, 3>(begin, end);
                break;
            case  4: multi_merge_f<OutputIterator, 4>(begin, end);
                break;
            case  5: multi_merge_f<OutputIterator, 5>(begin, end);
                break;
            case  6: multi_merge_f<OutputIterator, 6>(begin, end);
                break;
            case  7: multi_merge_f<OutputIterator, 7>(begin, end);
                break;
            case  8: multi_merge_f<OutputIterator, 8>(begin, end);
                break;
            case  9: multi_merge_f<OutputIterator, 9>(begin, end);
                break;
            case 10: multi_merge_f<OutputIterator, 10>(begin, end);
                break;
            default: multi_merge_k(begin, end);
                break;
            }


            size_ -= length;

            // compact tree if it got considerably smaller
            {
                const unsigned_type num_segments_used = std::min<unsigned_type>(arity, k) - free_segments.size();
                const unsigned_type num_segments_trigger = k - (3 * k / 5);
                // using k/2 would be worst case inefficient (for large k)
                // for k \in {2, 4, 8} the trigger is k/2 which is good
                // because we have special mergers for k \in {1, 2, 4}
                // there is also a special 3-way-merger, that will be
                // triggered if k == 4 && is_segment_empty(3)
                STXXL_VERBOSE3("ext_merger  compact? k=" << k << " #used=" << num_segments_used
                                                         << " <= #trigger=" << num_segments_trigger << " ==> "
                                                         << ((k > 1 && num_segments_used <= num_segments_trigger) ? "yes" : "no ")
                                                         << " || "
                                                         << ((k == 4 && !free_segments.empty() && !is_segment_empty(3)) ? "yes" : "no ")
                                                         << " #free=" << free_segments.size());
                if (k > 1 && ((num_segments_used <= num_segments_trigger) ||
                              (k == 4 && !free_segments.empty() && !is_segment_empty(3))))
                {
                    compactTree();
                }
            }
        }

    private:
        // multi-merge for arbitrary K
        template <class OutputIterator>
        void multi_merge_k(OutputIterator begin, OutputIterator end)
        {
            Entry * currentPos;
            Element currentKey;
            unsigned_type currentIndex; // leaf pointed to by current entry
            unsigned_type kReg = k;
            OutputIterator done = end;
            OutputIterator target = begin;
            unsigned_type winnerIndex = entry[0].index;
            Element winnerKey = entry[0].key;

            while (target != done)
            {
                // write result
                *target = *(states[winnerIndex]);

                // advance winner segment
                ++(states[winnerIndex]);

                winnerKey = *(states[winnerIndex]);

                // remove winner segment if empty now
                if (is_sentinel(winnerKey)) //
                    deallocate_segment(winnerIndex);


                // go up the entry-tree
                for (unsigned_type i = (winnerIndex + kReg) >> 1;  i > 0;  i >>= 1) {
                    currentPos = entry + i;
                    currentKey = currentPos->key;
                    if (cmp(winnerKey, currentKey)) {
                        currentIndex = currentPos->index;
                        currentPos->key = winnerKey;
                        currentPos->index = winnerIndex;
                        winnerKey = currentKey;
                        winnerIndex = currentIndex;
                    }
                }

                ++target;
            }
            entry[0].index = winnerIndex;
            entry[0].key = winnerKey;
        }

        template <class OutputIterator, unsigned LogK>
        void multi_merge_f(OutputIterator begin, OutputIterator end)
        {
            OutputIterator done = end;
            OutputIterator target = begin;
            unsigned_type winnerIndex = entry[0].index;
            Entry * regEntry = entry;
            sequence_state * regStates = states;
            Element winnerKey = entry[0].key;

            assert(logK >= LogK);
            while (target != done)
            {
                // write result
                *target = *(regStates[winnerIndex]);

                // advance winner segment
                ++(regStates[winnerIndex]);

                winnerKey = *(regStates[winnerIndex]);


                // remove winner segment if empty now
                if (is_sentinel(winnerKey))
                    deallocate_segment(winnerIndex);


                ++target;

                // update loser tree
#define TreeStep(L) \
    if (1 << LogK >= 1 << L) { \
        Entry * pos ## L = regEntry + ((winnerIndex + (1 << LogK)) >> (((int(LogK - L) + 1) >= 0) ? ((LogK - L) + 1) : 0)); \
        Element key ## L = pos ## L->key; \
        if (cmp(winnerKey, key ## L)) { \
            unsigned_type index ## L = pos ## L->index; \
            pos ## L->key = winnerKey; \
            pos ## L->index = winnerIndex; \
            winnerKey = key ## L; \
            winnerIndex = index ## L; \
        } \
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
            regEntry[0].index = winnerIndex;
            regEntry[0].key = winnerKey;
        }

    public:
        bool is_space_available() const // for new segment
        {
            return k < arity || !free_segments.empty();
        }


        // insert segment beginning at target
        // require: is_space_available() == 1
        template <class Merger>
        void insert_segment(Merger & another_merger, size_type segment_size)
        {
            STXXL_VERBOSE1("ext_merger::insert_segment(merger,...)" << this);

            if (segment_size > 0)
            {
                // get a free slot
                if (free_segments.empty()) { // tree is too small
                    doubleK();
                }
                assert(!free_segments.empty());
                unsigned_type free_slot = free_segments.top();
                free_segments.pop();


                // link new segment
                assert(segment_size);
                unsigned_type nblocks = segment_size / block_type::size;
                //assert(nblocks); // at least one block
                STXXL_VERBOSE1("ext_merger::insert_segment nblocks=" << nblocks);
                if (nblocks == 0)
                {
                    STXXL_VERBOSE1("ext_merger::insert_segment(merger,...) WARNING: inserting a segment with " <<
                                   nblocks << " blocks");
                    STXXL_VERBOSE1("THIS IS INEFFICIENT: TRY TO CHANGE PRIORITY QUEUE PARAMETERS");
                }
                unsigned_type first_size = segment_size % block_type::size;
                if (first_size == 0)
                {
                    first_size = block_type::size;
                    --nblocks;
                }
                block_manager * bm = block_manager::get_instance();
                std::list<bid_type> * bids = new std::list<bid_type>(nblocks);
                bm->new_blocks(alloc_strategy(), bids->begin(), bids->end());
                block_type * first_block = new block_type;

                another_merger.multi_merge(
                    first_block->begin() + (block_type::size - first_size),
                    first_block->end());

                STXXL_VERBOSE1("last element of first block " << *(first_block->end() - 1));
                assert(!cmp(*(first_block->begin() + (block_type::size - first_size)), *(first_block->end() - 1)));

                assert(w_pool->size() > 0);

                for (typename std::list<bid_type>::iterator curbid = bids->begin(); curbid != bids->end(); ++curbid)
                {
                    block_type * b = w_pool->steal();
                    another_merger.multi_merge(b->begin(), b->end());
                    STXXL_VERBOSE1("first element of following block " << *curbid << " " << *(b->begin()));
                    STXXL_VERBOSE1("last element of following block " << *curbid << " " << *(b->end() - 1));
                    assert(!cmp(*(b->begin()), *(b->end() - 1)));
                    w_pool->write(b, *curbid); //->wait() does not help
                    STXXL_VERBOSE1("written to block " << *curbid << " cached in " << b);
                }

                insert_segment(bids, first_block, first_size, free_slot);

                size_ += segment_size;

                // propagate new information up the tree
                Element dummyKey;
                unsigned_type dummyIndex;
                unsigned_type dummyMask;
                update_on_insert((free_slot + k) >> 1, *(states[free_slot]), free_slot,
                                 &dummyKey, &dummyIndex, &dummyMask);
            } else {
                // deallocate memory ?
                STXXL_VERBOSE1("Merged segment with zero size.");
            }
        }

        size_type size() const { return size_; }

    protected:
        /*! \param first_size number of elements in the first block
         */
        void insert_segment(std::list<bid_type> * bidlist, block_type * first_block,
                            unsigned_type first_size, unsigned_type slot)
        {
            STXXL_VERBOSE1("ext_merger::insert_segment(bidlist,...) " << this << " " << bidlist->size() << " " << slot);
            assert(!is_segment_allocated(slot));
            assert(first_size > 0);

            sequence_state & new_sequence = states[slot];
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
            assert(is_segment_allocated(slot));
        }

        // free an empty segment .
        void deallocate_segment(unsigned_type slot)
        {
            STXXL_VERBOSE1("ext_merger::deallocate_segment() deleting segment " << slot << " allocated=" << int(is_segment_allocated(slot)));
            assert(is_segment_allocated(slot));
            states[slot].allocated = false;
            states[slot].make_inf();

            // push on the stack of free segment indices
            free_segments.push(slot);
        }

        // is this segment empty ?
        bool is_segment_empty(unsigned_type slot) const
        {
            return is_sentinel(*(states[slot]));
        }

        // Is this segment allocated? Otherwise it's empty,
        // already on the stack of free segment indices and can be reused.
        bool is_segment_allocated(unsigned_type slot) const
        {
            return states[slot].allocated;
        }
    }; //ext_merger
} //priority_queue_local

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_PQ_EXT_MERGER_HEADER
// vim: et:ts=4:sw=4
