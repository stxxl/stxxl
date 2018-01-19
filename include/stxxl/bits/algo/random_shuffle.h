/***************************************************************************
 *  include/stxxl/bits/algo/random_shuffle.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007 Manuel Krings
 *  Copyright (C) 2007 Markus Westphal <mail@markuswestphal.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALGO_RANDOM_SHUFFLE_HEADER
#define STXXL_ALGO_RANDOM_SHUFFLE_HEADER

// TODO: improve main memory consumption in recursion
//        (free stacks buffers)
// TODO: shuffle small input in internal memory

#include <vector>

#include <tlx/logger.hpp>

#include <stxxl/bits/parallel.h>
#include <stxxl/bits/stream/stream.h>
#include <stxxl/scan>
#include <stxxl/stack>

namespace stxxl {

//! \addtogroup stlalgo
//! \{

//! External equivalent of std::random_shuffle
//! \param first begin of the range to shuffle
//! \param last end of the range to shuffle
//! \param rand random number generator object (functor)
//! \param M number of bytes for internal use
//! \param AS parallel disk block allocation strategy
//!
//! - BlockSize size of the block to use for external memory data structures
//! - PageSize page size in blocks to use for external memory data structures
template <typename ExtIterator,
          typename RandomNumberGenerator,
          size_t BlockSize,
          unsigned PageSize,
          typename AllocStrategy>
void random_shuffle(ExtIterator first,
                    ExtIterator last,
                    RandomNumberGenerator& rand,
                    size_t M,
                    AllocStrategy AS = foxxll::default_alloc_strategy())
{
    constexpr bool debug = false;
    tlx::unused(AS);  // FIXME: Why is this not being used?

    using value_type = typename ExtIterator::value_type;
    using stack_type = typename STACK_GENERATOR<
              value_type, external, grow_shrink2, PageSize,
              BlockSize, void, 0, AllocStrategy
              >::result;
    using block_type = typename stack_type::block_type;

    LOG << "random_shuffle: Plain Version";
    static_assert(int(BlockSize) < 0,
                  "This implementation was never tested. Please report to the stxxl developers if you have an ExtIterator that works with this implementation.");

    int64_t n = last - first; // the number of input elements

    // make sure we have at least 6 blocks + 1 page
    if (M < 6 * BlockSize + PageSize * BlockSize) {
        LOG1 << "random_shuffle: insufficient memory, " << M << " bytes supplied,";
        M = 6 * BlockSize + PageSize * BlockSize;
        LOG1 << "random_shuffle: increasing to " << M << " bytes (6 blocks + 1 page)";
    }

    size_t k = M / (3 * BlockSize); // number of buckets

    int64_t i, j, size = 0;

    value_type* temp_array;
    using temp_vector_type = typename stxxl::vector<
              value_type, PageSize, stxxl::lru_pager<4>, BlockSize, AllocStrategy
              >;
    temp_vector_type* temp_vector;

    LOG << "random_shuffle: " << M / BlockSize - k << " write buffers for " << k << " buckets";
    foxxll::read_write_pool<block_type> pool(0, M / BlockSize - k);  // no read buffers and M/B-k write buffers

    stack_type** buckets;

    // create and put buckets into container
    buckets = new stack_type*[k];
    for (j = 0; j < k; j++)
        buckets[j] = new stack_type(pool, 0);

    ///// Reading input /////////////////////
    using input_stream = typename stream::streamify_traits<ExtIterator>::stream_type;
    input_stream in = stream::streamify(first, last);

    // distribute input into random buckets
    size_t random_bucket = 0;
    for (i = 0; i < n; ++i) {
        random_bucket = rand(k);
        buckets[random_bucket]->push(*in); // reading the current input element
        ++in;                              // go to the next input element
    }

    ///// Processing //////////////////////
    // resize buffers
    pool.resize_write(0);
    pool.resize_prefetch(PageSize);

    size_t space_left = M - k * BlockSize -
                        PageSize * BlockSize;        // remaining int space
    ExtIterator Writer = first;
    ExtIterator it = first;

    for (i = 0; i < k; i++) {
        LOG << "random_shuffle: bucket no " << i << " contains " << buckets[i]->size() << " elements";
    }

    // shuffle each bucket
    for (i = 0; i < k; i++) {
        buckets[i]->set_prefetch_aggr(PageSize);
        size = buckets[i]->size();

        // does the bucket fit into memory?
        if (size * sizeof(value_type) < space_left) {
            LOG << "random_shuffle: no recursion";

            // copy bucket into temp. array
            temp_array = new value_type[size];
            for (j = 0; j < size; j++) {
                temp_array[j] = buckets[i]->top();
                buckets[i]->pop();
            }

            // shuffle
            potentially_parallel::
            random_shuffle(temp_array, temp_array + size, rand);

            // write back
            for (j = 0; j < size; j++) {
                *Writer = temp_array[j];
                ++Writer;
            }

            // free memory
            delete[] temp_array;
        }
        else {
            LOG << "random_shuffle: recursion";

            // copy bucket into temp. stxxl::vector
            temp_vector = new temp_vector_type(size);

            for (j = 0; j < size; j++) {
                (*temp_vector)[j] = buckets[i]->top();
                buckets[i]->pop();
            }

            pool.resize_prefetch(0);
            space_left += PageSize * BlockSize;
            LOG << "random_shuffle: Space left: " << space_left;

            // recursive shuffle
            stxxl::random_shuffle(temp_vector->begin(),
                                  temp_vector->end(), rand, space_left);

            pool.resize_prefetch(PageSize);

            // write back
            for (j = 0; j < size; j++) {
                *Writer = (*temp_vector)[j];
                ++Writer;
            }

            // free memory
            delete temp_vector;
        }

        // free bucket
        delete buckets[i];
        space_left += BlockSize;
    }

    delete[] buckets;
}

//! External equivalent of std::random_shuffle (specialization for stxxl::vector)
//! \param first begin of the range to shuffle
//! \param last end of the range to shuffle
//! \param rand random number generator object (functor)
//! \param M number of bytes for internal use
template <typename VectorConfig, typename RandomNumberGenerator>
void random_shuffle(
    stxxl::vector_iterator<VectorConfig> first,
    stxxl::vector_iterator<VectorConfig> last,
    RandomNumberGenerator& rand,
    size_t M)
{
    constexpr bool debug = false;

    using ExtIterator = stxxl::vector_iterator<VectorConfig>;
    using AllocStrategy = typename ExtIterator::vector_type::alloc_strategy_type;
    constexpr unsigned PageSize = ExtIterator::vector_type::page_size;
    constexpr unsigned BlockSize = ExtIterator::vector_type::block_size;
    using value_type = typename ExtIterator::value_type;
    using bids_container_iterator = typename ExtIterator::bids_container_iterator;
    using stack_type = typename stxxl::STACK_GENERATOR<value_type, stxxl::external,
                                                       stxxl::grow_shrink2, PageSize, BlockSize>::result;
    using block_type = typename stack_type::block_type;

    LOG << "random_shuffle: Vector Version";

    // make sure we have at least 6 blocks + 1 page
    if (M < 6 * BlockSize + PageSize * BlockSize) {
        LOG1 << "random_shuffle: insufficient memory, " << M << " bytes supplied,";
        M = 6 * BlockSize + PageSize * BlockSize;
        LOG1 << "random_shuffle: increasing to " << M << " bytes (6 blocks + 1 page)";
    }

    uint64_t n = last - first;          // the number of input elements
    size_t k = M / (3 * BlockSize);     // number of buckets

    uint64_t i, j, size = 0;

    value_type* temp_array;
    using temp_vector_type = typename stxxl::vector<
              value_type, PageSize, stxxl::lru_pager<4>, BlockSize, AllocStrategy
              >;
    temp_vector_type* temp_vector;

    // no read buffers and M/B-k write buffers
    foxxll::read_write_pool<block_type> pool(0, M / BlockSize - k);

    stack_type** buckets;

    // create and put buckets into container
    buckets = new stack_type*[k];
    for (j = 0; j < k; j++)
        buckets[j] = new stack_type(pool, 0);

    using buf_istream_type = foxxll::buf_istream<block_type, bids_container_iterator>;
    using buf_ostream_type = foxxll::buf_ostream<block_type, bids_container_iterator>;

    first.flush();     // flush container

    // create prefetching stream,
    buf_istream_type in(first.bid(), last.bid() + ((last.block_offset()) ? 1 : 0), 2);
    // create buffered write stream for blocks
    buf_ostream_type out(first.bid(), 2);

    ExtIterator _cur = first - first.block_offset();

    // leave part of the block before _begin untouched (e.g. copy)
    for ( ; _cur != first; ++_cur)
    {
        typename ExtIterator::value_type tmp;
        in >> tmp;
        out << tmp;
    }

    ///// Reading input /////////////////////

    // distribute input into random buckets
    size_t random_bucket = 0;
    for (i = 0; i < n; ++i, ++_cur) {
        random_bucket = rand(k);
        typename ExtIterator::value_type tmp;
        in >> tmp;
        buckets[random_bucket]->push(tmp); // reading the current input element
    }

    ///// Processing //////////////////////
    // resize buffers
    pool.resize_write(0);
    pool.resize_prefetch(PageSize);

    // remaining int space
    size_t space_left = M - k * BlockSize - PageSize * BlockSize;

    for (i = 0; i < k; i++) {
        LOG << "random_shuffle: bucket no " << i << " contains " << buckets[i]->size() << " elements";
    }

    // shuffle each bucket
    for (i = 0; i < k; i++) {
        buckets[i]->set_prefetch_aggr(PageSize);
        size = buckets[i]->size();

        // does the bucket fit into memory?
        if (size * sizeof(value_type) < space_left) {
            LOG << "random_shuffle: no recursion";

            // copy bucket into temp. array
            temp_array = new value_type[(size_t)size];
            for (j = 0; j < size; j++) {
                temp_array[j] = buckets[i]->top();
                buckets[i]->pop();
            }

            // shuffle
            potentially_parallel::
            random_shuffle(temp_array, temp_array + size, rand);

            // write back
            for (j = 0; j < size; j++) {
                typename ExtIterator::value_type tmp;
                tmp = temp_array[j];
                out << tmp;
            }

            // free memory
            delete[] temp_array;
        }
        else {
            LOG << "random_shuffle: recursion";
            // copy bucket into temp. stxxl::vector
            temp_vector = new temp_vector_type(size);

            for (j = 0; j < size; j++) {
                (*temp_vector)[j] = buckets[i]->top();
                buckets[i]->pop();
            }

            pool.resize_prefetch(0);
            space_left += PageSize * BlockSize;

            LOG << "random_shuffle: Space left: " << space_left;

            // recursive shuffle
            stxxl::random_shuffle(temp_vector->begin(),
                                  temp_vector->end(), rand, space_left);

            pool.resize_prefetch(PageSize);

            // write back
            for (j = 0; j < size; j++) {
                typename ExtIterator::value_type tmp;
                tmp = (*temp_vector)[j];
                out << tmp;
            }

            // free memory
            delete temp_vector;
        }

        // free bucket
        delete buckets[i];
        space_left += BlockSize;
    }

    delete[] buckets;

    // leave part of the block after _end untouched
    if (last.block_offset())
    {
        ExtIterator last_block_end = last + (block_type::size - last.block_offset());
        for ( ; _cur != last_block_end; ++_cur)
        {
            typename ExtIterator::value_type tmp;
            in >> tmp;
            out << tmp;
        }
    }
}

//! External equivalent of std::random_shuffle (specialization for stxxl::vector)
//! \param first begin of the range to shuffle
//! \param last end of the range to shuffle
//! \param M number of bytes for internal use
template <typename VectorConfig>
inline
void random_shuffle(
    stxxl::vector_iterator<VectorConfig> first,
    stxxl::vector_iterator<VectorConfig> last,
    size_t M)
{
    stxxl::random_number<> rand;
    stxxl::random_shuffle(first, last, rand, M);
}

//! \}

} // namespace stxxl

#endif // !STXXL_ALGO_RANDOM_SHUFFLE_HEADER
// vim: et:ts=4:sw=4
