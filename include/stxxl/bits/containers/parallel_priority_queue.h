/***************************************************************************
 *  include/stxxl/bits/containers/parallel_priority_queue.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_PARALLEL_PRIORITY_QUEUE_HEADER
#define STXXL_CONTAINERS_PARALLEL_PRIORITY_QUEUE_HEADER

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <list>
#include <utility>
#include <vector>

#if STXXL_PARALLEL
    #include <omp.h>
    #include <parallel/algorithm>
    #include <parallel/numeric>
#endif

#include <stxxl/bits/common/winner_tree.h>
#include <stxxl/bits/common/custom_stats.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/timer.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/io/request_operations.h>
#include <stxxl/bits/mng/block_alloc.h>
#include <stxxl/bits/mng/buf_ostream.h>
#include <stxxl/bits/mng/prefetch_pool.h>
#include <stxxl/bits/mng/block_manager.h>
#include <stxxl/bits/mng/read_write_pool.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/namespace.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/types>

STXXL_BEGIN_NAMESPACE

namespace ppq_local {

/*!
 * Internal arrays store a sorted sequence of values in RAM, which will be
 * merged together into the deletion buffer, when it needs to be
 * refilled. Internal arrays are constructed from the insertions heaps when
 * they overflow.
 */
template <class ValueType>
class internal_array
{
protected:
    //! Contains the items of the sorted sequence.
    std::vector<ValueType> m_values;

    //! Index of the current head
    size_t m_min_index;

public:
    //! The value iterator type used by begin() and end(). We use pointers as
    //! iterator so internal arrays are compatible to external arrays and can
    //! be merged together.
    typedef ValueType* iterator;

    //! Default constructor. Don't use this directy. Needed for regrowing in
    //! surrounding vector.
    internal_array() = default;

    //! Constructor which takes a value vector. The vector should not be used
    //! outside this class anymore!
    internal_array(std::vector<ValueType>& values)
        : m_values(), m_min_index(0)
    {
        std::swap(m_values, values);
    }

    //! Move constructor. Needed for regrowing in surrounding vector.
    internal_array(internal_array&& o)
        : m_values(std::move(o.m_values)),
          m_min_index(o.m_min_index)
    { }

    //! Delete copy assignment for emplace_back to use the move semantics.
    internal_array& operator = (internal_array& other) = delete;

    //! Delete copy constructor for emplace_back to use the move semantics.
    internal_array(const internal_array& other) = delete;

    //! Move assignment.
    internal_array& operator = (internal_array&&)
    {
        return *this;
    }

    //! Random access operator
    inline ValueType& operator [] (size_t i) const
    {
        return m_values[i];
    }

    //! Use inc_min if a value has been extracted.
    inline void inc_min()
    {
        m_min_index++;
    }

    //! Use inc_min(diff) if multiple values have been extracted.
    inline void inc_min(size_t diff)
    {
        m_min_index += diff;
    }

    //! The currently smallest element in the array.
    inline const ValueType & get_min() const
    {
        return m_values[m_min_index];
    }

    //! The index of the currently smallest element in the array.
    inline size_t get_min_index() const
    {
        return m_min_index;
    }

    //! The index of the largest element in the array.
    inline size_t get_max_index() const
    {
        return (m_values.size() - 1);
    }

    //! Returns if the array has run empty.
    inline bool empty() const
    {
        return (m_min_index >= m_values.size());
    }

    //! Returns the current size of the array.
    inline size_t size() const
    {
        return (m_values.size() - m_min_index);
    }

    //! Begin iterator
    inline iterator begin()
    {
        // We use &(*()) in order to get a pointer iterator. This is allowed
        // because values are guaranteed to be consecutive in std::vecotor.
        return &(*(m_values.begin() + m_min_index));
    }

    //! End iterator
    inline iterator end()
    {
        // We use &(*()) in order to get a pointer iterator. This is allowed
        // because values are guaranteed to be consecutive in std::vecotor.
        return &(*(m_values.end()));
    }
};

/*!
 * External array stores a sorted sequence of values on the hard disk and
 * allows access to the first block (containing the smallest values).  The
 * class uses buffering and prefetching in order to improve the performance.
 *
 * \tparam ValueType Type of the contained objects (POD with no references to
 * internal memory).
 *
 * \tparam BlockSize External block size. Default =
 * STXXL_DEFAULT_BLOCK_SIZE(ValueType).
 *
 * \tparam AllocStrategy Allocation strategy for the external memory. Default =
 * STXXL_DEFAULT_ALLOC_STRATEGY.
 */
template <
    class ValueType,
    unsigned_type BlockSize = STXXL_DEFAULT_BLOCK_SIZE(ValueType),
    class AllocStrategy = STXXL_DEFAULT_ALLOC_STRATEGY
    >
class external_array
{
public:
    typedef external_array<ValueType, BlockSize, AllocStrategy> self_type;
    typedef std::vector<BID<BlockSize> > bid_vector;
    typedef typed_block<BlockSize, ValueType> block_type;
    typedef ValueType* iterator;
    typedef const ValueType* const_iterator;
    typedef typename bid_vector::iterator bid_iterator;
    typedef typename block_type::iterator block_iterator;
    typedef read_write_pool<block_type> pool_type;

protected:
    //! Allocation strategy for new blocks
    AllocStrategy m_alloc_strategy;

    //! The total capacity of the external array. Cannot be changed after
    //! construction.
    size_t m_capacity;

    //! The total number of elements minus the number of extracted values
    size_t m_size;

    //! The number of elements fitting into one block
    static const unsigned_type num_elements_per_block = BlockSize / sizeof(ValueType);

    //! Number of blocks in external memory
    size_t m_num_bids;

    //! Number of blocks to prefetch from EM
    size_t m_num_prefetch_blocks;

    //! Size of the write buffer in number of blocks
    size_t m_num_write_buffer_blocks;

    //! The IDs of each block in external memory
    bid_vector m_bids;

    //! The block with the currently smallest elements
    block_type* m_first_block;

    //! Prefetch and write buffer pool
    pool_type* m_pool;

    //! The read_request can be used to wait until the block has been
    //! completely fetched.
    request_ptr m_read_request;

    //! The write position in the block currently being filled. Used by the
    //! <<-operator.
    size_t m_write_position;

    //! Index of the next block to be prefetched.
    size_t m_hint_index;

    /* Write phase: Index of the external block where the current block will be
     * stored in when it's filled. Read phase: Index of the external block
     * which will be fetched next.
     */
    size_t m_current_bid_index;

    //! True means write phase, false means read phase.
    bool m_write_phase;

    //! The first block is valid if wait_for_first_block() has already been
    //! called.
    bool m_first_block_valid;

    //! Indicates if first_block is actually the first block of all which has
    //! never been written to external memory.
    bool m_is_first_block;

    //! boundaries inside the first block
    size_t m_begin_index;

    //! boundaries inside the first block
    size_t m_end_index;

public:
    /*!
     * Constructs an external array
     *
     * \param size The total number of elements. Cannot be changed after
     * construction.
     *
     * \param num_prefetch_blocks Number of blocks to prefetch from hard disk
     *
     * \param num_write_buffer_blocks Size of the write buffer in number of
     * blocks
     */
    external_array(size_t size, size_t num_prefetch_blocks,
                   size_t num_write_buffer_blocks)
        : m_capacity(size),
          m_size(0),
          m_num_bids((size_t)div_ceil(m_capacity, num_elements_per_block)),
          m_num_prefetch_blocks(std::min(num_prefetch_blocks, m_num_bids)),
          m_num_write_buffer_blocks(std::min(num_write_buffer_blocks, m_num_bids)),
          m_bids(m_num_bids),
          m_first_block(new block_type),
          m_pool(new pool_type(0, m_num_write_buffer_blocks)),
          m_write_position(0),
          m_hint_index(0),
          m_current_bid_index(0),
          m_write_phase(true),
          m_first_block_valid(false),
          m_is_first_block(true),
          m_begin_index(0),
          m_end_index(0)
    {
        assert(size > 0);

        // allocate blocks in EM.
        block_manager* bm = block_manager::get_instance();
        bm->new_blocks(m_alloc_strategy, m_bids.begin(), m_bids.end());
    }

    //! Delete copy assignment for emplace_back to use the move semantics.
    self_type& operator = (const self_type& other) = delete;

    //! Delete copy constructor for emplace_back to use the move semantics.
    external_array(const self_type& other) = delete;

    //! Move assignment.
    self_type& operator = (self_type&& o)
    {
        m_capacity = o.m_capacity;
        m_size = o.m_size;
        m_num_bids = o.m_num_bids;
        m_num_prefetch_blocks = o.m_num_prefetch_blocks;
        m_num_write_buffer_blocks = o.m_num_write_buffer_blocks;

        m_bids = std::move(o.m_bids);

        m_first_block = o.m_first_block;
        m_pool = o.m_pool;
        m_write_position = o.m_write_position;
        m_hint_index = o.m_hint_index;
        m_current_bid_index = o.m_current_bid_index;
        m_write_phase = o.m_write_phase;
        m_first_block_valid = o.m_first_block_valid;
        m_is_first_block = o.m_is_first_block;
        m_begin_index = o.m_begin_index;
        m_end_index = o.m_end_index;

        o.m_first_block = nullptr;
        o.m_pool = nullptr;
        return *this;
    }

    //! Move constructor. Needed for regrowing in surrounding vector.
    external_array(self_type&& o)
        : m_capacity(o.m_capacity),
          m_size(o.m_size),
          m_num_bids(o.m_num_bids),
          m_num_prefetch_blocks(o.m_num_prefetch_blocks),
          m_num_write_buffer_blocks(o.m_num_write_buffer_blocks),

          m_bids(std::move(o.m_bids)),

          m_first_block(o.m_first_block),
          m_pool(o.m_pool),
          m_write_position(o.m_write_position),
          m_hint_index(o.m_hint_index),
          m_current_bid_index(o.m_current_bid_index),
          m_write_phase(o.m_write_phase),
          m_first_block_valid(o.m_first_block_valid),
          m_is_first_block(o.m_is_first_block),
          m_begin_index(o.m_begin_index),
          m_end_index(o.m_end_index)
    {
        o.m_first_block = nullptr;
        o.m_pool = nullptr;
    }

    //! Default constructor. Don't use this directy. Needed for regrowing in
    //! surrounding vector.
    external_array() = default;

    //! Destructor
    ~external_array()
    {
        block_manager* bm = block_manager::get_instance();
        if (m_pool != NULL) {
            // This could also be done step by step in remove_first_...() in
            // future.
            bm->delete_blocks(m_bids.begin(), m_bids.end());
            delete m_first_block;
            delete m_pool;
        }   // else: the array has been moved.
    }

    //! Returns the current size
    size_t size() const
    {
        return m_size;
    }

    //! Returns if the array is empty
    bool empty() const
    {
        return (m_size == 0);
    }

    //! Returns if one can safely access elements from the first block.
    bool first_block_valid() const
    {
        return m_first_block_valid;
    }

    //! Returns the smallest element in the array
    const ValueType & get_min_element() const
    {
        assert(m_first_block_valid);
        assert(m_end_index - 1 >= m_begin_index);
        return *begin_block();
    }

    //! Returns the largest element in the first block / internal memory
    const ValueType & get_current_max_element() const
    {
        assert(m_first_block_valid);
        assert(m_end_index - 1 >= m_begin_index);
        block_iterator i = end_block();
        return *(--i);
    }

    //! Begin iterator of the first block
    iterator begin_block() const
    {
        assert(m_first_block_valid);
        return m_first_block->begin() + m_begin_index;
    }

    //! End iterator of the first block
    iterator end_block() const
    {
        assert(m_first_block_valid);
        return m_first_block->begin() + m_end_index;
    }

    //! Random access operator for the first block
    const ValueType& operator [] (size_t i) const
    {
        assert(i >= m_begin_index);
        assert(i < m_end_index);
        return m_first_block->elem[i];
    }

    //! Pushes <record> at the end of the array
    void operator << (const ValueType& record)
    {
        assert(m_write_phase);
        assert(m_size < m_capacity);
        ++m_size;

        if (UNLIKELY(m_write_position >= block_type::size)) {
            m_write_position = 0;
            write_out();
        }

        m_first_block->elem[m_write_position++] = record;
    }

    //! Finish write phase. Afterwards the values can be extracted from bottom
    //! up (ascending order).
    void finish_write_phase()
    {
        // Idea for the future: If we write the block with the smallest values
        // in the end, we don't need to write it into EM.

        assert(m_capacity == m_size);

        if (m_write_position > 0) {
            // This works only if the <<-operator has been used.
            write_out();
        }

        m_pool->resize_write(0);
        m_pool->resize_prefetch(m_num_prefetch_blocks);
        m_write_phase = false;

        m_current_bid_index = 0;
        m_begin_index = 0;
        m_end_index = m_write_position;

        for (size_t i = 0; i < m_num_prefetch_blocks; ++i)
            hint();

        load_next_block();
    }

    //! Removes the first block and loads the next one if there's any
    void remove_first_block()
    {
        assert(m_first_block_valid);

        if (has_next_block()) {
            m_size -= num_elements_per_block;
            load_next_block();
        }
        else {
            m_size = 0;
            m_begin_index = m_end_index = 0;
            m_first_block_valid = true;
        }
    }

    //! Removes the first <n> elements from the array (first block).  Loads the
    //! next block if the current one has run empty.  Make shure there are at
    //! least <n> elements left in the first block.
    void remove_first_n_elements(size_t n)
    {
        assert(m_first_block_valid);

        if (m_begin_index + n < m_end_index) {
            m_size -= n;
            m_begin_index += n;
        }
        else {
            assert(m_begin_index + n == m_end_index);

            if (has_next_block()) {
                assert(m_size >= n);
                m_size -= n;
                load_next_block();
            }
            else {
                m_size -= n;
                assert(m_size == 0);
                m_begin_index = 0;
                m_end_index = 0;
                m_first_block_valid = true;
            }
        }
    }

    //! Has to be called before using begin_block() and end_block() in order to
    //! set _first_block_valid.
    void wait_for_first_block()
    {
        assert(!m_write_phase);

        if (m_first_block_valid)
            return;

        if (m_read_request)
            m_read_request->wait();

        m_first_block_valid = true;
    }

protected:
    //! Write the current block into external memory
    void write_out()
    {
        assert(m_write_phase);
        m_pool->write(m_first_block, m_bids[m_current_bid_index++]);
        m_first_block = m_pool->steal();
    }

    //! Gives the prefetcher a hint for the next block to prefetch
    void hint()
    {
        if (m_hint_index < m_num_bids) {
            m_pool->hint(m_bids[m_hint_index]);
        }
        ++m_hint_index;
    }

    //! Fetches the next block from the hard disk and calls hint() afterwards.
    void load_next_block()
    {
        m_first_block_valid = false;

        if (!m_is_first_block) {
            ++m_current_bid_index;
        }
        else {
            m_is_first_block = false;
        }

        m_read_request = m_pool->read(m_first_block, m_bids[m_current_bid_index]);
        hint();

        m_begin_index = 0;
        if (is_last_block()) {
            m_end_index = m_size % num_elements_per_block;
            m_end_index = (m_end_index == 0 ? num_elements_per_block : m_end_index);
        }
        else {
            m_end_index = num_elements_per_block;
        }
    }

    //! Returns if there is a next block on hard disk.
    bool has_next_block() const
    {
        return ((m_is_first_block && m_num_bids > 0) ||
                (m_current_bid_index + 1 < m_num_bids));
    }

    //! Returns if the current block is the last one.
    bool is_last_block() const
    {
        return ((m_current_bid_index == m_num_bids - 1) ||
                (m_capacity < num_elements_per_block));
    }
};

template <class Parent>
class minima_tree
{
    typedef minima_tree<Parent> self_type;
    typedef typename Parent::inv_compare_type compare_type;
    typedef typename Parent::value_type value_type;
    typedef typename Parent::heaps_type heaps_type;
    typedef typename Parent::internal_arrays_type ias_type;
    typedef typename Parent::external_arrays_type eas_type;

    static const unsigned initial_ia_size = 2;
    static const unsigned initial_ea_size = 2;

public:
    enum Types {
        HEAP = 0,
        EB = 1,
        IA = 2,
        EA = 3,
        ERROR = 4
    };

    minima_tree(Parent& parent)
        : m_parent(parent),
          m_compare(),

          m_cache_line_factor(m_parent.c_cache_line_factor),

          m_head_comp(*this, parent.insertion_heaps, parent.internal_arrays, parent.external_arrays, m_compare, m_cache_line_factor),
          m_heaps_comp(parent.insertion_heaps, m_compare, m_cache_line_factor),
          m_ia_comp(parent.internal_arrays, m_compare),
          m_ea_comp(parent.external_arrays, m_compare),

          m_head(4, m_head_comp),
          m_heaps(m_parent.num_insertion_heaps, m_heaps_comp),
          m_ia(initial_ia_size, m_ia_comp),
          m_ea(initial_ea_size, m_ea_comp) { }

    std::pair<unsigned, unsigned> top()
    {
        unsigned type = m_head.top();
        switch (type)
        {
        case HEAP: return std::make_pair(HEAP, m_heaps.top());
            break;
        case EB: return std::make_pair(EB, 0);
            break;
        case IA: return std::make_pair(IA, m_ia.top());
            break;
        case EA: return std::make_pair(EA, m_ea.top());
            break;
        default: return std::make_pair(ERROR, 0);
        }
    }

    void update_heap(unsigned index)
    {
        m_heaps.notify_change(index);
        m_head.notify_change(HEAP);
    }

    void update_extract_buffer()
    {
        m_head.notify_change(EB);
    }

    void update_internal_array(unsigned index)
    {
        m_ia.notify_change(index);
        m_head.notify_change(IA);
    }

    void update_external_array(unsigned index)
    {
        m_ea.notify_change(index);
        m_head.notify_change(EA);
    }

    void add_internal_array(unsigned index)
    {
        m_ia.activate_player(index);
        m_head.notify_change(IA);
    }

    void add_external_array(unsigned index)
    {
        m_ea.activate_player(index);
        m_head.notify_change(EA);
    }

    void deactivate_heap(unsigned index)
    {
        m_heaps.deactivate_player(index);
        if (!m_heaps.empty()) {
            m_head.notify_change(HEAP);
        }
        else {
            m_head.deactivate_player(HEAP);
        }
    }

    void deactivate_extract_buffer()
    {
        m_head.deactivate_player(EB);
    }

    void deactivate_internal_array(unsigned index)
    {
        m_ia.deactivate_player(index);
        if (!m_ia.empty()) {
            m_head.notify_change(IA);
        }
        else {
            m_head.deactivate_player(IA);
        }
    }

    void deactivate_external_array(unsigned index)
    {
        m_ea.deactivate_player(index);
        if (!m_ea.empty()) {
            m_head.notify_change(EA);
        }
        else {
            m_head.deactivate_player(EA);
        }
    }

    void clear_heaps()
    {
        m_heaps.clear();
        m_head.deactivate_player(HEAP);
    }

    void clear_internal_arrays()
    {
        m_ia.resize_and_clear(initial_ia_size);
        m_head.deactivate_player(IA);
    }

    void clear_external_arrays()
    {
        m_ea.resize_and_clear(initial_ea_size);
        m_head.deactivate_player(EA);
    }

    //! Returns a readable representation of the winner tree as string.
    std::string to_string() const
    {
        std::stringstream ss;
        ss << "Head:" << std::endl << m_head.to_string() << std::endl;
        ss << "Heaps:" << std::endl << m_heaps.to_string() << std::endl;
        ss << "IA:" << std::endl << m_ia.to_string() << std::endl;
        ss << "EA:" << std::endl << m_ea.to_string() << std::endl;
        return ss.str();
    }

    //! Prints statistical data.
    void print_stats() const
    {
        STXXL_MSG("Head winner tree stats:");
        m_head.print_stats();
        STXXL_MSG("Heaps winner tree stats:");
        m_heaps.print_stats();
        STXXL_MSG("IA winner tree stats:");
        m_ia.print_stats();
        STXXL_MSG("EA winner tree stats:");
        m_ea.print_stats();
    }

protected:
    //! Comparator for the head winner tree.  It accesses all relevant data
    //! structures from the priority queue.
    struct head_comp {
        self_type& m_parent;
        heaps_type& m_heaps;
        ias_type& m_ias;
        eas_type& m_eas;
        compare_type& m_compare;
        unsigned m_cache_line_factor;

        head_comp(self_type& parent, heaps_type& heaps,
                  ias_type& ias, eas_type& eas,
                  compare_type& compare, unsigned cache_line_factor)
            : m_parent(parent),
              m_heaps(heaps),
              m_ias(ias),
              m_eas(eas),
              m_compare(compare),
              m_cache_line_factor(cache_line_factor) { }

        bool operator () (const int a, const int b) const
        {
            value_type val_a;
            switch (a) {
            case HEAP: val_a = m_heaps[m_parent.m_heaps.top() * m_cache_line_factor][0];
                break;
            case EB: val_a = m_parent.m_parent.extract_buffer[m_parent.m_parent.extract_index];
                break;
            case IA: val_a = m_ias[m_parent.m_ia.top()].get_min();
                break;
            case EA: val_a = m_eas[m_parent.m_ea.top()].get_min_element();
                break;
            }
            value_type val_b;
            switch (b) {
            case HEAP: val_b = m_heaps[m_parent.m_heaps.top() * m_cache_line_factor][0];
                break;
            case EB: val_b = m_parent.m_parent.extract_buffer[m_parent.m_parent.extract_index];
                break;
            case IA: val_b = m_ias[m_parent.m_ia.top()].get_min();
                break;
            case EA: val_b = m_eas[m_parent.m_ea.top()].get_min_element();
                break;
            }
            return m_compare(val_a, val_b);
        }
    };

    //! Comparator for the insertion heaps winner tree.
    struct heaps_comp {
        heaps_type& m_heaps;
        compare_type& m_compare;
        unsigned m_cache_line_factor;
        heaps_comp(heaps_type& heaps, compare_type& compare, unsigned cache_line_factor)
            : m_heaps(heaps), m_compare(compare), m_cache_line_factor(cache_line_factor) { }
        bool operator () (const int a, const int b) const
        {
            return m_compare(m_heaps[a * m_cache_line_factor][0], m_heaps[b * m_cache_line_factor][0]);
        }
    };

    //! Comparator for the internal arrays winner tree.
    struct ia_comp {
        ias_type& m_ias;
        compare_type& m_compare;
        ia_comp(ias_type& ias, compare_type& compare)
            : m_ias(ias), m_compare(compare) { }
        bool operator () (const int a, const int b) const
        {
            return m_compare(m_ias[a].get_min(), m_ias[b].get_min());
        }
    };

    //! Comparator for the external arrays winner tree.
    struct ea_comp {
        eas_type& m_eas;
        compare_type& m_compare;
        ea_comp(eas_type& eas, compare_type& compare)
            : m_eas(eas), m_compare(compare) { }
        bool operator () (const int a, const int b) const
        {
            return m_compare(m_eas[a].get_min_element(), m_eas[b].get_min_element());
        }
    };

    //! The priority queue
    Parent& m_parent;

    //! value_type comparator
    compare_type m_compare;

    unsigned m_cache_line_factor;

    //! Comperator instances
    head_comp m_head_comp;
    heaps_comp m_heaps_comp;
    ia_comp m_ia_comp;
    ea_comp m_ea_comp;

    //! The winner trees
    winner_tree<head_comp> m_head;
    winner_tree<heaps_comp> m_heaps;
    winner_tree<ia_comp> m_ia;
    winner_tree<ea_comp> m_ea;
};

} // namespace ppq_local

/*!
 * Parallelized External Memory Priority Queue Config.
 *
 * \tparam ValueType Type of the contained objects (POD with no references to
 * internal memory).
 *
 * \tparam CompareType The comparator type used to determine whether one
 * element is smaller than another element.
 *
 * \tparam Ram Maximum memory consumption by the queue. Can be overwritten by
 * the constructor. Default = 8 GiB.
 *
 * \tparam MaxItems Maximum number of elements the queue contains at one
 * time. Default = 0 = unlimited. This is no hard limit and only used for
 * optimization. Can be overwritten by the constructor.
 *
 * \tparam BlockSize External block size. Default =
 * STXXL_DEFAULT_BLOCK_SIZE(ValueType).
 *
 * \tparam AllocStrategy Allocation strategy for the external memory. Default =
 * STXXL_DEFAULT_ALLOC_STRATEGY.
 */
template <
    class ValueType,
    class CompareType,
    class AllocStrategy = STXXL_DEFAULT_ALLOC_STRATEGY,
    stxxl::uint64 BlockSize = STXXL_DEFAULT_BLOCK_SIZE(ValueType),
    stxxl::uint64 Ram = 8* 1024L* 1024L* 1024L,
    stxxl::uint64 MaxItems = 0
    >
class parallel_priority_queue : private noncopyable
{
    //! \addtogroup types Types
    //! \{

public:
    typedef ValueType value_type;
    typedef CompareType compare_type;
    typedef stxxl::uint64 size_type;

protected:
    typedef typed_block<BlockSize, ValueType> block_type;
    typedef std::vector<BID<BlockSize> > bid_vector;
    typedef bid_vector bids_container_type;
    typedef ppq_local::external_array<ValueType, BlockSize, AllocStrategy> external_array_type;
    typedef typename std::vector<ValueType>::iterator extract_iterator;
    typedef typename std::vector<ValueType>::iterator value_iterator;
    typedef typename block_type::iterator block_iterator;
    typedef std::vector<ValueType> heap_type;

    struct inv_compare_type {
        CompareType compare;
        bool operator () (const value_type& x, const value_type& y) const
        {
            return compare(y, x);
        }
    };

    //! Defines if statistics are gathered: dummy_custom_stats_counter or
    //! custom_stats_counter
    typedef dummy_custom_stats_counter<uint64> stats_counter;

    //! Defines if statistics are gathered: fake_timer or timer
    typedef fake_timer stats_timer;

    //! \}

    //! \addtogroup ctparameters Compile-Time Parameters
    //! \{

    //! Merge sorted heaps when flushing into an internal array.
    //! Pro: Reduces the risk of a large winner tree
    //! Con: Flush insertion heaps becomes slower.
    static const bool c_merge_sorted_heaps = true;

    //! Also merge internal arrays into the extract buffer
    static const bool c_merge_ias_into_eb = true;

    //! Default number of prefetch blocks per external array.
    static const unsigned c_num_prefetch_buffer_blocks = 1;

    //! Default number of write buffer block for a new external array being
    //! filled.
    static const unsigned c_num_write_buffer_blocks = 14;

    //! Defines for how much external arrays memory should be reserved in the
    //! constructor.
    static const unsigned c_num_reserved_external_arrays = 10;

    //! Size of a single insertion heap in Byte, if not defined otherwise in
    //! the constructor
    static const size_type c_default_single_heap_ram = 1L * 1024L * 1024L; // 10 MiB

    //! Default limit of the extract buffer ram consumption as share of total ram
    constexpr static double c_default_extract_buffer_ram_part = 0.05;

    //! Leave out (c_cache_line_factor-1) slots between the heaps in the
    //! insertion_heaps vector each.  This ensures that the heap vectors
    //! (meaning: the begin and end pointers) are in different cache lines and
    //! therefore this improves the multicore performance.
    static const unsigned c_cache_line_factor = 128;

    //! Number of elements to reserve space for in the dummy insertion heap
    //! entries.  @see c_cache_line_factor
    static const unsigned c_cache_line_space = 128;

    //! \}

    typedef typename std::vector<heap_type> heaps_type;
    typedef typename std::vector<external_array_type> external_arrays_type;
    typedef typename std::vector<ppq_local::internal_array<ValueType> > internal_arrays_type;
    typedef ppq_local::minima_tree<parallel_priority_queue<ValueType, CompareType, AllocStrategy, BlockSize, Ram, MaxItems> > minima_type;
    friend class ppq_local::minima_tree<parallel_priority_queue<ValueType, CompareType, AllocStrategy, BlockSize, Ram, MaxItems> >;

    /*!
     * Limit the size of the extract buffer to an absolute value.
     *
     * The actual size can be set using the _extract_buffer_ram parameter of
     * the constructor. If this parameter is not set, the value is calculated
     * by (total_ram*c_default_extract_buffer_ram_part)
     *
     * If c_limit_extract_buffer==false, the memory consumption of the extract
     * buffer is only limited by the number of external and internal
     * arrays. This is considered in memory management using the
     * ram_per_external_array and ram_per_internal_array values. Attention:
     * Each internal array reserves space for the extract buffer in the size of
     * all heaps together.
     */
    static const bool c_limit_extract_buffer = true;

    //! For bulks of size up to c_single_insert_limit sequential single insert
    //! is faster than bulk_push.
    static const unsigned c_single_insert_limit = 100;

    //! Number of prefetch blocks per external array
    unsigned num_prefetchers;

    //! Number of write buffer blocks for a new external array being filled
    unsigned num_write_buffers;

    //! The size of the bulk currently being inserted
    size_type bulk_size;

    //! If the bulk currently being insered is very lare, this boolean is set
    //! and bulk_push_steps just accumulate the elements for eventual sorting.
    bool is_very_large_bulk;

    //! Index of the currently smallest element in the extract buffer
    size_type extract_index;

    //! Number of elements in the external arrays
    size_type external_size;

    //! Number of elements in the internal arrays
    size_type internal_size;

    //! Number of elements int the insertion heaps
    size_type insertion_size;

    //! Number of elements in the extract buffer
    size_type buffered_size;

    //! Number of insertion heaps. Usually equal to the number of CPUs.
    unsigned num_insertion_heaps;

    //! Capacity of one inserion heap
    size_type insertion_heap_capacity;

    //! Using this parameter you can reserve more space for the insertion heaps
    //! than visible to the algorithm.  This avoids memory allocation if the
    //! data is not distributed evenly among the heaps.
    double real_insertion_heap_size_factor;

    //! Total amount of internal memory
    size_type total_ram;

    //! Maximum size of extract buffer in number of elements
    //! Only relevant if c_limit_extract_buffer==true
    size_type extract_buffer_limit;

    //! Size of all insertion heaps together in bytes
    size_type ram_for_heaps;

    //! Free memory in bytes
    size_type ram_left;

    //! Amount of internal memory an external array needs during it's lifetime
    //! in bytes
    size_type ram_per_external_array;

    //! Amount of internal memory an internal array needs during it's lifetime
    //! in bytes
    size_type ram_per_internal_array;

    /*
     * Data.
     */

    //! The sorted arrays in external memory
    std::vector<external_array_type> external_arrays;

    //! The sorted arrays in internal memory
    internal_arrays_type internal_arrays;

    //! The buffer where external (and internal) arrays are merged into for
    //! extracting
    std::vector<ValueType> extract_buffer;

    //! The heaps where new elements are usually inserted into
    std::vector<heap_type> insertion_heaps;

    //! The aggregated pushes. They cannot be extracted yet.
    std::vector<ValueType> aggregated_pushes;

    //! <-Comparator for ValueType
    CompareType compare;

    //! >-Comparator for ValueType
    inv_compare_type inv_compare;

    /*
     * Helper class needed to remove empty external arrays.
     */

    //! Unary operator which returns true if the external array has run empty.
    struct empty_array_eraser {
        bool operator () (external_array_type& a) const { return a.empty(); }
    };

    //! Unary operator which returns true if the internal array has run empty.
    struct empty_internal_array_eraser {
        bool operator () (ppq_local::internal_array<ValueType>& a) const { return a.empty(); }
    };

    //! The winner tree containing the smallest values of all sources
    //! where the globally smallest element could come from.
    minima_type m_minima;

    const bool m_do_flush_directly_to_hd;

public:
    //! \addtogroup init Initialization
    //! \{

    /*!
     * Constructor.
     *
     * \param num_prefetch_buffer_blocks Number of prefetch blocks per external
     * array. Default = c_num_prefetch_buffer_blocks
     *
     * \param num_write_buffer_blocks Number of write buffer blocks for a new
     * external array being filled. 0 = Default = c_num_write_buffer_blocks
     *
     * \param _total_ram Maximum RAM usage. 0 = Default = Use the template
     * value Ram.
     *
     * \param _num_insertion_heaps Number of insertion heaps. 0 = Determine by
     * omp_get_max_threads(). Default = Determine by omp_get_max_threads().
     *
     * \param _single_heap_ram Memory usage for a single insertion heap. 0 =
     * Default = c_single_heap_ram.
     *
     * \param extract_buffer_ram Memory usage for the extract buffer. Only
     * relevant if c_limit_extract_buffer==true. 0 = Default = total_ram *
     * c_default_extract_buffer_ram_part.
     *
     * \param flush_directly_to_hd Do not flush into internal arrays when there
     * is RAM left but flush directly into an external array.
     */
    parallel_priority_queue(
        unsigned num_prefetch_buffer_blocks = 0,
        unsigned num_write_buffer_blocks = 0,
        size_type _total_ram = 0,
        unsigned _num_insertion_heaps = 0,
        size_type _single_heap_ram = 0,
        size_type extract_buffer_ram = 0,
        bool flush_directly_to_hd = false)
        : num_prefetchers((num_prefetch_buffer_blocks > 0) ? num_prefetch_buffer_blocks : c_num_prefetch_buffer_blocks),
          num_write_buffers((num_write_buffer_blocks > 0) ? num_write_buffer_blocks : c_num_write_buffer_blocks),
          bulk_size(0),
          is_very_large_bulk(false),
          extract_index(0),
          external_size(0),
          internal_size(0),
          insertion_size(0),
          buffered_size(0),
        #if STXXL_PARALLEL
          num_insertion_heaps((_num_insertion_heaps > 0) ? _num_insertion_heaps : omp_get_max_threads()),
        #else
          num_insertion_heaps((_num_insertion_heaps > 0) ? _num_insertion_heaps : 1),
        #endif
          real_insertion_heap_size_factor(1 + 1 / num_insertion_heaps),
          total_ram((_total_ram > 0) ? _total_ram : Ram),
          ram_for_heaps(num_insertion_heaps * ((_single_heap_ram > 0) ? _single_heap_ram : c_default_single_heap_ram)),
          external_arrays(0),
          internal_arrays(0),
          extract_buffer(0),
          insertion_heaps(num_insertion_heaps * c_cache_line_factor),
          aggregated_pushes(0),
          m_minima(*this),
          m_do_flush_directly_to_hd(flush_directly_to_hd)
    {
        srand(static_cast<unsigned>(time(NULL)));

        if (c_limit_extract_buffer) {
            extract_buffer_limit = (extract_buffer_ram > 0) ? extract_buffer_ram / sizeof(ValueType) : static_cast<size_type>((static_cast<double>(total_ram) * c_default_extract_buffer_ram_part / static_cast<double>(sizeof(ValueType))));
        }

        insertion_heap_capacity = ram_for_heaps / (num_insertion_heaps * sizeof(ValueType));

        for (unsigned i = 0; i < num_insertion_heaps * c_cache_line_factor; ++i) {
            insertion_heaps[i].reserve(c_cache_line_space);
        }

        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            insertion_heaps[i * c_cache_line_factor].reserve(static_cast<size_type>(real_insertion_heap_size_factor * static_cast<double>(insertion_heap_capacity)));
        }

        init_memmanagement();

        external_arrays.reserve(c_num_reserved_external_arrays);

        if (c_merge_sorted_heaps) {
            internal_arrays.reserve(total_ram / ram_for_heaps);
        }
        else {
            internal_arrays.reserve(total_ram * num_insertion_heaps / ram_for_heaps);
        }
    }

    //! Destructor.
    ~parallel_priority_queue() { }

protected:
    //! Initializes member variables concerning the memory management.
    void init_memmanagement()
    {
        // total_ram - ram for the heaps - ram for the heap merger - ram for the external array write buffer -
        ram_left = total_ram - 2 * ram_for_heaps - num_write_buffers * BlockSize;

        // prefetch blocks + first block of array
        ram_per_external_array = (num_prefetchers + 1) * BlockSize;

        if (c_merge_sorted_heaps) {
            // all heaps become one internal array
            ram_per_internal_array = ram_for_heaps;
        }
        else {
            // each heap becomes one internal array
            ram_per_internal_array = ram_for_heaps / num_insertion_heaps;
        }

        if (c_limit_extract_buffer) {
            // ram for the extract buffer
            ram_left -= extract_buffer_limit * sizeof(ValueType);
        }
        else {
            // each: part of the (maximum) ram for the extract buffer

            ram_per_external_array += BlockSize;

            if (c_merge_ias_into_eb) {
                // we have to reserve space in the size of the whole array: very inefficient!
                if (c_merge_sorted_heaps) {
                    ram_per_internal_array += ram_for_heaps;
                }
                else {
                    ram_per_internal_array += ram_for_heaps / num_insertion_heaps;
                }
            }
        }

        if (c_merge_sorted_heaps) {
            // part of the ram for the merge buffer
            ram_left -= ram_for_heaps;
        }

        if (m_do_flush_directly_to_hd) {
            if (ram_left < 2 * ram_per_external_array) {
                STXXL_ERRMSG("Insufficent memory.");
                exit(EXIT_FAILURE);
            }
            else if (ram_left < 4 * ram_per_external_array) {
                STXXL_ERRMSG("Warning: Low memory. Performance could suffer.");
            }
        }
        else {
            if (ram_left < 2 * ram_per_external_array + ram_per_internal_array) {
                STXXL_ERRMSG("Insufficent memory.");
                exit(EXIT_FAILURE);
            }
            else if (ram_left < 4 * ram_per_external_array + 2 * ram_per_internal_array) {
                STXXL_ERRMSG("Warning: Low memory. Performance could suffer.");
            }
        }
    }

    //! \}

    //! \addtogroup properties Properties
    //! \{

public:
    //! The number of elements in the queue.
    inline size_type size() const
    {
        return insertion_size + internal_size + external_size + buffered_size;
    }

    //! Returns if the queue is empty.
    inline bool empty() const
    {
        return (size() == 0);
    }

    //! The memory consumption in Bytes.
    inline size_type memory_consumption() const
    {
        return (total_ram - ram_left);
    }

protected:
    //! Returns if the extract buffer is empty.
    inline bool extract_buffer_empty() const
    {
        return (buffered_size == 0);
    }

    //! \}

public:
    //! \addtogroup bulkops Bulk Operations
    //! \{

    /*!
     * Start a sequence of push operations.
     * \param _bulk_size Number of elements to push before the next pop.
     */
    void bulk_push_begin(size_type _bulk_size)
    {
        bulk_size = _bulk_size;
        size_type heap_capacity = num_insertion_heaps * insertion_heap_capacity;
        if (_bulk_size > heap_capacity) {
            is_very_large_bulk = true;
            return;
        }
        if (_bulk_size + insertion_size > heap_capacity) {
            if (m_do_flush_directly_to_hd) {
                flush_directly_to_hd();
            }
            else {
                flush_insertion_heaps();
            }
        }
    }

    /*!
     * Push an element inside a sequence of pushes.
     * Run bulk_push_begin() before using this method.
     *
     * \param element The element to push.
     */
    void bulk_push_step(const ValueType& element, const int thread_num = -1)
    {
        assert(bulk_size > 0);

        if (is_very_large_bulk) {
            aggregate_push(element);
            return;
        }

        unsigned id;
        if (thread_num > -1) {
            id = thread_num;
        #if STXXL_PARALLEL
        }
        else if (omp_get_num_threads() > 1) {
            id = omp_get_thread_num();
        #endif
        }
        else {
            id = rand() % num_insertion_heaps;
        }

        // TODO: check if full? Alternative: real_insertion_heap_size_factor
        insertion_heaps[id * c_cache_line_factor].push_back(element);
        std::push_heap(insertion_heaps[id * c_cache_line_factor].begin(), insertion_heaps[id * c_cache_line_factor].end(), compare);
        // The following would avoid problems if the bulk size specified in
        // bulk_push_begin is not correct.
        // insertion_size += bulk_size; must then be removed from bulk_push_end().
        //__sync_fetch_and_add(&insertion_size, 1);
    }

    /*!
     * Ends a sequence of push operations. Run bulk_push_begin() and some
     * bulk_push_step() before this.
     */
    void bulk_push_end()
    {
        if (is_very_large_bulk) {
            flush_aggregated_pushes();
            is_very_large_bulk = false;
            return;
        }

        insertion_size += bulk_size;
        bulk_size = 0;
        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            if (!insertion_heaps[i * c_cache_line_factor].empty()) {
                m_minima.update_heap(i);
            }
        }
    }

    /*!
     * Insert multiple elements at one time.
     * \param elements Vector containing the elements to push.
     * Attention: elements vector may be owned by the PQ afterwards.
     */
    void bulk_push(std::vector<ValueType>& elements)
    {
        size_type heap_capacity = num_insertion_heaps * insertion_heap_capacity;
        if (elements.size() > heap_capacity / 2) {
            flush_array(elements);
            return;
        }

        bulk_push_begin(elements.size());
        #if STXXL_PARALLEL
        #pragma omp parallel
        {
            const unsigned thread_num = omp_get_thread_num();
            #pragma omp parallel for
            for (size_type i = 0; i < elements.size(); ++i) {
                bulk_push_step(elements[i], thread_num);
            }
        }
        #else
        const unsigned thread_num = rand() % num_insertion_heaps;
        for (size_type i = 0; i < elements.size(); ++i) {
            bulk_push_step(elements[i], thread_num);
        }
        #endif
        bulk_push_end();
    }

    //! \}

    //! \addtogroup aggrops Aggregation Operations
    //! \{

    /*!
     * Aggregate pushes. Use flush_aggregated_pushes() to finally push
     * them. extract_min is allowed is allowed in between the aggregation of
     * pushes if you can assure, that the extracted value is smaller than all
     * of the aggregated values.
     * \param element The element to push.
     */
    void aggregate_push(const ValueType& element)
    {
        aggregated_pushes.push_back(element);
    }

    /*!
     * Insert the aggregated values into the queue using push(), bulk insert,
     * or sorting, depending on the number of aggregated values.
     * \param element	The element to push.
     * \see  c_single_insert_limit
     */
    void flush_aggregated_pushes()
    {
        size_type size = aggregated_pushes.size();
        size_type ram_internal = 2 * size * sizeof(ValueType); // ram for the sorted array + part of the ram for the merge buffer
        size_type heap_capacity = num_insertion_heaps * insertion_heap_capacity;

        if (ram_internal > ram_for_heaps / 2) {
            flush_array(aggregated_pushes);
        }
        else if ((aggregated_pushes.size() > c_single_insert_limit) && (aggregated_pushes.size() < heap_capacity)) {
            bulk_push(aggregated_pushes);
        }
        else {
            for (value_iterator i = aggregated_pushes.begin(); i != aggregated_pushes.end(); ++i) {
                push(*i);
            }
        }

        aggregated_pushes.clear();
    }

    //! \}

    //! \addtogroup stdops std::priority_queue complient operations
    //! \{

    //! Insert new element
    //! \param element the element to insert.
    void push(const ValueType& element)
    {
        unsigned id = rand() % num_insertion_heaps;

        if (insertion_heaps[id * c_cache_line_factor].size() >= insertion_heap_capacity) {
            if (m_do_flush_directly_to_hd) {
                flush_directly_to_hd();
            }
            else {
                flush_insertion_heaps();
            }
        }

        ValueType old_min;
        if (insertion_heaps[id * c_cache_line_factor].size() > 0) {
            old_min = insertion_heaps[id * c_cache_line_factor][0];
        }

        insertion_heaps[id * c_cache_line_factor].push_back(element);
        std::push_heap(insertion_heaps[id * c_cache_line_factor].begin(), insertion_heaps[id * c_cache_line_factor].end(), compare);
        ++insertion_size;

        if (insertion_heaps[id * c_cache_line_factor].size() == 1 || inv_compare(insertion_heaps[id * c_cache_line_factor][0], old_min)) {
            m_minima.update_heap(id);
        }
    }

    //! Access the minimum element.
    ValueType top()
    {
        if (extract_buffer_empty()) {
            refill_extract_buffer();
        }

        std::pair<unsigned, unsigned> type_and_index = m_minima.top();
        unsigned type = type_and_index.first;
        unsigned index = type_and_index.second;

        assert(type < 4);

        switch (type) {
        case minima_type::HEAP:
            return insertion_heaps[index * c_cache_line_factor][0];
        case minima_type::EB:
            return extract_buffer[extract_index];
        case minima_type::IA:
            return internal_arrays[index].get_min();
        case minima_type::EA:
            // wait_for_first_block() already done by comparator....
            return external_arrays[index].get_min_element();
        default:
            STXXL_ERRMSG("Unknown extract type: " << type);
            abort();
        }
    }

    //! Access and remove the minimum element.
    ValueType pop()
    {
        stats.num_extracts++;

        if (extract_buffer_empty()) {
            refill_extract_buffer();
        }

        stats.extract_min_time.start();

        std::pair<unsigned, unsigned> type_and_index = m_minima.top();
        unsigned type = type_and_index.first;
        unsigned index = type_and_index.second;
        ValueType min;

        assert(type < 4);

        switch (type) {
        case minima_type::HEAP:
        {
            min = insertion_heaps[index * c_cache_line_factor][0];

            stats.pop_heap_time.start();
            std::pop_heap(insertion_heaps[index * c_cache_line_factor].begin(), insertion_heaps[index * c_cache_line_factor].end(), compare);
            insertion_heaps[index * c_cache_line_factor].pop_back();
            stats.pop_heap_time.stop();

            insertion_size--;

            if (!insertion_heaps[index * c_cache_line_factor].empty()) {
                m_minima.update_heap(index);
            }
            else {
                m_minima.deactivate_heap(index);
            }

            break;
        }
        case minima_type::EB:
        {
            min = extract_buffer[extract_index];
            ++extract_index;
            assert(buffered_size > 0);
            --buffered_size;

            if (!extract_buffer_empty()) {
                m_minima.update_extract_buffer();
            }
            else {
                m_minima.deactivate_extract_buffer();
            }

            break;
        }
        case minima_type::IA:
        {
            min = internal_arrays[index].get_min();
            internal_arrays[index].inc_min();
            internal_size--;

            if (!(internal_arrays[index].empty())) {
                m_minima.update_internal_array(index);
            }
            else {
                // internal array has run empty
                m_minima.deactivate_internal_array(index);
                ram_left += ram_per_internal_array;
            }

            break;
        }
        case minima_type::EA:
        {
            // wait_for_first_block() already done by comparator...
            min = external_arrays[index].get_min_element();
            assert(external_size > 0);
            --external_size;
            external_arrays[index].remove_first_n_elements(1);
            external_arrays[index].wait_for_first_block();

            if (!external_arrays[index].empty()) {
                m_minima.update_external_array(index);
            }
            else {
                // external array has run empty
                m_minima.deactivate_external_array(index);
                ram_left += ram_per_external_array;
            }

            break;
        }
        default:
            STXXL_ERRMSG("Unknown extract type: " << type);
            abort();
        }

        stats.extract_min_time.stop();
        return min;
    }

    //! \}

    /*!
     * Merges all external arrays into one external array.  Public for
     * benchmark purposes.
     */
    void merge_external_arrays()
    {
        stats.num_external_array_merges++;
        stats.external_array_merge_time.start();

        m_minima.clear_external_arrays();

        // clean up external arrays that have been deleted in extract_min!
        external_arrays.erase(std::remove_if(external_arrays.begin(), external_arrays.end(), empty_array_eraser()), external_arrays.end());

        size_type total_size = external_size;
        assert(total_size > 0);

        external_arrays.emplace_back(total_size, num_prefetchers, num_write_buffers);
        external_array_type& a = external_arrays[external_arrays.size() - 1];
        std::vector<ValueType> merge_buffer;

        ram_left += (external_arrays.size() - 1) * ram_per_external_array;

        while (external_arrays.size() > 0) {
            size_type eas = external_arrays.size();

            external_arrays[0].wait_for_first_block();
            ValueType min_max_value = external_arrays[0].get_current_max_element();

            for (size_type i = 1; i < eas; ++i) {
                external_arrays[i].wait_for_first_block();
                ValueType max_value = external_arrays[i].get_current_max_element();
                if (inv_compare(max_value, min_max_value)) {
                    min_max_value = max_value;
                }
            }

            std::vector<size_type> sizes(eas);
            std::vector<std::pair<block_iterator, block_iterator> > sequences(eas);
            size_type output_size = 0;

            #if STXXL_PARALLEL
            #pragma omp parallel for if(eas>num_insertion_heaps)
            #endif
            for (size_type i = 0; i < eas; ++i) {
                assert(!external_arrays[i].empty());

                external_arrays[i].wait_for_first_block();
                block_iterator begin = external_arrays[i].begin_block();
                block_iterator end = external_arrays[i].end_block();

                assert(begin != end);

                block_iterator ub = std::upper_bound(begin, end, min_max_value, inv_compare);
                sizes[i] = ub - begin;
                __sync_fetch_and_add(&output_size, ub - begin);
                sequences[i] = std::make_pair(begin, ub);
            }

            merge_buffer.resize(output_size);

            #if STXXL_PARALLEL
            stxxl::parallel::multiway_merge(sequences.begin(), sequences.end(),
                                            merge_buffer.begin(), inv_compare, output_size);
            #else
            // TODO
            #endif

            for (size_type i = 0; i < eas; ++i) {
                external_arrays[i].remove_first_n_elements(sizes[i]);
            }

            for (value_iterator i = merge_buffer.begin(); i != merge_buffer.end(); ++i) {
                a << *i;
            }

            for (size_type i = 0; i < eas; ++i) {
                external_arrays[i].wait_for_first_block();
            }

            external_arrays.erase(std::remove_if(external_arrays.begin(), external_arrays.end(), empty_array_eraser()), external_arrays.end());
        }

        a.finish_write_phase();

        if (!extract_buffer_empty()) {
            stats.num_new_external_arrays++;
            stats.max_num_new_external_arrays.set_max(stats.num_new_external_arrays);
            a.wait_for_first_block();
            m_minima.add_external_array(static_cast<unsigned>(external_arrays.size()) - 1);
        }

        stats.external_array_merge_time.stop();
    }

    //! Print statistics.
    void print_stats() const
    {
        STXXL_VARDUMP(c_merge_sorted_heaps);
        STXXL_VARDUMP(c_merge_ias_into_eb);
        STXXL_VARDUMP(c_limit_extract_buffer);
        STXXL_VARDUMP(c_single_insert_limit);

        if (c_limit_extract_buffer) {
            STXXL_VARDUMP(extract_buffer_limit);
            STXXL_MEMDUMP(extract_buffer_limit * sizeof(ValueType));
        }

        STXXL_VARDUMP(m_do_flush_directly_to_hd);
        #if STXXL_PARALLEL
        STXXL_VARDUMP(omp_get_max_threads());
        #endif

        STXXL_MEMDUMP(ram_for_heaps);
        STXXL_MEMDUMP(ram_left);
        STXXL_MEMDUMP(ram_per_external_array);
        STXXL_MEMDUMP(ram_per_internal_array);

        //if (num_extract_buffer_refills > 0) {
        //    STXXL_VARDUMP(total_extract_buffer_size / num_extract_buffer_refills);
        //    STXXL_MEMDUMP(total_extract_buffer_size / num_extract_buffer_refills * sizeof(ValueType));
        //}

        STXXL_MSG(stats);
        m_minima.print_stats();
    }

protected:
    //! Refills the extract buffer from the external arrays.
    inline void refill_extract_buffer()
    {
        assert(extract_buffer_empty());
        assert(buffered_size == 0);
        extract_index = 0;

        m_minima.clear_external_arrays();
        external_arrays.erase(std::remove_if(external_arrays.begin(), external_arrays.end(), empty_array_eraser()), external_arrays.end());
        size_type eas = external_arrays.size();

        size_type ias;

        if (c_merge_ias_into_eb) {
            m_minima.clear_internal_arrays();
            internal_arrays.erase(std::remove_if(internal_arrays.begin(), internal_arrays.end(), empty_internal_array_eraser()), internal_arrays.end());
            ias = internal_arrays.size();
        }
        else {
            ias = 0;
        }

        if (eas == 0 && ias == 0) {
            extract_buffer.resize(0);
            m_minima.deactivate_extract_buffer();
            return;
        }

        stats.num_extract_buffer_refills++;
        stats.refill_extract_buffer_time.start();
        stats.refill_time_before_merge.start();
        stats.refill_minmax_time.start();

        /*
         * determine maximum of each first block
         */

        ValueType min_max_value;

        // check only relevant if c_merge_ias_into_eb==true
        if (eas > 0) {
            stats.refill_wait_time.start();
            external_arrays[0].wait_for_first_block();
            stats.refill_wait_time.stop();
            assert(external_arrays[0].size() > 0);
            min_max_value = external_arrays[0].get_current_max_element();
        }

        for (size_type i = 1; i < eas; ++i) {
            stats.refill_wait_time.start();
            external_arrays[i].wait_for_first_block();
            stats.refill_wait_time.stop();

            ValueType max_value = external_arrays[i].get_current_max_element();
            if (inv_compare(max_value, min_max_value)) {
                min_max_value = max_value;
            }
        }

        stats.refill_minmax_time.stop();

        // the number of elements in each external array that are smaller than min_max_value or equal
        // plus the number of elements in the internal arrays
        std::vector<size_type> sizes(eas + ias);
        std::vector<std::pair<ValueType*, ValueType*> > sequences(eas + ias);

        /*
         * calculate size and create sequences to merge
         */

        #if STXXL_PARALLEL
        #pragma omp parallel for if(eas+ias>num_insertion_heaps)
        #endif
        for (size_type i = 0; i < eas + ias; ++i) {
            // check only relevant if c_merge_ias_into_eb==true
            if (i < eas) {
                assert(!external_arrays[i].empty());

                assert(external_arrays[i].first_block_valid());
                ValueType* begin = external_arrays[i].begin_block();
                ValueType* end = external_arrays[i].end_block();

                assert(begin != end);

                // remove if parallel
                //stats.refill_upper_bound_time.start();
                ValueType* ub = std::upper_bound(begin, end, min_max_value, inv_compare);
                //stats.refill_upper_bound_time.stop();

                sizes[i] = std::distance(begin, ub);
                sequences[i] = std::make_pair(begin, ub);
            }
            else {
                // else part only relevant if c_merge_ias_into_eb==true

                size_type j = i - eas;
                assert(!(internal_arrays[j].empty()));

                ValueType* begin = internal_arrays[j].begin();
                ValueType* end = internal_arrays[j].end();
                assert(begin != end);

                if (eas > 0) {
                    //remove if parallel
                    //stats.refill_upper_bound_time.start();
                    ValueType* ub = std::upper_bound(begin, end, min_max_value, inv_compare);
                    //stats.refill_upper_bound_time.stop();

                    sizes[i] = std::distance(begin, ub);
                    sequences[i] = std::make_pair(begin, ub);
                }
                else {
                    //there is no min_max_value
                    sizes[i] = std::distance(begin, end);
                    sequences[i] = std::make_pair(begin, end);
                }

                if (!c_limit_extract_buffer) { // otherwise see below...
                    // remove elements
                    internal_arrays[j].inc_min(sizes[i]);
                }
            }
        }

        size_type output_size = std::accumulate(sizes.begin(), sizes.end(), 0);

        if (c_limit_extract_buffer) {
            if (output_size > extract_buffer_limit) {
                output_size = extract_buffer_limit;
            }
        }

        stats.max_extract_buffer_size.set_max(output_size);
        stats.total_extract_buffer_size += output_size;

        assert(output_size > 0);
        extract_buffer.resize(output_size);
        buffered_size = output_size;

        stats.refill_time_before_merge.stop();
        stats.refill_merge_time.start();

        #if STXXL_PARALLEL
        stxxl::parallel::multiway_merge(sequences.begin(), sequences.end(),
                                        extract_buffer.begin(), inv_compare, output_size);
        #else
        // TODO
        #endif

        stats.refill_merge_time.stop();
        stats.refill_time_after_merge.start();

        // remove elements
        if (c_limit_extract_buffer) {
            for (size_type i = 0; i < eas + ias; ++i) {
                // dist represents the number of elements that haven't been merged
                size_type dist = std::distance(sequences[i].first, sequences[i].second);
                if (i < eas) {
                    external_arrays[i].remove_first_n_elements(sizes[i] - dist);
                    assert(external_size >= sizes[i] - dist);
                    external_size -= sizes[i] - dist;
                }
                else {
                    size_type j = i - eas;
                    internal_arrays[j].inc_min(sizes[i] - dist);
                    assert(internal_size >= sizes[i] - dist);
                    internal_size -= sizes[i] - dist;
                }
            }
        }
        else {
            for (size_type i = 0; i < eas; ++i) {
                external_arrays[i].remove_first_n_elements(sizes[i]);
                assert(external_size >= sizes[i]);
                external_size -= sizes[i];
            }
        }

        //stats.refill_wait_time.start();
        for (size_type i = 0; i < eas; ++i) {
            external_arrays[i].wait_for_first_block();
        }
        //stats.refill_wait_time.stop();

        // remove empty arrays - important for the next round
        external_arrays.erase(std::remove_if(external_arrays.begin(), external_arrays.end(), empty_array_eraser()), external_arrays.end());
        size_type num_deleted_arrays = eas - external_arrays.size();
        if (num_deleted_arrays > 0) {
            ram_left += num_deleted_arrays * ram_per_external_array;
        }

        stats.num_new_external_arrays = 0;

        if (c_merge_ias_into_eb) {
            internal_arrays.erase(std::remove_if(internal_arrays.begin(), internal_arrays.end(), empty_internal_array_eraser()), internal_arrays.end());
            size_type num_deleted_internal_arrays = ias - internal_arrays.size();
            if (num_deleted_internal_arrays > 0) {
                ram_left += num_deleted_internal_arrays * ram_per_internal_array;
            }
            stats.num_new_internal_arrays = 0;
        }

        m_minima.update_extract_buffer();

        stats.refill_time_after_merge.stop();
        stats.refill_extract_buffer_time.stop();
    }

    //! Flushes the insertions heaps into an internal array.
    inline void flush_insertion_heaps()
    {
        size_type ram_needed;

        if (c_merge_sorted_heaps) {
            ram_needed = ram_per_internal_array;
        }
        else {
            ram_needed = num_insertion_heaps * ram_per_internal_array;
        }

        if (ram_left < ram_needed) {
            if (internal_size > 0) {
                flush_internal_arrays();
                // still not enough?
                if (ram_left < ram_needed) {
                    merge_external_arrays();
                }
            }
            else {
                merge_external_arrays();
            }
        }

        stats.num_insertion_heap_flushes++;
        stats.insertion_heap_flush_time.start();

        size_type size = insertion_size;
        assert(size > 0);
        std::vector<std::pair<value_iterator, value_iterator> > sequences(num_insertion_heaps);

        #if STXXL_PARALLEL
        #pragma omp parallel for
        #endif
        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            // TODO: Use std::sort_heap instead? We would have to reverse the order...
            std::sort(insertion_heaps[i * c_cache_line_factor].begin(), insertion_heaps[i * c_cache_line_factor].end(), inv_compare);
            if (c_merge_sorted_heaps) {
                sequences[i] = std::make_pair(insertion_heaps[i * c_cache_line_factor].begin(), insertion_heaps[i * c_cache_line_factor].end());
            }
        }

        if (c_merge_sorted_heaps) {
            stats.merge_sorted_heaps_time.start();
            std::vector<ValueType> merged_array(size);

            #if STXXL_PARALLEL
            parallel::multiway_merge(sequences.begin(), sequences.end(),
                                     merged_array.begin(), inv_compare, size);
            #else
            // TODO
            #endif

            stats.merge_sorted_heaps_time.stop();

            internal_arrays.emplace_back(merged_array);
            // internal array owns merged_array now.

            if (c_merge_ias_into_eb) {
                if (!extract_buffer_empty()) {
                    stats.num_new_internal_arrays++;
                    stats.max_num_new_internal_arrays.set_max(stats.num_new_internal_arrays);
                    m_minima.add_internal_array(static_cast<unsigned>(internal_arrays.size()) - 1);
                }
            }
            else {
                m_minima.add_internal_array(static_cast<unsigned>(internal_arrays.size()) - 1);
            }

            for (unsigned i = 0; i < num_insertion_heaps; ++i) {
                insertion_heaps[i * c_cache_line_factor].clear();
                insertion_heaps[i * c_cache_line_factor].reserve(static_cast<size_type>(real_insertion_heap_size_factor * static_cast<double>(insertion_heap_capacity)));
            }
            m_minima.clear_heaps();

            ram_left -= ram_per_internal_array;
        }
        else {
            for (unsigned i = 0; i < num_insertion_heaps; ++i) {
                if (insertion_heaps[i * c_cache_line_factor].size() > 0) {
                    internal_arrays.emplace_back(insertion_heaps[i * c_cache_line_factor]);
                    // insertion_heaps[i*c_cache_line_factor] is empty now.

                    if (c_merge_ias_into_eb) {
                        if (!extract_buffer_empty()) {
                            stats.num_new_internal_arrays++;
                            stats.max_num_new_internal_arrays.set_max(stats.num_new_internal_arrays);
                            m_minima.add_internal_array(static_cast<unsigned>(internal_arrays.size()) - 1);
                        }
                    }
                    else {
                        m_minima.add_internal_array(static_cast<unsigned>(internal_arrays.size()) - 1);
                    }

                    insertion_heaps[i * c_cache_line_factor].reserve(static_cast<size_type>(real_insertion_heap_size_factor * static_cast<double>(insertion_heap_capacity)));
                }
            }

            m_minima.clear_heaps();
            ram_left -= num_insertion_heaps * ram_per_internal_array;
        }

        internal_size += size;
        insertion_size = 0;

        stats.max_num_internal_arrays.set_max(internal_arrays.size());
        stats.insertion_heap_flush_time.stop();

        if (ram_left < ram_per_external_array + ram_per_internal_array) {
            flush_internal_arrays();
        }
    }

    //! Flushes the internal arrays into an external array.
    inline void flush_internal_arrays()
    {
        stats.num_internal_array_flushes++;
        stats.internal_array_flush_time.start();

        m_minima.clear_internal_arrays();

        // clean up internal arrays that have been deleted in extract_min!
        internal_arrays.erase(std::remove_if(internal_arrays.begin(), internal_arrays.end(), empty_internal_array_eraser()), internal_arrays.end());

        size_type num_arrays = internal_arrays.size();
        size_type size = internal_size;
        std::vector<std::pair<ValueType*, ValueType*> > sequences(num_arrays);

        for (unsigned i = 0; i < num_arrays; ++i) {
            sequences[i] = std::make_pair(internal_arrays[i].begin(), internal_arrays[i].end());
        }

        external_arrays.emplace_back(size, num_prefetchers, num_write_buffers);
        external_array_type& a = external_arrays[external_arrays.size() - 1];

        // TODO: write in chunks in order to safe RAM?

        stats.max_merge_buffer_size.set_max(size);

        std::vector<ValueType> write_buffer(size);

        #if STXXL_PARALLEL
        parallel::multiway_merge(sequences.begin(), sequences.end(),
                                 write_buffer.begin(), inv_compare, size);
        #else
        // TODO
        #endif

        // TODO: directly write to block? -> no useless mem copy. Does not work if size > block size
        for (value_iterator i = write_buffer.begin(); i != write_buffer.end(); ++i) {
            a << *i;
        }

        a.finish_write_phase();

        external_size += size;
        internal_size = 0;

        internal_arrays.clear();
        stats.num_new_internal_arrays = 0;

        if (!extract_buffer_empty()) {
            stats.num_new_external_arrays++;
            stats.max_num_new_external_arrays.set_max(stats.num_new_external_arrays);
            a.wait_for_first_block();
            m_minima.add_external_array(static_cast<unsigned>(external_arrays.size()) - 1);
        }

        ram_left += num_arrays * ram_per_internal_array;
        ram_left -= ram_per_external_array;

        stats.max_num_external_arrays.set_max(external_arrays.size());
        stats.internal_array_flush_time.stop();
    }

    //! Flushes the insertion heaps into an external array.
    void flush_directly_to_hd()
    {
        if (ram_left < ram_per_external_array) {
            merge_external_arrays();
        }

        stats.num_direct_flushes++;
        stats.direct_flush_time.start();

        size_type size = insertion_size;
        std::vector<std::pair<value_iterator, value_iterator> > sequences(num_insertion_heaps);

        #if STXXL_PARALLEL
        #pragma omp parallel for
        #endif
        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            // TODO std::sort_heap? We would have to reverse the order...
            std::sort(insertion_heaps[i * c_cache_line_factor].begin(), insertion_heaps[i * c_cache_line_factor].end(), inv_compare);
            sequences[i] = std::make_pair(insertion_heaps[i * c_cache_line_factor].begin(), insertion_heaps[i * c_cache_line_factor].end());
        }

        external_arrays.emplace_back(size, num_prefetchers, num_write_buffers);
        external_array_type& a = external_arrays[external_arrays.size() - 1];

        // TODO: write in chunks in order to safe RAM
        std::vector<ValueType> write_buffer(size);

        #if STXXL_PARALLEL
        parallel::multiway_merge(sequences.begin(), sequences.end(),
                                 write_buffer.begin(), inv_compare, size);
        #else
        // TODO
        #endif

        // TODO: directly write to block -> no useless mem copy
        for (value_iterator i = write_buffer.begin(); i != write_buffer.end(); ++i) {
            a << *i;
        }

        a.finish_write_phase();

        external_size += size;
        insertion_size = 0;

        // inefficient...
        //#if STXXL_PARALLEL
        //#pragma omp parallel for
        //#endif
        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            insertion_heaps[i * c_cache_line_factor].clear();
            insertion_heaps[i * c_cache_line_factor].reserve(static_cast<size_type>(real_insertion_heap_size_factor * static_cast<double>(insertion_heap_capacity)));
        }
        m_minima.clear_heaps();

        if (!extract_buffer_empty()) {
            stats.num_new_external_arrays++;
            stats.max_num_new_external_arrays.set_max(stats.num_new_external_arrays);
            a.wait_for_first_block();
            m_minima.add_external_array(static_cast<unsigned>(external_arrays.size()) - 1);
        }

        ram_left -= ram_per_external_array;

        stats.max_num_external_arrays.set_max(external_arrays.size());
        stats.direct_flush_time.stop();
    }

    //! Sorts the values from values and writes them into an external array.
    //! \param values the vector to sort and store
    void flush_array_to_hd(std::vector<ValueType>& values)
    {
        #if STXXL_PARALLEL
        __gnu_parallel::sort(values.begin(), values.end(), inv_compare);
        #else
        std::sort(values.begin(), values.end(), inv_compare);
        #endif

        external_arrays.emplace_back(values.size(), num_prefetchers, num_write_buffers);
        external_array_type& a = external_arrays[external_arrays.size() - 1];

        for (value_iterator i = values.begin(); i != values.end(); ++i) {
            a << *i;
        }

        a.finish_write_phase();

        external_size += values.size();

        if (!extract_buffer_empty()) {
            stats.num_new_external_arrays++;
            stats.max_num_new_external_arrays.set_max(stats.num_new_external_arrays);
            a.wait_for_first_block();
            m_minima.add_external_array(static_cast<unsigned>(external_arrays.size()) - 1);
        }

        ram_left -= ram_per_external_array;
        stats.max_num_external_arrays.set_max(external_arrays.size());
    }

    /*!
     * Sorts the values from values and writes them into an internal array.
     * Don't use the value vector afterwards!
     *
     * \param values the vector to sort and store
     */
    void flush_array_internal(std::vector<ValueType>& values)
    {
        internal_size += values.size();

        #if STXXL_PARALLEL
        __gnu_parallel::sort(values.begin(), values.end(), inv_compare);
        #else
        std::sort(values.begin(), values.end(), inv_compare);
        #endif

        internal_arrays.emplace_back(values);
        // internal array owns values now.

        if (c_merge_ias_into_eb) {
            if (!extract_buffer_empty()) {
                stats.num_new_internal_arrays++;
                stats.max_num_new_internal_arrays.set_max(stats.num_new_internal_arrays);
                m_minima.add_internal_array(static_cast<unsigned>(internal_arrays.size()) - 1);
            }
        }
        else {
            m_minima.add_internal_array(static_cast<unsigned>(internal_arrays.size()) - 1);
        }

        // TODO: use real value size: ram_left -= 2*values->size()*sizeof(ValueType);
        ram_left -= ram_per_internal_array;
        stats.max_num_internal_arrays.set_max(internal_arrays.size());

        // Vector is now owned by PPQ...
    }

    /*!
     * Lets the priority queue decide if flush_array_to_hd() or
     * flush_array_internal() should be called.  Don't use the value vector
     * afterwards!
     *
     * \param values the vector to sort and store
     */
    void flush_array(std::vector<ValueType>& values)
    {
        size_type size = values.size();
        size_type ram_internal = 2 * size * sizeof(ValueType); // ram for the sorted array + part of the ram for the merge buffer

        size_type ram_for_all_internal_arrays = total_ram - 2 * ram_for_heaps - num_write_buffers * BlockSize - external_arrays.size() * ram_per_external_array;

        if (ram_internal > ram_for_all_internal_arrays) {
            flush_array_to_hd(values);
            return;
        }

        if (ram_left < ram_internal) {
            flush_internal_arrays();
        }

        if (ram_left < ram_internal) {
            if (ram_left < ram_per_external_array) {
                merge_external_arrays();
            }

            flush_array_to_hd(values);
            return;
        }

        if (m_do_flush_directly_to_hd) {
            flush_array_to_hd(values);
        }
        else {
            flush_array_internal(values);
        }
    }

    //! Struct of all statistical counters and timers.  Turn on/off statistics
    //! using the stats_counter and stats_timer typedefs.
    struct stats_type
    {
        //! Largest number of elements in the extract buffer at the same time
        stats_counter max_extract_buffer_size;

        //! Sum of the sizes of each extract buffer refill. Used for average
        //! size.
        stats_counter total_extract_buffer_size;

        //! Largest number of elements in the merge buffer when running
        //! flush_internal_arrays()
        stats_counter max_merge_buffer_size;

        //! Total number of extracts
        stats_counter num_extracts;

        //! Number of refill_extract_buffer() calls
        stats_counter num_extract_buffer_refills;

        //! Number of flush_insertion_heaps() calls
        stats_counter num_insertion_heap_flushes;

        //! Number of flush_directly_to_hd() calls
        stats_counter num_direct_flushes;

        //! Number of flush_internal_arrays() calls
        stats_counter num_internal_array_flushes;

        //! Number of merge_external_arrays() calls
        stats_counter num_external_array_merges;

        //! Largest number of internal arrays at the same time
        stats_counter max_num_internal_arrays;

        //! Largest number of external arrays at the same time
        stats_counter max_num_external_arrays;

        //! Temporary number of new external arrays at the same time (which
        //! were created while the extract buffer hadn't been empty)
        stats_counter num_new_external_arrays;

        //! Largest number of new external arrays at the same time (which were
        //! created while the extract buffer hadn't been empty)
        stats_counter max_num_new_external_arrays;

        //if (c_merge_ias_into_eb) {

        //! Temporary number of new internal arrays at the same time (which
        //! were created while the extract buffer hadn't been empty)
        stats_counter num_new_internal_arrays;

        //! Largest number of new internal arrays at the same time (which were
        //! created while the extract buffer hadn't been empty)
        stats_counter max_num_new_internal_arrays;

        //}

        //! Total time for flush_insertion_heaps()
        stats_timer insertion_heap_flush_time;

        //! Total time for flush_directly_to_hd()
        stats_timer direct_flush_time;

        //! Total time for flush_internal_arrays()
        stats_timer internal_array_flush_time;

        //! Total time for merge_external_arrays()
        stats_timer external_array_merge_time;

        //! Total time for extract_min()
        stats_timer extract_min_time;

        //! Total time for refill_extract_buffer()
        stats_timer refill_extract_buffer_time;

        //! Total time for the merging in refill_extract_buffer()
        //! Part of refill_extract_buffer_time.
        stats_timer refill_merge_time;

        //! Total time for all things before merging in refill_extract_buffer()
        //! Part of refill_extract_buffer_time.
        stats_timer refill_time_before_merge;

        //! Total time for all things after merging in refill_extract_buffer()
        //! Part of refill_extract_buffer_time.
        stats_timer refill_time_after_merge;

        //! Total time of wait_for_first_block() calls in first part of
        //! refill_extract_buffer(). Part of refill_time_before_merge and
        //! refill_extract_buffer_time.
        stats_timer refill_wait_time;

        //! Total time for pop_heap() in extract_min().
        //! Part of extract_min_time.
        stats_timer pop_heap_time;

        //! Total time for merging the sorted heaps.
        //! Part of flush_insertion_heaps.
        stats_timer merge_sorted_heaps_time;

        //! Total time for std::upper_bound calls in refill_extract_buffer()
        //! Part of refill_extract_buffer_time and refill_time_before_merge.
        // stats_timer refill_upper_bound_time;

        //! Total time for std::accumulate calls in refill_extract_buffer()
        //! Part of refill_extract_buffer_time and refill_time_before_merge.
        stats_timer refill_accumulate_time;

        //! Total time for determining the smallest max value in refill_extract_buffer()
        //! Part of refill_extract_buffer_time and refill_time_before_merge.
        stats_timer refill_minmax_time;

        friend std::ostream& operator << (std::ostream& os, const stats_type& o)
        {
            return os << "max_extract_buffer_size=" << o.max_extract_buffer_size.as_memory_amount(sizeof(ValueType)) << std::endl
                      << "total_extract_buffer_size=" << o.total_extract_buffer_size.as_memory_amount(sizeof(ValueType)) << std::endl
                      << "max_merge_buffer_size=" << o.max_merge_buffer_size.as_memory_amount(sizeof(ValueType)) << std::endl
                      << "num_extracts=" << o.num_extracts << std::endl
                      << "num_extract_buffer_refills=" << o.num_extract_buffer_refills << std::endl
                      << "num_insertion_heap_flushes=" << o.num_insertion_heap_flushes << std::endl
                      << "num_direct_flushes=" << o.num_direct_flushes << std::endl
                      << "num_internal_array_flushes=" << o.num_internal_array_flushes << std::endl
                      << "num_external_array_merges=" << o.num_external_array_merges << std::endl
                      << "max_num_internal_arrays=" << o.max_num_internal_arrays << std::endl
                      << "max_num_external_arrays=" << o.max_num_external_arrays << std::endl
                      << "num_new_external_arrays=" << o.num_new_external_arrays << std::endl
                      << "max_num_new_external_arrays=" << o.max_num_new_external_arrays << std::endl
                   //if (c_merge_ias_into_eb) {
                      << "num_new_internal_arrays=" << o.num_new_internal_arrays << std::endl
                      << "max_num_new_internal_arrays=" << o.max_num_new_internal_arrays << std::endl
                   //}
                      << "insertion_heap_flush_time=" << o.insertion_heap_flush_time << std::endl
                      << "direct_flush_time=" << o.direct_flush_time << std::endl
                      << "internal_array_flush_time=" << o.internal_array_flush_time << std::endl
                      << "external_array_merge_time=" << o.external_array_merge_time << std::endl
                      << "extract_min_time=" << o.extract_min_time << std::endl
                      << "refill_extract_buffer_time=" << o.refill_extract_buffer_time << std::endl
                      << "refill_merge_time=" << o.refill_merge_time << std::endl
                      << "refill_time_before_merge=" << o.refill_time_before_merge << std::endl
                      << "refill_time_after_merge=" << o.refill_time_after_merge << std::endl
                      << "refill_wait_time=" << o.refill_wait_time << std::endl
                      << "pop_heap_time=" << o.pop_heap_time << std::endl
                      << "merge_sorted_heaps_time=" << o.merge_sorted_heaps_time << std::endl
                   // << "refill_upper_bound_time=" << o.refill_upper_bound_time << std::endl
                      << "refill_accumulate_time=" << o.refill_accumulate_time << std::endl
                      << "refill_minmax_time=" << o.refill_minmax_time << std::endl;
        }
    };

    stats_type stats;
};

STXXL_END_NAMESPACE

#endif // !STXXL_CONTAINERS_PARALLEL_PRIORITY_QUEUE_HEADER
