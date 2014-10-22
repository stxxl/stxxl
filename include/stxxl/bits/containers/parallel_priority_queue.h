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
#include <algorithm>
#include <assert.h>
#include <assert.h>
#include <climits>
#include <cstdlib>
#include <list>
#include <stdlib.h>
#include <utility>
#include <vector>

#include <parallel/algorithm>
#include <parallel/numeric>

#include <stxxl/bits/common/custom_stats.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/containers/ppq_external_array.h>
#include <stxxl/bits/containers/ppq_minima_tree.h>
#include <stxxl/bits/io/request_operations.h>
#include <stxxl/bits/mng/block_alloc.h>
#include <stxxl/bits/mng/buf_ostream.h>
#include <stxxl/bits/mng/prefetch_pool.h>
#include <stxxl/bits/namespace.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/parallel.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/types>


STXXL_BEGIN_NAMESPACE

//! \brief Parallelized External Memory Priority Queue Config
//!
//! \tparam ValueType            Type of the contained objects (POD with no references to internal memory).
//!
//! \tparam CompareType		The comparator type used to determine whether one element is smaller than another element.
//!
//! \tparam Ram				Maximum memory consumption by the queue. Can be overwritten by the constructor. Default = 8 GiB.
//!
//! \tparam MaxItems			Maximum number of elements the queue contains at one time. Default = 0 = unlimited.
//!                                                      This is no hard limit and only used for optimization. Can be overwritten by the constructor.
//!
//! \tparam BlockSize		External block size. Default = STXXL_DEFAULT_BLOCK_SIZE(ValueType).
//!
//! \tparam AllocStrategy	Allocation strategy for the external memory. Default = STXXL_DEFAULT_ALLOC_STRATEGY.
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
    typedef external_array<ValueType, BlockSize, AllocStrategy> external_array_type;
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

    //! Record statistical data.
    // typedef custom_stats stats_type;
    
    //! Don't record statistical data.
    typedef dummy_custom_stats stats_type;

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

    //! Default number of write buffer block for a new external array being filled.
    static const unsigned c_num_write_buffer_blocks = 14;

    //! Defines for how much external arrays memory should be reserved in the constructor.
    static const unsigned c_num_reserved_external_arrays = 10;

    //! Size of a single insertion heap in Byte, if not defined otherwise in the constructor
    static const size_type c_default_single_heap_ram = 1L * 1024L * 1024L; // 10 MiB

    //! Default limit of the extract buffer ram consumption as share of total ram
    constexpr static double c_default_extract_buffer_ram_part = 0.05;

    //! Leave out (c_cache_line_factor-1) slots between the heaps in the insertion_heaps vector each.
    //! This ensures that the heap vectors (meaning: the begin and end pointers) are in different cache lines
    //! and therefore this improves the multicore performance.
    static const unsigned c_cache_line_factor = 128;

    //! Number of elements to reserve space for in the dummy insertion heap entries.
    //! @see c_cache_line_factor
    static const unsigned c_cache_line_space = 128;

    //! \}

    friend class minima_tree<parallel_priority_queue<ValueType, CompareType, AllocStrategy, BlockSize, Ram, MaxItems> >;

#include "ppq_internal_array.h"

    typedef typename std::vector<heap_type> heaps_type;
    typedef typename std::vector<external_array_type> external_arrays_type;
    typedef typename std::vector<internal_array> internal_arrays_type;

    //! Logger for statistical data
    stats_type stats;

    //! Limit the size of the extract buffer to an absolute value.
    //!
    //! The actual size can be set using the _extract_buffer_ram parameter of the constructor. If this parameter is not set,
    //! the value is calculated by (total_ram*c_default_extract_buffer_ram_part)
    //!
    //! If c_limit_extract_buffer==false, the memory consumption of the extract buffer is only limited by the number of
    //! external and internal arrays. This is considered in memory management using the ram_per_external_array and
    //! ram_per_internal_array values. Attention: Each internal
    //! array reserves space for the extract buffer in the size of all heaps together.
    static const bool c_limit_extract_buffer = true;

    //! For bulks of size up to c_single_insert_limit sequential single insert is faster than bulk_push.
    static const unsigned c_single_insert_limit = 100;

    //! Number of prefetch blocks per external array
    unsigned num_prefetchers;

    //! Number of write buffer blocks for a new external array being filled
    unsigned num_write_buffers;

    //! The size of the bulk currently being inserted
    size_type bulk_size;

    //! If the bulk currently being insered is very lare, this boolean is set and
    //! bulk_push_steps just accumulate the elements for eventual sorting.
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
    size_type num_insertion_heaps;

    //! Capacity of one inserion heap
    size_type insertion_heap_capacity;

    //! Using this parameter you can reserve more space for the insertion heaps than visible to the algorithm.
    //! This avoids memory allocation if the data is not distributed evenly among the heaps.
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

    //! Amount of internal memory an external array needs during it's lifetime in bytes
    size_type ram_per_external_array;

    //! Amount of internal memory an internal array needs during it's lifetime in bytes
    size_type ram_per_internal_array;

    /*
     * Data.
     */

    //! The sorted arrays in external memory
    std::vector<external_array_type> external_arrays;

    //! The sorted arrays in internal memory
    std::vector<internal_array> internal_arrays;

    //! The buffer where external (and internal) arrays are merged into for extracting
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
        bool operator () (internal_array& a) const { return a.empty(); }
    };

    typedef minima_tree<parallel_priority_queue<ValueType, CompareType, AllocStrategy, BlockSize, Ram, MaxItems> > minima_type;

    //! The winner tree containing the smallest values of all sources
    //! where the globally smallest element could come from.
    minima_type m_minima;

    const bool m_do_flush_directly_to_hd;

public:
    //! \addtogroup init Initialization
    //! \{

    //! Constructor.
    //!
    //! \param num_prefetch_buffer_blocks    Number of prefetch blocks per external array. Default = c_num_prefetch_buffer_blocks
    //!
    //! \param num_write_buffer_blocks	    Number of write buffer blocks for a new external array being filled. 0 = Default = c_num_write_buffer_blocks
    //!
    //! \param _total_ram			        Maximum RAM usage. 0 = Default = Use the template value Ram.
    //!
    //! \param _num_insertion_heaps	        Number of insertion heaps. 0 = Determine by omp_get_max_threads(). Default = Determine by omp_get_max_threads().
    //!
    //! \param _single_heap_ram		        Memory usage for a single insertion heap. 0 = Default = c_single_heap_ram.
    //!
    //! \param extract_buffer_ram		    Memory usage for the extract buffer. Only relevant if c_limit_extract_buffer==true.
    //!                                         0 = Default = total_ram * c_default_extract_buffer_ram_part.
    //!
    //! \param flush_directly_to_hd         Do not flush into internal arrays when there is RAM left but flush directly into an external array.
    parallel_priority_queue(
        unsigned num_prefetch_buffer_blocks = 0,
        unsigned num_write_buffer_blocks = 0,
        size_type _total_ram = 0,
        unsigned _num_insertion_heaps = 0,
        size_type _single_heap_ram = 0,
        size_type extract_buffer_ram = 0,
        bool flush_directly_to_hd = false) :
        num_prefetchers((num_prefetch_buffer_blocks > 0) ? num_prefetch_buffer_blocks : c_num_prefetch_buffer_blocks),
        num_write_buffers((num_write_buffer_blocks > 0) ? num_write_buffer_blocks : c_num_write_buffer_blocks),
        bulk_size(0),
        is_very_large_bulk(false),
        extract_index(0),
        external_size(0),
        internal_size(0),
        insertion_size(0),
        buffered_size(0),
        num_insertion_heaps((_num_insertion_heaps > 0) ? _num_insertion_heaps : omp_get_max_threads()),
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
        srand(time(NULL));

        if (c_limit_extract_buffer) {
            extract_buffer_limit = (extract_buffer_ram > 0) ? extract_buffer_ram / sizeof(ValueType) : (total_ram * c_default_extract_buffer_ram_part / sizeof(ValueType));
        }

        insertion_heap_capacity = ram_for_heaps / (num_insertion_heaps * sizeof(ValueType));

        for (unsigned i = 0; i < num_insertion_heaps * c_cache_line_factor; ++i) {
            insertion_heaps[i].reserve(c_cache_line_space);
        }

        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            insertion_heaps[i * c_cache_line_factor].reserve(real_insertion_heap_size_factor * insertion_heap_capacity);
        }

        init_memmanagement();

        external_arrays.reserve(c_num_reserved_external_arrays);

        if (c_merge_sorted_heaps) {
            internal_arrays.reserve(total_ram / ram_for_heaps);
        } else {
            internal_arrays.reserve(total_ram * num_insertion_heaps / ram_for_heaps);
        }

        init_stats();
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
        } else {
            // each heap becomes one internal array
            ram_per_internal_array = ram_for_heaps / num_insertion_heaps;
        }

        if (c_limit_extract_buffer) {
            // ram for the extract buffer
            ram_left -= extract_buffer_limit * sizeof(ValueType);
        } else {
            // each: part of the (maximum) ram for the extract buffer

            ram_per_external_array += BlockSize;

            if (c_merge_ias_into_eb) {
                // we have to reserve space in the size of the whole array: very inefficient!
                if (c_merge_sorted_heaps) {
                    ram_per_internal_array += ram_for_heaps;
                } else {
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
            } else if (ram_left < 4 * ram_per_external_array) {
                STXXL_ERRMSG("Warning: Low memory. Performance could suffer.");
            }
        } else {
            if (ram_left < 2 * ram_per_external_array + ram_per_internal_array) {
                STXXL_ERRMSG("Insufficent memory.");
                exit(EXIT_FAILURE);
            } else if (ram_left < 4 * ram_per_external_array + 2 * ram_per_internal_array) {
                STXXL_ERRMSG("Warning: Low memory. Performance could suffer.");
            }
        }
    }

    //! Initializes the counters and timers used for statistics.
    //! Turn on/off with the stats_type definition.
    void init_stats()
    {
        // Largest number of elements in the extract buffer at the same time
        stats.add_mem_counter("max_extract_buffer_size", sizeof(ValueType));

        // Sum of the sizes of each extract buffer refill. Used for average size.
        stats.add_mem_counter("total_extract_buffer_size", sizeof(ValueType));

        // Largest number of elements in the merge buffer when running flush_internal_arrays()
        stats.add_mem_counter("max_merge_buffer_size", sizeof(ValueType));

        // Total number of extracts
        stats.add_counter("num_extracts");

        // Number of refill_extract_buffer() calls
        stats.add_counter("num_extract_buffer_refills");

        // Number of flush_insertion_heaps() calls
        stats.add_counter("num_insertion_heap_flushes");

        // Number of flush_directly_to_hd() calls
        stats.add_counter("num_direct_flushes");

        // Number of flush_internal_arrays() calls
        stats.add_counter("num_internal_array_flushes");

        // Number of merge_external_arrays() calls
        stats.add_counter("num_external_array_merges");

        // Largest number of internal arrays at the same time
        stats.add_counter("max_num_internal_arrays");

        // Largest number of external arrays at the same time
        stats.add_counter("max_num_external_arrays");

        // Temporary number of new external arrays at the same time (which were created while the extract buffer hadn't been empty)
        stats.add_counter("num_new_external_arrays");

        // Largest number of new external arrays at the same time (which were created while the extract buffer hadn't been empty)
        stats.add_counter("max_num_new_external_arrays");

        if (c_merge_ias_into_eb) {
            // Temporary number of new internal arrays at the same time (which were created while the extract buffer hadn't been empty)
            stats.add_counter("num_new_internal_arrays");

            // Largest number of new internal arrays at the same time (which were created while the extract buffer hadn't been empty)
            stats.add_counter("max_num_new_internal_arrays");
        }

        // Total time for flush_insertion_heaps()
        stats.add_timer("insertion_heap_flush_time");

        // Total time for flush_directly_to_hd()
        stats.add_timer("direct_flush_time");

        // Total time for flush_internal_arrays()
        stats.add_timer("internal_array_flush_time");

        // Total time for merge_external_arrays()
        stats.add_timer("external_array_merge_time");

        // Total time for extract_min()
        stats.add_timer("extract_min_time");

        // Total time for refill_extract_buffer()
        stats.add_timer("refill_extract_buffer_time");

        // Total time for the merging in refill_extract_buffer()
        // Part of refill_extract_buffer_time.
        stats.add_timer("refill_merge_time");

        // Total time for all things before merging in refill_extract_buffer()
        // Part of refill_extract_buffer_time.
        stats.add_timer("refill_time_before_merge");

        // Total time for all things after merging in refill_extract_buffer()
        // Part of refill_extract_buffer_time.
        stats.add_timer("refill_time_after_merge");

        // Total time of wait_for_first_block() calls in first part of
        // refill_extract_buffer(). Part of refill_time_before_merge and refill_extract_buffer_time.
        stats.add_timer("refill_wait_time");

        // Total time for pop_heap() in extract_min().
        // Part of extract_min_time.
        stats.add_timer("pop_heap_time");

        // Total time for merging the sorted heaps.
        // Part of flush_insertion_heaps.
        stats.add_timer("merge_sorted_heaps_time");

        // Total time for std::upper_bound calls in refill_extract_buffer()
        // Part of refill_extract_buffer_time and refill_time_before_merge.
        //stats.add_timer(refill_upper_bound_time;

        // Total time for std::accumulate calls in refill_extract_buffer()
        // Part of refill_extract_buffer_time and refill_time_before_merge.
        stats.add_timer("refill_accumulate_time");

        // Total time for determining the smallest max value in refill_extract_buffer()
        // Part of refill_extract_buffer_time and refill_time_before_merge.
        stats.add_timer("refill_minmax_time");
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


    //! Start a sequence of push operations.
    //! \param _bulk_size	Number of elements to push before the next pop.
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
            } else {
                flush_insertion_heaps();
            }
        }
    }

    //! Push an element inside a sequence of pushes.
    //! Run bulk_push_begin() before using this method.
    //!
    //! \param element		The element to push.
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
        } else if (omp_get_num_threads() > 1) {
            id = omp_get_thread_num();
        } else {
            id = rand() % num_insertion_heaps;
        }

        // TODO: check if full? Alternative: real_insertion_heap_size_factor
        insertion_heaps[id * c_cache_line_factor].push_back(element);
        std::push_heap(insertion_heaps[id * c_cache_line_factor].begin(), insertion_heaps[id * c_cache_line_factor].end(), compare);
        // The following would avoid problems if the bulk size specified in bulk_push_begin is not correct.
        // insertion_size += bulk_size; must then be removed from bulk_push_end().
        //__sync_fetch_and_add(&insertion_size, 1);
    }

    //! Ends a sequence of push operations. Run bulk_push_begin()
    //! and some bulk_push_step() before this.
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

    //! \brief			Insert multiple elements at one time.
    //! \param elements	Vector containing the elements to push.
    //! Attention: elements vector may be owned by the PQ afterwards.
    //!
    void bulk_push(std::vector<ValueType>& elements)
    {
        size_type heap_capacity = num_insertion_heaps * insertion_heap_capacity;
        if (elements.size() > heap_capacity / 2) {
            flush_array(elements);
            return;
        }

        bulk_push_begin(elements.size());
#pragma omp parallel if (elements.size()>c_single_insert_limit)
        {
            unsigned int thread_num = omp_get_thread_num();
#pragma omp for
            for (size_type i = 0; i < elements.size(); ++i) {
                bulk_push_step(elements[i], thread_num);
            }
        }
        bulk_push_end();
    }

    //! \}

    //! \addtogroup aggrops Aggregation Operations
    //! \{

    //! \brief				Aggregate pushes. Use flush_aggregated_pushes() to finally
    //!                      push them. extract_min is allowed is allowed
    //!						in between the aggregation of pushes if you can assure,
    //!                      that the extracted value is smaller than all of
    //!						the aggregated values.
    //! \param element		The element to push.
    void aggregate_push(const ValueType& element)
    {
        aggregated_pushes.push_back(element);
    }

    //! \brief				Insert the aggregated values into the queue using push(),
    //!                      bulk insert, or sorting, depending on the
    //!						number of aggregated values.
    //! \param element		The element to push.
    //! \see                 c_single_insert_limit
    void flush_aggregated_pushes()
    {
        size_type size = aggregated_pushes.size();
        size_type ram_internal = 2 * size * sizeof(ValueType); // ram for the sorted array + part of the ram for the merge buffer
        size_type heap_capacity = num_insertion_heaps * insertion_heap_capacity;

        if (ram_internal > ram_for_heaps / 2) {
            flush_array(aggregated_pushes);
        } else if ((aggregated_pushes.size() > c_single_insert_limit) && (aggregated_pushes.size() < heap_capacity)) {
            bulk_push(aggregated_pushes);
        } else {
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
            } else {
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
            break;
        case minima_type::EB:
            return extract_buffer[extract_index];
            break;
        case minima_type::IA:
            return internal_arrays[index].get_min();
            break;
        case minima_type::EA:
            // wait_for_first_block() already done by comparator....
            return external_arrays[index].get_min_element();
            break;
        default:
            STXXL_ERRMSG("Unknown extract type: " << type);
            abort();
        }
    }

    //! Access and remove the minimum element.
    ValueType pop()
    {
        stats.inc_counter("num_extracts");

        if (extract_buffer_empty()) {
            refill_extract_buffer();
        }

        stats.start_timer("extract_min_time");

        std::pair<unsigned, unsigned> type_and_index = m_minima.top();
        unsigned type = type_and_index.first;
        unsigned index = type_and_index.second;
        ValueType min;

        assert(type < 4);

        switch (type) {
        case minima_type::HEAP:
        {
            min = insertion_heaps[index * c_cache_line_factor][0];

            stats.start_timer("pop_heap_time");
            std::pop_heap(insertion_heaps[index * c_cache_line_factor].begin(), insertion_heaps[index * c_cache_line_factor].end(), compare);
            insertion_heaps[index * c_cache_line_factor].pop_back();
            stats.stop_timer("pop_heap_time");

            insertion_size--;

            if (!insertion_heaps[index * c_cache_line_factor].empty()) {
                m_minima.update_heap(index);
            } else {
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
            } else {
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
            } else {
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
            } else {
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

        stats.stop_timer("extract_min_time");
        return min;
    }

    //! \}

    //! Merges all external arrays into one external array.
    //! Public for benchmark purposes.
    void merge_external_arrays()
    {
        stats.inc_counter("num_external_array_merges");
        stats.start_timer("external_array_merge_time");

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

#pragma omp parallel for if(eas>num_insertion_heaps)
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
            stxxl::parallel::multiway_merge(sequences.begin(), sequences.end(), merge_buffer.begin(), inv_compare, output_size);

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
            stats.inc_counter("num_new_external_arrays");
            stats.max_counter("max_num_new_external_arrays", stats.get_counter("num_new_external_arrays"));
            a.wait_for_first_block();
            m_minima.add_external_array(external_arrays.size() - 1);
        }

        stats.stop_timer("external_array_merge_time");
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
        STXXL_VARDUMP(omp_get_max_threads());

        STXXL_MEMDUMP(ram_for_heaps);
        STXXL_MEMDUMP(ram_left);
        STXXL_MEMDUMP(ram_per_external_array);
        STXXL_MEMDUMP(ram_per_internal_array);

        //if (num_extract_buffer_refills > 0) {
        //    STXXL_VARDUMP(total_extract_buffer_size / num_extract_buffer_refills);
        //    STXXL_MEMDUMP(total_extract_buffer_size / num_extract_buffer_refills * sizeof(ValueType));
        //}

        stats.print_all();
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
        } else {
            ias = 0;
        }

        if (eas == 0 && ias == 0) {
            extract_buffer.resize(0);
            m_minima.deactivate_extract_buffer();
            return;
        }

        stats.inc_counter("num_extract_buffer_refills");
        stats.start_timer("refill_extract_buffer_time");
        stats.start_timer("refill_time_before_merge");
        stats.start_timer("refill_minmax_time");

        /*
         * determine maximum of each first block
         */

        ValueType min_max_value;

        // check only relevant if c_merge_ias_into_eb==true
        if (eas > 0) {
            stats.start_timer("refill_wait_time");
            external_arrays[0].wait_for_first_block();
            stats.stop_timer("refill_wait_time");
            assert(external_arrays[0].size() > 0);
            min_max_value = external_arrays[0].get_current_max_element();
        }

        for (size_type i = 1; i < eas; ++i) {
            stats.start_timer("refill_wait_time");
            external_arrays[i].wait_for_first_block();
            stats.stop_timer("refill_wait_time");

            ValueType max_value = external_arrays[i].get_current_max_element();
            if (inv_compare(max_value, min_max_value)) {
                min_max_value = max_value;
            }
        }

        stats.stop_timer("refill_minmax_time");

        // the number of elements in each external array that are smaller than min_max_value or equal
        // plus the number of elements in the internal arrays
        std::vector<size_type> sizes(eas + ias);
        std::vector<std::pair<ValueType*, ValueType*> > sequences(eas + ias);

        /*
         * calculate size and create sequences to merge
         */

#pragma omp parallel for if(eas+ias>num_insertion_heaps)
        for (size_type i = 0; i < eas + ias; ++i) {
            // check only relevant if c_merge_ias_into_eb==true
            if (i < eas) {
                assert(!external_arrays[i].empty());

                assert(external_arrays[i].first_block_valid());
                ValueType* begin = external_arrays[i].begin_block();
                ValueType* end = external_arrays[i].end_block();

                assert(begin != end);

                // remove if parallel
                //stats.start_timer("refill_upper_bound_time");
                ValueType* ub = std::upper_bound(begin, end, min_max_value, inv_compare);
                //stats.stop_timer("refill_upper_bound_time");

                sizes[i] = std::distance(begin, ub);
                sequences[i] = std::make_pair(begin, ub);
            } else {
                // else part only relevant if c_merge_ias_into_eb==true

                size_type j = i - eas;
                assert(!(internal_arrays[j].empty()));

                ValueType* begin = internal_arrays[j].begin();
                ValueType* end = internal_arrays[j].end();
                assert(begin != end);

                if (eas > 0) {
                    //remove if parallel
                    //stats.start_timer("refill_upper_bound_time");
                    ValueType* ub = std::upper_bound(begin, end, min_max_value, inv_compare);
                    //stats.stop_timer("refill_upper_bound_time");

                    sizes[i] = std::distance(begin, ub);
                    sequences[i] = std::make_pair(begin, ub);
                } else {
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

        stats.max_mem_counter("max_extract_buffer_size", output_size);
        stats.inc_mem_counter("total_extract_buffer_size", output_size);

        assert(output_size > 0);
        extract_buffer.resize(output_size);
        buffered_size = output_size;

        stats.stop_timer("refill_time_before_merge");
        stats.start_timer("refill_merge_time");

        stxxl::parallel::multiway_merge(sequences.begin(), sequences.end(), extract_buffer.begin(), inv_compare, output_size);

        stats.stop_timer("refill_merge_time");
        stats.start_timer("refill_time_after_merge");

        // remove elements
        if (c_limit_extract_buffer) {
            for (size_type i = 0; i < eas + ias; ++i) {
                // dist represents the number of elements that haven't been merged
                size_type dist = std::distance(sequences[i].first, sequences[i].second);
                if (i < eas) {
                    external_arrays[i].remove_first_n_elements(sizes[i] - dist);
                    assert(external_size >= sizes[i] - dist);
                    external_size -= sizes[i] - dist;
                } else {
                    size_type j = i - eas;
                    internal_arrays[j].inc_min(sizes[i] - dist);
                    assert(internal_size >= sizes[i] - dist);
                    internal_size -= sizes[i] - dist;
                }
            }
        } else {
            for (size_type i = 0; i < eas; ++i) {
                external_arrays[i].remove_first_n_elements(sizes[i]);
                assert(external_size >= sizes[i]);
                external_size -= sizes[i];
            }
        }

        //stats.start_timer("refill_wait_time");
        for (size_type i = 0; i < eas; ++i) {
            external_arrays[i].wait_for_first_block();
        }
        //stats.stop_timer("refill_wait_time");

        // remove empty arrays - important for the next round
        external_arrays.erase(std::remove_if(external_arrays.begin(), external_arrays.end(), empty_array_eraser()), external_arrays.end());
        size_type num_deleted_arrays = eas - external_arrays.size();
        if (num_deleted_arrays > 0) {
            ram_left += num_deleted_arrays * ram_per_external_array;
        }

        stats.set_counter("num_new_external_arrays", 0);

        if (c_merge_ias_into_eb) {
            internal_arrays.erase(std::remove_if(internal_arrays.begin(), internal_arrays.end(), empty_internal_array_eraser()), internal_arrays.end());
            size_type num_deleted_internal_arrays = ias - internal_arrays.size();
            if (num_deleted_internal_arrays > 0) {
                ram_left += num_deleted_internal_arrays * ram_per_internal_array;
            }
            stats.set_counter("num_new_internal_arrays", 0);
        }

        m_minima.update_extract_buffer();

        stats.stop_timer("refill_time_after_merge");
        stats.stop_timer("refill_extract_buffer_time");
    }


    //! Flushes the insertions heaps into an internal array.
    inline void flush_insertion_heaps()
    {
        size_type ram_needed;

        if (c_merge_sorted_heaps) {
            ram_needed = ram_per_internal_array;
        } else {
            ram_needed = num_insertion_heaps * ram_per_internal_array;
        }

        if (ram_left < ram_needed) {
            if (internal_size > 0) {
                flush_internal_arrays();
                // still not enough?
                if (ram_left < ram_needed) {
                    merge_external_arrays();
                }
            } else {
                merge_external_arrays();
            }
        }

        stats.inc_counter("num_insertion_heap_flushes");
        stats.start_timer("insertion_heap_flush_time");

        size_type size = insertion_size;
        assert(size > 0);
        std::vector<std::pair<value_iterator, value_iterator> > sequences(num_insertion_heaps);

#pragma omp parallel for
        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            // TODO: Use std::sort_heap instead? We would have to reverse the order...
            std::sort(insertion_heaps[i * c_cache_line_factor].begin(), insertion_heaps[i * c_cache_line_factor].end(), inv_compare);
            if (c_merge_sorted_heaps) {
                sequences[i] = std::make_pair(insertion_heaps[i * c_cache_line_factor].begin(), insertion_heaps[i * c_cache_line_factor].end());
            }
        }

        if (c_merge_sorted_heaps) {
            stats.start_timer("merge_sorted_heaps_time");
            std::vector<ValueType> merged_array(size);
            parallel::multiway_merge(sequences.begin(), sequences.end(), merged_array.begin(), inv_compare, size);
            stats.stop_timer("merge_sorted_heaps_time");

            internal_arrays.emplace_back(merged_array);
            // internal array owns merged_array now.

            if (c_merge_ias_into_eb) {
                if (!extract_buffer_empty()) {
                    stats.inc_counter("num_new_internal_arrays");
                    stats.max_counter("max_num_new_internal_arrays", stats.get_counter("num_new_internal_arrays"));
                    m_minima.add_internal_array(internal_arrays.size() - 1);
                }
            } else {
                m_minima.add_internal_array(internal_arrays.size() - 1);
            }

            for (unsigned i = 0; i < num_insertion_heaps; ++i) {
                insertion_heaps[i * c_cache_line_factor].clear();
                insertion_heaps[i * c_cache_line_factor].reserve(real_insertion_heap_size_factor * insertion_heap_capacity);
            }
            m_minima.clear_heaps();

            ram_left -= ram_per_internal_array;
        } else {
            for (unsigned i = 0; i < num_insertion_heaps; ++i) {
                if (insertion_heaps[i * c_cache_line_factor].size() > 0) {
                    internal_arrays.emplace_back(insertion_heaps[i * c_cache_line_factor]);
                    // insertion_heaps[i*c_cache_line_factor] is empty now.

                    if (c_merge_ias_into_eb) {
                        if (!extract_buffer_empty()) {
                            stats.inc_counter("num_new_internal_arrays");
                            stats.max_counter("max_num_new_internal_arrays", stats.get_counter("num_new_internal_arrays"));
                            m_minima.add_internal_array(internal_arrays.size() - 1);
                        }
                    } else {
                        m_minima.add_internal_array(internal_arrays.size() - 1);
                    }

                    insertion_heaps[i * c_cache_line_factor].reserve(real_insertion_heap_size_factor * insertion_heap_capacity);
                }
            }

            m_minima.clear_heaps();
            ram_left -= num_insertion_heaps * ram_per_internal_array;
        }

        internal_size += size;
        insertion_size = 0;

        stats.max_counter("max_num_internal_arrays", (size_type)internal_arrays.size());
        stats.stop_timer("insertion_heap_flush_time");

        if (ram_left < ram_per_external_array + ram_per_internal_array) {
            flush_internal_arrays();
        }
    }

    //! Flushes the internal arrays into an external array.
    inline void flush_internal_arrays()
    {
        stats.inc_counter("num_internal_array_flushes");
        stats.start_timer("internal_array_flush_time");

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

        stats.max_mem_counter("max_merge_buffer_size", size);

        std::vector<ValueType> write_buffer(size);
        parallel::multiway_merge(sequences.begin(), sequences.end(), write_buffer.begin(), inv_compare, size);

        // TODO: directly write to block? -> no useless mem copy. Does not work if size > block size
        for (value_iterator i = write_buffer.begin(); i != write_buffer.end(); ++i) {
            a << *i;
        }

        a.finish_write_phase();

        external_size += size;
        internal_size = 0;

        internal_arrays.clear();
        stats.set_counter("num_new_internal_arrays", 0);

        if (!extract_buffer_empty()) {
            stats.inc_counter("num_new_external_arrays");
            stats.max_counter("max_num_new_external_arrays", stats.get_counter("num_new_external_arrays"));
            a.wait_for_first_block();
            m_minima.add_external_array(external_arrays.size() - 1);
        }

        ram_left += num_arrays * ram_per_internal_array;
        ram_left -= ram_per_external_array;

        stats.max_counter("max_num_external_arrays", (size_type)external_arrays.size());
        stats.stop_timer("internal_array_flush_time");
    }

    //! Flushes the insertion heaps into an external array.
    void flush_directly_to_hd()
    {
        if (ram_left < ram_per_external_array) {
            merge_external_arrays();
        }

        stats.inc_counter("num_direct_flushes");
        stats.start_timer("direct_flush_time");

        size_type size = insertion_size;
        std::vector<std::pair<value_iterator, value_iterator> > sequences(num_insertion_heaps);

#pragma omp parallel for
        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            // TODO std::sort_heap? We would have to reverse the order...
            std::sort(insertion_heaps[i * c_cache_line_factor].begin(), insertion_heaps[i * c_cache_line_factor].end(), inv_compare);
            sequences[i] = std::make_pair(insertion_heaps[i * c_cache_line_factor].begin(), insertion_heaps[i * c_cache_line_factor].end());
        }

        external_arrays.emplace_back(size, num_prefetchers, num_write_buffers);
        external_array_type& a = external_arrays[external_arrays.size() - 1];

        // TODO: write in chunks in order to safe RAM
        std::vector<ValueType> write_buffer(size);
        parallel::multiway_merge(sequences.begin(), sequences.end(), write_buffer.begin(), inv_compare, size);

        // TODO: directly write to block -> no useless mem copy
        for (value_iterator i = write_buffer.begin(); i != write_buffer.end(); ++i) {
            a << *i;
        }

        a.finish_write_phase();

        external_size += size;
        insertion_size = 0;

//#pragma omp parallel for
        for (unsigned i = 0; i < num_insertion_heaps; ++i) {
            insertion_heaps[i * c_cache_line_factor].clear();
            insertion_heaps[i * c_cache_line_factor].reserve(real_insertion_heap_size_factor * insertion_heap_capacity);
        }
        m_minima.clear_heaps();

        if (!extract_buffer_empty()) {
            stats.inc_counter("num_new_external_arrays");
            stats.max_counter("max_num_new_external_arrays", stats.get_counter("num_new_external_arrays"));
            a.wait_for_first_block();
            m_minima.add_external_array(external_arrays.size() - 1);
        }

        ram_left -= ram_per_external_array;

        stats.max_counter("max_num_external_arrays", (size_type)external_arrays.size());
        stats.stop_timer("direct_flush_time");
    }

    //! Sorts the values from values and writes them into an external array.
    //! \param values the vector to sort and store
    void flush_array_to_hd(std::vector<ValueType>& values)
    {
        __gnu_parallel::sort(values.begin(), values.end(), inv_compare);

        external_arrays.emplace_back(values.size(), num_prefetchers, num_write_buffers);
        external_array_type& a = external_arrays[external_arrays.size() - 1];

        for (value_iterator i = values.begin(); i != values.end(); ++i) {
            a << *i;
        }

        a.finish_write_phase();

        external_size += values.size();

        if (!extract_buffer_empty()) {
            stats.inc_counter("num_new_external_arrays");
            stats.max_counter("max_num_new_external_arrays", stats.get_counter("num_new_external_arrays"));
            a.wait_for_first_block();
            m_minima.add_external_array(external_arrays.size() - 1);
        }

        ram_left -= ram_per_external_array;
        stats.max_counter("max_num_external_arrays", (size_type)external_arrays.size());
    }

    //! Sorts the values from values and writes them into an internal array.
    //! Don't use the value vector afterwards!
    //! \param values the vector to sort and store
    void flush_array_internal(std::vector<ValueType>& values)
    {
        internal_size += values.size();
        __gnu_parallel::sort(values.begin(), values.end(), inv_compare);
        internal_arrays.emplace_back(values);
        // internal array owns values now.

        if (c_merge_ias_into_eb) {
            if (!extract_buffer_empty()) {
                stats.inc_counter("num_new_internal_arrays");
                stats.max_counter("max_num_new_internal_arrays", stats.get_counter("num_new_internal_arrays"));
                m_minima.add_internal_array(internal_arrays.size() - 1);
            }
        } else {
            m_minima.add_internal_array(internal_arrays.size() - 1);
        }

        // TODO: use real value size: ram_left -= 2*values->size()*sizeof(ValueType);
        ram_left -= ram_per_internal_array;
        stats.max_counter("max_num_internal_arrays", (size_type)internal_arrays.size());

        // Vector is now owned by PPQ...
    }

    //! Lets the priority queue decide if flush_array_to_hd() or flush_array_internal() should be called.
    //! Don't use the value vector afterwards!
    //! \param values the vector to sort and store
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
        } else {
            flush_array_internal(values);
        }
    }
};

STXXL_END_NAMESPACE
#endif // !STXXL_CONTAINERS_PARALLEL_PRIORITY_QUEUE_HEADER
