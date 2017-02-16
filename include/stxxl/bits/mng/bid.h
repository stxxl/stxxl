/***************************************************************************
 *  include/stxxl/bits/mng/bid.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG_BID_HEADER
#define STXXL_MNG_BID_HEADER

#include <stxxl/bits/common/simple_vector.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/io/file.h>

#include <cstring>
#include <iomanip>
#include <ostream>

#ifndef STXXL_VERBOSE_BLOCK_LIFE_CYCLE
#define STXXL_VERBOSE_BLOCK_LIFE_CYCLE STXXL_VERBOSE2
#endif
#define FMT_BID(_bid_) "[" << (_bid_).storage->get_allocator_id() << "]0x" << std::hex << std::setfill('0') << std::setw(8) << (_bid_).offset << "/0x" << std::setw(8) << (_bid_).size

namespace stxxl {

//! \addtogroup mnglayer
//! \{

//! Block identifier class.
//!
//! Stores block identity, given by file and offset within the file
template <size_t Size>
struct BID
{
    //! Block size
    static constexpr size_t size = Size;
    //! Blocks size, given by the parameter
    static constexpr size_t t_size = Size;

    //! pointer to the file of the block
    file* storage = nullptr;
    //! offset within the file of the block (uint64_t)
    external_size_type offset = 0;

    BID() = default;

    BID(file* s, const external_size_type& o) : storage(s), offset(o) { }

    template <size_t BlockSize>
    explicit BID(const BID<BlockSize>& obj)
        : storage(obj.storage), offset(obj.offset) { }

    template <size_t BlockSize>
    BID& operator = (const BID<BlockSize>& obj)
    {
        storage = obj.storage;
        offset = obj.offset;
        return *this;
    }

    bool valid() const
    {
        return storage != nullptr;
    }

    bool is_managed() const
    {
        return storage->get_allocator_id() != file::NO_ALLOCATOR;
    }

    //! Writes data to the disk(s).
    request_ptr write(void* data, size_t data_size,
                      completion_handler on_complete = completion_handler())
    {
        return storage->awrite(data, offset, data_size, on_complete);
    }

    //! Reads data from the disk(s).
    request_ptr read(void* data, size_t data_size,
                     completion_handler on_complete = completion_handler())
    {
        return storage->aread(data, offset, data_size, on_complete);
    }

    bool operator == (const BID<Size>& b) const
    {
        return storage == b.storage && offset == b.offset;
    }

    bool operator != (const BID<Size>& b) const
    {
        return !operator == (b);
    }
};

/*!
 * Specialization of block identifier class (BID) for variable size block size.
 *
 * Stores block identity, given by file, offset within the file, and size of the
 * block
 */
template <>
struct BID<0>
{
    //! pointer to the file of the block
    file* storage = nullptr;
    //! offset within the file of the block (uint64_t)
    external_size_type offset = 0;
    //! size of the block in bytes
    size_t size = 0;

    //! Blocks size, given by the parameter
    static constexpr size_t t_size = 0;

    BID() = default;

    BID(file* f, const external_size_type& o, size_t s)
        : storage(f), offset(o), size(s) { }

    bool valid() const
    {
        return (storage != nullptr);
    }

    bool is_managed() const
    {
        return storage->get_allocator_id() != file::NO_ALLOCATOR;
    }

    //! Writes data to the disk(s).
    request_ptr write(void* data, size_t data_size,
                      completion_handler on_complete = completion_handler())
    {
        return storage->awrite(data, offset, data_size, on_complete);
    }

    //! Reads data from the disk(s).
    request_ptr read(void* data, size_t data_size,
                     completion_handler on_complete = completion_handler())
    {
        return storage->aread(data, offset, data_size, on_complete);
    }

    bool operator == (const BID<0>& b) const
    {
        return storage == b.storage && offset == b.offset && size == b.size;
    }

    bool operator != (const BID<0>& b) const
    {
        return !operator == (b);
    }
};

template <size_t BlockSize>
std::ostream& operator << (std::ostream& s, const BID<BlockSize>& bid)
{
    // [0x12345678|0]0x00100000/0x00010000
    // [file ptr|file id]offset/size

    std::ios state(nullptr);
    state.copyfmt(s);

    s << "[" << bid.storage << "|";
    if (bid.storage)
        s << bid.storage->get_allocator_id();
    else
        s << "?";
    s << "]0x" << std::hex << std::setfill('0') << std::setw(8) << bid.offset << "/0x" << std::setw(8) << bid.size << std::dec;

    s.copyfmt(state);
    return s;
}

template <size_t BlockSize>
using BIDArray = simple_vector<BID<BlockSize> >;

//! \}

} // namespace stxxl

#endif // !STXXL_MNG_BID_HEADER
// vim: et:ts=4:sw=4
