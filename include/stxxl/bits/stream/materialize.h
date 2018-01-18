/***************************************************************************
 *  include/stxxl/bits/stream/materialize.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_STREAM_MATERIALIZE_HEADER
#define STXXL_STREAM_MATERIALIZE_HEADER

#include <cassert>

#include <stxxl/vector>

namespace stxxl {

//! Stream package subnamespace.
namespace stream {

//! \addtogroup streampack
//! \{

////////////////////////////////////////////////////////////////////////
//     MATERIALIZE                                                    //
////////////////////////////////////////////////////////////////////////

//! Stores consecutively stream content to an output iterator.
//! \param in stream to be stored used as source
//! \param out output iterator used as destination
//! \return value of the output iterator after all increments,
//! i.e. points to the first unwritten value
//! \pre Output (range) is large enough to hold the all elements in the input stream
template <typename OutputIterator, typename StreamAlgorithm>
OutputIterator materialize(StreamAlgorithm& in, OutputIterator out)
{
    while (!in.empty())
    {
        *out = *in;
        ++out;
        ++in;
    }
    return out;
}

//! Stores consecutively stream content to an output iterator range \b until end of the stream or end of the iterator range is reached.
//! \param in stream to be stored used as source
//! \param outbegin output iterator used as destination
//! \param outend output end iterator, pointing beyond the output range
//! \return value of the output iterator after all increments,
//! i.e. points to the first unwritten value
//! \pre Output range is large enough to hold the all elements in the input stream
//!
//! This function is useful when you do not know the length of the stream beforehand.
template <typename OutputIterator, typename StreamAlgorithm>
OutputIterator
materialize(StreamAlgorithm& in, OutputIterator outbegin, OutputIterator outend)
{
    while ((!in.empty()) && outend != outbegin)
    {
        *outbegin = *in;
        ++outbegin;
        ++in;
    }
    return outbegin;
}

//! Stores consecutively stream content to an output \c stxxl::vector iterator \b until end of the stream or end of the iterator range is reached.
//! \param in stream to be stored used as source
//! \param outbegin output \c stxxl::vector iterator used as destination
//! \param outend output end iterator, pointing beyond the output range
//! \param nbuffers number of blocks used for overlapped writing (0 is default,
//! which equals to (2 * number_of_disks)
//! \return value of the output iterator after all increments,
//! i.e. points to the first unwritten value
//! \pre Output range is large enough to hold the all elements in the input stream
//!
//! This function is useful when you do not know the length of the stream beforehand.
template <typename StreamAlgorithm, typename VectorConfig>
stxxl::vector_iterator<VectorConfig> materialize(
    StreamAlgorithm& in,
    stxxl::vector_iterator<VectorConfig> outbegin,
    stxxl::vector_iterator<VectorConfig> outend,
    size_t nbuffers = 0)
{
    using ExtIterator = stxxl::vector_iterator<VectorConfig>;
    using ConstExtIterator = stxxl::const_vector_iterator<VectorConfig>;
    using buf_ostream_type =
              foxxll::buf_ostream<typename ExtIterator::block_type,
                                  typename ExtIterator::bids_container_iterator>;

    while (outbegin.block_offset())     //  go to the beginning of the block
    //  of the external vector
    {
        if (in.empty() || outbegin == outend)
            return outbegin;

        *outbegin = *in;
        ++outbegin;
        ++in;
    }

    if (nbuffers == 0)
        nbuffers = 2 * foxxll::config::get_instance()->disks_number();

    outbegin.flush();     // flush container

    // create buffered write stream for blocks
    buf_ostream_type outstream(outbegin.bid(), nbuffers);

    assert(outbegin.block_offset() == 0);

    // delay calling block_externally_updated() until the block is
    // completely filled (and written out) in outstream
    ConstExtIterator prev_block = outbegin;

    while (!in.empty() && outend != outbegin)
    {
        if (outbegin.block_offset() == 0) {
            if (prev_block != outbegin) {
                prev_block.block_externally_updated();
                prev_block = outbegin;
            }
        }

        *outstream = *in;
        ++outbegin;
        ++outstream;
        ++in;
    }

    ConstExtIterator const_out = outbegin;

    while (const_out.block_offset())     // filling the rest of the block
    {
        *outstream = *const_out;
        ++const_out;
        ++outstream;
    }

    if (prev_block != outbegin)
        prev_block.block_externally_updated();

    outbegin.flush();

    return outbegin;
}

//! Stores consecutively stream content to an output \c stxxl::vector iterator.
//! \param in stream to be stored used as source
//! \param out output \c stxxl::vector iterator used as destination
//! \param nbuffers number of blocks used for overlapped writing (0 is default,
//! which equals to (2 * number_of_disks)
//! \return value of the output iterator after all increments,
//! i.e. points to the first unwritten value
//! \pre Output (range) is large enough to hold the all elements in the input stream
template <typename StreamAlgorithm, typename VectorConfig>
stxxl::vector_iterator<VectorConfig> materialize(
    StreamAlgorithm& in,
    stxxl::vector_iterator<VectorConfig> out,
    size_t nbuffers = 0)
{
    using ExtIterator = stxxl::vector_iterator<VectorConfig>;
    using ConstExtIterator = stxxl::const_vector_iterator<VectorConfig>;
    using buf_ostream_type =
              foxxll::buf_ostream<typename ExtIterator::block_type,
                                  typename ExtIterator::bids_container_iterator>;

    // on the I/O complexity of "materialize":
    // crossing block boundary causes O(1) I/Os
    // if you stay in a block, then materialize function accesses only the cache of the
    // vector (only one block indeed), amortized complexity should apply here

    while (out.block_offset())     //  go to the beginning of the block
    //  of the external vector
    {
        if (in.empty())
            return out;

        *out = *in;
        ++out;
        ++in;
    }

    if (nbuffers == 0)
        nbuffers = 2 * foxxll::config::get_instance()->disks_number();

    out.flush();     // flush container

    // create buffered write stream for blocks
    buf_ostream_type outstream(out.bid(), nbuffers);

    assert(out.block_offset() == 0);

    // delay calling block_externally_updated() until the block is
    // completely filled (and written out) in outstream
    ConstExtIterator prev_block = out;

    while (!in.empty())
    {
        if (out.block_offset() == 0) {
            if (prev_block != out) {
                prev_block.block_externally_updated();
                prev_block = out;
            }
        }

        // tells the vector that the block was modified
        *outstream = *in;
        ++out;
        ++outstream;
        ++in;
    }

    ConstExtIterator const_out = out;

    // copy over items remaining in block from vector.
    while (const_out.block_offset())
    {
        *outstream = *const_out;                 // might cause I/Os for loading the page that
        ++const_out;                             // contains data beyond out
        ++outstream;
    }

    if (prev_block != out)
        prev_block.block_externally_updated();

    out.flush();

    return out;
}

//! Reads stream content and discards it.
//! Useful where you do not need the processed stream anymore,
//! but are just interested in side effects, or just for debugging.
//! \param in input stream
template <typename StreamAlgorithm>
void discard(StreamAlgorithm& in)
{
    while (!in.empty())
    {
        *in;
        ++in;
    }
}

//! \}

} // namespace stream
} // namespace stxxl

#endif // !STXXL_STREAM_MATERIALIZE_HEADER
// vim: et:ts=4:sw=4
