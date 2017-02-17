/***************************************************************************
 *  include/stxxl/bits/mng/typed_block.h
 *
 *  Constructs a typed_block object containing as many elements elements plus
 *  some metadata as fits into the given block size.
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG_TYPED_BLOCK_HEADER
#define STXXL_MNG_TYPED_BLOCK_HEADER

#include <stxxl/bits/common/aligned_alloc.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/mng/bid.h>

#ifndef STXXL_VERBOSE_TYPED_BLOCK
#define STXXL_VERBOSE_TYPED_BLOCK STXXL_VERBOSE2
#endif

namespace stxxl {

//! \addtogroup mnglayer
//! \{

//! Block Manager Internals \internal
namespace mng_local {

//! \defgroup mnglayer_internals Internals
//! \ingroup mnglayer
//! Internals and support classes
//! \{

template <size_t Bytes>
class filler_struct
{
    using byte_type = unsigned char;
    byte_type filler_array[Bytes];

public:
    filler_struct() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] filler_struct is constructed"); }
};

template <>
class filler_struct<0>
{
    using byte_type = unsigned char;

public:
    filler_struct() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] filler_struct<> is constructed"); }
};

//! Contains data elements for \c stxxl::typed_block , not intended for direct use.
template <typename Type, size_t Size>
class element_block
{
public:
    using type = Type;
    using value_type = Type;
    using reference = Type &;
    using const_reference = const Type &;
    using pointer = type *;
    using iterator = pointer;
    using const_iterator = const type *;

    static constexpr size_t size = Size; //!< number of elements in the block

    //! Array of elements of type Type
    value_type elem[size];

    element_block() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] element_block is constructed"); }

    //! An operator to access elements in the block
    reference operator [] (size_t i)
    {
        return elem[i];
    }

    //! Returns \c iterator pointing to the first element.
    iterator begin()
    {
        return elem;
    }

    //! Returns \c const_iterator pointing to the first element.
    const_iterator begin() const
    {
        return elem;
    }

    //! Returns \c const_iterator pointing to the first element.
    const_iterator cbegin() const
    {
        return begin();
    }

    //! Returns \c iterator pointing to the end element.
    iterator end()
    {
        return elem + size;
    }

    //! Returns \c const_iterator pointing to the end element.
    const_iterator end() const
    {
        return elem + size;
    }

    //! Returns \c const_iterator pointing to the end element.
    const_iterator cend() const
    {
        return end();
    }
};

//! Contains BID references for \c stxxl::typed_block , not intended for direct use.
template <typename Type, size_t Size, size_t RawSize, size_t NBids = 0>
class block_w_bids : public element_block<Type, Size>
{
public:
    static constexpr size_t raw_size = RawSize;
    static constexpr size_t kNBIDs = NBids;

    using bid_type = BID<raw_size>;

    //! Array of BID references
    bid_type ref[kNBIDs];

    //! An operator to access bid references
    bid_type& operator () (size_t i)
    {
        return ref[i];
    }

    block_w_bids() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] block_w_bids is constructed"); }
};

template <typename Type, size_t Size, size_t RawSize>
class block_w_bids<Type, Size, RawSize, 0>
    : public element_block<Type, Size>
{
public:
    static constexpr size_t raw_size = RawSize;
    static constexpr size_t kNBIDs = 0;

    using bid_type = BID<raw_size>;

    block_w_bids() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] block_w_bids<> is constructed"); }
};

//! Contains per block information for \c stxxl::typed_block , not intended for direct use.
template <typename Type, size_t RawSize, size_t NBids, typename MetaInfoType = void>
class block_w_info
    : public block_w_bids<Type, ((RawSize - sizeof(BID<RawSize>)* NBids - sizeof(MetaInfoType)) / sizeof(Type)), RawSize, NBids>
{
public:
    //! Type of per block information element.
    using info_type = MetaInfoType;

    //! Per block information element.
    info_type info;

    block_w_info() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] block_w_info is constructed"); }
};

template <typename Type, size_t RawSize, size_t NBids>
class block_w_info<Type, RawSize, NBids, void>
    : public block_w_bids<Type, ((RawSize - sizeof(BID<RawSize>)* NBids) / sizeof(Type)), RawSize, NBids>
{
public:
    using info_type = void;

    block_w_info() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] block_w_info<> is constructed"); }
};

//! Contains per block filler for \c stxxl::typed_block , not intended for direct use.
template <typename BaseType, size_t FillSize = 0>
class add_filler : public BaseType
{
private:
    //! Per block filler element.
    filler_struct<FillSize> filler;

public:
    add_filler() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] add_filler is constructed"); }
};

template <typename BaseType>
class add_filler<BaseType, 0>
    : public BaseType
{
public:
    add_filler() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] add_filler<> is constructed"); }
};

//! Helper to compute the size of the filler , not intended for direct use.
template <typename Type, size_t RawSize>
class expand_struct : public add_filler<Type, RawSize - sizeof(Type)>
{ };

//! \}

} // namespace mng_local

//! Block containing elements of fixed length.
//!
//! \tparam RawSize size of block in bytes
//! \tparam Type type of block's records
//! \tparam NRef number of block references (BIDs) that can be stored in the block (default is 0)
//! \tparam MetaInfoType type of per block information (default is no information - void)
//!
//! The data array of type Type is contained in the parent class \c stxxl::element_block, see related information there.
//! The BID array of references is contained in the parent class \c stxxl::block_w_bids, see related information there.
//! The "per block information" is contained in the parent class \c stxxl::block_w_info, see related information there.
//!  \warning If \c RawSize > 2MB object(s) of this type can not be allocated on the stack (as a
//! function variable for example), because Linux POSIX library limits the stack size for the
//! main thread to (2MB - system page size)
template <size_t RawSize, typename Type, size_t NRef = 0, typename MetaInfoType = void>
class typed_block
    : public mng_local::expand_struct<mng_local::block_w_info<Type, RawSize, NRef, MetaInfoType>, RawSize>
{
    using Base = mng_local::expand_struct<mng_local::block_w_info<Type, RawSize, NRef, MetaInfoType>, RawSize>;

public:
    using value_type = Type;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using iterator = pointer;
    using const_pointer = const value_type *;
    using const_iterator = const_pointer;

    static constexpr size_t raw_size = RawSize;                                      //!< size of block in bytes
    static constexpr size_t size = Base::size;                                       //!< number of elements in block
    static constexpr bool has_only_data = (raw_size == (size * sizeof(value_type))); //!< no meta info, bids or (non-empty) fillers included in the block, allows value_type array addressing across block boundaries

    using bid_type = BID<raw_size>;

    typed_block()
    {
        static_assert(sizeof(typed_block) == raw_size,
                      "sizeof(typed_block) == raw_size");
        STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] typed_block is constructed");
#if 0
        assert(((long)this) % STXXL_BLOCK_ALIGN == 0);
#endif
    }

#if 0
    typed_block(const typed_block& tb)
    {
        static_assert(sizeof(typed_block) == raw_size,
                      "sizeof(typed_block) == raw_size");
        STXXL_MSG("[" << (void*)this << "] typed_block is copy constructed from [" << (void*)&tb << "]");
        STXXL_UNUSED(tb);
    }
#endif

    /*!
     * Writes block to the disk(s).
     * \param bid block identifier, points the file(disk) and position
     * \param on_complete completion handler
     * \return \c pointer_ptr object to track status I/O operation after the call
     */
    request_ptr write(const bid_type& bid,
                      completion_handler on_complete = completion_handler())
    {
        STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:write  " << FMT_BID(bid));
        return bid.storage->awrite(this, bid.offset, raw_size, on_complete);
    }

    /*!
     * Reads block from the disk(s).
     * \param bid block identifier, points the file(disk) and position
     * \param on_complete completion handler
     * \return \c pointer_ptr object to track status I/O operation after the call
     */
    request_ptr read(const bid_type& bid,
                     completion_handler on_complete = completion_handler())
    {
        STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:read   " << FMT_BID(bid));
        return bid.storage->aread(this, bid.offset, raw_size, on_complete);
    }

    /*!
     * Writes block to the disk(s).
     * \param bid block identifier, points the file(disk) and position
     * \param on_complete completion handler
     * \return \c pointer_ptr object to track status I/O operation after the call
     */
    request_ptr write(const BID<0>& bid,
                      completion_handler on_complete = completion_handler())
    {
        STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:write  " << FMT_BID(bid));
        assert(bid.size >= raw_size);
        return bid.storage->awrite(this, bid.offset, raw_size, on_complete);
    }

    /*!
     * Reads block from the disk(s).
     * \param bid block identifier, points the file(disk) and position
     * \param on_complete completion handler
     * \return \c pointer_ptr object to track status I/O operation after the call
     */
    request_ptr read(const BID<0>& bid,
                     completion_handler on_complete = completion_handler())
    {
        STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:read   " << FMT_BID(bid));
        assert(bid.size >= raw_size);
        return bid.storage->aread(this, bid.offset, raw_size, on_complete);
    }

    static void* operator new (size_t bytes)
    {
        size_t meta_info_size = bytes % raw_size;
        STXXL_VERBOSE_TYPED_BLOCK("typed::block operator new[]: bytes=" << bytes << ", meta_info_size=" << meta_info_size);

        void* result = aligned_alloc<STXXL_BLOCK_ALIGN>(
            bytes - meta_info_size, meta_info_size);

#if STXXL_WITH_VALGRIND || STXXL_TYPED_BLOCK_INITIALIZE_ZERO
        memset(result, 0, bytes);
#endif
        return result;
    }

    static void* operator new[] (size_t bytes)
    {
        size_t meta_info_size = bytes % raw_size;
        STXXL_VERBOSE_TYPED_BLOCK("typed::block operator new[]: bytes=" << bytes << ", meta_info_size=" << meta_info_size);

        void* result = aligned_alloc<STXXL_BLOCK_ALIGN>(
            bytes - meta_info_size, meta_info_size);

#if STXXL_WITH_VALGRIND || STXXL_TYPED_BLOCK_INITIALIZE_ZERO
        memset(result, 0, bytes);
#endif
        return result;
    }

    static void* operator new (size_t /*bytes*/, void* ptr)       // construct object in existing memory
    {
        return ptr;
    }

    static void operator delete (void* ptr)
    {
        aligned_dealloc<STXXL_BLOCK_ALIGN>(ptr);
    }

    static void operator delete[] (void* ptr)
    {
        aligned_dealloc<STXXL_BLOCK_ALIGN>(ptr);
    }

    static void operator delete (void*, void*)
    { }

#if 1
    // STRANGE: implementing destructor makes g++ allocate
    // additional 4 bytes in the beginning of every array
    // of this type !? makes aligning to 4K boundaries difficult
    //
    // http://www.cc.gatech.edu/grads/j/Seung.Won.Jun/tips/pl/node4.html :
    // "One interesting thing is the array allocator requires more memory
    //  than the array size multiplied by the size of an element, by a
    //  difference of delta for metadata a compiler needs. It happens to
    //  be 8 bytes long in g++."
    ~typed_block()
    {
        STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] typed_block is destructed");
    }
#endif
};

//! \}

} // namespace stxxl

#endif // !STXXL_MNG_TYPED_BLOCK_HEADER
// vim: et:ts=4:sw=4
