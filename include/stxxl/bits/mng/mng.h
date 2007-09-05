#ifndef MNG_HEADER
#define MNG_HEADER

/***************************************************************************
 *            mng.h
 *
 *  Sat Aug 24 23:55:27 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/io"
#include "stxxl/bits/common/rand.h"
#include "stxxl/bits/common/aligned_alloc.h"
#include "stxxl/bits/common/debug.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <string>
#include <cstdlib>

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef BOOST_MSVC
#include <memory.h>
#endif


__STXXL_BEGIN_NAMESPACE

//! \defgroup mnglayer Block management layer
//! Group of classes which help controlling external memory space,
//! managing disks, and allocating and deallocating blocks of external storage
//! \{

//! \brief Block identifier class

//! Stores block identity, given by file and offset within the file
template < unsigned SIZE >
struct BID
{
    enum
    {
        size = SIZE,      //!< Block size
        t_size = SIZE        //!< Blocks size, given by the parameter
    };
    file * storage;     //!< pointer to the file of the block
    stxxl::int64 offset;     //!< offset within the file of the block
    BID() : storage(NULL), offset(0) { }
    bool valid() const
    {
        return storage;
    }
    BID(file * s, stxxl::int64 o) : storage(s), offset(o) { }
    BID(const BID &obj) : storage(obj.storage), offset(obj.offset) { }
    template <unsigned BlockSize>
    explicit BID(const BID<BlockSize> & obj) : storage(obj.storage), offset(obj.offset) { }
};



//! \brief Specialization of block identifier class (BID) for variable size block size

//! Stores block identity, given by file, offset within the file, and size of the block
template <>
struct BID < 0 >
{
    file * storage;     //!< pointer to the file of the block
    stxxl::int64 offset;     //!< offset within the file of the block
    unsigned size;  //!< size of the block in bytes
    enum
    {
        t_size = 0        //!< Blocks size, given by the parameter
    };
    BID() : storage(NULL), offset(0), size(0) { }
    BID(file * f, stxxl::int64 o, unsigned s) : storage(f), offset(o), size(s) { }
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
    s << " storage file addr: " << bid.storage;
    s << " offset: " << bid.offset;
    s << " size: " << bid.size;
    return s;
}


template <unsigned bytes>
class filler_struct__
{
    typedef unsigned char byte_type;
    byte_type filler_array_[bytes];
public:
    filler_struct__ () { STXXL_VERBOSE2("filler_struct__ is allocated"); }
};

template <>
class filler_struct__ < 0 >
{
    typedef unsigned char byte_type;
public:
    filler_struct__ () { STXXL_VERBOSE2("filler_struct__ is allocated"); }
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

    element_block () { STXXL_VERBOSE2("element_block is allocated"); }

    //! An operator to access elements in the block
    reference operator [](int i)
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
    bid_type & operator ()(int i)
    {
        return ref[i];
    }

    block_w_bids () { STXXL_VERBOSE2("block_w_bids is allocated"); }
};

template <class T, unsigned Size_, unsigned RawSize_>
class block_w_bids < T, Size_, RawSize_, 0 > : public element_block<T, Size_>
{
public:
    enum
    {
        raw_size = RawSize_,
        nbids = 0
    };
    typedef BID<raw_size> bid_type;

    block_w_bids () { STXXL_VERBOSE2("block_w_bids is allocated"); }
};

//! \brief Contains per block information for \c stxxl::typed_block , not intended for direct use
template <class T_, unsigned RawSize_, unsigned NBids_, class InfoType_ = void>
class block_w_info :
    public block_w_bids < T_, ((RawSize_ - sizeof(BID < RawSize_ > ) * NBids_ - sizeof(InfoType_)) / sizeof(T_)), RawSize_, NBids_ >
{
public:
    //! \brief Type of per block information element
    typedef InfoType_ info_type;

    //! \brief Per block information element
    info_type info;

    enum { size = ((RawSize_ - sizeof(BID < RawSize_ >) * NBids_ - sizeof(InfoType_) ) / sizeof(T_)) };
public:
    block_w_info () { STXXL_VERBOSE2("block_w_info is allocated"); }
};

template <class T_, unsigned RawSize_, unsigned NBids_>
class block_w_info<T_, RawSize_, NBids_, void> :
    public block_w_bids < T_, ((RawSize_ - sizeof(BID < RawSize_ >) * NBids_) / sizeof(T_)), RawSize_, NBids_ >
{
public:
    typedef void info_type;
    enum {size = ((RawSize_ - sizeof(BID < RawSize_ >) * NBids_) / sizeof(T_)) };
public:
    block_w_info () { STXXL_VERBOSE2("block_w_info is allocated"); }
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
    public filler_struct__ < (RawSize_ - sizeof(block_w_info < T_, RawSize_, NRef_, InfoType_ >)) >
{
public:
    typedef T_ type;
    typedef T_ value_type;
    typedef T_ & reference;
    typedef const T_ & const_reference;
    typedef type * pointer;
    typedef pointer iterator;
    typedef type const * const_iterator;

    enum { has_filler = (RawSize_ != sizeof(block_w_info < T_, RawSize_, NRef_, InfoType_ >) ) };

    typedef BID<RawSize_> bid_type;

    typed_block()
    {
        STXXL_VERBOSE2("typed_block is allocated");
    }

    enum
    {
        raw_size = RawSize_,     //!< size of block in bytes
        size = block_w_info < T_, RawSize_, NRef_, InfoType_ > ::size //!< number of elements in block
    };

    /*! \brief Writes block to the disk(s)
     \param bid block identifier, points the file(disk) and position
     \param on_cmpl completion handler
     \return \c pointer_ptr object to track status I/O operation after the call
     */
    request_ptr write (const BID<raw_size> & bid,
                       completion_handler on_cmpl = default_completion_handler())
    {
        return bid.storage->awrite(
                   this,
                   bid.offset,
                   raw_size,
                   on_cmpl);
    }

    /*!     \brief Reads block from the disk(s)
     \param bid block identifier, points the file(disk) and position
     \param on_cmpl completion handler
     \return \c pointer_ptr object to track status I/O operation after the call
     */
    request_ptr read (const BID < raw_size > &bid,
                      completion_handler on_cmpl = default_completion_handler())
    {
        return bid.storage->aread(this, bid.offset, raw_size, on_cmpl);
    }

    static void *operator new[] (size_t bytes)
    {
        unsigned_type meta_info_size = bytes % raw_size;
        STXXL_VERBOSE1("typed::block operator new: Meta info size: " << meta_info_size);

        void * result = aligned_alloc < BLOCK_ALIGN > (bytes, meta_info_size);
        char * tmp = (char *)result;
        debugmon::get_instance()->block_allocated(tmp, tmp + bytes, RawSize_);
        tmp += RawSize_;
        while (tmp < ((char *)result) + bytes)
        {
            debugmon::get_instance()->block_allocated(tmp, ((char *)result) + bytes, RawSize_);
            tmp += RawSize_;
        }
        return result;
    }
    static void *operator new (size_t bytes)
    {
        unsigned_type meta_info_size = bytes % raw_size;
        STXXL_VERBOSE1("typed::block operator new: Meta info size: " << meta_info_size);

        void * result = aligned_alloc < BLOCK_ALIGN > (bytes, meta_info_size);
        char * tmp = (char *)result;
        debugmon::get_instance()->block_allocated(tmp, tmp + bytes, RawSize_);
        tmp += RawSize_;
        while (tmp < ((char *)result) + bytes)
        {
            debugmon::get_instance()->block_allocated(tmp, ((char *)result) + bytes, RawSize_);
            tmp += RawSize_;
        }
        return result;
    }

    static void *  operator new (size_t /*bytes*/, void * ptr)     // construct object in existing memory
    {
        return ptr;
    }

    static void operator delete (void *ptr)
    {
        debugmon::get_instance()->block_deallocated((char *)ptr);
        aligned_dealloc < BLOCK_ALIGN > (ptr);
    }
    void operator delete[] (void *ptr)
    {
        debugmon::get_instance()->block_deallocated((char *)ptr);
        aligned_dealloc < BLOCK_ALIGN > (ptr);
    }

    // STRANGE: implementing destructor makes g++ allocate
    // additional 4 bytes in the beginning of every array
    // of this type !? makes aligning to 4K boundaries difficult
    //
    // http://www.cc.gatech.edu/grads/j/Seung.Won.Jun/tips/pl/node4.html :
    // "One interesting thing is the array allocator requires more memory
    //  than the array size multiplied by the size of an element, by a
    //  difference of delta for metadata a compiler needs. It happens to
    //  be 8 bytes long in g++."
    // ~typed_block() { }
};


/*
   template <unsigned BLK_SIZE>
   class BIDArray: public std::vector< BID <BLK_SIZE> >
   {
   public:
   BIDArray(std::vector< BID <BLK_SIZE> >::size_type size = 0) : std::vector< BID <BLK_SIZE> >(size) {};
   };
 */

template<unsigned BLK_SIZE>
class BIDArray
{
protected:
    unsigned_type _size;
    BID < BLK_SIZE > *array;
public:
    typedef BID<BLK_SIZE> & reference;
    typedef BID<BLK_SIZE> * iterator;
    typedef const BID<BLK_SIZE> * const_iterator;
    BIDArray () : _size (0), array (NULL)
    { };
    iterator begin ()
    {
        return array;
    }
    iterator end ()
    {
        return array + _size;
    }

    BIDArray (unsigned_type size) : _size (size)
    {
        array = new BID < BLK_SIZE >[size];
    };
    unsigned_type size ()
    {
        return _size;
    }
    reference operator [](int_type i)
    {
        return array[i];
    }
    void resize (unsigned_type newsize)
    {
        if (array)
        {
            stxxl_debug (std::cerr <<
                         "Warning: resizing nonempty BIDArray"
                                   << std::endl);
            BID < BLK_SIZE > *tmp = array;
            array = new BID < BLK_SIZE >[newsize];
            memcpy ((void *)array, (void *)tmp,
                    sizeof(BID < BLK_SIZE >) * (STXXL_MIN (_size, newsize)));
            delete [] tmp;
            _size = newsize;
        }
        else
        {
            array = new BID < BLK_SIZE >[newsize];
            _size = newsize;
        }
    }
    ~BIDArray ()
    {
        if (array)
            delete[] array;
    };
};


class DiskAllocator
{
    typedef std::pair < stxxl::int64, stxxl::int64 > place;
    struct FirstFit : public std::binary_function < place, stxxl::int64, bool >
    {
        bool operator     () (
            const place & entry,
            const stxxl::int64 size) const
        {
            return (entry.second >= size);
        }
    };
    struct OffCmp
    {
        bool operator      () (const stxxl::int64 & off1, const stxxl::int64 & off2)
        {
            return off1 < off2;
        }
    };

    DiskAllocator ()
    { };
protected:

    typedef std::map < stxxl::int64, stxxl::int64 > sortseq;
    sortseq free_space;
    //  sortseq used_space;
    stxxl::int64 free_bytes;
    stxxl::int64 disk_bytes;

    void dump();

    void check_corruption(stxxl::int64 region_pos, stxxl::int64 region_size,
                          sortseq::iterator pred, sortseq::iterator succ)
    {
        if (pred != free_space.end ())
        {
            if (pred->first <= region_pos && pred->first + pred->second > region_pos)
            {
                STXXL_FORMAT_ERROR_MSG(msg, "DiskAllocator::check_corruption Error: double deallocation of external memory " <<
                                       "System info: P " << pred->first << " " << pred->second << " " << region_pos)
                throw bad_ext_alloc(msg.str());
            }
        }
        if (succ != free_space.end ())
        {
            if (region_pos <= succ->first && region_pos + region_size > succ->first)
            {
                STXXL_FORMAT_ERROR_MSG(msg, "DiskAllocator::check_corruption Error: double deallocation of external memory "
                     << "System info: S " << region_pos << " " << region_size << " " << succ->first)
                throw bad_ext_alloc(msg.str());
            }
        }
    }

public:
    inline DiskAllocator (stxxl::int64 disk_size);

    inline stxxl::int64 get_free_bytes () const
    {
        return free_bytes;
    }
    inline stxxl::int64 get_used_bytes () const
    {
        return disk_bytes - free_bytes;
    }
    inline stxxl::int64 get_total_bytes () const
    {
        return disk_bytes;
    }

    template < unsigned BLK_SIZE >
    stxxl::int64 new_blocks (BIDArray < BLK_SIZE > &bids);

    template < unsigned BLK_SIZE >
    stxxl::int64 new_blocks (BID < BLK_SIZE > * begin,
                             BID< BLK_SIZE > * end);
/*
        template < unsigned BLK_SIZE >
        void delete_blocks (const BIDArray < BLK_SIZE > &bids); */
    template < unsigned BLK_SIZE >
    void delete_block (const BID <BLK_SIZE > & bid);
};

DiskAllocator::DiskAllocator (stxxl::int64 disk_size) :
    free_bytes(disk_size),
    disk_bytes(disk_size)
{
    free_space[0] = disk_size;
}


template < unsigned BLK_SIZE >
stxxl::int64 DiskAllocator::new_blocks (BIDArray < BLK_SIZE > & bids)
{
    return new_blocks(bids.begin(), bids.end());
}

template < unsigned BLK_SIZE >
stxxl::int64 DiskAllocator::new_blocks (BID < BLK_SIZE > * begin,
                                        BID<BLK_SIZE> * end)
{
    STXXL_VERBOSE2("DiskAllocator::new_blocks<BLK_SIZE>,  BLK_SIZE = " << BLK_SIZE
                                                                       << ", free:" << free_bytes << " total:" << disk_bytes <<
                   " begin: " << ((void *)(begin)) << " end: " << ((void *)(end)));

    stxxl::int64 requested_size = 0;

    typename BIDArray<BLK_SIZE>::iterator cur = begin;
    for ( ; cur != end; ++cur)
    {
        STXXL_VERBOSE2("Asking for a block with size: " << (cur->size) );
        requested_size += cur->size;
    }

    if (free_bytes < requested_size)
    {
        STXXL_ERRMSG( "External memory block allocation error: " << requested_size <<
                      " bytes requested, " << free_bytes <<
                      " bytes free. Trying to extend the external memory space..." );


        begin->offset = disk_bytes; // allocate at the end
        for (++begin; begin != end; ++begin)
        {
            begin->offset = (begin - 1)->offset + (begin - 1)->size;
        }
        disk_bytes += requested_size;

        return disk_bytes;
    }

    // dump();

    sortseq::iterator space =
        std::find_if (free_space.begin (), free_space.end (),
                      bind2nd(FirstFit (), requested_size));

    if (space != free_space.end ())
    {
        stxxl::int64 region_pos = (*space).first;
        stxxl::int64 region_size = (*space).second;
        free_space.erase (space);
        if (region_size > requested_size)
            free_space[region_pos + requested_size] = region_size - requested_size;

        begin->offset = region_pos;
        for (++begin; begin != end; ++begin)
        {
            begin->offset = (begin - 1)->offset + (begin - 1)->size;
        }
        free_bytes -= requested_size;
        //dump();
        return disk_bytes;
    }

    // no contiguous region found
    STXXL_VERBOSE1("Warning, when allocation an external memory space, no contiguous region found");
    STXXL_VERBOSE1("It might harm the performance");
    if (requested_size == BLK_SIZE )
    {
        assert(end - begin == 1);

        STXXL_ERRMSG("Warning: Severe external memory space defragmentation!");
        dump();

        STXXL_ERRMSG( "External memory block allocation error: " << requested_size <<
                      " bytes requested, " << free_bytes <<
                      " bytes free. Trying to extend the external memory space..." );

        begin->offset = disk_bytes; // allocate at the end
        disk_bytes += BLK_SIZE;

        return disk_bytes;
    }

    assert(requested_size > BLK_SIZE);
    assert(end - begin > 1);

    typename  BIDArray<BLK_SIZE>::iterator middle = begin + ((end - begin) / 2);
    new_blocks(begin, middle);
    new_blocks(middle, end);

    return disk_bytes;
}



template < unsigned BLK_SIZE >
void DiskAllocator::delete_block (const BID < BLK_SIZE > &bid)
{
    STXXL_VERBOSE2("DiskAllocator::delete_block<BLK_SIZE>,  BLK_SIZE = " << BLK_SIZE
                                                                         << ", free:" << free_bytes << " total:" << disk_bytes);
    STXXL_VERBOSE2("Deallocating a block with size: " << bid.size);
    //assert(bid.size);

    //dump();
    stxxl::int64 region_pos = bid.offset;
    stxxl::int64 region_size = bid.size;
    STXXL_VERBOSE2("Deallocating a block with size: " << region_size << " position: " << region_pos);
    if (!free_space.empty())
    {
        sortseq::iterator succ = free_space.upper_bound (region_pos);
        sortseq::iterator pred = succ;
        pred--;
        check_corruption(region_pos, region_size, pred, succ);
        if (succ == free_space.end ())
        {
            if (pred == free_space.end ())
            {
                STXXL_ERRMSG("Error deallocating block at " << bid.offset << " size " << bid.size);
                STXXL_ERRMSG(((pred == succ) ? "pred==succ" : "pred!=succ"));
                STXXL_ERRMSG(((pred == free_space.begin ()) ? "pred==free_space.begin()" : "pred!=free_space.begin()"));
                STXXL_ERRMSG(((pred == free_space.end ()) ? "pred==free_space.end()" : "pred!=free_space.end()"));
                STXXL_ERRMSG(((succ == free_space.begin ()) ? "succ==free_space.begin()" : "succ!=free_space.begin()"));
                STXXL_ERRMSG(((succ == free_space.end ()) ? "succ==free_space.end()" : "succ!=free_space.end()"));
                dump();
                assert(pred != free_space.end ());
            }
            if ((*pred).first + (*pred).second == region_pos)
            {
                // coalesce with predecessor
                region_size += (*pred).second;
                region_pos = (*pred).first;
                free_space.erase (pred);
            }
        }
        else
        {
            if (free_space.size() > 1 )
            {
                /*
                   if(pred == succ)
                   {
                      STXXL_ERRMSG("Error deallocating block at "<<bid.offset<<" size "<<bid.size);
                      STXXL_ERRMSG(((pred==succ)?"pred==succ":"pred!=succ"));
                      STXXL_ERRMSG(((pred==free_space.begin ())?"pred==free_space.begin()":"pred!=free_space.begin()"));
                      STXXL_ERRMSG(((pred==free_space.end ())?"pred==free_space.end()":"pred!=free_space.end()"));
                      STXXL_ERRMSG(((succ==free_space.begin ())?"succ==free_space.begin()":"succ!=free_space.begin()"));
                      STXXL_ERRMSG(((succ==free_space.end ())?"succ==free_space.end()":"succ!=free_space.end()"));
                      dump();
                      assert(pred != succ);
                   } */
                bool succ_is_not_the_first = (succ != free_space.begin());
                if ((*succ).first == region_pos + region_size)
                {
                    // coalesce with successor
                    region_size += (*succ).second;
                    free_space.erase (succ);
                }
                if (succ_is_not_the_first)
                {
                    if (pred == free_space.end ())
                    {
                        STXXL_ERRMSG("Error deallocating block at " << bid.offset << " size " << bid.size);
                        STXXL_ERRMSG(((pred == succ) ? "pred==succ" : "pred!=succ"));
                        STXXL_ERRMSG(((pred == free_space.begin ()) ? "pred==free_space.begin()" : "pred!=free_space.begin()"));
                        STXXL_ERRMSG(((pred == free_space.end ()) ? "pred==free_space.end()" : "pred!=free_space.end()"));
                        STXXL_ERRMSG(((succ == free_space.begin ()) ? "succ==free_space.begin()" : "succ!=free_space.begin()"));
                        STXXL_ERRMSG(((succ == free_space.end ()) ? "succ==free_space.end()" : "succ!=free_space.end()"));
                        dump();
                        assert(pred != free_space.end ());
                    }
                    if ((*pred).first + (*pred).second == region_pos)
                    {
                        // coalesce with predecessor
                        region_size += (*pred).second;
                        region_pos = (*pred).first;
                        free_space.erase (pred);
                    }
                }
            }
            else
            {
                if ((*succ).first == region_pos + region_size)
                {
                    // coalesce with successor
                    region_size += (*succ).second;
                    free_space.erase (succ);
                }
            }
        }
    }

    free_space[region_pos] = region_size;
    free_bytes += stxxl::int64 (bid.size);

    //dump();
}
/*
    template < unsigned BLK_SIZE >
        void DiskAllocator::delete_blocks (const BIDArray < BLK_SIZE > &bids)
    {
        STXXL_VERBOSE2("DiskAllocator::delete_blocks<BLK_SIZE> BLK_SIZE="<< BLK_SIZE <<
      ", free:" << free_bytes << " total:"<< disk_bytes );

        unsigned i=0;
        for (; i < bids.size (); ++i)
        {
            stxxl::int64 region_pos = bids[i].offset;
            stxxl::int64 region_size = bids[i].size;
              STXXL_VERBOSE2("Deallocating a block with size: "<<region_size);
              assert(bids[i].size);

            if(!free_space.empty())
            {
                sortseq::iterator succ =
                    free_space.upper_bound (region_pos);
                sortseq::iterator pred = succ;
                pred--;

                if (succ != free_space.end ()
                    && (*succ).first == region_pos + region_size)
                {
                    // coalesce with successor

                    region_size += (*succ).second;
                    free_space.erase (succ);
                }
                if (pred != free_space.end ()
                    && (*pred).first + (*pred).second == region_pos)
                {
                    // coalesce with predecessor

                    region_size += (*pred).second;
                    region_pos = (*pred).first;
                    free_space.erase (pred);
                }
            }
            free_space[region_pos] = region_size;

        }
        for(i=0;i<bids.size();++i)
                  free_bytes += stxxl::int64(bids[i].size);
    } */

//! \brief Access point to disks properties
//! \remarks is a singleton
class config
{
    struct DiskEntry
    {
        std::string path;
        std::string io_impl;
        stxxl::int64 size;
    };
    std::vector < DiskEntry > disks_props;

    config (const char *config_path = "./.stxxl");
public:
    //! \brief Returns number of disks available to user
    //! \return number of disks
    inline unsigned disks_number()
    {
        return disks_props.size ();
    }

    inline unsigned ndisks()
    {
        return disks_props.size ();
    }

    //! \brief Returns path of disks
    //! \param disk disk's identifier
    //! \return string that contains the disk's path name
    inline const std::string & disk_path (int disk)
    {
        return disks_props[disk].path;
    }
    //! \brief Returns disk size
    //! \param disk disk's identifier
    //! \return disk size in bytes
    inline stxxl::int64 disk_size (int disk)
    {
        return disks_props[disk].size;
    }
    //! \brief Returns name of I/O implementation of particular disk
    //! \param disk disk's identifier
    inline const std::string & disk_io_impl (int disk)
    {
        return disks_props[disk].io_impl;
    }

    //! \brief Returns instance of config
    //! \return pointer to the instance of config
    static config * get_instance ();
private:
    static config *instance;
};



class FileCreator
{
public:
    virtual stxxl::file * create (const std::string & io_impl,
                                  const std::string & filename,
                                  int options, int disk)
    {
        if (io_impl == "syscall")
        {
            stxxl::ufs_file_base * result = new stxxl::syscall_file (filename,
                                                                     options,
                                                                     disk);
            result->lock();
            return result;
        }
            #ifndef BOOST_MSVC
        else if (io_impl == "mmap")
        {
            stxxl::ufs_file_base * result = new stxxl::mmap_file (filename,
                                                                  options, disk);
            result->lock();
            return result;
        }
        else if (io_impl == "simdisk")
        {
            stxxl::ufs_file_base * result = new stxxl::sim_disk_file (filename,
                                                                      options,
                                                                      disk);
            result->lock();
            return result;
        }
            #else
        else if (io_impl == "wincall")
        {
            stxxl::wfs_file_base * result = new stxxl::wincall_file (filename,
                                                                     options, disk);
            result->lock();
            return result;
        }
            #endif
      #ifdef STXXL_BOOST_CONFIG
        else if (io_impl == "boostfd")
            return new stxxl::boostfd_file (filename,
                                            options, disk);
      #endif

        STXXL_FORMAT_ERROR_MSG(msg, "FileCreator::create Unsupported disk I/O implementation " <<
                               io_impl << " ." )
        throw std::runtime_error(msg.str());

        return NULL;
    }

    virtual ~FileCreator() { }
};

//! \weakgroup alloc Allocation functors
//! Standard allocation strategies encapsulated in functors
//! \{

//! \brief striping disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct striping
{
    int begin, diff;
    striping (int b, int e) : begin (b), diff (e - b)
    { };
    striping () : begin (0)
    {
        diff = config::get_instance ()->disks_number ();
    };
    int operator     () (int i) const
    {
        return begin + i % diff;
    }
    static const char * name()
    {
        return "striping";
    }
    virtual ~striping()
    { }
};

//! \brief fully randomized disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct FR : public striping
{
    random_number<random_uniform_fast> rnd;
    FR (int b, int e) : striping (b, e)
    { };
    FR () : striping ()
    { };
    int operator     () (int /*i*/) const
    {
        return begin + rnd(diff);
    }
    static const char * name()
    {
        return "fully randomized striping";
    }
};

//! \brief simple randomized disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct SR : public striping
{
    random_number<random_uniform_fast> rnd;
    int offset;
    SR (int b, int e) : striping (b, e)
    {
        offset = rnd(diff);
    };
    SR() : striping ()
    {
        offset = rnd(diff);
    };
    int operator     () (int i) const
    {
        return begin + (i + offset) % diff;
    }
    static const char * name()
    {
        return "simple randomized striping";
    }
};

//! \brief randomized cycling disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct RC : public striping
{
    std::vector<int> perm;

    RC (int b, int e) : striping (b, e), perm (diff)
    {
        for (int i = 0; i < diff; i++)
            perm[i] = i;

        stxxl::random_number<random_uniform_fast> rnd;
        std::random_shuffle (perm.begin (), perm.end (), rnd);
    }
    RC () : striping (), perm (diff)
    {
        for (int i = 0; i < diff; i++)
            perm[i] = i;

        random_number<random_uniform_fast> rnd;
        std::random_shuffle (perm.begin (), perm.end (), rnd);
    }
    int operator     () (int i) const
    {
        return begin + perm[i % diff];
    }
    static const char * name()
    {
        return "randomized cycling striping";
    }
};

//! \brief 'single disk' disk allocation scheme functor
//! \remarks model of \b allocation_strategy concept
struct single_disk
{
    const int disk;
    single_disk(int d) : disk(d)
    { };

    single_disk() : disk(0)
    { };
    int operator() (int /*i*/) const
    {
        return disk;
    }
    static const char * name()
    {
        return "single disk";
    }
};

//! \brief Allocator functor adaptor

//! Gives offset to disk number sequence defined in constructor
template <class BaseAllocator_>
struct offset_allocator
{
    BaseAllocator_ base;
    int_type offset;
    //! \brief Creates functor based on \c BaseAllocator_ functor created
    //! with default constructor with offset \c offset_
    //! \param offset_ offset
    offset_allocator(int_type offset_) : base(), offset(offset_) { }
    //! \brief Creates functor based on instance of \c BaseAllocator_ functor
    //! with offset \c offset_
    //! \param offset_ offset
    //! \param base_ used to create a copy
    offset_allocator(int_type offset_, BaseAllocator_ & base_) : base(base_), offset(offset_) { }
    int operator() (int_type i)
    {
        return base(offset + i);
    }
};

//! \}

/* deprecated

   //! \brief Traits for models of \b bid_iterator concept
   template < class bid_it >
   struct bid_iterator_traits
   {
      bid_it *a;
      enum
      {
          block_size = bid_it::block_size
      };
   };

   template < unsigned blk_sz >
   struct bid_iterator_traits <BID <blk_sz > *>
   {
      enum
      {
          block_size = blk_sz
      };
   };


   template < unsigned blk_sz,class X >
   struct bid_iterator_traits< __gnu_cxx::__normal_iterator< BID<blk_sz> *,  X> >
   {
      enum
      {
          block_size = blk_sz
      };
   };

   template < unsigned blk_sz,class X , class Y>
   struct bid_iterator_traits< std::_List_iterator<BID<blk_sz>,X,Y > >
   {
      enum
      {
          block_size = blk_sz
      };
   };

   template < unsigned blk_sz, class X >
   struct bid_iterator_traits< typename std::vector< BID<blk_sz> , X >::iterator >
   {
      enum
      {
          block_size = blk_sz
      };
   };
 */
//! \brief Block manager class

//! Manages allocation and deallocation of blocks in multiple/single disk setting
//! \remarks is a singleton
class block_manager
{
    DiskAllocator * *disk_allocators;
    file * * disk_files;

    unsigned ndisks;
    block_manager ();

protected:
    template < class BIDType, class DiskAssgnFunctor, class BIDIteratorClass >
    void new_blocks_int (
        const unsigned_type nblocks,
        DiskAssgnFunctor functor,
        BIDIteratorClass out);
public:
    //! \brief Returns instance of block_manager
    //! \return pointer to the only instance of block_manager
    static block_manager * get_instance ();

    //! \brief Allocates new blocks

    //! Allocates new blocks according to the strategy
    //! given by \b functor and stores block identifiers
    //! to the range [ \b bidbegin, \b bidend)
    //! \param functor object of model of \b allocation_strategy concept
    //! \param bidbegin bidirectional BID iterator object
    //! \param bidend bidirectional BID iterator object
    template < class DiskAssgnFunctor, class BIDIteratorClass >
    void new_blocks (
        DiskAssgnFunctor functor,
        BIDIteratorClass bidbegin,
        BIDIteratorClass bidend);

    //! Allocates new blocks according to the strategy
    //! given by \b functor and stores block identifiers
    //! to the output iterator \b out
    //! \param nblocks the number of blocks to allocate
    //! \param functor object of model of \b allocation_strategy concept
    //! \param out iterator object of OutputIterator concept
    //!
    //! The \c BlockType template parameter defines the type of block to allocate
    template < class BlockType, class DiskAssgnFunctor, class BIDIteratorClass >
    void new_blocks (
        const unsigned_type nblocks,
        DiskAssgnFunctor functor,
        BIDIteratorClass out);


    //! \brief Deallocates blocks

    //! Deallocates blocks in the range [ \b bidbegin, \b bidend)
    //! \param bidbegin iterator object of \b bid_iterator concept
    //! \param bidend iterator object of \b bid_iterator concept
    template < class BIDIteratorClass >
    void delete_blocks (const BIDIteratorClass & bidbegin, const BIDIteratorClass & bidend);

    //! \brief Deallocates a block
    //! \param bid block identifier
    template < unsigned BLK_SIZE >
    void delete_block (const BID < BLK_SIZE > &bid);

    ~block_manager ();
private:
    static block_manager *instance;
};


template < class BIDType, class DiskAssgnFunctor, class OutputIterator >
void block_manager::new_blocks_int (
    const unsigned_type nblocks,
    DiskAssgnFunctor functor,
    OutputIterator out)
{
    typedef BIDType bid_type;
    typedef  BIDArray<bid_type::t_size> bid_array_type;

    // bid_type tmpbid;
    int_type * bl = new int_type[ndisks];
    bid_array_type * disk_bids = new bid_array_type[ndisks];
    file * * disk_ptrs = new file * [nblocks];

    memset(bl, 0, ndisks * sizeof(int_type));

    unsigned_type i = 0;
    //OutputIterator  it = out;
    for ( ; i < nblocks; ++i /* , ++it*/)
    {
        const int disk = functor (i);
        disk_ptrs[i] = disk_files[disk];
        //(*it).storage = disk_files[disk];
        bl[disk]++;
    }

    for (i = 0; i < ndisks; ++i)
    {
        if (bl[i])
        {
            disk_bids[i].resize (bl[i]);
            const stxxl::int64 old_capacity =
                disk_allocators[i]->get_total_bytes();
            const stxxl::int64 new_capacity =
                disk_allocators[i]->new_blocks (disk_bids[i]);
            if (old_capacity != new_capacity)
            {
                // resize the file
                disk_files[i]->set_size(new_capacity);
            }
        }
    }

    memset (bl, 0, ndisks * sizeof(int_type));

    OutputIterator it = out;
    for (i = 0 /*,it = out */; i != nblocks; ++it, ++i)
    {
        //int disk = (*it).storage->get_disk_number ();
        //(*it).offset = disk_bids[disk][bl[disk]++].offset;
        const int disk = disk_ptrs[i]->get_disk_number();
        *it = bid_type(disk_ptrs[i], disk_bids[disk][bl[disk]++].offset);
    }

    delete [] bl;
    delete [] disk_bids;
    delete [] disk_ptrs;
}

template < class BlockType, class DiskAssgnFunctor, class OutputIterator >
void block_manager::new_blocks (
    const unsigned_type nblocks,
    DiskAssgnFunctor functor,
    OutputIterator out)
{
    typedef typename BlockType::bid_type bid_type;
    new_blocks_int<bid_type>(nblocks, functor, out);
}

template < class DiskAssgnFunctor, class BIDIteratorClass >
void block_manager::new_blocks (
    DiskAssgnFunctor functor,
    BIDIteratorClass bidbegin,
    BIDIteratorClass bidend)
{
    typedef typename std::iterator_traits<BIDIteratorClass>::value_type bid_type;

    unsigned_type nblocks = 0;

    BIDIteratorClass bidbegin_copy(bidbegin);
    while (bidbegin_copy != bidend)
    {
        ++bidbegin_copy;
        ++nblocks;
    }

    new_blocks_int<bid_type>(nblocks, functor, bidbegin);
}



template < unsigned BLK_SIZE >
void block_manager::delete_block (const BID < BLK_SIZE > &bid)
{
    // do not uncomment it
    //assert(bid.storage->get_disk_number () < config::get_instance ()->disks_number ());
    if (bid.storage->get_disk_number () == -1)
        return; // self managed disk
    assert(bid.storage->get_disk_number () >= 0 );
    disk_allocators[bid.storage->get_disk_number ()]->delete_block (bid);
}


template < class BIDIteratorClass >
void block_manager::delete_blocks (
    const BIDIteratorClass & bidbegin,
    const BIDIteratorClass & bidend)
{
    for (BIDIteratorClass it = bidbegin; it != bidend; it++)
    {
        delete_block (*it);
    }
}


  #define STXXL_DEFAULT_ALLOC_STRATEGY RC
  #define STXXL_DEFAULT_BLOCK_SIZE(type) (2 * 1024 * 1024) // use traits

//! \}

__STXXL_END_NAMESPACE


#endif
// vim: et:ts=4:sw=4
