/***************************************************************************
 *  include/stxxl/bits/containers/priority_queue.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 1999 Peter Sanders <sanders@mpi-sb.mpg.de>
 *  Copyright (C) 2003, 2004, 2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PRIORITY_QUEUE_HEADER
#define STXXL_PRIORITY_QUEUE_HEADER

#include "stxxl/bits/mng/mng.h"
#include "stxxl/bits/mng/prefetch_pool.h"
#include "stxxl/bits/mng/write_pool.h"
#include "stxxl/bits/common/tmeta.h"

#include <list>
#include <iterator>
#include <vector>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local
{
    /**
     * @brief Similar to std::priority_queue, with the following differences:
     * - Maximum size is fixed at construction time, so an array can be used.
     * - Provides access to underlying heap, so (parallel) sorting in place is possible.
     * - Can be cleared "at once", without reallocation.
     */
    template <typename _Tp, typename _Sequence = std::vector<_Tp>,
              typename _Compare = std::less<typename _Sequence::value_type> >
    class internal_priority_queue
    {
        // concept requirements
        typedef typename _Sequence::value_type _Sequence_value_type;

    public:
        typedef typename _Sequence::value_type value_type;
        typedef typename _Sequence::reference reference;
        typedef typename _Sequence::const_reference const_reference;
        typedef typename _Sequence::size_type size_type;
        typedef          _Sequence container_type;

    protected:
        //  See queue::c for notes on these names.
        _Sequence c;
        _Compare comp;
        size_type N;

    public:
        /**
         *  @brief  Default constructor creates no elements.
         */
        explicit
        internal_priority_queue(size_type capacity)
            : c(capacity), N(0)
        { }

        /**
         *  Returns true if the %queue is empty.
         */
        bool
        empty() const
        { return N == 0; }

        /**  Returns the number of elements in the %queue.  */
        size_type
        size() const
        { return N; }

        /**
         *  Returns a read-only (constant) reference to the data at the first
         *  element of the %queue.
         */
        const_reference
        top() const
        {
            return c.front();
        }

        /**
         *  @brief  Add data to the %queue.
         *  @param  __x  Data to be added.
         *
         *  This is a typical %queue operation.
         *  The time complexity of the operation depends on the underlying
         *  sequence.
         */
        void
        push(const value_type & __x)
        {
            c[N] = __x;
            ++N;
            std::push_heap(c.begin(), c.begin() + N, comp);
        }

        /**
         *  @brief  Removes first element.
         *
         *  This is a typical %queue operation.  It shrinks the %queue
         *  by one.  The time complexity of the operation depends on the
         *  underlying sequence.
         *
         *  Note that no data is returned, and if the first element's
         *  data is needed, it should be retrieved before pop() is
         *  called.
         */
        void
        pop()
        {
            std::pop_heap(c.begin(), c.begin() + N, comp);
            --N;
        }

        /**
         * @brief Sort all contained elements, write result to @c target.
         */
        void sort_to(value_type * target)
        {
            std::sort(c.begin(), c.begin() + N, comp);
            std::reverse_copy(c.begin(), c.begin() + N, target);
        }

        /**
         * @brief Remove all contained elements.
         */
        void clear()
        {
            N = 0;
        }
    };


/////////////////////////////////////////////////////////////////////
// auxiliary functions

// merge sz element from the two sentinel terminated input
// sequences from0 and from1 to "to"
// advance from0 and from1 accordingly
// require: at least sz nonsentinel elements available in from0, from1
// require: to may overwrite one of the sources as long as
//   *(fromx + sz) is before the end of fromx
    template <class InputIterator, class OutputIterator, class Cmp_>
    void merge_iterator(
        InputIterator & from0,
        InputIterator & from1,
        OutputIterator to, unsigned_type sz, Cmp_ cmp)
    {
        OutputIterator done = to + sz;

        while (to != done)
        {
            if (cmp(*from0, *from1))
            {
                *to = *from1;
                ++from1;
            }
            else
            {
                *to = *from0;
                ++from0;
            }
            ++to;
        }
    }

// merge sz element from the three sentinel terminated input
// sequences from0, from1 and from2 to "to"
// advance from0, from1 and from2 accordingly
// require: at least sz nonsentinel elements available in from0, from1 and from2
// require: to may overwrite one of the sources as long as
//   *(fromx + sz) is before the end of fromx
    template <class InputIterator, class OutputIterator, class Cmp_>
    void merge3_iterator(
        InputIterator & from0,
        InputIterator & from1,
        InputIterator & from2,
        OutputIterator to, unsigned_type sz, Cmp_ cmp)
    {
        OutputIterator done = to + sz;

        if (cmp(*from1, *from0)) {
            if (cmp(*from2, *from1)) {
                goto s012;
            }
            else {
                if (cmp(*from0, *from2)) {
                    goto s201;
                }
                else {
                    goto s021;
                }
            }
        } else {
            if (cmp(*from2, *from1)) {
                if (cmp(*from2, *from0)) {
                    goto s102;
                }
                else {
                    goto s120;
                }
            } else {
                goto s210;
            }
        }

#define Merge3Case(a, b, c) \
    s ## a ## b ## c : \
    if (to == done) \
        return;\
    *to = *from ## a; \
    ++to; \
    ++from ## a; \
    if (cmp(*from ## b, *from ## a)) \
        goto s ## a ## b ## c;\
    if (cmp(*from ## c, *from ## a)) \
        goto s ## b ## a ## c;\
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


// merge sz element from the four sentinel terminated input
// sequences from0, from1, from2 and from3 to "to"
// advance from0, from1, from2 and from3 accordingly
// require: at least sz nonsentinel elements available in from0, from1, from2 and from3
// require: to may overwrite one of the sources as long as
//   *(fromx + sz) is before the end of fromx
    template <class InputIterator, class OutputIterator, class Cmp_>
    void merge4_iterator(
        InputIterator & from0,
        InputIterator & from1,
        InputIterator & from2,
        InputIterator & from3,
        OutputIterator to, unsigned_type sz, Cmp_ cmp)
    {
        OutputIterator done = to + sz;

#define StartMerge4(a, b, c, d) \
    if ((!cmp(*from ## a, *from ## b)) && (!cmp(*from ## b, *from ## c)) && (!cmp(*from ## c, *from ## d))) \
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

#define Merge4Case(a, b, c, d) \
    s ## a ## b ## c ## d : \
    if (to == done) \
        return;\
    *to = *from ## a; \
    ++to; \
    ++from ## a; \
    if (cmp(*from ## c, *from ## a)) \
    { \
        if (cmp(*from ## b, *from ## a)) \
            goto s ## a ## b ## c ## d;\
        else \
            goto s ## b ## a ## c ## d;\
    } \
    else \
    { \
        if (cmp(*from ## d, *from ## a)) \
            goto s ## b ## c ## a ## d;\
        else \
            goto s ## b ## c ## d ## a;\
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


    /**
     * \brief Similar to std::stack, with the following differences:
     * - Maximum size is fixed at compilation time, so an array can be used.
     * - Can be cleared "at once", without reallocation.
     */
    template <typename Tp_, unsigned_type max_size_>
    class internal_bounded_stack
    {
        typedef Tp_ value_type;
        typedef unsigned_type size_type;
        enum { max_size = max_size_ };

        size_type size_;
        value_type array[max_size];

    public:
        internal_bounded_stack() : size_(0) { }

        void push(const value_type & x)
        {
            assert(size_ < max_size);
            array[size_++] = x;
        }

        const value_type & top() const
        {
            assert(size_ > 0);
            return array[size_ - 1];
        }

        void pop()
        {
            assert(size_ > 0);
            --size_;
        }

        void clear()
        {
            size_ = 0;
        }

        size_type size() const
        {
            return size_;
        }

        bool empty() const
        {
            return size_ == 0;
        }
    };


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
            unsigned_type current;      //current index
            block_type * block;         //current block
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
            value_type key;      // Key of Loser element (winner for 0)
            unsigned_type index; // the number of losing segment
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
                // a) newKey <= winnerKey => even more reason for the other tree to loose
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

            unsigned_type to = 0;
            for (unsigned_type from = 0; from < k; from++)
            {
                if (!is_segment_empty(from))
                {
                    assert(is_segment_allocated(from));
                    if (from != to) {
                        assert(!is_segment_allocated(to));
                        states[to].swap(states[from]);
                    }
                    ++to;
                }
            }

            // half degree as often as possible
            while (k > 1 && to <= (k / 2)) {
                k /= 2;
                logK--;
            }

            // overwrite garbage and compact the stack of free segment indices
            free_segments.clear(); // none free
            for ( ;  to < k;  to++) {
                assert(!is_segment_allocated(to));
                states[to].make_inf();
                if (to < arity)
                    free_segments.push(to);
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
                //memcpy(to, states[0], length * sizeof(Element));
                //std::copy(states[0],states[0]+length,to);
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
            OutputIterator to = begin;
            unsigned_type winnerIndex = entry[0].index;
            Element winnerKey = entry[0].key;

            while (to != done)
            {
                // write result
                *to = *(states[winnerIndex]);

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

                ++to;
            }
            entry[0].index = winnerIndex;
            entry[0].key = winnerKey;
        }

        template <class OutputIterator, unsigned LogK>
        void multi_merge_f(OutputIterator begin, OutputIterator end)
        {
            OutputIterator done = end;
            OutputIterator to = begin;
            unsigned_type winnerIndex = entry[0].index;
            Entry * regEntry = entry;
            sequence_state * regStates = states;
            Element winnerKey = entry[0].key;

            assert(logK >= LogK);
            while (to != done)
            {
                // write result
                *to = *(regStates[winnerIndex]);

                // advance winner segment
                ++(regStates[winnerIndex]);

                winnerKey = *(regStates[winnerIndex]);


                // remove winner segment if empty now
                if (is_sentinel(winnerKey))
                    deallocate_segment(winnerIndex);


                ++to;

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
        bool spaceIsAvailable() const // for new segment
        {
            return k < arity || !free_segments.empty();
        }


        // insert segment beginning at to
        // require: spaceIsAvailable() == 1
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


    //////////////////////////////////////////////////////////////////////
// The data structure from Knuth, "Sorting and Searching", Section 5.4.1
    /**
     *!  \brief  Loser tree from Knuth, "Sorting and Searching", Section 5.4.1
     *!  \param  KNKMAX  maximum arity of loser tree, has to be a power of two
     */
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    class loser_tree : private noncopyable
    {
    public:
        typedef ValTp_ value_type;
        typedef Cmp_ comparator_type;
        typedef value_type Element;

    private:
        struct Entry
        {
            value_type key;      // Key of Loser element (winner for 0)
            unsigned_type index; // number of losing segment
        };

        comparator_type cmp;
        // stack of free segment indices
        internal_bounded_stack<unsigned_type, KNKMAX> free_segments;

        unsigned_type size_; // total number of elements stored
        unsigned_type logK;  // log of current tree size
        unsigned_type k;     // invariant (k == 1 << logK), always a power of two

        Element sentinel;    // target of free segment pointers

        // upper levels of loser trees
        // entry[0] contains the winner info
        Entry entry[KNKMAX];

        // leaf information
        // note that Knuth uses indices k..k-1
        // while we use 0..k-1
        Element * current[KNKMAX];          // pointer to current element
        Element * segment[KNKMAX];          // start of Segments
        unsigned_type segment_size[KNKMAX]; // just to count the internal memory consumption

        unsigned_type mem_cons_;

        // private member functions
        unsigned_type initWinner(unsigned_type root);
        void update_on_insert(unsigned_type node, const Element & newKey, unsigned_type newIndex,
                              Element * winnerKey, unsigned_type * winnerIndex, unsigned_type * mask);
        void deallocate_segment(unsigned_type slot);
        void doubleK();
        void compactTree();
        void rebuildLoserTree();
        bool is_segment_empty(unsigned_type slot);
        void multi_merge_k(Element * to, unsigned_type length);

        template <unsigned LogK>
        void multi_merge_f(Element * to, unsigned_type length)
        {
            //Entry *currentPos;
            //Element currentKey;
            //int currentIndex; // leaf pointed to by current entry
            Element * done = to + length;
            Entry * regEntry = entry;
            Element ** regStates = current;
            unsigned_type winnerIndex = regEntry[0].index;
            Element winnerKey = regEntry[0].key;
            Element * winnerPos;
            //Element sup = sentinel; // supremum

            assert(logK >= LogK);
            while (to != done)
            {
                winnerPos = regStates[winnerIndex];

                // write result
                *to = winnerKey;

                // advance winner segment
                ++winnerPos;
                regStates[winnerIndex] = winnerPos;
                winnerKey = *winnerPos;

                // remove winner segment if empty now
                if (is_sentinel(winnerKey))
                {
                    deallocate_segment(winnerIndex);
                }
                ++to;

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
        bool is_sentinel(const Element & a)
        {
            return !(cmp(cmp.min_value(), a));
        }
        bool not_sentinel(const Element & a)
        {
            return cmp(cmp.min_value(), a);
        }

    public:
        loser_tree();
        ~loser_tree();
        void init();

        void swap(loser_tree & obj)
        {
            std::swap(cmp, obj.cmp);
            std::swap(free_segments, obj.free_segments);
            std::swap(size_, obj.size_);
            std::swap(logK, obj.logK);
            std::swap(k, obj.k);
            std::swap(sentinel, obj.sentinel);
            swap_1D_arrays(entry, obj.entry, KNKMAX);
            swap_1D_arrays(current, obj.current, KNKMAX);
            swap_1D_arrays(segment, obj.segment, KNKMAX);
            swap_1D_arrays(segment_size, obj.segment_size, KNKMAX);
            std::swap(mem_cons_, obj.mem_cons_);
        }

        void multi_merge(Element * begin, Element * end)
        {
            multi_merge(begin, end - begin);
        }
        void multi_merge(Element *, unsigned_type length);

        unsigned_type mem_cons() const { return mem_cons_; }

        bool spaceIsAvailable() const // for new segment
        {
            return k < KNKMAX || !free_segments.empty();
        }

        void insert_segment(Element * to, unsigned_type sz); // insert segment beginning at to
        unsigned_type size() { return size_; }
    };

///////////////////////// LoserTree ///////////////////////////////////
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    loser_tree<ValTp_, Cmp_, KNKMAX>::loser_tree() : size_(0), logK(0), k(1), mem_cons_(0)
    {
        free_segments.push(0);
        segment[0] = 0;
        current[0] = &sentinel;
        // entry and sentinel are initialized by init
        // since they need the value of supremum
        init();
    }

    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::init()
    {
        assert(!cmp(cmp.min_value(), cmp.min_value())); // verify strict weak ordering
        sentinel = cmp.min_value();
        rebuildLoserTree();
        assert(current[entry[0].index] == &sentinel);
    }


// rebuild loser tree information from the values in current
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::rebuildLoserTree()
    {
        assert(LOG2<KNKMAX>::floor == LOG2<KNKMAX>::ceil); // KNKMAX needs to be a power of two
        unsigned_type winner = initWinner(1);
        entry[0].index = winner;
        entry[0].key = *(current[winner]);
    }


// given any values in the leaves this
// routing recomputes upper levels of the tree
// from scratch in linear time
// initialize entry[root].index and the subtree rooted there
// return winner index
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    unsigned_type loser_tree<ValTp_, Cmp_, KNKMAX>::initWinner(unsigned_type root)
    {
        if (root >= k) { // leaf reached
            return root - k;
        } else {
            unsigned_type left = initWinner(2 * root);
            unsigned_type right = initWinner(2 * root + 1);
            Element lk = *(current[left]);
            Element rk = *(current[right]);
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
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::update_on_insert(
        unsigned_type node,
        const Element & newKey,
        unsigned_type newIndex,
        Element * winnerKey,
        unsigned_type * winnerIndex,       // old winner
        unsigned_type * mask)              // 1 << (ceil(log KNK) - dist-from-root)
    {
        if (node == 0) {                   // winner part of root
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
            // a) newKey <= winnerKey => even more reason for the other tree to loose
            // b) newKey >  winnerKey => the old winner will beat the new
            //                           entry further down the tree
            // also the same old winner is handed down the tree

            *mask >>= 1; // next level
        }
    }


// make the tree two times as wide
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::doubleK()
    {
        STXXL_VERBOSE3("loser_tree::doubleK (before) k=" << k << " logK=" << logK << " KNKMAX=" << KNKMAX << " #free=" << free_segments.size());
        assert(k > 0);
        assert(k < KNKMAX);
        assert(free_segments.empty());                   // stack was free (probably not needed)

        // make all new entries free
        // and push them on the free stack
        for (unsigned_type i = 2 * k - 1;  i >= k;  i--) // backwards
        {
            current[i] = &sentinel;
            segment[i] = NULL;
            free_segments.push(i);
        }

        // double the size
        k *= 2;
        logK++;

        STXXL_VERBOSE3("loser_tree::doubleK (after)  k=" << k << " logK=" << logK << " KNKMAX=" << KNKMAX << " #free=" << free_segments.size());
        assert(!free_segments.empty());

        // recompute loser tree information
        rebuildLoserTree();
    }


// compact nonempty segments in the left half of the tree
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::compactTree()
    {
        STXXL_VERBOSE3("loser_tree::compactTree (before) k=" << k << " logK=" << logK << " #free=" << free_segments.size());
        assert(logK > 0);

        // compact all nonempty segments to the left
        unsigned_type from = 0;
        unsigned_type to = 0;
        for ( ;  from < k;  from++)
        {
            if (not_sentinel(*(current[from])))
            {
                segment_size[to] = segment_size[from];
                current[to] = current[from];
                segment[to] = segment[from];
                to++;
            }/*
                else
                {
                if(segment[from])
                {
                STXXL_VERBOSE2("loser_tree::compactTree() deleting segment "<<from<<
                                        " address: "<<segment[from]<<" size: "<<segment_size[from]);
                delete [] segment[from];
                segment[from] = 0;
                mem_cons_ -= segment_size[from];
                }
                }*/
        }

        // half degree as often as possible
        while (k > 1 && to <= (k / 2)) {
            k /= 2;
            logK--;
        }

        // overwrite garbage and compact the stack of free segment indices
        free_segments.clear(); // none free
        for ( ;  to < k;  to++) {
            current[to] = &sentinel;
            free_segments.push(to);
        }

        STXXL_VERBOSE3("loser_tree::compactTree (after)  k=" << k << " logK=" << logK << " #free=" << free_segments.size());

        // recompute loser tree information
        rebuildLoserTree();
    }


// insert segment beginning at to
// require: spaceIsAvailable() == 1
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::insert_segment(Element * to, unsigned_type sz)
    {
        STXXL_VERBOSE2("loser_tree::insert_segment(" << to << "," << sz << ")");
        //std::copy(to,to + sz,std::ostream_iterator<ValTp_>(std::cout, "\n"));

        if (sz > 0)
        {
            assert(not_sentinel(to[0]));
            assert(not_sentinel(to[sz - 1]));
            assert(is_sentinel(to[sz]));

            // get a free slot
            if (free_segments.empty()) { // tree is too small
                doubleK();
            }
            assert(!free_segments.empty());
            unsigned_type index = free_segments.top();
            free_segments.pop();


            // link new segment
            current[index] = segment[index] = to;
            segment_size[index] = (sz + 1) * sizeof(value_type);
            mem_cons_ += (sz + 1) * sizeof(value_type);
            size_ += sz;

            // propagate new information up the tree
            Element dummyKey;
            unsigned_type dummyIndex;
            unsigned_type dummyMask;
            update_on_insert((index + k) >> 1, *to, index,
                             &dummyKey, &dummyIndex, &dummyMask);
        } else {
            // immediately deallocate
            // this is not only an optimization
            // but also needed to keep free segments from
            // clogging up the tree
            delete[] to;
        }
    }


    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    loser_tree<ValTp_, Cmp_, KNKMAX>::~loser_tree()
    {
        STXXL_VERBOSE1("loser_tree::~loser_tree()");
        for (unsigned_type i = 0; i < k; ++i)
        {
            if (segment[i])
            {
                STXXL_VERBOSE2("loser_tree::~loser_tree() deleting segment " << i);
                delete[] segment[i];
                mem_cons_ -= segment_size[i];
            }
        }
        // check whether we did not loose memory
        assert(mem_cons_ == 0);
    }

// free an empty segment .
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::deallocate_segment(unsigned_type slot)
    {
        // reroute current pointer to some empty sentinel segment
        // with a sentinel key
        STXXL_VERBOSE2("loser_tree::deallocate_segment() deleting segment " <<
                       slot << " address: " << segment[slot] << " size: " << segment_size[slot]);
        current[slot] = &sentinel;

        // free memory
        delete[] segment[slot];
        segment[slot] = NULL;
        mem_cons_ -= segment_size[slot];

        // push on the stack of free segment indices
        free_segments.push(slot);
    }


// delete the length smallest elements and write them to "to"
// empty segments are deallocated
// require:
// - there are at least length elements
// - segments are ended by sentinels
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::multi_merge(Element * to, unsigned_type length)
    {
        STXXL_VERBOSE3("loser_tree::multi_merge(to=" << to << ", len=" << length << ") k=" << k);

        if (length == 0)
            return;

        assert(k > 0);
        assert(length <= size_);

        //This is the place to make statistics about internal multi_merge calls.

        switch (logK) {
        case 0:
            assert(k == 1);
            assert(entry[0].index == 0);
            assert(free_segments.empty());
            //memcpy(to, current[0], length * sizeof(Element));
            std::copy(current[0], current[0] + length, to);
            current[0] += length;
            entry[0].key = **current;
            if (is_segment_empty(0))
                deallocate_segment(0);

            break;
        case 1:
            assert(k == 2);
            merge_iterator(current[0], current[1], to, length, cmp);
            rebuildLoserTree();
            if (is_segment_empty(0))
                deallocate_segment(0);

            if (is_segment_empty(1))
                deallocate_segment(1);

            break;
        case 2:
            assert(k == 4);
            if (is_segment_empty(3))
                merge3_iterator(current[0], current[1], current[2], to, length, cmp);
            else
                merge4_iterator(current[0], current[1], current[2], current[3], to, length, cmp);

            rebuildLoserTree();
            if (is_segment_empty(0))
                deallocate_segment(0);

            if (is_segment_empty(1))
                deallocate_segment(1);

            if (is_segment_empty(2))
                deallocate_segment(2);

            if (is_segment_empty(3))
                deallocate_segment(3);

            break;
        case  3: multi_merge_f<3>(to, length);
            break;
        case  4: multi_merge_f<4>(to, length);
            break;
        case  5: multi_merge_f<5>(to, length);
            break;
        case  6: multi_merge_f<6>(to, length);
            break;
        case  7: multi_merge_f<7>(to, length);
            break;
        case  8: multi_merge_f<8>(to, length);
            break;
        case  9: multi_merge_f<9>(to, length);
            break;
        case 10: multi_merge_f<10>(to, length);
            break;
        default: multi_merge_k(to, length);
            break;
        }


        size_ -= length;

        // compact tree if it got considerably smaller
        {
            const unsigned_type num_segments_used = k - free_segments.size();
            const unsigned_type num_segments_trigger = k - (3 * k / 5);
            // using k/2 would be worst case inefficient (for large k)
            // for k \in {2, 4, 8} the trigger is k/2 which is good
            // because we have special mergers for k \in {1, 2, 4}
            // there is also a special 3-way-merger, that will be
            // triggered if k == 4 && is_segment_empty(3)
            STXXL_VERBOSE3("loser_tree  compact? k=" << k << " #used=" << num_segments_used
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
        //std::copy(to,to + length,std::ostream_iterator<ValTp_>(std::cout, "\n"));
    }


// is this segment empty and does not point to sentinel yet?
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    inline bool loser_tree<ValTp_, Cmp_, KNKMAX>::is_segment_empty(unsigned_type slot)
    {
        return (is_sentinel(*(current[slot])) && (current[slot] != &sentinel));
    }

// multi-merge for arbitrary K
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void loser_tree<ValTp_, Cmp_, KNKMAX>::
    multi_merge_k(Element * to, unsigned_type length)
    {
        Entry * currentPos;
        Element currentKey;
        unsigned_type currentIndex; // leaf pointed to by current entry
        unsigned_type kReg = k;
        Element * done = to + length;
        unsigned_type winnerIndex = entry[0].index;
        Element winnerKey = entry[0].key;
        Element * winnerPos;

        while (to != done)
        {
            winnerPos = current[winnerIndex];

            // write result
            *to = winnerKey;

            // advance winner segment
            ++winnerPos;
            current[winnerIndex] = winnerPos;
            winnerKey = *winnerPos;

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

            ++to;
        }
        entry[0].index = winnerIndex;
        entry[0].key = winnerKey;
    }
}

/*

   KNBufferSize1 = 32;
   KNN = 512; // bandwidth
   KNKMAX = 64;  // maximal arity
   KNLevels = 4; // overall capacity >= KNN*KNKMAX^KNLevels
   LogKNKMAX = 6;  // ceil(log KNK)
 */

// internal memory consumption >= N_*(KMAX_^Levels_) + ext

template <
    class Tp_,
    class Cmp_,
    unsigned BufferSize1_ = 32,       // equalize procedure call overheads etc.
    unsigned N_ = 512,                // bandwidth
    unsigned IntKMAX_ = 64,           // maximal arity for internal mergers
    unsigned IntLevels_ = 4,
    unsigned BlockSize_ = (2 * 1024 * 1024),
    unsigned ExtKMAX_ = 64,           // maximal arity for external mergers
    unsigned ExtLevels_ = 2,
    class AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY
    >
struct priority_queue_config
{
    typedef Tp_ value_type;
    typedef Cmp_ comparator_type;
    typedef AllocStr_ alloc_strategy_type;
    enum
    {
        BufferSize1 = BufferSize1_,
        N = N_,
        IntKMAX = IntKMAX_,
        IntLevels = IntLevels_,
        ExtLevels = ExtLevels_,
        BlockSize = BlockSize_,
        ExtKMAX = ExtKMAX_,
        E = sizeof(Tp_)
    };
};

__STXXL_END_NAMESPACE

namespace std
{
    template <class BlockType_,
              class Cmp_,
              unsigned Arity_,
              class AllocStr_>
    void swap(stxxl::priority_queue_local::ext_merger<BlockType_, Cmp_, Arity_, AllocStr_> & a,
              stxxl::priority_queue_local::ext_merger<BlockType_, Cmp_, Arity_, AllocStr_> & b)
    {
        a.swap(b);
    }
    template <class ValTp_, class Cmp_, unsigned KNKMAX>
    void swap(stxxl::priority_queue_local::loser_tree<ValTp_, Cmp_, KNKMAX> & a,
              stxxl::priority_queue_local::loser_tree<ValTp_, Cmp_, KNKMAX> & b)
    {
        a.swap(b);
    }
}

__STXXL_BEGIN_NAMESPACE

//! \brief External priority queue data structure
template <class Config_>
class priority_queue : private noncopyable
{
public:
    typedef Config_ Config;
    enum
    {
        BufferSize1 = Config::BufferSize1,
        N = Config::N,
        IntKMAX = Config::IntKMAX,
        IntLevels = Config::IntLevels,
        ExtLevels = Config::ExtLevels,
        Levels = Config::IntLevels + Config::ExtLevels,
        BlockSize = Config::BlockSize,
        ExtKMAX = Config::ExtKMAX
    };

    //! \brief The type of object stored in the \b priority_queue
    typedef typename Config::value_type value_type;
    //! \brief Comparison object
    typedef typename Config::comparator_type comparator_type;
    typedef typename Config::alloc_strategy_type alloc_strategy_type;
    //! \brief An unsigned integral type (64 bit)
    typedef stxxl::uint64 size_type;
    typedef typed_block<BlockSize, value_type> block_type;

protected:
    typedef priority_queue_local::internal_priority_queue<value_type, std::vector<value_type>, comparator_type>
    insert_heap_type;

    typedef priority_queue_local::loser_tree<
        value_type,
        comparator_type,
        IntKMAX> int_merger_type;

    typedef priority_queue_local::ext_merger<
        block_type,
        comparator_type,
        ExtKMAX,
        alloc_strategy_type> ext_merger_type;


    int_merger_type itree[IntLevels];
    prefetch_pool<block_type> & p_pool;
    write_pool<block_type> & w_pool;
    ext_merger_type * etree;

    // one delete buffer for each tree => group buffer
    value_type buffer2[Levels][N + 1]; // tree->buffer2->buffer1 (extra space for sentinel)
    value_type * minBuffer2[Levels];   // minBuffer2[i] is current start of buffer2[i], end is buffer2[i] + N

    // overall delete buffer
    value_type buffer1[BufferSize1 + 1];
    value_type * minBuffer1;           // is current start of buffer1, end is buffer1 + BufferSize1

    comparator_type cmp;

    // insert buffer
    insert_heap_type insertHeap;

    // how many levels are active
    unsigned_type activeLevels;

    // total size not counting insertBuffer and buffer1
    size_type size_;
    bool deallocate_pools;

    // private member functions
    void refillBuffer1();
    unsigned_type refillBuffer2(unsigned_type k);

    unsigned_type makeSpaceAvailable(unsigned_type level);
    void emptyInsertHeap();

    value_type getSupremum() const { return cmp.min_value(); } //{ return buffer2[0][KNN].key; }
    unsigned_type getSize1() const { return (buffer1 + BufferSize1) - minBuffer1; }
    unsigned_type getSize2(unsigned_type i) const { return &(buffer2[i][N]) - minBuffer2[i]; }

public:
    //! \brief Constructs external priority queue object
    //! \param p_pool_ pool of blocks that will be used
    //! for data prefetching for the disk<->memory transfers
    //! happening in the priority queue. Larger pool size
    //! helps to speed up operations.
    //! \param w_pool_ pool of blocks that will be used
    //! for writing data for the memory<->disk transfers
    //! happening in the priority queue. Larger pool size
    //! helps to speed up operations.
    priority_queue(prefetch_pool<block_type> & p_pool_, write_pool<block_type> & w_pool_);

    //! \brief Constructs external priority queue object
    //! \param p_pool_mem memory (in bytes) for prefetch pool that will be used
    //! for data prefetching for the disk<->memory transfers
    //! happening in the priority queue. Larger pool size
    //! helps to speed up operations.
    //! \param w_pool_mem memory (in bytes) for buffered write pool that will be used
    //! for writing data for the memory<->disk transfers
    //! happening in the priority queue. Larger pool size
    //! helps to speed up operations.
    priority_queue(unsigned_type p_pool_mem, unsigned_type w_pool_mem);

    void swap(priority_queue & obj)
    {
        //swap_1D_arrays(itree,obj.itree,IntLevels); // does not work in g++ 3.4.3 :( bug?
        for (unsigned_type i = 0; i < IntLevels; ++i)
            std::swap(itree[i], obj.itree[i]);

        // std::swap(p_pool,obj.p_pool);
        // std::swap(w_pool,obj.w_pool);
        std::swap(etree, obj.etree);
        for (unsigned_type i1 = 0; i1 < Levels; ++i1)
            for (unsigned_type i2 = 0; i2 < (N + 1); ++i2)
                std::swap(buffer2[i1][i2], obj.buffer2[i1][i2]);

        swap_1D_arrays(minBuffer2, obj.minBuffer2, Levels);
        swap_1D_arrays(buffer1, obj.buffer1, BufferSize1 + 1);
        std::swap(minBuffer1, obj.minBuffer1);
        std::swap(cmp, obj.cmp);
        std::swap(insertHeap, obj.insertHeap);
        std::swap(activeLevels, obj.activeLevels);
        std::swap(size_, obj.size_);
        //std::swap(deallocate_pools,obj.deallocate_pools);
    }

    virtual ~priority_queue();

    //! \brief Returns number of elements contained
    //! \return number of elements contained
    size_type size() const;

    //! \brief Returns true if queue has no elements
    //! \return \b true if queue has no elements, \b false otherwise
    bool empty() const { return (size() == 0); }

    //! \brief Returns "largest" element
    //!
    //! Returns a const reference to the element at the
    //! top of the priority_queue. The element at the top is
    //! guaranteed to be the largest element in the \b priority queue,
    //! as determined by the comparison function \b Config_::comparator_type
    //! (the same as the second parameter of PRIORITY_QUEUE_GENERATOR utility
    //! class). That is,
    //! for every other element \b x in the priority_queue,
    //! \b Config_::comparator_type(Q.top(), x) is false.
    //! Precondition: \c empty() is false.
    const value_type & top() const;

    //! \brief Removes the element at the top
    //!
    //! Removes the element at the top of the priority_queue, that
    //! is, the largest element in the \b priority_queue.
    //! Precondition: \c empty() is \b false.
    //! Postcondition: \c size() will be decremented by 1.
    void pop();

    //! \brief Inserts x into the priority_queue.
    //!
    //! Inserts x into the priority_queue. Postcondition:
    //! \c size() will be incremented by 1.
    void push(const value_type & obj);

    //! \brief Returns number of bytes consumed by
    //! the \b priority_queue
    //! \brief number of bytes consumed by the \b priority_queue from
    //! the internal memory not including pools (see the constructor)
    unsigned_type mem_cons() const
    {
        unsigned_type dynam_alloc_mem(0), i(0);
        //dynam_alloc_mem += w_pool.mem_cons();
        //dynam_alloc_mem += p_pool.mem_cons();
        for ( ; i < IntLevels; ++i)
            dynam_alloc_mem += itree[i].mem_cons();

        for (i = 0; i < ExtLevels; ++i)
            dynam_alloc_mem += etree[i].mem_cons();


        return (sizeof(*this) +
                sizeof(ext_merger_type) * ExtLevels +
                dynam_alloc_mem);
    }
};


template <class Config_>
inline typename priority_queue<Config_>::size_type priority_queue<Config_>::size() const
{
    return
           size_ +
           insertHeap.size() - 1 +
           ((buffer1 + BufferSize1) - minBuffer1);
}


template <class Config_>
inline const typename priority_queue<Config_>::value_type & priority_queue<Config_>::top() const
{
    assert(!insertHeap.empty());

    const typename priority_queue<Config_>::value_type & t = insertHeap.top();
    if ( /*(!insertHeap.empty()) && */ cmp(*minBuffer1, t))
        return t;


    return *minBuffer1;
}

template <class Config_>
inline void priority_queue<Config_>::pop()
{
    //STXXL_VERBOSE1("priority_queue::pop()");
    assert(!insertHeap.empty());

    if ( /*(!insertHeap.empty()) && */ cmp(*minBuffer1, insertHeap.top()))
    {
        insertHeap.pop();
    }
    else
    {
        assert(minBuffer1 < buffer1 + BufferSize1);
        ++minBuffer1;
        if (minBuffer1 == buffer1 + BufferSize1)
            refillBuffer1();
    }
}

template <class Config_>
inline void priority_queue<Config_>::push(const value_type & obj)
{
    //STXXL_VERBOSE3("priority_queue::push("<< obj <<")");
    assert(itree->not_sentinel(obj));
    if (insertHeap.size() == N + 1)
        emptyInsertHeap();


    assert(!insertHeap.empty());

    insertHeap.push(obj);
}


////////////////////////////////////////////////////////////////

template <class Config_>
priority_queue<Config_>::priority_queue(prefetch_pool<block_type> & p_pool_, write_pool<block_type> & w_pool_) :
    p_pool(p_pool_), w_pool(w_pool_),
    insertHeap(N + 2),
    activeLevels(0), size_(0),
    deallocate_pools(false)
{
    STXXL_VERBOSE2("priority_queue::priority_queue()");
    assert(!cmp(cmp.min_value(), cmp.min_value())); // verify strict weak ordering
    //etree = new ext_merger_type[ExtLevels](p_pool,w_pool);
    etree = new ext_merger_type[ExtLevels];
    for (unsigned_type j = 0; j < ExtLevels; ++j)
        etree[j].set_pools(&p_pool, &w_pool);

    value_type sentinel = cmp.min_value();
    buffer1[BufferSize1] = sentinel;      // sentinel
    insertHeap.push(sentinel);            // always keep the sentinel
    minBuffer1 = buffer1 + BufferSize1;   // empty
    for (unsigned_type i = 0;  i < Levels;  i++)
    {
        buffer2[i][N] = sentinel;         // sentinel
        minBuffer2[i] = &(buffer2[i][N]); // empty
    }
}

template <class Config_>
priority_queue<Config_>::priority_queue(unsigned_type p_pool_mem, unsigned_type w_pool_mem) :
    p_pool(*(new prefetch_pool<block_type>(p_pool_mem / BlockSize))),
    w_pool(*(new write_pool<block_type>(w_pool_mem / BlockSize))),
    insertHeap(N + 2),
    activeLevels(0), size_(0),
    deallocate_pools(true)
{
    STXXL_VERBOSE2("priority_queue::priority_queue()");
    assert(!cmp(cmp.min_value(), cmp.min_value())); // verify strict weak ordering
    etree = new ext_merger_type[ExtLevels];
    for (unsigned_type j = 0; j < ExtLevels; ++j)
        etree[j].set_pools(&p_pool, &w_pool);

    value_type sentinel = cmp.min_value();
    buffer1[BufferSize1] = sentinel;      // sentinel
    insertHeap.push(sentinel);            // always keep the sentinel
    minBuffer1 = buffer1 + BufferSize1;   // empty
    for (unsigned_type i = 0;  i < Levels;  i++)
    {
        buffer2[i][N] = sentinel;         // sentinel
        minBuffer2[i] = &(buffer2[i][N]); // empty
    }
}

template <class Config_>
priority_queue<Config_>::~priority_queue()
{
    STXXL_VERBOSE2("priority_queue::~priority_queue()");
    if (deallocate_pools)
    {
        delete &p_pool;
        delete &w_pool;
    }

    delete[] etree;
}

//--------------------- Buffer refilling -------------------------------

// refill buffer2[j] and return number of elements found
template <class Config_>
unsigned_type priority_queue<Config_>::refillBuffer2(unsigned_type j)
{
    STXXL_VERBOSE2("priority_queue::refillBuffer2(" << j << ")");

    value_type * oldTarget;
    unsigned_type deleteSize;
    size_type treeSize = (j < IntLevels) ? itree[j].size() : etree[j - IntLevels].size();  //elements left in segments
    unsigned_type bufferSize = buffer2[j] + N - minBuffer2[j];                             //elements left in target buffer
    if (treeSize + bufferSize >= size_type(N))
    {                                                                                      // buffer will be filled completely
        oldTarget = buffer2[j];
        deleteSize = N - bufferSize;
    }
    else
    {
        oldTarget = buffer2[j] + N - treeSize - bufferSize;
        deleteSize = treeSize;
    }

    if (deleteSize > 0)
    {
        // shift  rest to beginning
        // possible hack:
        // - use memcpy if no overlap
        memmove(oldTarget, minBuffer2[j], bufferSize * sizeof(value_type));
        minBuffer2[j] = oldTarget;

        // fill remaining space from tree
        if (j < IntLevels)
            itree[j].multi_merge(oldTarget + bufferSize, deleteSize);

        else
        {
            //external
            etree[j - IntLevels].multi_merge(oldTarget + bufferSize,
                                             oldTarget + bufferSize + deleteSize);
        }
    }


    //STXXL_MSG(deleteSize + bufferSize);
    //std::copy(oldTarget,oldTarget + deleteSize + bufferSize,std::ostream_iterator<value_type>(std::cout, "\n"));

    return deleteSize + bufferSize;
}


// move elements from the 2nd level buffers
// to the buffer
template <class Config_>
void priority_queue<Config_>::refillBuffer1()
{
    STXXL_VERBOSE2("priority_queue::refillBuffer1()");

    size_type totalSize = 0;
    unsigned_type sz;
    //activeLevels is <= 4
    for (int_type i = activeLevels - 1;  i >= 0;  i--)
    {
        if ((buffer2[i] + N) - minBuffer2[i] < BufferSize1)
        {
            sz = refillBuffer2(i);
            // max active level dry now?
            if (sz == 0 && unsigned_type(i) == activeLevels - 1)
                --activeLevels;

            else
                totalSize += sz;
        }
        else
        {
            totalSize += BufferSize1;    // actually only a sufficient lower bound
        }
    }

    if (totalSize >= BufferSize1)        // buffer can be filled completely
    {
        minBuffer1 = buffer1;
        sz = BufferSize1;                // amount to be copied
        size_ -= size_type(BufferSize1); // amount left in buffer2
    }
    else
    {
        minBuffer1 = buffer1 + BufferSize1 - totalSize;
        sz = totalSize;
        assert(size_ == size_type(sz)); // trees and buffer2 get empty
        size_ = 0;
    }

    // now call simplified refill routines
    // which can make the assumption that
    // they find all they are asked to find in the buffers
    minBuffer1 = buffer1 + BufferSize1 - sz;
    STXXL_VERBOSE2("Active levels = " << activeLevels);
    switch (activeLevels)
    {
    case 0: break;
    case 1:
        std::copy(minBuffer2[0], minBuffer2[0] + sz, minBuffer1);
        minBuffer2[0] += sz;
        break;
    case 2:
        priority_queue_local::merge_iterator(
            minBuffer2[0],
            minBuffer2[1], minBuffer1, sz, cmp);
        break;
    case 3:
        priority_queue_local::merge3_iterator(
            minBuffer2[0],
            minBuffer2[1],
            minBuffer2[2], minBuffer1, sz, cmp);
        break;
    case 4:
        priority_queue_local::merge4_iterator(
            minBuffer2[0],
            minBuffer2[1],
            minBuffer2[2],
            minBuffer2[3], minBuffer1, sz, cmp); //side effect free
        break;
    default:
        STXXL_THROW(std::runtime_error, "priority_queue<...>::refillBuffer1()",
                    "Overflow! The number of buffers on 2nd level in stxxl::priority_queue is currently limited to 4");
    }

    //std::copy(minBuffer1,minBuffer1 + sz,std::ostream_iterator<value_type>(std::cout, "\n"));
}

//--------------------------------------------------------------------

// check if space is available on level k and
// empty this level if necessary leading to a recursive call.
// return the level where space was finally available
template <class Config_>
unsigned_type priority_queue<Config_>::makeSpaceAvailable(unsigned_type level)
{
    STXXL_VERBOSE2("priority_queue::makeSpaceAvailable(" << level << ")");
    unsigned_type finalLevel;
    assert(level < Levels);
    assert(level <= activeLevels);

    if (level == activeLevels)
        activeLevels++;


    const bool spaceIsAvailable_ =
        (level < IntLevels) ? itree[level].spaceIsAvailable()
        : ((level == Levels - 1) ? true : (etree[level - IntLevels].spaceIsAvailable()));

    if (spaceIsAvailable_)
    {
        finalLevel = level;
    }
    else
    {
        finalLevel = makeSpaceAvailable(level + 1);

        if (level < IntLevels - 1)                             // from internal to internal tree
        {
            unsigned_type segmentSize = itree[level].size();
            value_type * newSegment = new value_type[segmentSize + 1];
            itree[level].multi_merge(newSegment, segmentSize); // empty this level

            newSegment[segmentSize] = buffer1[BufferSize1];    // sentinel
            // for queues where size << #inserts
            // it might make sense to stay in this level if
            // segmentSize < alpha * KNN * k^level for some alpha < 1
            itree[level + 1].insert_segment(newSegment, segmentSize);
        }
        else
        {
            if (level == IntLevels - 1) // from internal to external tree
            {
                const unsigned_type segmentSize = itree[IntLevels - 1].size();
                STXXL_VERBOSE1("Inserting segment into first level external: " << level << " " << segmentSize);
                etree[0].insert_segment(itree[IntLevels - 1], segmentSize);
            }
            else // from external to external tree
            {
                const size_type segmentSize = etree[level - IntLevels].size();
                STXXL_VERBOSE1("Inserting segment into second level external: " << level << " " << segmentSize);
                etree[level - IntLevels + 1].insert_segment(etree[level - IntLevels], segmentSize);
            }
        }
    }
    return finalLevel;
}


// empty the insert heap into the main data structure
template <class Config_>
void priority_queue<Config_>::emptyInsertHeap()
{
    STXXL_VERBOSE2("priority_queue::emptyInsertHeap()");
    assert(insertHeap.size() == (N + 1));

    const value_type sup = getSupremum();

    // build new segment
    value_type * newSegment = new value_type[N + 1];
    value_type * newPos = newSegment;

    // put the new data there for now
    //insertHeap.sortTo(newSegment);
    value_type * SortTo = newSegment;

    insertHeap.sort_to(SortTo);

    SortTo = newSegment + N;
    insertHeap.clear();
    insertHeap.push(*SortTo);

    assert(insertHeap.size() == 1);

    newSegment[N] = sup; // sentinel

    // copy the buffer1 and buffer2[0] to temporary storage
    // (the temporary can be eliminated using some dirty tricks)
    const unsigned_type tempSize = N + BufferSize1;
    value_type temp[tempSize + 1];
    unsigned_type sz1 = getSize1();
    unsigned_type sz2 = getSize2(0);
    value_type * pos = temp + tempSize - sz1 - sz2;
    std::copy(minBuffer1, minBuffer1 + sz1, pos);
    std::copy(minBuffer2[0], minBuffer2[0] + sz2, pos + sz1);
    temp[tempSize] = sup; // sentinel

    // refill buffer1
    // (using more complicated code it could be made somewhat fuller
    // in certain circumstances)
    priority_queue_local::merge_iterator(pos, newPos, minBuffer1, sz1, cmp);

    // refill buffer2[0]
    // (as above we might want to take the opportunity
    // to make buffer2[0] fuller)
    priority_queue_local::merge_iterator(pos, newPos, minBuffer2[0], sz2, cmp);

    // merge the rest to the new segment
    // note that merge exactly trips into the footsteps
    // of itself
    priority_queue_local::merge_iterator(pos, newPos, newSegment, N, cmp);

    // and insert it
    unsigned_type freeLevel = makeSpaceAvailable(0);
    assert(freeLevel == 0 || itree[0].size() == 0);
    itree[0].insert_segment(newSegment, N);

    // get rid of invalid level 2 buffers
    // by inserting them into tree 0 (which is almost empty in this case)
    if (freeLevel > 0)
    {
        for (int_type i = freeLevel;  i >= 0;  i--)
        {                                                 // reverse order not needed
          // but would allow immediate refill

            newSegment = new value_type[getSize2(i) + 1]; // with sentinel
            std::copy(minBuffer2[i], minBuffer2[i] + getSize2(i) + 1, newSegment);
            itree[0].insert_segment(newSegment, getSize2(i));
            minBuffer2[i] = buffer2[i] + N;               // empty
        }
    }

    // update size
    size_ += size_type(N);

    // special case if the tree was empty before
    if (minBuffer1 == buffer1 + BufferSize1)
        refillBuffer1();
}

namespace priority_queue_local
{
    struct Parameters_for_priority_queue_not_found_Increase_IntM
    {
        enum { fits = false };
        typedef Parameters_for_priority_queue_not_found_Increase_IntM result;
    };

    struct dummy
    {
        enum { fits = false };
        typedef dummy result;
    };

    template <unsigned_type E_, unsigned_type IntM_, unsigned_type MaxS_, unsigned_type B_, unsigned_type m_, bool stop = false>
    struct find_B_m
    {
        typedef find_B_m<E_, IntM_, MaxS_, B_, m_, stop> Self;
        enum {
            k = IntM_ / B_, // number of blocks that fit into M
            E = E_,         // element size
            IntM = IntM_,
            B = B_,         // block size
            m = m_,         // number of blocks fitting into buffers
            c = k - m_,
            // memory occ. by block must be at least 10 times larger than size of ext sequence
            // && satisfy memory req && if we have two ext mergers their degree must be at least 64=m/2
            fits = c > 10 && ((k - m) * (m) * (m * B / (E * 4 * 1024))) >= MaxS_ && (MaxS_ < ((k - m) * m / (2 * E)) * 1024 || m >= 128),
            step = 1
        };

        typedef typename find_B_m<E, IntM, MaxS_, B, m + step, fits || (m >= k - step)>::result candidate1;
        typedef typename find_B_m<E, IntM, MaxS_, B / 2, 1, fits || candidate1::fits>::result candidate2;
        typedef typename IF<fits, Self, typename IF<candidate1::fits, candidate1, candidate2>::result>::result result;
    };

    // specialization for the case when no valid parameters are found
    template <unsigned_type E_, unsigned_type IntM_, unsigned_type MaxS_, bool stop>
    struct find_B_m<E_, IntM_, MaxS_, 2048, 1, stop>
    {
        enum { fits = false };
        typedef Parameters_for_priority_queue_not_found_Increase_IntM result;
    };

    // to speedup search
    template <unsigned_type E_, unsigned_type IntM_, unsigned_type MaxS_, unsigned_type B_, unsigned_type m_>
    struct find_B_m<E_, IntM_, MaxS_, B_, m_, true>
    {
        enum { fits = false };
        typedef dummy result;
    };

    // E_ size of element in bytes
    template <unsigned_type E_, unsigned_type IntM_, unsigned_type MaxS_>
    struct find_settings
    {
        // start from block size (8*1024*1024) bytes
        typedef typename find_B_m<E_, IntM_, MaxS_, (8 * 1024 * 1024), 1>::result result;
    };

    struct Parameters_not_found_Try_to_change_the_Tune_parameter
    {
        typedef Parameters_not_found_Try_to_change_the_Tune_parameter result;
    };


    template <unsigned_type AI_, unsigned_type X_, unsigned_type CriticalSize_>
    struct compute_N
    {
        typedef compute_N<AI_, X_, CriticalSize_> Self;
        enum
        {
            X = X_,
            AI = AI_,
            N = X / (AI * AI) // two stage internal
        };
        typedef typename IF<(N >= CriticalSize_), Self, typename compute_N<AI / 2, X, CriticalSize_>::result>::result result;
    };

    template <unsigned_type X_, unsigned_type CriticalSize_>
    struct compute_N<1, X_, CriticalSize_>
    {
        typedef Parameters_not_found_Try_to_change_the_Tune_parameter result;
    };
}

//! \}

//! \addtogroup stlcont
//! \{

//! \brief Priority queue type generator

//! Implements a data structure from "Peter Sanders. Fast Priority Queues
//! for Cached Memory. ALENEX'99" for external memory.
//! <BR>
//! Template parameters:
//! - Tp_ type of the contained objects
//! - Cmp_ the comparison type used to determine
//! whether one element is smaller than another element.
//! If Cmp_(x,y) is true, then x is smaller than y. The element
//! returned by Q.top() is the largest element in the priority
//! queue. That is, it has the property that, for every other
//! element \b x in the priority queue, Cmp_(Q.top(), x) is false.
//! Cmp_ must also provide min_value method, that returns value of type Tp_ that is
//! smaller than any element of the queue \b x , i.e. Cmp_(Cmp_.min_value(),x) is
//! always \b true . <BR>
//! <BR>
//! Example: comparison object for priority queue
//! where \b top() returns the \b smallest contained integer:
//! \verbatim
//! struct CmpIntGreater
//! {
//!   bool operator () (const int & a, const int & b) const { return a>b; }
//!   int min_value() const  { return std::numeric_limits<int>::max(); }
//! };
//! \endverbatim
//! Example: comparison object for priority queue
//! where \b top() returns the \b largest contained integer:
//! \verbatim
//! struct CmpIntLess
//! {
//!   bool operator () (const int & a, const int & b) const { return a<b; }
//!   int min_value() const  { return std::numeric_limits<int>::min(); }
//! };
//! \endverbatim
//! Note that Cmp_ must define strict weak ordering.
//! (<A HREF="http://www.sgi.com/tech/stl/StrictWeakOrdering.html">see what it is</A>)
//! - \c IntM_ upper limit for internal memory consumption in bytes.
//! - \c MaxS_ upper limit for number of elements contained in the priority queue (in 1024 units).
//! Example: if you are sure that priority queue contains no more than
//! one million elements in a time, then the right parameter is (1000000/1024)= 976 .
//! - \c Tune_ tuning parameter. Try to play with it if the code does not compile
//! (larger than default values might help). Code does not compile
//! if no suitable internal parameters were found for given IntM_ and MaxS_.
//! It might also happen that given IntM_ is too small for given MaxS_, try larger values.
//! <BR>
//! \c PRIORITY_QUEUE_GENERATOR is template meta program that searches
//! for \b 7 configuration parameters of \b stxxl::priority_queue that both
//! minimize internal memory consumption of the priority queue to
//! match IntM_ and maximize performance of priority queue operations.
//! Actual memory consumption might be larger (use
//! \c stxxl::priority_queue::mem_cons() method to track it), since the search
//! assumes rather optimistic schedule of push'es and pop'es for the
//! estimation of the maximum memory consumption. To keep actual memory
//! requirements low increase the value of MaxS_ parameter.
//! <BR>
//! For functioning a priority queue object requires two pools of blocks
//! (See constructor of \c priority_queue ). To construct \c \<stxxl\> block
//! pools you might need \b block \b type that will be used by priority queue.
//! Note that block's size and hence it's type is generated by
//! the \c PRIORITY_QUEUE_GENERATOR in compile type from IntM_, MaxS_ and sizeof(Tp_) and
//! not given directly by user as a template parameter. Block type can be extracted as
//! \c PRIORITY_QUEUE_GENERATOR<some_parameters>::result::block_type .
//! For an example see p_queue.cpp .
//! Configured priority queue type is available as \c PRIORITY_QUEUE_GENERATOR<>::result. <BR> <BR>
template <class Tp_, class Cmp_, unsigned_type IntM_, unsigned MaxS_, unsigned Tune_ = 6>
class PRIORITY_QUEUE_GENERATOR
{
public:
    // actual calculation of B, m, k and E
    typedef typename priority_queue_local::find_settings<sizeof(Tp_), IntM_, MaxS_>::result settings;
    enum {
        B = settings::B,
        m = settings::m,
        X = B * (settings::k - m) / settings::E,  // interpretation of result
        Buffer1Size = 32                          // fixed
    };
    // derivation of N, AI, AE
    typedef typename priority_queue_local::compute_N<(1 << Tune_), X, 4 * Buffer1Size>::result ComputeN;
    enum
    {
        N = ComputeN::N,
        AI = ComputeN::AI,
        AE = (m / 2 < 2) ? 2 : (m / 2)            // at least 2
    };
    enum {
        // Estimation of maximum internal memory consumption (in bytes)
        EConsumption = X * settings::E + settings::B * AE + ((MaxS_ / X) / AE) * settings::B * 1024
    };
    /*
        unsigned BufferSize1_ = 32, // equalize procedure call overheads etc.
        unsigned N_ = 512,          // bandwidth
        unsigned IntKMAX_ = 64,     // maximal arity for internal mergers
        unsigned IntLevels_ = 4,
        unsigned BlockSize_ = (2*1024*1024),
        unsigned ExtKMAX_ = 64,     // maximal arity for external mergers
        unsigned ExtLevels_ = 2,
     */
    typedef priority_queue<priority_queue_config<Tp_, Cmp_, Buffer1Size, N, AI, 2, B, AE, 2> > result;
};

//! \}

__STXXL_END_NAMESPACE


namespace std
{
    template <class Config_>
    void swap(stxxl::priority_queue<Config_> & a,
              stxxl::priority_queue<Config_> & b)
    {
        a.swap(b);
    }
}

#endif // !STXXL_PRIORITY_QUEUE_HEADER
// vim: et:ts=4:sw=4
