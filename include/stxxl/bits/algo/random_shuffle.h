/***************************************************************************
 *  include/stxxl/bits/algo/random_shuffle.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Manuel Krings
 *  Copyright (C) 2007 Markus Westphal
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_RANDOM_SHUFFLE_HEADER
#define STXXL_RANDOM_SHUFFLE_HEADER

// TODO: improve main memory consumption in recursion
//        (free stacks buffers)
// TODO: shuffle small input in internal memory


#include <stxxl/bits/stream/stream.h>
#include <stxxl/scan>
#include <stxxl/stack>


__STXXL_BEGIN_NAMESPACE


//! \addtogroup stlalgo
//! \{


template <typename Tp_, typename AllocStrategy_, typename SzTp_, typename DiffTp_,
          unsigned BlockSize_, typename PgTp_, unsigned PageSize_, typename RandomNumberGenerator_>
void random_shuffle(stxxl::vector_iterator<Tp_, AllocStrategy_, SzTp_, DiffTp_, BlockSize_, PgTp_, PageSize_> first,
                    stxxl::vector_iterator<Tp_, AllocStrategy_, SzTp_, DiffTp_, BlockSize_, PgTp_, PageSize_> last,
                    RandomNumberGenerator_ & rand,
                    unsigned_type M);


//! \brief External equivalent of std::random_shuffle
//! \param first begin of the range to shuffle
//! \param last end of the range to shuffle
//! \param rand random number generator object (functor)
//! \param M number of bytes for internal use
//! \param AS parallel disk allocation strategy
//!
//! - BlockSize_ size of the block to use for external memory data structures
//! - PageSize_ page size in blocks to use for external memory data structures
template <typename ExtIterator_,
          typename RandomNumberGenerator_,
          unsigned BlockSize_,
          unsigned PageSize_,
          typename AllocStrategy_>
void random_shuffle(ExtIterator_ first,
                    ExtIterator_ last,
                    RandomNumberGenerator_ & rand,
                    unsigned_type M,
                    AllocStrategy_ AS = STXXL_DEFAULT_ALLOC_STRATEGY())
{
    typedef typename ExtIterator_::value_type value_type;
    typedef typename stxxl::STACK_GENERATOR<value_type, stxxl::external,
                                            stxxl::grow_shrink2, PageSize_,
                                            BlockSize_, void, 0, AllocStrategy_>::result stack_type;
    typedef typename stack_type::block_type block_type;

    STXXL_VERBOSE1("random_shuffle: Plain Version");

    stxxl::int64 n = last - first; // the number of input elements

    // make sure we have at least 6 blocks + 1 page
    if (M < 6 * BlockSize_ + PageSize_ * BlockSize_) {
        STXXL_ERRMSG("random_shuffle: insufficient memory, " << M << " bytes supplied,");
        M = 6 * BlockSize_ + PageSize_ * BlockSize_;
        STXXL_ERRMSG("random_shuffle: increasing to " << M << " bytes (6 blocks + 1 page)");
    }

    int_type k = M / (3 * BlockSize_); // number of buckets


    stxxl::int64 i, j, size = 0;

    value_type * temp_array;
    typedef typename stxxl::VECTOR_GENERATOR<value_type,
                                             PageSize_, 4, BlockSize_, AllocStrategy_>::result temp_vector_type;
    temp_vector_type * temp_vector;

    stxxl::prefetch_pool<block_type> p_pool(0);               // no read buffers
    STXXL_VERBOSE1("random_shuffle: " << M / BlockSize_ - k << " write buffers for " << k << " buckets");
    stxxl::write_pool<block_type> w_pool(M / BlockSize_ - k); // M/B-k write buffers

    stack_type ** buckets;

    // create and put buckets into container
    buckets = new stack_type *[k];
    for (j = 0; j < k; j++)
        buckets[j] = new stack_type(p_pool, w_pool, 0);


    ///// Reading input /////////////////////
    typedef typename stream::streamify_traits<ExtIterator_>::stream_type input_stream;
    input_stream in = stxxl::stream::streamify(first, last);

    // distribute input into random buckets
    int_type random_bucket = 0;
    for (i = 0; i < n; ++i) {
        random_bucket = rand(k);
        buckets[random_bucket]->push(*in); // reading the current input element
        ++in;                              // go to the next input element
    }

    ///// Processing //////////////////////
    // resize buffers
    w_pool.resize(0);
    p_pool.resize(PageSize_);

    unsigned_type space_left = M - k * BlockSize_ -
                               PageSize_ * BlockSize_; // remaining int space
    ExtIterator_ Writer = first;
    ExtIterator_ it = first;

    for (i = 0; i < k; i++) {
        STXXL_VERBOSE1("random_shuffle: bucket no " << i << " contains " << buckets[i]->size() << " elements");
    }

    // shuffle each bucket
    for (i = 0; i < k; i++) {
        buckets[i]->set_prefetch_aggr(PageSize_);
        size = buckets[i]->size();

        // does the bucket fit into memory?
        if (size * sizeof(value_type) < space_left) {
            STXXL_VERBOSE1("random_shuffle: no recursion");

            // copy bucket into temp. array
            temp_array = new value_type[size];
            for (j = 0; j < size; j++) {
                temp_array[j] = buckets[i]->top();
                buckets[i]->pop();
            }

            // shuffle
            std::random_shuffle(temp_array, temp_array + size, rand);

            // write back
            for (j = 0; j < size; j++) {
                *Writer = temp_array[j];
                ++Writer;
            }

            // free memory
            delete[] temp_array;
        }
        else {
            STXXL_VERBOSE1("random_shuffle: recursion");

            // copy bucket into temp. stxxl::vector
            temp_vector = new temp_vector_type(size);

            for (j = 0; j < size; j++) {
                (*temp_vector)[j] = buckets[i]->top();
                buckets[i]->pop();
            }

            p_pool.resize(0);
            space_left += PageSize_ * BlockSize_;
            STXXL_VERBOSE1("random_shuffle: Space left: " << space_left);

            // recursive shuffle
            stxxl::random_shuffle(temp_vector->begin(),
                                  temp_vector->end(), rand, space_left);

            p_pool.resize(PageSize_);

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
        space_left += BlockSize_;
    }

    delete[] buckets;
}

//! \brief External equivalent of std::random_shuffle (specialization for stxxl::vector)
//! \param first begin of the range to shuffle
//! \param last end of the range to shuffle
//! \param rand random number generator object (functor)
//! \param M number of bytes for internal use
template <typename Tp_, typename AllocStrategy_, typename SzTp_, typename DiffTp_,
          unsigned BlockSize_, typename PgTp_, unsigned PageSize_, typename RandomNumberGenerator_>
void random_shuffle(stxxl::vector_iterator<Tp_, AllocStrategy_, SzTp_, DiffTp_, BlockSize_, PgTp_, PageSize_> first,
                    stxxl::vector_iterator<Tp_, AllocStrategy_, SzTp_, DiffTp_, BlockSize_, PgTp_, PageSize_> last,
                    RandomNumberGenerator_ & rand,
                    unsigned_type M)
{
    typedef stxxl::vector_iterator<Tp_, AllocStrategy_, SzTp_, DiffTp_, BlockSize_, PgTp_, PageSize_> ExtIterator_;
    typedef typename ExtIterator_::value_type value_type;
    typedef typename stxxl::STACK_GENERATOR<value_type, stxxl::external,
                                            stxxl::grow_shrink2, PageSize_, BlockSize_>::result stack_type;
    typedef typename stack_type::block_type block_type;

    STXXL_VERBOSE1("random_shuffle: Vector Version");

    // make sure we have at least 6 blocks + 1 page
    if (M < 6 * BlockSize_ + PageSize_ * BlockSize_) {
        STXXL_ERRMSG("random_shuffle: insufficient memory, " << M << " bytes supplied,");
        M = 6 * BlockSize_ + PageSize_ * BlockSize_;
        STXXL_ERRMSG("random_shuffle: increasing to " << M << " bytes (6 blocks + 1 page)");
    }

    stxxl::int64 n = last - first;     // the number of input elements
    int_type k = M / (3 * BlockSize_); // number of buckets

    stxxl::int64 i, j, size = 0;

    value_type * temp_array;
    typedef typename stxxl::VECTOR_GENERATOR<value_type,
                                             PageSize_, 4, BlockSize_, AllocStrategy_>::result temp_vector_type;
    temp_vector_type * temp_vector;

    stxxl::prefetch_pool<block_type> p_pool(0);               // no read buffers
    stxxl::write_pool<block_type> w_pool(M / BlockSize_ - k); // M/B-k write buffers

    stack_type ** buckets;

    // create and put buckets into container
    buckets = new stack_type *[k];
    for (j = 0; j < k; j++)
        buckets[j] = new stack_type(p_pool, w_pool, 0);


    typedef buf_istream<block_type, typename ExtIterator_::bids_container_iterator> buf_istream_type;
    typedef buf_ostream<block_type, typename ExtIterator_::bids_container_iterator> buf_ostream_type;

    first.flush();     // flush container

    // create prefetching stream,
    buf_istream_type in(first.bid(), last.bid() + ((last.block_offset()) ? 1 : 0), 2);
    // create buffered write stream for blocks
    buf_ostream_type out(first.bid(), 2);

    ExtIterator_ _cur = first - first.block_offset();

    // leave part of the block before _begin untouched (e.g. copy)
    for ( ; _cur != first; ++_cur)
    {
        typename ExtIterator_::value_type tmp;
        in >> tmp;
        out << tmp;
    }

    ///// Reading input /////////////////////

    // distribute input into random buckets
    int_type random_bucket = 0;
    for (i = 0; i < n; ++i, ++_cur) {
        random_bucket = rand(k);
        typename ExtIterator_::value_type tmp;
        in >> tmp;
        buckets[random_bucket]->push(tmp); // reading the current input element
    }

    ///// Processing //////////////////////
    // resize buffers
    w_pool.resize(0);
    p_pool.resize(PageSize_);

    unsigned_type space_left = M - k * BlockSize_ -
                               PageSize_ * BlockSize_; // remaining int space

    for (i = 0; i < k; i++) {
        STXXL_VERBOSE1("random_shuffle: bucket no " << i << " contains " << buckets[i]->size() << " elements");
    }

    // shuffle each bucket
    for (i = 0; i < k; i++) {
        buckets[i]->set_prefetch_aggr(PageSize_);
        size = buckets[i]->size();

        // does the bucket fit into memory?
        if (size * sizeof(value_type) < space_left) {
            STXXL_VERBOSE1("random_shuffle: no recursion");

            // copy bucket into temp. array
            temp_array = new value_type[size];
            for (j = 0; j < size; j++) {
                temp_array[j] = buckets[i]->top();
                buckets[i]->pop();
            }

            // shuffle
            std::random_shuffle(temp_array, temp_array + size, rand);

            // write back
            for (j = 0; j < size; j++) {
                typename ExtIterator_::value_type tmp;
                tmp = temp_array[j];
                out << tmp;
            }

            // free memory
            delete[] temp_array;
        } else {
            STXXL_VERBOSE1("random_shuffle: recursion");
            // copy bucket into temp. stxxl::vector
            temp_vector = new temp_vector_type(size);

            for (j = 0; j < size; j++) {
                (*temp_vector)[j] = buckets[i]->top();
                buckets[i]->pop();
            }

            p_pool.resize(0);
            space_left += PageSize_ * BlockSize_;

            STXXL_VERBOSE1("random_shuffle: Space left: " << space_left);

            // recursive shuffle
            stxxl::random_shuffle(temp_vector->begin(),
                                  temp_vector->end(), rand, space_left);

            p_pool.resize(PageSize_);

            // write back
            for (j = 0; j < size; j++) {
                typename ExtIterator_::value_type tmp;
                tmp = (*temp_vector)[j];
                out << tmp;
            }

            // free memory
            delete temp_vector;
        }

        // free bucket
        delete buckets[i];
        space_left += BlockSize_;
    }

    delete[] buckets;

    // leave part of the block after _end untouched
    if (last.block_offset())
    {
        ExtIterator_ _last_block_end = last + (block_type::size - last.block_offset());
        for ( ; _cur != _last_block_end; ++_cur)
        {
            typename ExtIterator_::value_type tmp;
            in >> tmp;
            out << tmp;
        }
    }
}

//! \brief External equivalent of std::random_shuffle (specialization for stxxl::vector)
//! \param first begin of the range to shuffle
//! \param last end of the range to shuffle
//! \param M number of bytes for internal use
template <typename Tp_, typename AllocStrategy_, typename SzTp_, typename DiffTp_,
          unsigned BlockSize_, typename PgTp_, unsigned PageSize_>
inline
void random_shuffle(stxxl::vector_iterator<Tp_, AllocStrategy_, SzTp_, DiffTp_, BlockSize_, PgTp_, PageSize_> first,
                    stxxl::vector_iterator<Tp_, AllocStrategy_, SzTp_, DiffTp_, BlockSize_, PgTp_, PageSize_> last,
                    unsigned_type M)
{
    stxxl::random_number<> rand;
    stxxl::random_shuffle(first, last, rand, M);
}

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_RANDOM_SHUFFLE_HEADER
// vim: et:ts=4:sw=4
