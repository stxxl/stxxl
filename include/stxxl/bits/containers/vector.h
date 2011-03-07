/***************************************************************************
 *  include/stxxl/bits/containers/vector.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_VECTOR_HEADER
#define STXXL_VECTOR_HEADER

#include <vector>
#include <queue>
#include <algorithm>

#include <stxxl/bits/deprecated.h>
#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/mng/typed_block.h>
#include <stxxl/bits/common/tmeta.h>
#include <stxxl/bits/containers/pager.h>
#include <stxxl/bits/common/is_sorted.h>


__STXXL_BEGIN_NAMESPACE

//! \weakgroup stlcont Containers
//! \ingroup stllayer
//! Containers with STL-compatible interface


//! \weakgroup stlcontinternals Internals
//! \ingroup stlcont
//! \{

template <typename size_type, size_type modulo2, size_type modulo1>
class double_blocked_index
{
    static const size_type modulo12 = modulo1 * modulo2;

    size_type pos, block1, block2, offset;

    //! \invariant block2 * modulo12 + block1 * modulo1 + offset == pos && 0 <= block1 &lt; modulo2 && 0 <= offset &lt; modulo1

    void set(size_type pos)
    {
        this->pos = pos;
        block2 = pos / modulo12;
        pos -= block2 * modulo12;
        block1 = pos / modulo1;
        offset = (pos - block1 * modulo1);

        assert(block2 * modulo12 + block1 * modulo1 + offset == this->pos);
        assert(/* 0 <= block1 && */ block1 < modulo2);
        assert(/* 0 <= offset && */ offset < modulo1);
    }

public:
    double_blocked_index()
    {
        set(0);
    }

    double_blocked_index(size_type pos)
    {
        set(pos);
    }

    double_blocked_index(size_type block2, size_type block1, size_type offset)
    {
        assert(/* 0 <= block1 && */ block1 < modulo2);
        assert(/* 0 <= offset && */ offset < modulo1);

        this->block2 = block2;
        this->block1 = block1;
        this->offset = offset;
        pos = block2 * modulo12 + block1 * modulo1 + offset;
    }

    double_blocked_index & operator = (size_type pos)
    {
        set(pos);
        return *this;
    }

    //pre-increment operator
    double_blocked_index & operator ++ ()
    {
        ++pos;
        ++offset;
        if (offset == modulo1)
        {
            offset = 0;
            ++block1;
            if (block1 == modulo2)
            {
                block1 = 0;
                ++block2;
            }
        }

        assert(block2 * modulo12 + block1 * modulo1 + offset == this->pos);
        assert(/* 0 <= block1 && */ block1 < modulo2);
        assert(/* 0 <= offset && */ offset < modulo1);

        return *this;
    }

    //post-increment operator
    double_blocked_index operator ++ (int)
    {
        double_blocked_index former(*this);
        operator ++ ();
        return former;
    }

    //pre-increment operator
    double_blocked_index & operator -- ()
    {
        --pos;
        if (offset == 0)
        {
            offset = modulo1;
            if (block1 == 0)
            {
                block1 = modulo2;
                --block2;
            }
            --block1;
        }
        --offset;

        assert(block2 * modulo12 + block1 * modulo1 + offset == this->pos);
        assert(/*0 <= block1 &&*/ block1 < modulo2);
        assert(/*0 <= offset &&*/ offset < modulo1);

        return *this;
    }

    //post-increment operator
    double_blocked_index operator -- (int)
    {
        double_blocked_index former(*this);
        operator -- ();
        return former;
    }

    double_blocked_index operator + (size_type addend) const
    {
        return double_blocked_index(pos + addend);
    }

    double_blocked_index & operator += (size_type addend)
    {
        set(pos + addend);
        return *this;
    }

    double_blocked_index operator - (size_type addend) const
    {
        return double_blocked_index(pos - addend);
    }

    size_type operator - (const double_blocked_index & dbi2) const
    {
        return pos - dbi2.pos;
    }

    double_blocked_index & operator -= (size_type subtrahend)
    {
        set(pos - subtrahend);
        return *this;
    }

    bool operator == (const double_blocked_index & dbi2) const
    {
        return pos == dbi2.pos;
    }

    bool operator != (const double_blocked_index & dbi2) const
    {
        return pos != dbi2.pos;
    }

    bool operator < (const double_blocked_index & dbi2) const
    {
        return pos < dbi2.pos;
    }

    bool operator <= (const double_blocked_index & dbi2) const
    {
        return pos <= dbi2.pos;
    }

    bool operator > (const double_blocked_index & dbi2) const
    {
        return pos > dbi2.pos;
    }

    bool operator >= (const double_blocked_index & dbi2) const
    {
        return pos >= dbi2.pos;
    }

    double_blocked_index & operator >>= (size_type shift)
    {
        set(pos >> shift);
        return *this;
    }

    size_type get_pos() const
    {
        return pos;
    }

    const size_type & get_block2() const
    {
        return block2;
    }

    const size_type & get_block1() const
    {
        return block1;
    }

    const size_type & get_offset() const
    {
        return offset;
    }
};


template <unsigned BlkSize_>
class bid_vector : public std::vector<BID<BlkSize_> >
{
public:
    typedef std::vector<BID<BlkSize_> > _Derived;
    typedef typename _Derived::size_type size_type;
    typedef typename _Derived::value_type bid_type;

    bid_vector(size_type _sz) : _Derived(_sz)
    { }
};


template <
    typename Tp_,
    unsigned PgSz_,
    typename PgTp_,
    unsigned BlkSize_,
    typename AllocStr_,
    typename SzTp_>
class vector;


template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
          unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
class const_vector_iterator;


//! \brief External vector iterator, model of \c ext_random_access_iterator concept
template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
          unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
class vector_iterator
{
    typedef vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> _Self;
    typedef const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> _CIterator;

    friend class const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>;

public:
    typedef _CIterator const_iterator;
    typedef _Self iterator;

    typedef unsigned block_offset_type;
    typedef vector<Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_, SzTp_> vector_type;
    friend class vector<Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_, SzTp_>;
    typedef typename vector_type::bids_container_type bids_container_type;
    typedef typename bids_container_type::iterator bids_container_iterator;
    typedef typename bids_container_type::bid_type bid_type;
    typedef typename vector_type::block_type block_type;
    typedef typename vector_type::blocked_index_type blocked_index_type;

    typedef std::random_access_iterator_tag iterator_category;
    typedef typename vector_type::size_type size_type;
    typedef typename vector_type::difference_type difference_type;
    typedef typename vector_type::value_type value_type;
    typedef typename vector_type::reference reference;
    typedef typename vector_type::const_reference const_reference;
    typedef typename vector_type::pointer pointer;
    typedef typename vector_type::const_pointer const_pointer;

protected:
    blocked_index_type offset;
    vector_type * p_vector;

private:
    vector_iterator(vector_type * v, size_type o) : offset(o),
                                                    p_vector(v)
    { }

public:
    vector_iterator() : offset(0), p_vector(NULL) { }
    vector_iterator(const _Self & a) :
        offset(a.offset),
        p_vector(a.p_vector) { }

    block_offset_type block_offset() const
    {
        return static_cast<block_offset_type>(offset.get_offset());
    }
    bids_container_iterator bid() const
    {
        return p_vector->bid(offset);
    }

    difference_type operator - (const _Self & a) const
    {
        return offset - a.offset;
    }

    difference_type operator - (const const_iterator & a) const
    {
        return offset - a.offset;
    }

    _Self operator - (size_type op) const
    {
        return _Self(p_vector, offset.get_pos() - op);
    }

    _Self operator + (size_type op) const
    {
        return _Self(p_vector, offset.get_pos() + op);
    }

    _Self & operator -= (size_type op)
    {
        offset -= op;
        return *this;
    }

    _Self & operator += (size_type op)
    {
        offset += op;
        return *this;
    }

    reference operator * ()
    {
        return p_vector->element(offset);
    }

    pointer operator -> ()
    {
        return &(p_vector->element(offset));
    }

    const_reference operator * () const
    {
        return p_vector->const_element(offset);
    }

    const_pointer operator -> () const
    {
        return &(p_vector->const_element(offset));
    }

    reference operator [] (size_type op)
    {
        return p_vector->element(offset.get_pos() + op);
    }

    const_reference operator [] (size_type op) const
    {
        return p_vector->const_element(offset.get_pos() + op);
    }

    void block_externally_updated()
    {
        p_vector->block_externally_updated(offset);
    }

    _STXXL_DEPRECATED(void touch())
    {
        block_externally_updated();
    }

    _Self & operator ++ ()
    {
        offset++;
        return *this;
    }
    _Self operator ++ (int)
    {
        _Self __tmp = *this;
        offset++;
        return __tmp;
    }
    _Self & operator -- ()
    {
        offset--;
        return *this;
    }
    _Self operator -- (int)
    {
        _Self __tmp = *this;
        offset--;
        return __tmp;
    }
    bool operator == (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset == a.offset;
    }
    bool operator != (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset != a.offset;
    }
    bool operator < (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset < a.offset;
    }
    bool operator <= (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset <= a.offset;
    }
    bool operator > (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset > a.offset;
    }
    bool operator >= (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset >= a.offset;
    }

    bool operator == (const const_iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset == a.offset;
    }
    bool operator != (const const_iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset != a.offset;
    }
    bool operator < (const const_iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset < a.offset;
    }
    bool operator <= (const const_iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset <= a.offset;
    }
    bool operator > (const const_iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset > a.offset;
    }
    bool operator >= (const const_iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset >= a.offset;
    }

    void flush()
    {
        p_vector->flush();
    }
#if 0
    std::ostream & operator << (std::ostream & o) const
    {
        o << "vectorpointer: " << ((void *)p_vector) << " offset: " << offset;
        return o;
    }
#endif
};

//! \brief Const external vector iterator, model of \c ext_random_access_iterator concept
template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
          unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
class const_vector_iterator
{
    typedef const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> _Self;
    typedef vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> _NonConstIterator;

    friend class vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>;

public:
    typedef _Self const_iterator;
    typedef _NonConstIterator iterator;

    typedef unsigned block_offset_type;
    typedef vector<Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_, SzTp_> vector_type;
    friend class vector<Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_, SzTp_>;
    typedef typename vector_type::bids_container_type bids_container_type;
    typedef typename bids_container_type::iterator bids_container_iterator;
    typedef typename bids_container_type::bid_type bid_type;
    typedef typename vector_type::block_type block_type;
    typedef typename vector_type::blocked_index_type blocked_index_type;

    typedef std::random_access_iterator_tag iterator_category;
    typedef typename vector_type::size_type size_type;
    typedef typename vector_type::difference_type difference_type;
    typedef typename vector_type::value_type value_type;
    typedef typename vector_type::const_reference reference;
    typedef typename vector_type::const_reference const_reference;
    typedef typename vector_type::const_pointer pointer;
    typedef typename vector_type::const_pointer const_pointer;

protected:
    blocked_index_type offset;
    const vector_type * p_vector;

private:
    const_vector_iterator(const vector_type * v, size_type o) : offset(o),
                                                                p_vector(v)
    { }

public:
    const_vector_iterator() : offset(0), p_vector(NULL)
    { }
    const_vector_iterator(const _Self & a) :
        offset(a.offset),
        p_vector(a.p_vector)
    { }

    const_vector_iterator(const iterator & a) :
        offset(a.offset),
        p_vector(a.p_vector)
    { }

    block_offset_type block_offset() const
    {
        return static_cast<block_offset_type>(offset.get_offset());
    }
    bids_container_iterator bid() const
    {
        return ((vector_type *)p_vector)->bid(offset);
    }

    difference_type operator - (const _Self & a) const
    {
        return offset - a.offset;
    }

    difference_type operator - (const iterator & a) const
    {
        return offset - a.offset;
    }

    _Self operator - (size_type op) const
    {
        return _Self(p_vector, offset.get_pos() - op);
    }

    _Self operator + (size_type op) const
    {
        return _Self(p_vector, offset.get_pos() + op);
    }

    _Self & operator -= (size_type op)
    {
        offset -= op;
        return *this;
    }

    _Self & operator += (size_type op)
    {
        offset += op;
        return *this;
    }

    const_reference operator * () const
    {
        return p_vector->const_element(offset);
    }

    const_pointer operator -> () const
    {
        return &(p_vector->const_element(offset));
    }

    const_reference operator [] (size_type op) const
    {
        return p_vector->const_element(offset.get_pos() + op);
    }

    void block_externally_updated()
    {
        p_vector->block_externally_updated(offset);
    }

    _STXXL_DEPRECATED(void touch())
    {
        block_externally_updated();
    }

    _Self & operator ++ ()
    {
        offset++;
        return *this;
    }
    _Self operator ++ (int)
    {
        _Self tmp_ = *this;
        offset++;
        return tmp_;
    }
    _Self & operator -- ()
    {
        offset--;
        return *this;
    }
    _Self operator -- (int)
    {
        _Self __tmp = *this;
        offset--;
        return __tmp;
    }
    bool operator == (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset == a.offset;                 // or (offset + stxxl::int64(p_vector))
    }
    bool operator != (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset != a.offset;
    }
    bool operator < (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset < a.offset;
    }
    bool operator <= (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset <= a.offset;
    }
    bool operator > (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset > a.offset;
    }
    bool operator >= (const _Self & a) const
    {
        assert(p_vector == a.p_vector);
        return offset >= a.offset;
    }

    bool operator == (const iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset == a.offset; // or (offset + stxxl::int64(p_vector))
    }
    bool operator != (const iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset != a.offset;
    }
    bool operator < (const iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset < a.offset;
    }
    bool operator <= (const iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset <= a.offset;
    }
    bool operator > (const iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset > a.offset;
    }
    bool operator >= (const iterator & a) const
    {
        assert(p_vector == a.p_vector);
        return offset >= a.offset;
    }

    void flush()
    {
        p_vector->flush();
    }

#if 0
    std::ostream & operator << (std::ostream & o) const
    {
        o << "vector pointer: " << ((void *)p_vector) << " offset: " << offset;
        return o;
    }
#endif
};


//! \brief External vector container

//! For semantics of the methods see documentation of the STL std::vector
//! \tparam Tp_ type of contained objects (POD with no references to internal memory)
//! \tparam PgSz_ number of blocks in a page
//! \tparam PgTp_ pager type, \c random_pager<x> or \c lru_pager<x>, where x is number of pages,
//!  default is \c lru_pager<8>
//! \tparam BlkSize_ external block size in bytes, default is 2 MiB
//! \tparam AllocStr_ one of allocation strategies: \c striping , \c RC , \c SR , or \c FR
//!  default is RC
//!
//! Memory consumption: BlkSize_*x*PgSz_ bytes
//! \warning Do not store references to the elements of an external vector. Such references
//! might be invalidated during any following access to elements of the vector
template <
    typename Tp_,
    unsigned PgSz_ = 4,
    typename PgTp_ = lru_pager<8>,
    unsigned BlkSize_ = STXXL_DEFAULT_BLOCK_SIZE(Tp_),
    typename AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY,
    typename SzTp_ = stxxl::uint64       // will be deprecated soon
    >
class vector
{
public:
    typedef Tp_ value_type;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef value_type * pointer;
    typedef SzTp_ size_type;
    typedef stxxl::int64 difference_type;
    typedef const value_type * const_pointer;

    typedef PgTp_ pager_type;
    typedef AllocStr_ alloc_strategy_type;

    enum constants {
        block_size = BlkSize_,
        page_size = PgSz_,
        n_pages = pager_type::n_pages,
        on_disk = -1
    };

    typedef vector_iterator<value_type, alloc_strategy_type, size_type,
                            difference_type, block_size, pager_type, page_size> iterator;
    friend class vector_iterator<value_type, alloc_strategy_type, size_type, difference_type, block_size, pager_type, page_size>;
    typedef const_vector_iterator<value_type, alloc_strategy_type,
                                  size_type, difference_type, block_size, pager_type, page_size> const_iterator;
    friend class const_vector_iterator<value_type, alloc_strategy_type, size_type, difference_type, block_size, pager_type, page_size>;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef bid_vector<block_size> bids_container_type;
    typedef typename bids_container_type::iterator bids_container_iterator;
    typedef typename bids_container_type::const_iterator const_bids_container_iterator;

    typedef typed_block<BlkSize_, Tp_> block_type;
    typedef double_blocked_index<SzTp_, PgSz_, block_type::size> blocked_index_type;

private:
    alloc_strategy_type alloc_strategy;
    size_type _size;
    bids_container_type _bids;
    mutable pager_type pager;

    enum { valid_on_disk = 0, uninitialized = 1, dirty = 2 };
    mutable std::vector<unsigned char> _page_status;
    mutable std::vector<int_type> _page_to_slot;
    mutable simple_vector<int_type> _slot_to_page;
    mutable std::queue<int_type> _free_slots;
    mutable simple_vector<block_type> * _cache;
    file * _from;
    block_manager * bm;
    config * cfg;
    bool exported;

    size_type size_from_file_length(stxxl::uint64 file_length) const
    {
        stxxl::uint64 blocks_fit = file_length / stxxl::uint64(block_type::raw_size);
        size_type cur_size = blocks_fit * stxxl::uint64(block_type::size);
        stxxl::uint64 rest = file_length - blocks_fit * stxxl::uint64(block_type::raw_size);
        return (cur_size + rest / stxxl::uint64(sizeof(value_type)));
    }

    stxxl::uint64 file_length() const
    {
        typedef stxxl::uint64 file_size_type;
        size_type cur_size = size();
        size_type num_full_blocks = cur_size / block_type::size;
        if (cur_size % block_type::size != 0)
        {
            size_type rest = cur_size - num_full_blocks * block_type::size;
            return file_size_type(num_full_blocks) * block_type::raw_size + rest * sizeof(value_type);
        }
        return file_size_type(num_full_blocks) * block_type::raw_size;
    }

public:
    vector(size_type n = 0) :
        _size(n),
        _bids(div_ceil(n, block_type::size)),
        _page_status(div_ceil(_bids.size(), page_size)),
        _page_to_slot(div_ceil(_bids.size(), page_size)),
        _slot_to_page(n_pages),
        _cache(NULL),
        _from(NULL),
        exported(false)
    {
        bm = block_manager::get_instance();
        cfg = config::get_instance();

        allocate_page_cache();
        int_type all_pages = div_ceil(_bids.size(), page_size);
        int_type i = 0;
        for ( ; i < all_pages; ++i)
        {
            _page_status[i] = uninitialized;
            _page_to_slot[i] = on_disk;
        }

        for (i = 0; i < n_pages; ++i)
            _free_slots.push(i);

        bm->new_blocks(alloc_strategy, _bids.begin(), _bids.end(), 0);
    }

    void swap(vector & obj)
    {
        std::swap(alloc_strategy, obj.alloc_strategy);
        std::swap(_size, obj._size);
        std::swap(_bids, obj._bids);
        std::swap(pager, obj.pager);
        std::swap(_page_status, obj._page_status);
        std::swap(_page_to_slot, obj._page_to_slot);
        std::swap(_slot_to_page, obj._slot_to_page);
        std::swap(_free_slots, obj._free_slots);
        std::swap(_cache, obj._cache);
        std::swap(_from, obj._from);
        std::swap(exported, obj.exported);
    }

    void allocate_page_cache() const
    {
        if (!_cache)
            _cache = new simple_vector<block_type>(n_pages * page_size);
    }

    // allows to free the cache, but you may not access any element until call allocate_pacge_cache() again
    void deallocate_page_cache() const
    {
        flush();
        delete _cache;
        _cache = NULL;
    }

    size_type capacity() const
    {
        return size_type(_bids.size()) * block_type::size;
    }
    //! \brief Returns the number of bytes that the vector has allocated on disks
    size_type raw_capacity() const
    {
        return size_type(_bids.size()) * block_type::raw_size;
    }

    void reserve(size_type n)
    {
        if (n <= capacity())
            return;

        unsigned_type old_bids_size = _bids.size();
        unsigned_type new_bids_size = div_ceil(n, block_type::size);
        unsigned_type new_pages = div_ceil(new_bids_size, page_size);
        _page_status.resize(new_pages, uninitialized);
        _page_to_slot.resize(new_pages, on_disk);

        _bids.resize(new_bids_size);
        if (_from == NULL)
        {
            bm->new_blocks(alloc_strategy, _bids.begin() + old_bids_size, _bids.end(), old_bids_size);
        }
        else
        {
            size_type offset = size_type(old_bids_size) * size_type(block_type::raw_size);
            bids_container_iterator it = _bids.begin() + old_bids_size;
            for ( ; it != _bids.end(); ++it, offset += size_type(block_type::raw_size))
            {
                (*it).storage = _from;
                (*it).offset = offset;
            }
            STXXL_VERBOSE1("vector::reserve(): Changing size of file " << ((void *)_from) << " to "
                                                                       << offset);
            _from->set_size(offset);
        }
    }

    void resize(size_type n)
    {
        _resize(n);
    }

    void resize(size_type n, bool shrink_capacity)
    {
        if (shrink_capacity)
            _resize_shrink_capacity(n);
        else
            _resize(n);
    }

private:
    void _resize(size_type n)
    {
        reserve(n);
        if (n < _size) {
            // mark excess pages as uninitialized and evict them from cache
            unsigned_type first_page_to_evict = div_ceil(n, block_type::size * page_size);
            for (unsigned_type i = first_page_to_evict; i < _page_status.size(); ++i) {
                if (_page_to_slot[i] != on_disk) {
                    _free_slots.push(_page_to_slot[i]);
                    _page_to_slot[i] = on_disk;
                }
                _page_status[i] = uninitialized;
            }
        }
        _size = n;
    }

    void _resize_shrink_capacity(size_type n)
    {
        unsigned_type old_bids_size = _bids.size();
        unsigned_type new_bids_size = div_ceil(n, block_type::size);

        if (new_bids_size > old_bids_size)
        {
            reserve(n);
        }
        else if (new_bids_size < old_bids_size)
        {
            if (_from != NULL)
                _from->set_size(new_bids_size * block_type::raw_size);
            else
                bm->delete_blocks(_bids.begin() + old_bids_size, _bids.end());

            _bids.resize(new_bids_size);
            unsigned_type new_pages = div_ceil(new_bids_size, page_size);
            _page_status.resize(new_pages);

            unsigned_type first_page_to_evict = div_ceil(new_bids_size, page_size);
            // clear dirty flag, so these pages will be never written
            std::fill(_page_status.begin() + first_page_to_evict,
                      _page_status.end(), (unsigned char)valid_on_disk);
        }

        _size = n;
    }

public:
    void clear()
    {
        _size = 0;
        if (_from == NULL)
            bm->delete_blocks(_bids.begin(), _bids.end());

        _bids.clear();
        _page_status.clear();
        _page_to_slot.clear();
        while (!_free_slots.empty())
            _free_slots.pop();


        for (int_type i = 0; i < n_pages; ++i)
            _free_slots.push(i);
    }

    void push_back(const_reference obj)
    {
        size_type old_size = _size;
        resize(old_size + 1);
        element(old_size) = obj;
    }
    void pop_back()
    {
        resize(_size - 1);
    }

    reference back()
    {
        return element(_size - 1);
    }
    reference front()
    {
        return element(0);
    }
    const_reference back() const
    {
        return const_element(_size - 1);
    }
    const_reference front() const
    {
        return const_element(0);
    }

    //! \brief Construct vector from a file
    //! \param from file to be constructed from
    //! \warning Only one \c vector can be assigned to a particular (physical) file.
    //! The block size of the vector must be a multiple of the element size
    //! \c sizeof(Tp_) and the page size (4096).
    vector(file * from, size_type size = size_type(-1)) :
        _size((size == size_type(-1)) ? size_from_file_length(from->size()) : size),
        _bids(div_ceil(_size, size_type(block_type::size))),
        _page_status(div_ceil(_bids.size(), page_size)),
        _page_to_slot(div_ceil(_bids.size(), page_size)),
        _slot_to_page(n_pages),
        _cache(NULL),
        _from(from),
        exported(false)
    {
        // initialize from file
        if (!block_type::has_only_data)
        {
            std::ostringstream str_;
            str_ << "The block size for a vector that is mapped to a file must be a multiple of the element size (" <<
            sizeof(value_type) << ") and the page size (4096).";
            throw std::runtime_error(str_.str());
        }

        bm = block_manager::get_instance();
        cfg = config::get_instance();

        allocate_page_cache();
        int_type all_pages = div_ceil(_bids.size(), page_size);
        int_type i = 0;
        for ( ; i < all_pages; ++i)
        {
            _page_status[i] = valid_on_disk;
            _page_to_slot[i] = on_disk;
        }

        for (i = 0; i < n_pages; ++i)
            _free_slots.push(i);


        //allocate blocks equidistantly and in-order
        size_type offset = 0;
        bids_container_iterator it = _bids.begin();
        for ( ; it != _bids.end(); ++it, offset += size_type(block_type::raw_size))
        {
            (*it).storage = from;
            (*it).offset = offset;
        }
        from->set_size(offset);
    }

    vector(const vector & obj) :
        _size(obj.size()),
        _bids(div_ceil(obj.size(), block_type::size)),
        _page_status(div_ceil(_bids.size(), page_size)),
        _page_to_slot(div_ceil(_bids.size(), page_size)),
        _slot_to_page(n_pages),
        _cache(NULL),
        _from(NULL),
        exported(false)
    {
        assert(!obj.exported);
        bm = block_manager::get_instance();
        cfg = config::get_instance();

        allocate_page_cache();
        int_type all_pages = div_ceil(_bids.size(), page_size);
        int_type i = 0;
        for ( ; i < all_pages; ++i)
        {
            _page_status[i] = uninitialized;
            _page_to_slot[i] = on_disk;
        }

        for (i = 0; i < n_pages; ++i)
            _free_slots.push(i);

        bm->new_blocks(alloc_strategy, _bids.begin(), _bids.end(), 0);

        const_iterator inbegin = obj.begin();
        const_iterator inend = obj.end();
        std::copy(inbegin, inend, begin());
    }

    vector & operator = (const vector & obj)
    {
        if (&obj != this)
        {
            vector tmp(obj);
            this->swap(tmp);
        }
        return *this;
    }

    size_type size() const
    {
        return _size;
    }
    bool empty() const
    {
        return (!_size);
    }
    iterator begin()
    {
        return iterator(this, 0);
    }
    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        return iterator(this, _size);
    }
    const_iterator end() const
    {
        return const_iterator(this, _size);
    }
    const_iterator cend() const
    {
        return end();
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(end());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(begin());
    }

    reference operator [] (size_type offset)
    {
        return element(offset);
    }
    const_reference operator [] (size_type offset) const
    {
        return const_element(offset);
    }

    bool is_element_cached(size_type offset) const
    {
        return is_page_cached(blocked_index_type(offset));
    }

    void flush() const
    {
        simple_vector<bool> non_free_slots(n_pages);
        int_type i = 0;
        for ( ; i < n_pages; i++)
            non_free_slots[i] = true;

        while (!_free_slots.empty())
        {
            non_free_slots[_free_slots.front()] = false;
            _free_slots.pop();
        }

        for (i = 0; i < n_pages; i++)
        {
            _free_slots.push(i);
            int_type page_no = _slot_to_page[i];
            if (non_free_slots[i])
            {
                STXXL_VERBOSE1("vector: flushing page " << i << " at address "
                                                        << (int64(page_no) * int64(block_type::size) * int64(page_size)));
                write_page(page_no, i);

                _page_to_slot[page_no] = on_disk;
            }
        }
    }

    ~vector()
    {
        STXXL_VERBOSE("~vector()");
        try
        {
            flush();
        }
        catch (io_error e)
        {
            STXXL_ERRMSG("io_error thrown in ~vector(): " << e.what());
        }
        catch (...)
        {
            STXXL_ERRMSG("Exception thrown in ~vector()");
        }

        if (!exported)
        {
            if (_from == NULL)
                bm->delete_blocks(_bids.begin(), _bids.end());
            else        // file must be truncated
            {
                STXXL_VERBOSE1("~vector(): Changing size of file " << ((void *)_from) << " to "
                                                                   << file_length());
                STXXL_VERBOSE1("~vector(): size of the vector is " << size());
                try
                {
                    _from->set_size(file_length());
                }
                catch (...)
                {
                    STXXL_ERRMSG("Exception thrown in ~vector()...set_size()");
                }
            }
        }
        delete _cache;
    }

    //! \brief Export data such that it is persistent on the file system.
    //! Resulting files will be numbered ascending.
    void export_files(std::string filename_prefix)
    {
        int64 no = 0;
        for (bids_container_iterator i = _bids.begin(); i != _bids.end(); ++i) {
            std::ostringstream number;
            number << std::setw(9) << std::setfill('0') << no;
            size_type current_block_size =
                ((i + 1) == _bids.end() && _size % block_type::size > 0) ?
                (_size % block_type::size) * sizeof(value_type) :
                block_type::size * sizeof(value_type);
            (*i).storage->export_files((*i).offset, current_block_size, filename_prefix + number.str());
            ++no;
        }
        exported = true;
    }

    //! \brief Get the file associated with this vector, or NULL.
    file * get_file() const
    {
        return _from;
    }

    //! \brief Set the blocks and the size of this container explicitly.
    //! The vector must be completely empty before.
    template <typename ForwardIterator>
    void set_content(const ForwardIterator & bid_begin, const ForwardIterator & bid_end, size_type n)
    {
        unsigned_type new_bids_size = div_ceil(n, block_type::size);
        _bids.resize(new_bids_size);
        std::copy(bid_begin, bid_end, _bids.begin());
        unsigned_type new_pages = div_ceil(new_bids_size, page_size);
        _page_status.resize(new_pages, valid_on_disk);
        _page_to_slot.resize(new_pages, on_disk);
        _size = n;
    }

private:
    bids_container_iterator bid(const size_type & offset)
    {
        return (_bids.begin() +
                static_cast<typename bids_container_type::size_type>
                (offset / block_type::size));
    }
    bids_container_iterator bid(const blocked_index_type & offset)
    {
        return (_bids.begin() +
                static_cast<typename bids_container_type::size_type>
                (offset.get_block2() * PgSz_ + offset.get_block1()));
    }
    const_bids_container_iterator bid(const size_type & offset) const
    {
        return (_bids.begin() +
                static_cast<typename bids_container_type::size_type>
                (offset / block_type::size));
    }
    const_bids_container_iterator bid(const blocked_index_type & offset) const
    {
        return (_bids.begin() +
                static_cast<typename bids_container_type::size_type>
                (offset.get_block2() * PgSz_ + offset.get_block1()));
    }

    void read_page(int_type page_no, int_type cache_slot) const
    {
        if (_page_status[page_no] == uninitialized)
            return;
        STXXL_VERBOSE1("vector " << this << ": reading page_no=" << page_no << " cache_slot=" << cache_slot);
        request_ptr * reqs = new request_ptr[page_size];
        int_type block_no = page_no * page_size;
        int_type last_block = STXXL_MIN(block_no + page_size, int_type(_bids.size()));
        int_type i = cache_slot * page_size, j = 0;
        for ( ; block_no < last_block; ++block_no, ++i, ++j)
        {
            reqs[j] = (*_cache)[i].read(_bids[block_no]);
        }
        assert(last_block - page_no * page_size > 0);
        wait_all(reqs, last_block - page_no * page_size);
        delete[] reqs;
    }
    void write_page(int_type page_no, int_type cache_slot) const
    {
        if (!(_page_status[page_no] & dirty))
            return;
        STXXL_VERBOSE1("vector " << this << ": writing page_no=" << page_no << " cache_slot=" << cache_slot);
        request_ptr * reqs = new request_ptr[page_size];
        int_type block_no = page_no * page_size;
        int_type last_block = STXXL_MIN(block_no + page_size, int_type(_bids.size()));
        int_type i = cache_slot * page_size, j = 0;
        for ( ; block_no < last_block; ++block_no, ++i, ++j)
        {
            reqs[j] = (*_cache)[i].write(_bids[block_no]);
        }
        _page_status[page_no] = valid_on_disk;
        assert(last_block - page_no * page_size > 0);
        wait_all(reqs, last_block - page_no * page_size);
        delete[] reqs;
    }

    reference element(size_type offset)
    {
        #ifdef STXXL_RANGE_CHECK
        assert(offset < size());
        #endif
        return element(blocked_index_type(offset));
    }

    reference element(const blocked_index_type & offset)
    {
        #ifdef STXXL_RANGE_CHECK
        assert(offset.get_pos() < size());
        #endif
        int_type page_no = offset.get_block2();
        assert(page_no < int_type(_page_to_slot.size()));   // fails if offset is too large, out of bound access
        int_type cache_slot = _page_to_slot[page_no];
        if (cache_slot < 0)                                 // == on_disk
        {
            if (_free_slots.empty())                        // has to kick
            {
                int_type kicked_slot = pager.kick();
                pager.hit(kicked_slot);
                int_type old_page_no = _slot_to_page[kicked_slot];
                _page_to_slot[page_no] = kicked_slot;
                _page_to_slot[old_page_no] = on_disk;
                _slot_to_page[kicked_slot] = page_no;

                write_page(old_page_no, kicked_slot);
                read_page(page_no, kicked_slot);

                _page_status[page_no] = dirty;

                return (*_cache)[kicked_slot * page_size + offset.get_block1()][offset.get_offset()];
            }
            else
            {
                int_type free_slot = _free_slots.front();
                _free_slots.pop();
                pager.hit(free_slot);
                _page_to_slot[page_no] = free_slot;
                _slot_to_page[free_slot] = page_no;

                read_page(page_no, free_slot);

                _page_status[page_no] = dirty;

                return (*_cache)[free_slot * page_size + offset.get_block1()][offset.get_offset()];
            }
        }
        else
        {
            _page_status[page_no] = dirty;
            pager.hit(cache_slot);
            return (*_cache)[cache_slot * page_size + offset.get_block1()][offset.get_offset()];
        }
    }

    // don't forget to first flush() the vector's cache before updating pages externally
    void page_externally_updated(unsigned_type page_no) const
    {
        // fails if offset is too large, out of bound access
        assert(page_no < _page_status.size());
        // "A dirty page has been marked as newly initialized. The page content will be lost."
        assert(!(_page_status[page_no] & dirty));
        if (_page_to_slot[page_no] != on_disk) {
            // remove page from cache
            _free_slots.push(_page_to_slot[page_no]);
            _page_to_slot[page_no] = on_disk;
        }
        _page_status[page_no] = valid_on_disk;
    }

    void block_externally_updated(size_type offset) const
    {
        page_externally_updated(offset / (block_type::size * page_size));
    }

    void block_externally_updated(const blocked_index_type & offset) const
    {
        page_externally_updated(offset.get_block2());
    }

    _STXXL_DEPRECATED(void touch(size_type offset) const)
    {
        page_externally_updated(offset / (block_type::size * page_size));
    }

    _STXXL_DEPRECATED(void touch(const blocked_index_type & offset) const)
    {
        page_externally_updated(offset.get_block2());
    }

    const_reference const_element(size_type offset) const
    {
        return const_element(blocked_index_type(offset));
    }

    const_reference const_element(const blocked_index_type & offset) const
    {
        int_type page_no = offset.get_block2();
        assert(page_no < int_type(_page_to_slot.size()));   // fails if offset is too large, out of bound access
        int_type cache_slot = _page_to_slot[page_no];
        if (cache_slot < 0)                                 // == on_disk
        {
            if (_free_slots.empty())                        // has to kick
            {
                int_type kicked_slot = pager.kick();
                pager.hit(kicked_slot);
                int_type old_page_no = _slot_to_page[kicked_slot];
                _page_to_slot[page_no] = kicked_slot;
                _page_to_slot[old_page_no] = on_disk;
                _slot_to_page[kicked_slot] = page_no;

                write_page(old_page_no, kicked_slot);
                read_page(page_no, kicked_slot);

                return (*_cache)[kicked_slot * page_size + offset.get_block1()][offset.get_offset()];
            }
            else
            {
                int_type free_slot = _free_slots.front();
                _free_slots.pop();
                pager.hit(free_slot);
                _page_to_slot[page_no] = free_slot;
                _slot_to_page[free_slot] = page_no;

                read_page(page_no, free_slot);

                return (*_cache)[free_slot * page_size + offset.get_block1()][offset.get_offset()];
            }
        }
        else
        {
            pager.hit(cache_slot);
            return (*_cache)[cache_slot * page_size + offset.get_block1()][offset.get_offset()];
        }
    }

    bool is_page_cached(const blocked_index_type & offset) const
    {
        int_type page_no = offset.get_block2();
        assert(page_no < int_type(_page_to_slot.size()));   // fails if offset is too large, out of bound access
        int_type cache_slot = _page_to_slot[page_no];
        return (cache_slot >= 0);                           // on_disk == -1
    }
};

template <
    typename Tp_,
    unsigned PgSz_,
    typename PgTp_,
    unsigned BlkSize_,
    typename AllocStr_,
    typename SzTp_>
inline bool operator == (stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                       AllocStr_, SzTp_> & a,
                         stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                       AllocStr_, SzTp_> & b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

template <
    typename Tp_,
    unsigned PgSz_,
    typename PgTp_,
    unsigned BlkSize_,
    typename AllocStr_,
    typename SzTp_>
inline bool operator != (stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                       AllocStr_, SzTp_> & a,
                         stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                       AllocStr_, SzTp_> & b)
{
    return !(a == b);
}

template <
    typename Tp_,
    unsigned PgSz_,
    typename PgTp_,
    unsigned BlkSize_,
    typename AllocStr_,
    typename SzTp_>
inline bool operator < (stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                      AllocStr_, SzTp_> & a,
                        stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                      AllocStr_, SzTp_> & b)
{
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <
    typename Tp_,
    unsigned PgSz_,
    typename PgTp_,
    unsigned BlkSize_,
    typename AllocStr_,
    typename SzTp_>
inline bool operator > (stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                      AllocStr_, SzTp_> & a,
                        stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                      AllocStr_, SzTp_> & b)
{
    return b < a;
}

template <
    typename Tp_,
    unsigned PgSz_,
    typename PgTp_,
    unsigned BlkSize_,
    typename AllocStr_,
    typename SzTp_>
inline bool operator <= (stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                       AllocStr_, SzTp_> & a,
                         stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                       AllocStr_, SzTp_> & b)
{
    return !(b < a);
}

template <
    typename Tp_,
    unsigned PgSz_,
    typename PgTp_,
    unsigned BlkSize_,
    typename AllocStr_,
    typename SzTp_>
inline bool operator >= (stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                       AllocStr_, SzTp_> & a,
                         stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_,
                                       AllocStr_, SzTp_> & b)
{
    return !(a < b);
}

//! \}

////////////////////////////////////////////////////////////////////////////

// specialization for stxxl::vector, to use only const_iterators
template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
          unsigned BlkSize_, typename PgTp_, unsigned PgSz_>
bool is_sorted(
    stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> __first,
    stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> __last)
{
    return is_sorted_helper(
               stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>(__first),
               stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>(__last));
}

template <typename Tp_, typename AllocStr_, typename SzTp_, typename DiffTp_,
          unsigned BlkSize_, typename PgTp_, unsigned PgSz_, typename _StrictWeakOrdering>
bool is_sorted(
    stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> __first,
    stxxl::vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_> __last,
    _StrictWeakOrdering __comp)
{
    return is_sorted_helper(
               stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>(__first),
               stxxl::const_vector_iterator<Tp_, AllocStr_, SzTp_, DiffTp_, BlkSize_, PgTp_, PgSz_>(__last),
               __comp);
}

////////////////////////////////////////////////////////////////////////////

//! \addtogroup stlcont
//! \{

//! \brief External vector type generator

//!  \tparam Tp_ type of contained objects (POD with no references to internal memory)
//!  \tparam PgSz_ number of blocks in a page
//!  \tparam Pages_ number of pages
//!  \tparam BlkSize_ external block size in bytes, default is 2 MiB
//!  \tparam AllocStr_ one of allocation strategies: \c striping , \c RC , \c SR , or \c FR
//!  default is RC
//!  \tparam Pager_ pager type:
//!    - \c random ,
//!    - \c lru , is default
//!
//! Memory consumption of constructed vector is BlkSize_*Pages_*PgSz_ bytes
//! Configured vector type is available as \c STACK_GENERATOR<>::result. <BR> <BR>
//! Examples:
//!    - \c VECTOR_GENERATOR<double>::result external vector of \c double's ,
//!    - \c VECTOR_GENERATOR<double,8>::result external vector of \c double's ,
//!      with 8 blocks per page,
//!    - \c VECTOR_GENERATOR<double,8,2,512*1024,RC,lru>::result external vector of \c double's ,
//!      with 8 blocks per page, 2 pages, 512 KiB blocks, Random Cyclic allocation
//!      and lru cache replacement strategy
//! \warning Do not store references to the elements of an external vector. Such references
//! might be invalidated during any following access to elements of the vector
template <
    typename Tp_,
    unsigned PgSz_ = 4,
    unsigned Pages_ = 8,
    unsigned BlkSize_ = STXXL_DEFAULT_BLOCK_SIZE(Tp_),
    typename AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY,
    pager_type Pager_ = lru
    >
struct VECTOR_GENERATOR
{
    typedef typename IF<Pager_ == lru,
                        lru_pager<Pages_>, random_pager<Pages_> >::result PagerType;

    typedef vector<Tp_, PgSz_, PagerType, BlkSize_, AllocStr_> result;
};


//! \}

__STXXL_END_NAMESPACE


namespace std
{
    template <
        typename Tp_,
        unsigned PgSz_,
        typename PgTp_,
        unsigned BlkSize_,
        typename AllocStr_,
        typename SzTp_>
    void swap(stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_, SzTp_> & a,
              stxxl::vector<Tp_, PgSz_, PgTp_, BlkSize_, AllocStr_, SzTp_> & b)
    {
        a.swap(b);
    }
}

#endif // !STXXL_VECTOR_HEADER
// vim: et:ts=4:sw=4
