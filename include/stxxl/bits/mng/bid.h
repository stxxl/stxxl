/***************************************************************************
 *  include/stxxl/bits/mng/bid.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_BID_HEADER
#define STXXL_BID_HEADER

#include <cstring>
#include <ostream>
#include <iomanip>

#include <stxxl/bits/io/file.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/common/simple_vector.h>

#ifndef STXXL_VERBOSE_BLOCK_LIFE_CYCLE
#define STXXL_VERBOSE_BLOCK_LIFE_CYCLE STXXL_VERBOSE2
#endif
#define FMT_BID(_bid_) "[" << (_bid_).storage->get_allocator_id() << "]0x" << std::hex << std::setfill('0') << std::setw(8) << (_bid_).offset << "/0x" << std::setw(8) << (_bid_).size


__STXXL_BEGIN_NAMESPACE

//! \ingroup mnglayer
//! \{

//! Block identifier class.
//!
//! Stores block identity, given by file and offset within the file
template <unsigned SIZE>
struct BID
{
    enum
    {
        size = SIZE,         //!< Block size
        t_size = SIZE        //!< Blocks size, given by the parameter
    };

    file * storage;          //!< pointer to the file of the block
    stxxl::int64 offset;     //!< offset within the file of the block

    BID() : storage(NULL), offset(0)
    { }

    bool valid() const
    {
        return storage != NULL;
    }

    BID(file * s, stxxl::int64 o) : storage(s), offset(o)
    { }

    BID(const BID & obj) : storage(obj.storage), offset(obj.offset)
    { }

    template <unsigned BlockSize>
    explicit BID(const BID<BlockSize> & obj) : storage(obj.storage), offset(obj.offset)
    { }

    template <unsigned BlockSize>
    BID & operator = (const BID<BlockSize> & obj)
    {
        storage = obj.storage;
        offset = obj.offset;
        return *this;
    }

    bool is_managed() const
    {
        return storage->get_allocator_id() != file::NO_ALLOCATOR;
    }
};


//! Specialization of block identifier class (BID) for variable size block size.
//!
//! Stores block identity, given by file, offset within the file, and size of the block
template <>
struct BID<0>
{
    file * storage;          //!< pointer to the file of the block
    stxxl::int64 offset;     //!< offset within the file of the block
    unsigned size;           //!< size of the block in bytes

    enum
    {
        t_size = 0           //!< Blocks size, given by the parameter
    };

    BID() : storage(NULL), offset(0), size(0)
    { }

    BID(file * f, stxxl::int64 o, unsigned s) : storage(f), offset(o), size(s)
    { }

    bool valid() const
    {
        return storage;
    }
};

template <unsigned blk_sz>
bool operator == (const BID<blk_sz> & a, const BID<blk_sz> & b)
{
    return (a.storage == b.storage) && (a.offset == b.offset) && (a.size == b.size);
}

template <unsigned blk_sz>
bool operator != (const BID<blk_sz> & a, const BID<blk_sz> & b)
{
    return (a.storage != b.storage) || (a.offset != b.offset) || (a.size != b.size);
}

template <unsigned blk_sz>
std::ostream & operator << (std::ostream & s, const BID<blk_sz> & bid)
{
    // [0x12345678|0]0x00100000/0x00010000
    // [file ptr|file id]offset/size

    s << "[" << bid.storage << "|";
    if (bid.storage)
        s << bid.storage->get_allocator_id();
    else
        s << "?";
    s << "]0x" << std::hex << std::setfill('0') << std::setw(8) << bid.offset << "/0x" << std::setw(8) << bid.size << std::dec;
    return s;
}

template <unsigned BLK_SIZE>
class BIDArray : public simple_vector< BID<BLK_SIZE> >
{
public:
    BIDArray()
        : simple_vector< BID<BLK_SIZE> >()
    {
    }

    BIDArray(unsigned_type size)
        : simple_vector< BID<BLK_SIZE> >(size)
    {
    }
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_BID_HEADER
// vim: et:ts=4:sw=4
