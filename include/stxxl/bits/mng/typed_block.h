/***************************************************************************
 *  include/stxxl/bits/mng/typed_block.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_TYPED_BLOCK_HEADER
#define STXXL_TYPED_BLOCK_HEADER

#include <stxxl/bits/common/aligned_alloc.h>
#include <stxxl/bits/common/debug.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/mng/mng.h>

#ifndef STXXL_VERBOSE_TYPED_BLOCK
#define STXXL_VERBOSE_TYPED_BLOCK STXXL_VERBOSE2
#endif


__STXXL_BEGIN_NAMESPACE

//! \ingroup mnglayer
//! \{


template <unsigned bytes>
class filler_struct__
{
    typedef unsigned char byte_type;
    byte_type filler_array_[bytes];

public:
    filler_struct__() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] filler_struct__ is constructed"); }
};

template <>
class filler_struct__<0>
{
    typedef unsigned char byte_type;

public:
    filler_struct__() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] filler_struct__<> is constructed"); }
};

//! \brief Contains data elements for \c stxxl::typed_block , not intended for direct use
template <class T, unsigned Size_>
class element_block
{
public:
    typedef T type;
    typedef T value_type;
    typedef T & reference;
    typedef const T & const_reference;
    typedef type * pointer;
    typedef pointer iterator;
    typedef const type * const_iterator;

    enum
    {
        size = Size_ //!< number of elements in the block
    };

    //! Array of elements of type T
    T elem[size];

    element_block() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] element_block is constructed"); }

    //! An operator to access elements in the block
    reference operator [] (int i)
    {
        return elem[i];
    }

    //! \brief Returns \c iterator pointing to the first element
    iterator begin()
    {
        return elem;
    }
    //! \brief Returns \c const_iterator pointing to the first element
    const_iterator begin() const
    {
        return elem;
    }
    //! \brief Returns \c const_iterator pointing to the first element
    const_iterator cbegin() const
    {
        return begin();
    }
    //! \brief Returns \c iterator pointing to the end element
    iterator end()
    {
        return elem + size;
    }
    //! \brief Returns \c const_iterator pointing to the end element
    const_iterator end() const
    {
        return elem + size;
    }
    //! \brief Returns \c const_iterator pointing to the end element
    const_iterator cend() const
    {
        return end();
    }
};

//! \brief Contains BID references for \c stxxl::typed_block , not intended for direct use
template <class T, unsigned Size_, unsigned RawSize_, unsigned NBids_ = 0>
class block_w_bids : public element_block<T, Size_>
{
public:
    enum
    {
        raw_size = RawSize_,
        nbids = NBids_
    };
    typedef BID<raw_size> bid_type;

    //! Array of BID references
    bid_type ref[nbids];

    //! An operator to access bid references
    bid_type & operator () (int i)
    {
        return ref[i];
    }

    block_w_bids() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] block_w_bids is constructed"); }
};

template <class T, unsigned Size_, unsigned RawSize_>
class block_w_bids<T, Size_, RawSize_, 0>: public element_block<T, Size_>
{
public:
    enum
    {
        raw_size = RawSize_,
        nbids = 0
    };
    typedef BID<raw_size> bid_type;

    block_w_bids() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] block_w_bids<> is constructed"); }
};

//! \brief Contains per block information for \c stxxl::typed_block , not intended for direct use
template <class T_, unsigned RawSize_, unsigned NBids_, class InfoType_ = void>
class block_w_info :
    public block_w_bids<T_, ((RawSize_ - sizeof(BID<RawSize_>) * NBids_ - sizeof(InfoType_)) / sizeof(T_)), RawSize_, NBids_>
{
public:
    //! \brief Type of per block information element
    typedef InfoType_ info_type;

    //! \brief Per block information element
    info_type info;

    enum { size = ((RawSize_ - sizeof(BID<RawSize_>) * NBids_ - sizeof(InfoType_)) / sizeof(T_)) };

public:
    block_w_info() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] block_w_info is constructed"); }
};

template <class T_, unsigned RawSize_, unsigned NBids_>
class block_w_info<T_, RawSize_, NBids_, void>:
    public block_w_bids<T_, ((RawSize_ - sizeof(BID<RawSize_>) * NBids_) / sizeof(T_)), RawSize_, NBids_>
{
public:
    typedef void info_type;
    enum { size = ((RawSize_ - sizeof(BID<RawSize_>) * NBids_) / sizeof(T_)) };

public:
    block_w_info() { STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] block_w_info<> is constructed"); }
};

//! \brief Block containing elements of fixed length

//! Template parameters:
//! - \c RawSize_ size of block in bytes
//! - \c T_ type of block's records
//! - \c NRef_ number of block references (BIDs) that can be stored in the block (default is 0)
//! - \c InfoType_ type of per block information (default is no information - void)
//!
//! The data array of type T_ is contained in the parent class \c stxxl::element_block, see related information there.
//! The BID array of references is contained in the parent class \c stxxl::block_w_bids, see related information there.
//! The "per block information" is contained in the parent class \c stxxl::block_w_info, see related information there.
//!  \warning If \c RawSize_ > 2MB object(s) of this type can not be allocated on the stack (as a
//! function variable for example), because Linux POSIX library limits the stack size for the
//! main thread to (2MB - system page size)
template <unsigned RawSize_, class T_, unsigned NRef_ = 0, class InfoType_ = void>
class typed_block :
    public block_w_info<T_, RawSize_, NRef_, InfoType_>,
    public filler_struct__<(RawSize_ - sizeof(block_w_info<T_, RawSize_, NRef_, InfoType_>))>
{
public:
    typedef T_ type;
    typedef T_ value_type;
    typedef T_ & reference;
    typedef const T_ & const_reference;
    typedef type * pointer;
    typedef pointer iterator;
    typedef type const * const_iterator;

    enum { has_filler = (RawSize_ != sizeof(block_w_info<T_, RawSize_, NRef_, InfoType_>)) };

    typedef BID<RawSize_> bid_type;

    typed_block()
    {
        STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] typed_block is constructed");
        STXXL_STATIC_ASSERT(sizeof(typed_block<RawSize_, T_, NRef_, InfoType_>) == RawSize_);
    }

    enum
    {
        raw_size = RawSize_,                                      //!< size of block in bytes
        size = block_w_info<T_, RawSize_, NRef_, InfoType_>::size //!< number of elements in block
    };

    /*! \brief Writes block to the disk(s)
     *! \param bid block identifier, points the file(disk) and position
     *! \param on_cmpl completion handler
     *! \return \c pointer_ptr object to track status I/O operation after the call
     */
    request_ptr write(const BID<raw_size> & bid,
                      completion_handler on_cmpl = default_completion_handler())
    {
        STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:write  " << FMT_BID(bid));
        return bid.storage->awrite(
                   this,
                   bid.offset,
                   raw_size,
                   on_cmpl);
    }

    /*! \brief Reads block from the disk(s)
     *! \param bid block identifier, points the file(disk) and position
     *! \param on_cmpl completion handler
     *! \return \c pointer_ptr object to track status I/O operation after the call
     */
    request_ptr read(const BID<raw_size> & bid,
                     completion_handler on_cmpl = default_completion_handler())
    {
        STXXL_VERBOSE_BLOCK_LIFE_CYCLE("BLC:read   " << FMT_BID(bid));
        return bid.storage->aread(this, bid.offset, raw_size, on_cmpl);
    }

    static void * operator new (size_t bytes)
    {
        unsigned_type meta_info_size = bytes % raw_size;
        STXXL_VERBOSE1("typed::block operator new: Meta info size: " << meta_info_size);

        void * result = aligned_alloc<BLOCK_ALIGN>(bytes - meta_info_size, meta_info_size);
        #ifdef STXXL_VALGRIND_TYPED_BLOCK_INITIALIZE_ZERO
        memset(result, 0, bytes);
        #endif
        // FIXME: these STXXL_DEBUGMON_DO calls do not look sane w.r.t. meta_info_size != 0
        char * tmp = (char *)result;
        STXXL_DEBUGMON_DO(block_allocated(tmp, tmp + bytes, RawSize_));
        tmp += RawSize_;
        while (tmp < ((char *)result) + bytes)
        {
            STXXL_DEBUGMON_DO(block_allocated(tmp, ((char *)result) + bytes, RawSize_));
            tmp += RawSize_;
        }
        return result;
    }

    static void * operator new[] (size_t bytes)
    {
        unsigned_type meta_info_size = bytes % raw_size;
        STXXL_VERBOSE1("typed::block operator new[]: Meta info size: " << meta_info_size);

        void * result = aligned_alloc<BLOCK_ALIGN>(bytes - meta_info_size, meta_info_size);
        #ifdef STXXL_VALGRIND_TYPED_BLOCK_INITIALIZE_ZERO
        memset(result, 0, bytes);
        #endif
        // FIXME: these STXXL_DEBUGMON_DO calls do not look sane w.r.t. meta_info_size != 0
        char * tmp = (char *)result;
        STXXL_DEBUGMON_DO(block_allocated(tmp, tmp + bytes, RawSize_));
        tmp += RawSize_;
        while (tmp < ((char *)result) + bytes)
        {
            STXXL_DEBUGMON_DO(block_allocated(tmp, ((char *)result) + bytes, RawSize_));
            tmp += RawSize_;
        }
        return result;
    }

    static void * operator new (size_t /*bytes*/, void * ptr)     // construct object in existing memory
    {
        return ptr;
    }

    static void operator delete (void * ptr)
    {
        STXXL_DEBUGMON_DO(block_deallocated(ptr));
        aligned_dealloc<BLOCK_ALIGN>(ptr);
    }

    static void operator delete[] (void * ptr)
    {
        STXXL_DEBUGMON_DO(block_deallocated(ptr));
        aligned_dealloc<BLOCK_ALIGN>(ptr);
    }

    static void operator delete (void *, void *)
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
    ~typed_block() { 
        STXXL_VERBOSE_TYPED_BLOCK("[" << (void*)this << "] typed_block is destructed");
    }
#endif
};


//! \}

__STXXL_END_NAMESPACE


#endif // !STXXL_TYPED_BLOCK_HEADER
// vim: et:ts=4:sw=4
