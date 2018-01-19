/***************************************************************************
 *  include/stxxl/bits/containers/vector.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002-2008 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_VECTOR_HEADER
#define STXXL_CONTAINERS_VECTOR_HEADER

#include <algorithm>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <tlx/define.hpp>
#include <tlx/logger.hpp>

#include <foxxll/common/tmeta.hpp>
#include <foxxll/common/types.hpp>
#include <foxxll/io/request_operations.hpp>
#include <foxxll/mng/block_manager.hpp>
#include <foxxll/mng/buf_istream.hpp>
#include <foxxll/mng/buf_istream_reverse.hpp>
#include <foxxll/mng/buf_ostream.hpp>
#include <foxxll/mng/typed_block.hpp>

#include <stxxl/bits/common/is_sorted.h>
#include <stxxl/bits/containers/pager.h>
#include <stxxl/bits/defines.h>
#include <stxxl/bits/deprecated.h>
#include <stxxl/types>

namespace stxxl {

//! \defgroup stlcont Containers
//! \ingroup stllayer
//! Containers with STL-compatible interface

//! \defgroup stlcont_vector vector
//! \ingroup stlcont
//! Vector and support classes
//! \{

template <typename SizeType, SizeType modulo2, SizeType modulo1>
class double_blocked_index
{
    static_assert(std::is_unsigned<SizeType>::value, "SizeType is expected to be unsigned");
    using size_type = SizeType;

    static constexpr size_type modulo12 = modulo1 * modulo2;

    size_type pos;
    size_t block1;
    size_t block2;
    size_t offset;

    //! \invariant block2 * modulo12 + block1 * modulo1 + offset == pos && 0 <= block1 &lt; modulo2 && 0 <= offset &lt; modulo1

    void set(size_type pos)
    {
        this->pos = pos;
        block2 = static_cast<size_t>(pos / modulo12);
        pos -= block2 * modulo12;
        block1 = static_cast<size_t>(pos / modulo1);
        offset = static_cast<size_t>(pos - block1 * modulo1);

        assert(block2 * modulo12 + block1 * modulo1 + offset == this->pos);
        assert(/* 0 <= block1 && */ block1 < modulo2);
        assert(/* 0 <= offset && */ offset < modulo1);
    }

public:
    explicit double_blocked_index(const size_type& pos = 0)
    {
        set(pos);
    }

    double_blocked_index(const size_t block2, const size_t block1, const size_t offset)
    {
        assert(/* 0 <= block1 && */ block1 < modulo2);
        assert(/* 0 <= offset && */ offset < modulo1);

        this->block2 = block2;
        this->block1 = block1;
        this->offset = offset;
        pos = block2 * modulo12 + block1 * modulo1 + offset;
    }

    double_blocked_index& operator = (const size_type pos)
    {
        set(pos);
        return *this;
    }

    //pre-increment operator
    double_blocked_index& operator ++ ()
    {
        ++pos;
        ++offset;
        if (TLX_UNLIKELY(offset == modulo1))
        {
            offset = 0;
            ++block1;
            if (TLX_UNLIKELY(block1 == modulo2))
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
    double_blocked_index& operator -- ()
    {
        --pos;
        if (TLX_UNLIKELY(offset == 0))
        {
            offset = modulo1;
            if (TLX_UNLIKELY(block1 == 0))
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

    double_blocked_index operator + (const size_type addend) const
    {
        return double_blocked_index(pos + addend);
    }

    double_blocked_index& operator += (const size_type addend)
    {
        set(pos + addend);
        return *this;
    }

    double_blocked_index operator - (const size_type addend) const
    {
        return double_blocked_index(pos - addend);
    }

    size_type operator - (const double_blocked_index& dbi2) const
    {
        return pos - dbi2.pos;
    }

    double_blocked_index& operator -= (const size_type subtrahend)
    {
        set(pos - subtrahend);
        return *this;
    }

    bool operator == (const double_blocked_index& dbi2) const
    {
        return pos == dbi2.pos;
    }

    bool operator != (const double_blocked_index& dbi2) const
    {
        return pos != dbi2.pos;
    }

    bool operator < (const double_blocked_index& dbi2) const
    {
        return pos < dbi2.pos;
    }

    bool operator <= (const double_blocked_index& dbi2) const
    {
        return pos <= dbi2.pos;
    }

    bool operator > (const double_blocked_index& dbi2) const
    {
        return pos > dbi2.pos;
    }

    bool operator >= (const double_blocked_index& dbi2) const
    {
        return pos >= dbi2.pos;
    }

    double_blocked_index& operator >>= (size_type shift)
    {
        set(pos >> shift);
        return *this;
    }

    const size_type & get_pos() const
    {
        return pos;
    }

    const size_t & get_block2() const
    {
        return block2;
    }

    const size_t & get_block1() const
    {
        return block1;
    }

    const size_t & get_offset() const
    {
        return offset;
    }
};

////////////////////////////////////////////////////////////////////////////

template <
    typename ValueType,
    unsigned PageSize,
    typename PagerType,
    size_t BlockSize,
    typename AllocStr>
class vector;

template <
    typename ValueType,
    unsigned PageSize,
    typename PagerType,
    size_t BlockSize,
    typename AllocStr>
struct vector_configuration {
    using value_type = ValueType;
    static constexpr unsigned page_size = PageSize;
    using pager_type = PagerType;
    static constexpr size_t block_size = BlockSize;
    using alloc_str = AllocStr;

    using vector_type = vector<ValueType, PageSize, PagerType, BlockSize, AllocStr>;
};

template <typename VectorConfig>
class const_vector_iterator;

template <typename VectorIteratorType>
class vector_bufreader;

template <typename VectorIteratorType>
class vector_bufreader_reverse;

template <typename VectorIteratorType>
class vector_bufwriter;

////////////////////////////////////////////////////////////////////////////

//! External vector iterator, model of \c ext_random_access_iterator concept.
template <typename VectorConfig>
class vector_iterator
{
    using self_type = vector_iterator<VectorConfig>;

    using const_self_type = const_vector_iterator<VectorConfig>;

    friend class const_vector_iterator<VectorConfig>;

public:
    //! \name Types
    //! \{

    using iterator = self_type;
    using const_iterator = const_self_type;

    using block_offset_type = unsigned;
    using vector_type = typename VectorConfig::vector_type;
    friend vector_type;
    using bids_container_type = typename vector_type::bids_container_type;
    using bids_container_iterator = typename bids_container_type::iterator;
    using bid_type = typename bids_container_type::bid_type;
    using block_type = typename vector_type::block_type;
    using blocked_index_type = typename vector_type::blocked_index_type;

    using iterator_category = std::random_access_iterator_tag;
    using size_type = typename vector_type::size_type;
    using difference_type = typename vector_type::difference_type;
    using value_type = typename vector_type::value_type;
    using reference = typename vector_type::reference;
    using const_reference = typename vector_type::const_reference;
    using pointer = typename vector_type::pointer;
    using const_pointer = typename vector_type::const_pointer;

    //! \}

protected:
    blocked_index_type offset;
    vector_type* p_vector;

private:
    //! private constructor for initializing other iterators
    vector_iterator(vector_type* v, size_type o)
        : offset(o), p_vector(v)
    { }

public:
    //! constructs invalid iterator
    vector_iterator()
        : offset(0), p_vector(nullptr)
    { }
    //! copy-constructor
    vector_iterator(const vector_iterator& a)
        : offset(a.offset),
          p_vector(a.p_vector)
    { }

    //! \name Iterator Properties
    //! \{

    //! return pointer to vector containing iterator
    vector_type * parent_vector() const
    {
        return p_vector;
    }
    //! return block offset of current element
    block_offset_type block_offset() const
    {
        return static_cast<block_offset_type>(offset.get_offset());
    }
    //! return iterator to BID containg current element
    bids_container_iterator bid() const
    {
        return p_vector->bid(offset);
    }

    //! \}

    //! \name Access Operators
    //! \{

    //! return current element
    reference operator * ()
    {
        return p_vector->element(offset);
    }
    //! return pointer to current element
    pointer operator -> ()
    {
        return &(p_vector->element(offset));
    }
    //! return const reference to current element
    const_reference operator * () const
    {
        return p_vector->const_element(offset);
    }
    //! return const pointer to current element
    const_pointer operator -> () const
    {
        return &(p_vector->const_element(offset));
    }
    //! return mutable reference to element +i after the current element
    reference operator [] (size_type i)
    {
        return p_vector->element(offset.get_pos() + i);
    }

#ifdef _LIBCPP_VERSION
    //-tb 2013-11: libc++ defines std::reverse_iterator::operator[] in such a
    // way that it expects vector_iterator::operator[] to return a (mutable)
    // reference. Thus to remove confusion about the compiler error, we remove
    // the operator[] const for libc++. The const_reference actually violates
    // some version of the STL standard, but works well in gcc's libstdc++.
#else
    //! return const reference to element +i after the current element
    const_reference operator [] (size_type i) const
    {
        return p_vector->const_element(offset.get_pos() + i);
    }
#endif
    //! \}

    //! \name Relative Calculation of Iterators
    //! \{

    //! calculate different between two iterator
    difference_type operator - (const self_type& a) const
    {
        return offset - a.offset;
    }
    //! calculate different between two iterator
    difference_type operator - (const const_self_type& a) const
    {
        return offset - a.offset;
    }
    //! return iterator advanced -i positions in the vector
    self_type operator - (size_type i) const
    {
        return self_type(p_vector, offset.get_pos() - i);
    }
    //! return iterator advanced +i positions in the vector
    self_type operator + (size_type i) const
    {
        return self_type(p_vector, offset.get_pos() + i);
    }
    //! advance this iterator -i positions in the vector
    self_type& operator -= (size_type i)
    {
        offset -= i;
        return *this;
    }
    //! advance this iterator +i positions in the vector
    self_type& operator += (size_type i)
    {
        offset += i;
        return *this;
    }
    //! advance this iterator to next position in the vector
    self_type& operator ++ ()
    {
        offset++;
        return *this;
    }
    //! advance this iterator to next position in the vector
    self_type operator ++ (int)
    {
        self_type tmp = *this;
        offset++;
        return tmp;
    }
    //! advance this iterator to preceding position in the vector
    self_type& operator -- ()
    {
        offset--;
        return *this;
    }
    //! advance this iterator to preceding position in the vector
    self_type operator -- (int)
    {
        self_type tmp = *this;
        offset--;
        return tmp;
    }

    //! \}

    //! \name Comparison Operators
    //! \{

    bool operator == (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset == a.offset;
    }
    bool operator != (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset != a.offset;
    }
    bool operator < (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset < a.offset;
    }
    bool operator <= (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset <= a.offset;
    }
    bool operator > (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset > a.offset;
    }
    bool operator >= (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset >= a.offset;
    }

    bool operator == (const const_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset == a.offset;
    }
    bool operator != (const const_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset != a.offset;
    }
    bool operator < (const const_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset < a.offset;
    }
    bool operator <= (const const_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset <= a.offset;
    }
    bool operator > (const const_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset > a.offset;
    }
    bool operator >= (const const_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset >= a.offset;
    }

    //! \}

    //! \name Flushing Operation
    //! \{

    void block_externally_updated()
    {
        p_vector->block_externally_updated(offset);
    }

    void flush()
    {
        p_vector->flush();
    }

    //! \}
};

////////////////////////////////////////////////////////////////////////////

//! Const external vector iterator, model of \c ext_random_access_iterator concept.
template <typename VectorConfig>
class const_vector_iterator
{
    using self_type = const_vector_iterator<VectorConfig>;

    using mutable_self_type = vector_iterator<VectorConfig>;

    friend class vector_iterator<VectorConfig>;

public:
    //! \name Types
    //! \{

    using const_iterator = self_type;
    using iterator = mutable_self_type;

    using block_offset_type = unsigned;
    using vector_type = typename VectorConfig::vector_type;
    friend vector_type;
    using bids_container_type = typename vector_type::bids_container_type;
    using bids_container_iterator = typename bids_container_type::iterator;
    using bid_type = typename bids_container_type::bid_type;
    using block_type = typename vector_type::block_type;
    using blocked_index_type = typename vector_type::blocked_index_type;

    using iterator_category = std::random_access_iterator_tag;
    using size_type = typename vector_type::size_type;
    using difference_type = typename vector_type::difference_type;
    using value_type = typename vector_type::value_type;
    using reference = typename vector_type::const_reference;
    using const_reference = typename vector_type::const_reference;
    using pointer = typename vector_type::const_pointer;
    using const_pointer = typename vector_type::const_pointer;

    //! \}

protected:
    blocked_index_type offset;
    const vector_type* p_vector;

private:
    //! private constructor for initializing other iterators
    const_vector_iterator(const vector_type* v, size_type o)
        : offset(o), p_vector(v)
    { }

public:
    //! constructs invalid iterator
    const_vector_iterator()
        : offset(0), p_vector(nullptr)
    { }
    //! copy-constructor
    const_vector_iterator(const const_vector_iterator& a)
        : offset(a.offset), p_vector(a.p_vector)
    { }
    //! implicit conversion from mutable iterator
    const_vector_iterator(const mutable_self_type& a) // NOLINT
        : offset(a.offset), p_vector(a.p_vector)
    { }

    //! \name Iterator Properties
    //! \{

    //! return pointer to vector containing iterator
    const vector_type * parent_vector() const
    {
        return p_vector;
    }
    //! return block offset of current element
    block_offset_type block_offset() const
    {
        return static_cast<block_offset_type>(offset.get_offset());
    }
    //! return iterator to BID containg current element
    bids_container_iterator bid() const
    {
        return ((vector_type*)p_vector)->bid(offset);
    }

    //! \}

    //! \name Access Operators
    //! \{

    //! return current element
    const_reference operator * () const
    {
        return p_vector->const_element(offset);
    }
    //! return pointer to current element
    const_pointer operator -> () const
    {
        return &(p_vector->const_element(offset));
    }
    //! return const reference to element +i after the current element
    const_reference operator [] (size_type i) const
    {
        return p_vector->const_element(offset.get_pos() + i);
    }

    //! \}

    //! \name Relative Calculation of Iterators
    //! \{

    //! calculate different between two iterator
    difference_type operator - (const self_type& a) const
    {
        return offset - a.offset;
    }
    //! calculate different between two iterator
    difference_type operator - (const mutable_self_type& a) const
    {
        return offset - a.offset;
    }
    //! return iterator advanced -i positions in the vector
    self_type operator - (size_type i) const
    {
        return self_type(p_vector, offset.get_pos() - i);
    }
    //! return iterator advanced +i positions in the vector
    self_type operator + (size_type i) const
    {
        return self_type(p_vector, offset.get_pos() + i);
    }
    //! advance this iterator -i positions in the vector
    self_type& operator -= (size_type i)
    {
        offset -= i;
        return *this;
    }
    //! advance this iterator +i positions in the vector
    self_type& operator += (size_type i)
    {
        offset += i;
        return *this;
    }
    //! advance this iterator to next position in the vector
    self_type& operator ++ ()
    {
        offset++;
        return *this;
    }
    //! advance this iterator to next position in the vector
    self_type operator ++ (int)
    {
        self_type tmp_ = *this;
        offset++;
        return tmp_;
    }
    //! advance this iterator to preceding position in the vector
    self_type& operator -- ()
    {
        offset--;
        return *this;
    }
    //! advance this iterator to preceding position in the vector
    self_type operator -- (int)
    {
        self_type tmp = *this;
        offset--;
        return tmp;
    }

    //! \}

    //! \name Comparison Operators
    //! \{

    bool operator == (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset == a.offset;
    }
    bool operator != (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset != a.offset;
    }
    bool operator < (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset < a.offset;
    }
    bool operator <= (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset <= a.offset;
    }
    bool operator > (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset > a.offset;
    }
    bool operator >= (const self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset >= a.offset;
    }

    bool operator == (const mutable_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset == a.offset;
    }
    bool operator != (const mutable_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset != a.offset;
    }
    bool operator < (const mutable_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset < a.offset;
    }
    bool operator <= (const mutable_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset <= a.offset;
    }
    bool operator > (const mutable_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset > a.offset;
    }
    bool operator >= (const mutable_self_type& a) const
    {
        assert(p_vector == a.p_vector);
        return offset >= a.offset;
    }

    //! \}

    //! \name Flushing Operation
    //! \{

    void block_externally_updated()
    {
        p_vector->block_externally_updated(offset);
    }

    void flush()
    {
        p_vector->flush();
    }

    //! \}
};

////////////////////////////////////////////////////////////////////////////

//! External vector container. \n
//! <b>Introduction</b> to vector container: see \ref tutorial_vector tutorial. \n
//! <b>Design and Internals</b> of vector container: see \ref design_vector
//!
//! For semantics of the methods see documentation of the STL std::vector
//! \tparam ValueType type of contained objects (POD with no references to internal memory)
//! \tparam PageSize number of blocks in a page, default: \b 4 (recommended >= D)
//! \tparam PagerType type of the pager: \c random_pager or \c lru_pager, default: \b lru_pager. Both take the number of pages as template parameters, default: \b 8 (recommended >= 2)
//! \tparam BlockSize external block size in bytes, default is <b>2 MiB</b>
//! \tparam AllocStr parallel disk block allocation strategies: \c striping , \c random_cyclic , \c simple_random , or \c fully_random
//!  default is \c random_cyclic
//!
//! Memory consumption: BlockSize*x*PageSize bytes
//! \warning Do not store references to the elements of an external vector. Such references
//! might be invalidated during any following access to elements of the vector
template <
    typename ValueType,
    unsigned PageSize = 4,
    typename PagerType = lru_pager<8>,
    size_t BlockSize = STXXL_DEFAULT_BLOCK_SIZE(ValueType),
    typename AllocStr = foxxll::default_alloc_strategy>
class vector
{
    constexpr static bool debug = false;

public:
    //! \name Standard Types
    //! \{

    //! The type of elements stored in the vector.
    using value_type = ValueType;
    //! reference to value_type
    using reference = value_type &;
    //! constant reference to value_type
    using const_reference = const value_type &;
    //! pointer to value_type
    using pointer = value_type *;
    //! constant pointer to value_type
    using const_pointer = const value_type *;

    using size_type = foxxll::external_size_type;
    using difference_type = foxxll::external_diff_type;

    using pager_type = PagerType;
    using alloc_strategy_type = AllocStr;

    static constexpr size_t page_size = PageSize;
    static constexpr size_t block_size = BlockSize;

    enum { on_disk = -1 };

    using configuration_type = vector_configuration<ValueType, PageSize, PagerType, BlockSize, AllocStr>;
    static_assert(std::is_same<vector, typename configuration_type::vector_type>::value, "Inconsistent vector type in config");

    //! iterator used to iterate through a vector, see \ref design_vector_notes.
    using iterator = vector_iterator<configuration_type>;
    friend iterator;

    //! constant iterator used to iterate through a vector, see \ref design_vector_notes.
    using const_iterator = const_vector_iterator<configuration_type>;
    friend const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    //! \}

    //! \name Extra Types
    //! \{

    //! vector_bufwriter compatible with this vector
    using bufwriter_type = vector_bufwriter<iterator>;

    //! vector_bufreader compatible with this vector
    using bufreader_type = vector_bufreader<const_iterator>;

    //! vector_bufreader compatible with this vector
    using bufreader_reverse_type = vector_bufreader_reverse<const_iterator>;

    //! \internal
    class bid_vector : public std::vector<foxxll::BID<block_size> >
    {
    public:
        using super_type = std::vector<foxxll::BID<block_size> >;
        using size_type = typename super_type::size_type;
        using bid_type = typename super_type::value_type;

        explicit bid_vector(size_type sz) : super_type(sz) { }
    };

    using bids_container_type = bid_vector;
    using bids_container_iterator = typename bids_container_type::iterator;
    using const_bids_container_iterator = typename bids_container_type::const_iterator;

    //! type of the block used in disk-memory transfers
    using block_type = foxxll::typed_block<BlockSize, ValueType>;
    //! double-index type to reference individual elements in a block
    using blocked_index_type = double_blocked_index<size_type, PageSize, block_type::size>;

    //! \}

private:
    alloc_strategy_type m_alloc_strategy;
    size_type m_size;
    bids_container_type m_bids;
    mutable pager_type m_pager;

    // enum specifying status of a page of the vector
    enum page_status : uint8_t {
        valid_on_disk = 0,
        uninitialized = 1,
        dirty = 2
    };

    //! status of each page (valid_on_disk, uninitialized or dirty)
    mutable std::vector<page_status> m_page_status;
    mutable std::vector<ptrdiff_t> m_page_to_slot;
    mutable tlx::simple_vector<size_t> m_slot_to_page;
    mutable std::queue<size_t> m_free_slots;
    mutable tlx::simple_vector<block_type>* m_cache;

    foxxll::file_ptr m_from;
    foxxll::block_manager* m_bm;
    bool m_exported;

    size_type size_from_file_length(foxxll::external_size_type file_length) const
    {
        size_t blocks_fit = file_length / external_size_type(block_type::raw_size);
        size_type cur_size = blocks_fit * external_size_type(block_type::size);
        foxxll::external_size_type rest = file_length - blocks_fit * external_size_type(block_type::raw_size);
        return (cur_size + rest / external_size_type(sizeof(value_type)));
    }

    size_type file_length() const
    {
        size_type cur_size = size();
        size_type num_full_blocks = cur_size / block_type::size;
        if (cur_size % block_type::size != 0)
        {
            size_type rest = cur_size - num_full_blocks * block_type::size;
            return external_size_type(num_full_blocks) * block_type::raw_size + rest * sizeof(value_type);
        }
        return external_size_type(num_full_blocks) * block_type::raw_size;
    }

public:
    //! \name Constructors/Destructors
    //! \{

    //! Constructs external vector with n elements.
    //!
    //! \param n Number of elements.
    //! \param npages Number of cached pages.
    explicit vector(const size_type n = 0, const size_t npages = pager_type::default_npages)
        : m_size(n),
          m_bids((size_t)foxxll::div_ceil(n, block_type::size)),
          m_pager(npages),
          m_page_status(foxxll::div_ceil(m_bids.size(), page_size), uninitialized),
          m_page_to_slot(foxxll::div_ceil(m_bids.size(), page_size), on_disk),
          m_slot_to_page(npages),
          m_cache(nullptr),
          m_exported(false)
    {
        m_bm = foxxll::block_manager::get_instance();

        allocate_page_cache();

        for (size_t i = 0; i < numpages(); ++i)
            m_free_slots.push(i);

        m_bm->new_blocks(m_alloc_strategy, m_bids.begin(), m_bids.end(), 0);
    }

    //! \}

    //! \name Modifier
    //! \{

    //! swap content
    void swap(vector& obj)
    {
        std::swap(m_alloc_strategy, obj.m_alloc_strategy);
        std::swap(m_size, obj.m_size);
        std::swap(m_bids, obj.m_bids);
        std::swap(m_pager, obj.m_pager);
        std::swap(m_page_status, obj.m_page_status);
        std::swap(m_page_to_slot, obj.m_page_to_slot);
        std::swap(m_slot_to_page, obj.m_slot_to_page);
        std::swap(m_free_slots, obj.m_free_slots);
        std::swap(m_cache, obj.m_cache);
        std::swap(m_from, obj.m_from);
        std::swap(m_exported, obj.m_exported);
    }

    //! \}

    //! \name Miscellaneous
    //! \{

    //! Allocate page cache, must be called to allow access to elements.
    void allocate_page_cache() const
    {
        //  numpages() might be zero
        if (!m_cache && numpages() > 0)
            m_cache = new tlx::simple_vector<block_type>(numpages() * page_size);
    }

    //! allows to free the cache, but you may not access any element until call
    //! allocate_page_cache() again
    void deallocate_page_cache() const
    {
        flush();
        delete m_cache;
        m_cache = nullptr;
    }

    //! \}

    //! \name Size and Capacity
    //! \{

    //! return the size of the vector.
    size_type size() const
    {
        return m_size;
    }
    //! true if the vector's size is zero.
    bool empty() const
    {
        return (!m_size);
    }

    //! Return the number of elelemtsn for which \a external memory has been
    //! allocated. capacity() is always greator than or equal to size().
    size_type capacity() const
    {
        return size_type(m_bids.size()) * block_type::size;
    }
    //! Returns the number of bytes that the vector has allocated on disks.
    size_type raw_capacity() const
    {
        return size_type(m_bids.size()) * block_type::raw_size;
    }

    /*! Reserves at least n elements in external memory.
     *
     * If n is less than or equal to capacity(), this call has no
     * effect. Otherwise, it is a request for allocation of additional \b
     * external memory. If the request is successful, then capacity() is
     * greater than or equal to n; otherwise capacity() is unchanged. In either
     * case, size() is unchanged.
     */
    void reserve(size_type n)
    {
        if (n <= capacity())
            return;

        const size_t old_bids_size = m_bids.size();
        const size_t new_bids_size = static_cast<size_t>(
            foxxll::div_ceil(n, block_type::size));
        const size_t new_pages = foxxll::div_ceil(new_bids_size, page_size);
        m_page_status.resize(new_pages, uninitialized);
        m_page_to_slot.resize(new_pages, on_disk);

        m_bids.resize(new_bids_size);
        if (!m_from)
        {
            m_bm->new_blocks(m_alloc_strategy,
                             m_bids.begin() + old_bids_size, m_bids.end(),
                             old_bids_size);
        }
        else
        {
            size_type offset = size_type(old_bids_size) * size_type(block_type::raw_size);
            for (bids_container_iterator it = m_bids.begin() + old_bids_size;
                 it != m_bids.end(); ++it, offset += size_type(block_type::raw_size))
            {
                (*it).storage = m_from.get();
                (*it).offset = offset;
            }
            LOG << "reserve(): Changing size of file " << m_from << " to " << offset;
            m_from->set_size(offset);
        }
    }

    //! Resize vector contents to n items.
    //! \warning this will not call the constructor of objects in external memory!
    void resize(size_type n)
    {
        _resize(n);
    }

    //! Resize vector contents to n items, and allow the allocated external
    //! memory to shrink. Internal memory allocation remains unchanged.
    //! \warning this will not call the constructor of objects in external memory!
    void resize(size_type n, bool shrink_capacity)
    {
        if (shrink_capacity)
            _resize_shrink_capacity(n);
        else
            _resize(n);
    }

    //! \}

private:
    //! Resize vector, only allow capacity growth.
    void _resize(size_type n)
    {
        reserve(n);
        if (n < m_size) {
            // mark excess pages as uninitialized and evict them from cache
            const size_t first_page_to_evict = static_cast<size_t>(
                foxxll::div_ceil(n, block_type::size * page_size));
            for (size_t i = first_page_to_evict; i < m_page_status.size(); ++i) {
                if (m_page_to_slot[i] != on_disk) {
                    m_free_slots.push(m_page_to_slot[i]);
                    m_page_to_slot[i] = on_disk;
                }
                m_page_status[i] = uninitialized;
            }
        }
        m_size = n;
    }

    //! Resize vector, also allow reduction of external memory capacity.
    void _resize_shrink_capacity(size_type n)
    {
        const size_t old_bids_size = m_bids.size();
        const size_t new_bids_size = static_cast<size_t>(foxxll::div_ceil(n, block_type::size));

        if (new_bids_size > old_bids_size)
        {
            reserve(n);
        }
        else if (new_bids_size < old_bids_size)
        {
            const size_t new_pages_size = foxxll::div_ceil(new_bids_size, page_size);

            LOG << "shrinking from " << old_bids_size << " to " <<
                new_bids_size << " blocks = from " <<
                m_page_status.size() << " to " <<
                new_pages_size << " pages";

            // release blocks
            if (m_from)
                m_from->set_size(new_bids_size * block_type::raw_size);
            else
                m_bm->delete_blocks(m_bids.begin() + old_bids_size, m_bids.end());

            m_bids.resize(new_bids_size);

            // don't resize m_page_to_slot or m_page_status, because it is
            // still needed to check page status and match the mapping
            // m_slot_to_page

            // clear dirty flag, so these pages will be never written
            std::fill(m_page_status.begin() + new_pages_size,
                      m_page_status.end(), valid_on_disk);
        }

        m_size = n;
    }

public:
    //! \name Modifiers
    //! \{

    //! Erases all of the elements and deallocates all external memory that is
    //! occupied.
    void clear()
    {
        m_size = 0;
        if (!m_from)
            m_bm->delete_blocks(m_bids.begin(), m_bids.end());

        m_bids.clear();
        m_page_status.clear();
        m_page_to_slot.clear();
        while (!m_free_slots.empty())
            m_free_slots.pop();

        for (size_t i = 0; i < numpages(); ++i)
            m_free_slots.push(i);
    }

    //! \}

    //! \name Front and Back Access
    //! \{

    //! Append a new element at the end.
    void push_back(const_reference obj)
    {
        size_type old_size = m_size;
        resize(old_size + 1);
        element(old_size) = obj;
    }
    //! Removes the last element (without returning it, see back()).
    void pop_back()
    {
        resize(m_size - 1);
    }

    //! \}

    //! \name Operators
    //! \{

    //! Returns a reference to the last element, see \ref design_vector_notes.
    reference back()
    {
        return element(m_size - 1);
    }
    //! Returns a reference to the first element, see \ref design_vector_notes.
    reference front()
    {
        return element(0);
    }
    //! Returns a constant reference to the last element, see \ref design_vector_notes.
    const_reference back() const
    {
        return const_element(m_size - 1);
    }
    //! Returns a constant reference to the first element, see \ref design_vector_notes.
    const_reference front() const
    {
        return const_element(0);
    }

    //! \}

    //! \name Constructors/Destructors
    //! \{

    //! Construct vector from a file.
    //! \param from file to be constructed from
    //! \param size Number of elements.
    //! \param npages Number of cached pages.
    //! \warning Only one \c vector can be assigned to a particular (physical) file.
    //! The block size of the vector must be a multiple of the element size
    //! \c sizeof(ValueType) and the page size (4096).
    explicit vector(foxxll::file_ptr from, const size_type size = size_type(-1),
                    const size_t npages = pager_type().size())
        : m_size((size == size_type(-1)) ? size_from_file_length(from->size()) : size),
          m_bids((size_t)foxxll::div_ceil(m_size, size_type(block_type::size))),
          m_pager(npages),
          m_page_status(foxxll::div_ceil(m_bids.size(), page_size), valid_on_disk),
          m_page_to_slot(foxxll::div_ceil(m_bids.size(), page_size), on_disk),
          m_slot_to_page(npages),
          m_cache(nullptr),
          m_from(from),
          m_exported(false)
    {
        // initialize from file
        if (!block_type::has_only_data)
        {
            std::ostringstream str;
            str << "The block size for a vector that is mapped to a file must be a multiple of the element size (" <<
                sizeof(value_type) << ") and the page size (4096).";
            throw std::runtime_error(str.str());
        }

        m_bm = foxxll::block_manager::get_instance();

        allocate_page_cache();

        for (size_t i = 0; i < numpages(); ++i)
            m_free_slots.push(i);

        // allocate blocks equidistantly and in-order
        size_type offset = 0;
        for (bids_container_iterator it = m_bids.begin();
             it != m_bids.end(); ++it, offset += size_type(block_type::raw_size))
        {
            (*it).storage = from.get();
            (*it).offset = offset;
        }
        from->set_size(offset);
    }

    //! copy-constructor
    vector(const vector& obj)
        : m_size(obj.size()),
          m_bids((size_t)foxxll::div_ceil(obj.size(), block_type::size)),
          m_pager(obj.numpages()),
          m_page_status(foxxll::div_ceil(m_bids.size(), page_size), uninitialized),
          m_page_to_slot(foxxll::div_ceil(m_bids.size(), page_size), on_disk),
          m_slot_to_page(obj.numpages()),
          m_cache(nullptr),
          m_exported(false)
    {
        assert(!obj.m_exported);
        m_bm = foxxll::block_manager::get_instance();

        allocate_page_cache();

        for (size_t i = 0; i < numpages(); ++i)
            m_free_slots.push(i);

        m_bm->new_blocks(m_alloc_strategy, m_bids.begin(), m_bids.end(), 0);

        const_iterator inbegin = obj.begin();
        const_iterator inend = obj.end();
        std::copy(inbegin, inend, begin());
    }

    //! \}

    //! \name Operators
    //! \{

    //! assignment operator
    vector& operator = (const vector& obj)
    {
        if (&obj != this)
        {
            vector tmp(obj);
            this->swap(tmp);
        }
        return *this;
    }

    //! \}

    //! \name Iterator Construction
    //! \{

    //! returns an iterator pointing to the beginning of the vector, see \ref design_vector_notes.
    iterator begin()
    {
        return iterator(this, 0);
    }
    //! returns a const_iterator pointing to the beginning of the vector, see \ref design_vector_notes.
    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }
    //! returns a const_iterator pointing to the beginning of the vector, see \ref design_vector_notes.
    const_iterator cbegin() const
    {
        return begin();
    }
    //! returns an iterator pointing beyond the end of the vector, see \ref design_vector_notes.
    iterator end()
    {
        return iterator(this, m_size);
    }
    //! returns a const_iterator pointing beyond the end of the vector, see \ref design_vector_notes.
    const_iterator end() const
    {
        return const_iterator(this, m_size);
    }
    //! returns a const_iterator pointing beyond the end of the vector, see \ref design_vector_notes.
    const_iterator cend() const
    {
        return end();
    }

    //! returns a reverse_iterator pointing to the end of the vector.
    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    //! returns a reverse_iterator pointing to the end of the vector.
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }
    //! returns a reverse_iterator pointing to the end of the vector.
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(end());
    }
    //! returns a reverse_iterator pointing beyond the beginning of the vector.
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    //! returns a reverse_iterator pointing beyond the beginning of the vector.
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }
    //! returns a reverse_iterator pointing beyond the beginning of the vector.
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(begin());
    }

    //! \}

    //! \name Direct Element Access
    //! \{

    //! access the element at the given vector's offset
    reference operator [] (size_type offset)
    {
        return element(offset);
    }
    //! access the element at the given vector's offset
    const_reference operator [] (size_type offset) const
    {
        return const_element(offset);
    }

    //! access the element at the given vector's offset
    reference at(size_type offset)
    {
        assert(offset < (size_type)size());
        return element(offset);
    }
    //! access the element at the given vector's offset
    const_reference at(size_type offset) const
    {
        assert(offset < (size_type)size());
        return const_element(offset);
    }

    //! return true if the given vector offset is in cache
    bool is_element_cached(size_type offset) const
    {
        return is_page_cached(blocked_index_type(offset));
    }

    //! \}

    //! \name Modifiers
    //! \{

    //! Flushes the cache pages to the external memory.
    void flush() const
    {
        tlx::simple_vector<bool> non_free_slots(numpages());

        for (size_t i = 0; i < numpages(); i++)
            non_free_slots[i] = true;

        while (!m_free_slots.empty())
        {
            non_free_slots[m_free_slots.front()] = false;
            m_free_slots.pop();
        }

        for (size_t i = 0; i < numpages(); i++)
        {
            m_free_slots.push(i);
            const size_t& page_no = m_slot_to_page[i];
            if (non_free_slots[i])
            {
                LOG << "flush(: flushing page " << i << " at address " <<
                (static_cast<uint64_t>(page_no)
                 * static_cast<uint64_t>(block_type::size)
                 * static_cast<uint64_t>(page_size));
                write_page(page_no, i);

                m_page_to_slot[page_no] = on_disk;
            }
        }
    }

    //! \}

    //! \name Constructors/Destructors
    //! \{

    ~vector()
    {
        LOG << "~vector()";
        try
        {
            flush();
        }
        catch (foxxll::io_error e)
        {
            LOG1 << "io_error thrown in ~vector(): " << e.what();
        }
        catch (...)
        {
            LOG1 << "Exception thrown in ~vector()";
        }

        if (!m_exported)
        {
            if (!m_from) {
                m_bm->delete_blocks(m_bids.begin(), m_bids.end());
            }
            else // file must be truncated
            {
                LOG << "~vector(: Changing size of file " <<
                    m_from << " to " << file_length();
                LOG << "~vector(): size of the vector is " << size();
                try
                {
                    m_from->set_size(file_length());
                }
                catch (...)
                {
                    LOG1 << "Exception thrown in ~vector()...set_size()";
                }
            }
        }
        delete m_cache;
    }

    //! \}

    //! \name Miscellaneous
    //! \{

    //! Export data such that it is persistent on the file system. Resulting
    //! files will be numbered ascending.
    void export_files(std::string filename_prefix)
    {
        size_t no = 0;
        for (bids_container_iterator i = m_bids.begin(); i != m_bids.end(); ++i) {
            std::ostringstream number;
            number << std::setw(9) << std::setfill('0') << no;
            size_type current_block_size =
                ((i + 1) == m_bids.end() && m_size % block_type::size > 0) ?
                (m_size % block_type::size) * sizeof(value_type) :
                block_type::size * sizeof(value_type);
            (*i).storage->export_files((*i).offset, current_block_size, filename_prefix + number.str());
            ++no;
        }
        m_exported = true;
    }

    //! Get the file associated with this vector, or nullptr.
    const foxxll::file_ptr & get_file() const
    {
        return m_from;
    }

    //! \}

    //! \name Capacity
    //! \{

    //! Set the blocks and the size of this container explicitly.
    //! The vector must be completely empty before.
    template <typename ForwardIterator>
    void set_content(const ForwardIterator& bid_begin, const ForwardIterator& bid_end, size_type n)
    {
        const size_t new_bids_size = foxxll::div_ceil(n, block_type::size);
        m_bids.resize(new_bids_size);
        std::copy(bid_begin, bid_end, m_bids.begin());
        const size_t new_pages = foxxll::div_ceil(new_bids_size, page_size);
        m_page_status.resize(new_pages, valid_on_disk);
        m_page_to_slot.resize(new_pages, on_disk);
        m_size = n;
    }

    //! Number of pages used by the pager.
    inline size_t numpages() const
    {
        return m_pager.size();
    }

    //! \}

private:
    bids_container_iterator bid(const size_type& offset)
    {
        return (m_bids.begin() +
                static_cast<typename bids_container_type::size_type>
                (offset / block_type::size));
    }
    bids_container_iterator bid(const blocked_index_type& offset)
    {
        return (m_bids.begin() +
                static_cast<typename bids_container_type::size_type>
                (offset.get_block2() * PageSize + offset.get_block1()));
    }
    const_bids_container_iterator bid(const size_type& offset) const
    {
        return (m_bids.begin() +
                static_cast<typename bids_container_type::size_type>
                (offset / block_type::size));
    }
    const_bids_container_iterator bid(const blocked_index_type& offset) const
    {
        return (m_bids.begin() +
                static_cast<typename bids_container_type::size_type>
                (offset.get_block2() * PageSize + offset.get_block1()));
    }

    void read_page(const size_t& page_no, const size_t& cache_slot) const
    {
        assert(page_no < m_page_status.size());

        if (m_page_status[page_no] == uninitialized)
            return;

        LOG << "read_page(): page_no=" << page_no << " cache_slot=" << cache_slot;
        std::vector<foxxll::request_ptr> reqs;
        reqs.reserve(page_size);

        size_t block_no = page_no * page_size;
        const size_t last_block = std::min<size_t>(block_no + page_size, m_bids.size());
        for (size_t i = cache_slot * page_size; block_no < last_block; ++block_no, ++i) {
            reqs.push_back((*m_cache)[i].read(m_bids[block_no]));
        }

        assert(last_block - page_no * page_size > 0);
        wait_all(reqs.data(), last_block - page_no * page_size);
    }

    void write_page(const size_t& page_no, const size_t& cache_slot) const
    {
        assert(page_no < m_page_status.size());

        if (!(m_page_status[page_no] & dirty))
            return;

        LOG << "write_page(): page_no=" << page_no << " cache_slot=" << cache_slot;

        std::vector<foxxll::request_ptr> reqs;
        reqs.reserve(page_size);

        size_t block_no = page_no * page_size;

        const size_t last_block = std::min<size_t>(block_no + page_size, m_bids.size());
        assert(block_no < last_block);
        for (size_t i = cache_slot * page_size; block_no < last_block; ++block_no, ++i) {
            reqs.push_back((*m_cache)[i].write(m_bids[block_no]));
        }

        m_page_status[page_no] = valid_on_disk;
        assert(last_block - page_no * page_size > 0);

        wait_all(reqs.data(), last_block - page_no * page_size);
    }

    reference element(size_type offset)
    {
        return element(blocked_index_type(offset));
    }

    reference element(const blocked_index_type& offset)
    {
        assert(offset.get_pos() < size());
        const size_t page_no = offset.get_block2();
        assert(page_no < m_page_to_slot.size());   // fails if offset is too large, out of bound access
        const auto cache_slot = m_page_to_slot[page_no];
        if (cache_slot < 0)                        // == on_disk
        {
            if (m_free_slots.empty())              // has to kick
            {
                const size_t kicked_slot = m_pager.kick();
                m_pager.hit(kicked_slot);
                const size_t old_page_no = m_slot_to_page[kicked_slot];
                m_page_to_slot[page_no] = kicked_slot;
                m_page_to_slot[old_page_no] = on_disk;
                m_slot_to_page[kicked_slot] = page_no;

                write_page(old_page_no, kicked_slot);
                read_page(page_no, kicked_slot);

                m_page_status[page_no] = dirty;

                return (*m_cache)[kicked_slot * page_size + offset.get_block1()][offset.get_offset()];
            }
            else
            {
                const size_t free_slot = m_free_slots.front();
                m_free_slots.pop();
                m_pager.hit(free_slot);
                m_page_to_slot[page_no] = free_slot;
                m_slot_to_page[free_slot] = page_no;

                read_page(page_no, free_slot);

                m_page_status[page_no] = dirty;

                return (*m_cache)[free_slot * page_size + offset.get_block1()][offset.get_offset()];
            }
        }
        else
        {
            m_page_status[page_no] = dirty;
            m_pager.hit(cache_slot);
            return (*m_cache)[cache_slot * page_size + offset.get_block1()][offset.get_offset()];
        }
    }

    // don't forget to first flush() the vector's cache before updating pages externally
    void page_externally_updated(const size_t page_no) const
    {
        // fails if offset is too large, out of bound access
        assert(page_no < m_page_status.size());
        // "A dirty page has been marked as newly initialized. The page content will be lost."
        assert(!(m_page_status[page_no] & dirty));
        if (m_page_to_slot[page_no] >= 0) { // != on_disk
            // remove page from cache
            m_free_slots.push(m_page_to_slot[page_no]);
            m_page_to_slot[page_no] = on_disk;
            LOG << "page_externally_updated(): page_no=" << page_no << " flushed from cache.";
        }
        else {
            LOG << "page_externally_updated(): page_no=" << page_no << " no need to flush.";
        }
        m_page_status[page_no] = valid_on_disk;
    }

    void block_externally_updated(const size_type offset) const
    {
        page_externally_updated(
            static_cast<size_t>(offset / (block_type::size * page_size)));
    }

    void block_externally_updated(const blocked_index_type& offset) const
    {
        page_externally_updated(offset.get_block2());
    }

    const_reference const_element(size_type offset) const
    {
        return const_element(blocked_index_type(offset));
    }

    const_reference const_element(const blocked_index_type& offset) const
    {
        const size_t& page_no = offset.get_block2();
        assert(page_no < m_page_to_slot.size());   // fails if offset is too large, out of bound access
        const auto cache_slot = m_page_to_slot[page_no];
        if (cache_slot < 0)                        // == on_disk
        {
            if (m_free_slots.empty())              // has to kick
            {
                const size_t kicked_slot = m_pager.kick();
                m_pager.hit(kicked_slot);
                const size_t old_page_no = m_slot_to_page[kicked_slot];
                m_page_to_slot[page_no] = kicked_slot;
                m_page_to_slot[old_page_no] = on_disk;
                m_slot_to_page[kicked_slot] = page_no;

                write_page(old_page_no, kicked_slot);
                read_page(page_no, kicked_slot);

                return (*m_cache)[kicked_slot * page_size + offset.get_block1()][offset.get_offset()];
            }
            else
            {
                const size_t free_slot = m_free_slots.front();
                m_free_slots.pop();
                m_pager.hit(free_slot);
                m_page_to_slot[page_no] = free_slot;
                m_slot_to_page[free_slot] = page_no;

                read_page(page_no, free_slot);

                return (*m_cache)[free_slot * page_size + offset.get_block1()][offset.get_offset()];
            }
        }
        else
        {
            m_pager.hit(cache_slot);
            return (*m_cache)[cache_slot * page_size + offset.get_block1()][offset.get_offset()];
        }
    }

    bool is_page_cached(const blocked_index_type& offset) const
    {
        const size_t& page_no = offset.get_block2();
        assert(page_no < m_page_to_slot.size());   // fails if offset is too large, out of bound access
        return m_page_to_slot[page_no] >= 0;       // != on_disk;
    }

public:
    //! \name Comparison Operators
    //! \{

    friend bool operator == (vector& a, vector& b)
    {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
    }

    friend bool operator != (vector& a, vector& b)
    {
        return !(a == b);
    }

    friend bool operator < (vector& a, vector& b)
    {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    friend bool operator > (vector& a, vector& b)
    {
        return b < a;
    }

    friend bool operator <= (vector& a, vector& b)
    {
        return !(b < a);
    }

    friend bool operator >= (vector& a, vector& b)
    {
        return !(a < b);
    }

    //! \}
};

////////////////////////////////////////////////////////////////////////////

// specialization for stxxl::vector, to use only const_iterators
template <typename VectorConfig>
bool is_sorted(
    stxxl::vector_iterator<VectorConfig> first,
    stxxl::vector_iterator<VectorConfig> last)
{
    return stxxl::is_sorted(
        stxxl::const_vector_iterator<VectorConfig>(first),
        stxxl::const_vector_iterator<VectorConfig>(last));
}

template <typename VectorConfig, typename StrictWeakOrdering>
bool is_sorted(
    stxxl::vector_iterator<VectorConfig> first,
    stxxl::vector_iterator<VectorConfig> last,
    StrictWeakOrdering comp)
{
    return stxxl::is_sorted(
        stxxl::const_vector_iterator<VectorConfig>(first),
        stxxl::const_vector_iterator<VectorConfig>(last),
        comp);
}

////////////////////////////////////////////////////////////////////////////

template <typename VectorBufReaderType>
class vector_bufreader_iterator;

/*!
 * Buffered sequential reader from a vector using overlapped I/O.
 *
 * This buffered reader can be used to read a large sequential region of a
 * vector using overlapped I/O. The object is created from an iterator range,
 * which can then be read to using operator<<(), or with operator*() and
 * operator++().
 *
 * The interface also fulfills all requirements of a stream. Actually most of
 * the code is identical to stream::vector_iterator2stream.
 *
 * Note that this buffered reader is inefficient for reading small ranges. This
 * is intentional, as one can just use operator[] on the vector for that.
 *
 * See \ref tutorial_vector_buf
 */
template <typename VectorIterator>
class vector_bufreader
{
public:
    //! template parameter: the vector iterator type
    using vector_iterator = VectorIterator;

    //! value type of the output vector
    using value_type = typename vector_iterator::value_type;

    //! block type used in the vector
    using block_type = typename vector_iterator::block_type;

    //! type of the input vector
    using vector_type = typename vector_iterator::vector_type;

    //! block identifier iterator of the vector
    using bids_container_iterator = typename vector_iterator::bids_container_iterator;

    //! construct output buffered stream used for overlapped reading
    using buf_istream_type = foxxll::buf_istream<block_type, bids_container_iterator>;

    //! construct an iterator for vector_bufreader (for C++11 range-based for loop)
    using bufreader_iterator = vector_bufreader_iterator<vector_bufreader>;

    //! size of remaining data
    using size_type = typename vector_type::size_type;

protected:
    //! iterator to the beginning of the range.
    vector_iterator m_begin;

    //! internal "current" iterator into the vector.
    vector_iterator m_iter;

    //! iterator to the end of the range.
    vector_iterator m_end;

    //! buffered input stream used to overlapped I/O.
    buf_istream_type* m_bufin;

    //! number of blocks to use as buffers.
    size_t m_nbuffers;

    //! allow vector_bufreader_iterator to check m_iter against its current value
    friend class vector_bufreader_iterator<vector_bufreader>;

public:
    //! Create overlapped reader for the given iterator range.
    //! \param begin iterator to position were to start reading in vector
    //! \param end iterator to position were to end reading in vector
    //! \param nbuffers number of buffers used for overlapped I/O (>= 2*D recommended)
    vector_bufreader(vector_iterator begin, vector_iterator end,
                     const size_t nbuffers = 0)
        : m_begin(begin), m_end(end),
          m_bufin(nullptr),
          m_nbuffers(nbuffers)
    {
        m_begin.flush(); // flush container

        if (m_nbuffers == 0)
            m_nbuffers = 2 * foxxll::config::get_instance()->disks_number();

        rewind();
    }

    //! Create overlapped reader for the whole vector's content.
    //! \param vec vector to read
    //! \param nbuffers number of buffers used for overlapped I/O (>= 2*D recommended)
    explicit vector_bufreader(const vector_type& vec, const size_t nbuffers = 0)
        : m_begin(vec.begin()), m_end(vec.end()),
          m_bufin(nullptr),
          m_nbuffers(nbuffers)
    {
        m_begin.flush(); // flush container

        if (m_nbuffers == 0)
            m_nbuffers = 2 * foxxll::config::get_instance()->disks_number();

        rewind();
    }

    //! Rewind stream back to begin. Note that this recreates the buffered
    //! reader and is thus not cheap.
    void rewind()
    {
        m_iter = m_begin;
        if (empty()) return;

        if (m_bufin) delete m_bufin;

        // find last bid to read
        bids_container_iterator end_bid = m_end.bid() + (m_end.block_offset() ? 1 : 0);

        // construct buffered istream for range
        m_bufin = new buf_istream_type(m_begin.bid(), end_bid, m_nbuffers);

        // skip the beginning of the block, up to real beginning
        vector_iterator curr = m_begin - m_begin.block_offset();

        for ( ; curr != m_begin; ++curr)
            ++(*m_bufin);
    }

    //! non-copyable: delete copy-constructor
    vector_bufreader(const vector_bufreader&) = delete;
    //! non-copyable: delete assignment operator
    vector_bufreader& operator = (const vector_bufreader&) = delete;

    //! Finish reading and free buffered reader.
    ~vector_bufreader()
    {
        if (m_bufin) delete m_bufin;
    }

    //! Return constant reference to current item
    const value_type& operator * () const
    {
        return *(*m_bufin);
    }

    //! Return constant pointer to current item
    const value_type* operator -> () const
    {
        return &(*(*m_bufin));
    }

    //! Advance to next item (asserts if !empty()).
    vector_bufreader& operator ++ ()
    {
        assert(!empty());
        ++m_iter;
        ++(*m_bufin);

        if (TLX_UNLIKELY(empty())) {
            delete m_bufin;
            m_bufin = nullptr;
        }

        return *this;
    }

    //! Read current item into variable and advance to next one.
    vector_bufreader& operator >> (value_type& v)
    {
        v = operator * ();
        operator ++ ();

        return *this;
    }

    //! Return remaining size.
    size_type size() const
    {
        assert(m_begin <= m_iter && m_iter <= m_end);
        return (size_type)(m_end - m_iter);
    }

    //! Returns true once the whole range has been read.
    bool empty() const
    {
        return (m_iter == m_end);
    }

    //! Return vector_bufreader_iterator for C++11 range-based for loop
    bufreader_iterator begin()
    {
        return bufreader_iterator(*this, m_begin);
    }

    //! Return vector_bufreader_iterator for C++11 range-based for loop
    bufreader_iterator end()
    {
        return bufreader_iterator(*this, m_end);
    }
};

////////////////////////////////////////////////////////////////////////////

/*!
 * Adapter for vector_bufreader to match iterator requirements of C++11
 * range-based loop construct.
 *
 * Since vector_bufreader itself points to only one specific item, this
 * iterator is merely a counter facade. The functions operator*() and
 * operator++() must only be called when it is in _sync_ with the bufreader
 * object. This is generally only the case for an iterator constructed with
 * begin() and then advanced with operator++(). The class checks this using
 * asserts(), the operators will fail if used wrong.
 *
 * See \ref tutorial_vector_buf
 */
template <typename VectorBufReaderType>
class vector_bufreader_iterator
{
public:
    //! The underlying buffered reader type
    using vector_bufreader_type = VectorBufReaderType;

    //! Value type of vector
    using value_type = typename vector_bufreader_type::value_type;

    //! Use vector_iterator to reference a point in the vector.
    using vector_iterator = typename vector_bufreader_type::vector_iterator;

protected:
    //! Buffered reader used to access elements in vector
    vector_bufreader_type& m_bufreader;

    //! Use vector_iterator to reference a point in the vector.
    vector_iterator m_iter;

public:
    //! Construct iterator using vector_iterator
    vector_bufreader_iterator(vector_bufreader_type& bufreader, const vector_iterator& iter)
        : m_bufreader(bufreader), m_iter(iter)
    { }

    //! Return constant reference to current item
    const value_type& operator * () const
    {
        assert(m_bufreader.m_iter == m_iter);
        return m_bufreader.operator * ();
    }

    //! Return constant pointer to current item
    const value_type* operator -> () const
    {
        assert(m_bufreader.m_iter == m_iter);
        return m_bufreader.operator -> ();
    }

    //! Make bufreader advance to next item (asserts if !empty() or if iterator
    //! does not point to current).
    vector_bufreader_iterator& operator ++ ()
    {
        assert(m_bufreader.m_iter == m_iter);
        m_bufreader.operator ++ ();
        m_iter++;
        return *this;
    }

    //! Equality comparison operator
    bool operator == (const vector_bufreader_iterator& vbi) const
    {
        assert(&m_bufreader == &vbi.m_bufreader);
        return (m_iter == vbi.m_iter);
    }

    //! Inequality comparison operator
    bool operator != (const vector_bufreader_iterator& vbi) const
    {
        assert(&m_bufreader == &vbi.m_bufreader);
        return (m_iter != vbi.m_iter);
    }
};

////////////////////////////////////////////////////////////////////////////

/*!
 * Buffered sequential reverse reader from a vector using overlapped I/O.
 *
 * This buffered reader can be used to read a large sequential region of a
 * vector _in_reverse_ using overlapped I/O. The object is created from an
 * iterator range, which can then be read to using operator<<(), or with
 * operator*() and operator++(), where ++ actually goes to the preceding
 * element.
 *
 * The interface also fulfills all requirements of a stream. Actually most of
 * the code is identical to stream::vector_iterator2stream.
 *
 * Note that this buffered reader is inefficient for reading small ranges. This
 * is intentional, as one can just use operator[] on the vector for that.
 *
 * See \ref tutorial_vector_buf
 */
template <typename VectorIterator>
class vector_bufreader_reverse
{
public:
    //! template parameter: the vector iterator type
    using vector_iterator = VectorIterator;

    //! value type of the output vector
    using value_type = typename vector_iterator::value_type;

    //! block type used in the vector
    using block_type = typename vector_iterator::block_type;

    //! type of the input vector
    using vector_type = typename vector_iterator::vector_type;

    //! block identifier iterator of the vector
    using bids_container_iterator = typename vector_iterator::bids_container_iterator;

    //! construct output buffered stream used for overlapped reading
    using buf_istream_type = foxxll::buf_istream_reverse<block_type, bids_container_iterator>;

    //! size of remaining data
    using size_type = typename vector_type::size_type;

protected:
    //! iterator to the beginning of the range.
    vector_iterator m_begin;

    //! internal "current" iterator into the vector.
    vector_iterator m_iter;

    //! iterator to the end of the range.
    vector_iterator m_end;

    //! buffered input stream used to overlapped I/O.
    buf_istream_type* m_bufin;

    //! number of blocks to use as buffers.
    size_t m_nbuffers;

public:
    //! Create overlapped reader for the given iterator range.
    //! \param begin iterator to position were to start reading in vector
    //! \param end iterator to position were to end reading in vector
    //! \param nbuffers number of buffers used for overlapped I/O (>= 2*D recommended)
    vector_bufreader_reverse(vector_iterator begin, vector_iterator end,
                             size_t nbuffers = 0)
        : m_begin(begin), m_end(end),
          m_bufin(nullptr),
          m_nbuffers(nbuffers)
    {
        m_begin.flush(); // flush container

        if (m_nbuffers == 0)
            m_nbuffers = 2 * foxxll::config::get_instance()->disks_number();

        rewind();
    }

    //! Create overlapped reader for the whole vector's content.
    //! \param vec vector to read
    //! \param nbuffers number of buffers used for overlapped I/O (>= 2*D recommended)
    explicit vector_bufreader_reverse(const vector_type& vec, size_t nbuffers = 0)
        : m_begin(vec.begin()), m_end(vec.end()),
          m_bufin(nullptr),
          m_nbuffers(nbuffers)
    {
        m_begin.flush(); // flush container

        if (m_nbuffers == 0)
            m_nbuffers = 2 * foxxll::config::get_instance()->disks_number();

        rewind();
    }

    //! non-copyable: delete copy-constructor
    vector_bufreader_reverse(const vector_bufreader_reverse&) = delete;
    //! non-copyable: delete assignment operator
    vector_bufreader_reverse& operator = (const vector_bufreader_reverse&) = delete;

    //! Rewind stream back to begin. Note that this recreates the buffered
    //! reader and is thus not cheap.
    void rewind()
    {
        m_iter = m_end;
        if (empty())
            return;

        if (m_bufin)
            delete m_bufin;

        // find last bid to read
        bids_container_iterator end_bid = m_end.bid() + (m_end.block_offset() ? 1 : 0);

        // construct buffered istream_reverse for range
        m_bufin = new buf_istream_type(m_begin.bid(), end_bid, m_nbuffers);

        // skip to beginning of reverse sequence.
        size_t endoff = m_end.block_offset();
        if (endoff != 0) {
            // let ifstream_reverse skip last elements at end of block, up to real end
            for ( ; endoff != block_type::size; endoff++)
                ++(*m_bufin);
        }
    }

    //! Finish reading and free buffered reader.
    ~vector_bufreader_reverse()
    {
        if (m_bufin) delete m_bufin;
    }

    //! Return constant reference to current item
    const value_type& operator * () const
    {
        return *(*m_bufin);
    }

    //! Return constant pointer to current item
    const value_type* operator -> () const
    {
        return &(*(*m_bufin));
    }

    //! Advance to next item (asserts if !empty()).
    vector_bufreader_reverse& operator ++ ()
    {
        assert(!empty());
        --m_iter;
        ++(*m_bufin);

        if (TLX_UNLIKELY(empty())) {
            delete m_bufin;
            m_bufin = nullptr;
        }

        return *this;
    }

    //! Read current item into variable and advance to next one.
    vector_bufreader_reverse& operator >> (value_type& v)
    {
        v = operator * ();
        operator ++ ();

        return *this;
    }

    //! Return remaining size.
    size_type size() const
    {
        assert(m_begin <= m_iter && m_iter <= m_end);
        return (size_type)(m_iter - m_begin);
    }

    //! Returns true once the whole range has been read.
    bool empty() const
    {
        return (m_iter == m_begin);
    }
};

////////////////////////////////////////////////////////////////////////////

/*!
 * Buffered sequential writer to a vector using overlapped I/O.
 *
 * This buffered writer can be used to write a large sequential region of a
 * vector using overlapped I/O. The object is created from an iterator range,
 * which can then be written to using operator << (), or with operator * () and
 * operator ++ ().
 *
 * The buffered writer is given one iterator in the constructor. When writing,
 * this iterator advances in the vector and will \b enlarge the vector once it
 * reaches the end(). The vector size is doubled each time; nevertheless, it is
 * better to preinitialize the vector's size using stxxl::vector::resize().
 *
 * See \ref tutorial_vector_buf
 */
template <typename VectorIterator>
class vector_bufwriter
{
public:
    //! template parameter: the vector iterator type
    using iterator = VectorIterator;

    //! type of the output vector
    using vector_type = typename iterator::vector_type;

    //! value type of the output vector
    using value_type = typename iterator::value_type;

    //! block type used in the vector
    using block_type = typename iterator::block_type;

    //! block identifier iterator of the vector
    using bids_container_iterator = typename iterator::bids_container_iterator;

    //! iterator type of vector
    using vector_iterator = typename iterator::iterator;
    using vector_const_iterator = typename iterator::const_iterator;

    //! construct output buffered stream used for overlapped writing
    using buf_ostream_type = foxxll::buf_ostream<block_type, bids_container_iterator>;

protected:
    //! internal iterator into the vector.
    vector_iterator m_iter;

    //! iterator to the current end of the vector.
    vector_const_iterator m_end;

    //! boolean whether the vector was grown, will shorten at finish().
    bool m_grown;

    //! iterator into vector of the last block accessed (used to issue updates
    //! when the block is switched).
    vector_const_iterator m_prevblk;

    //! buffered output stream used to overlapped I/O.
    buf_ostream_type* m_bufout;

    //! number of blocks to use as buffers.
    size_t m_nbuffers;

public:
    //! Create overlapped writer beginning at the given iterator.
    //! \param begin iterator to position were to start writing in vector
    //! \param nbuffers number of buffers used for overlapped I/O (>= 2D recommended)
    explicit vector_bufwriter(vector_iterator begin,
                              size_t nbuffers = 0)
        : m_iter(begin),
          m_end(m_iter.parent_vector()->end()),
          m_grown(false),
          m_bufout(nullptr),
          m_nbuffers(nbuffers)
    {
        if (m_nbuffers == 0)
            m_nbuffers = 2 * foxxll::config::get_instance()->disks_number();

        assert(m_iter <= m_end);
    }

    //! Create overlapped writer for the vector's beginning
    //! \param vec vector to write
    //! \param nbuffers number of buffers used for overlapped I/O (>= 2D recommended)
    explicit vector_bufwriter(vector_type& vec,
                              size_t nbuffers = 0)
        : m_iter(vec.begin()),
          m_end(m_iter.parent_vector()->end()),
          m_grown(false),
          m_bufout(nullptr),
          m_nbuffers(nbuffers)
    {
        if (m_nbuffers == 0)
            m_nbuffers = 2 * foxxll::config::get_instance()->disks_number();

        assert(m_iter <= m_end);
    }

    //! non-copyable: delete copy-constructor
    vector_bufwriter(const vector_bufwriter&) = delete;
    //! non-copyable: delete assignment operator
    vector_bufwriter& operator = (const vector_bufwriter&) = delete;

    //! Finish writing and flush output back to vector.
    ~vector_bufwriter()
    {
        finish();
    }

    //! Return mutable reference to item at the position of the internal
    //! iterator.
    value_type& operator * ()
    {
        if (TLX_UNLIKELY(m_iter == m_end))
        {
            // iterator points to end of vector -> double vector's size

            if (m_bufout) {
                // fixes issue with buf_ostream writing invalid blocks: when
                // buf_ostream::current_elem advances to next block, flush()
                // will write to block beyond bid().end.
                if (m_iter.block_offset() != 0)
                    m_bufout->flush(); // flushes overlap buffers

                delete m_bufout;
                m_bufout = nullptr;

                if (m_iter.block_offset() != 0)
                    m_iter.block_externally_updated();
            }

            vector_type& v = *m_iter.parent_vector();
            if (v.size() < 2 * block_type::size) {
                v.resize(2 * block_type::size);
            }
            else {
                v.resize(2 * v.size());
            }
            m_end = v.end();
            m_grown = true;
        }

        assert(m_iter < m_end);

        if (TLX_UNLIKELY(m_bufout == nullptr))
        {
            if (m_iter.block_offset() != 0)
            {
                // output position is not at the start of the block, we
                // continue to use the iterator initially passed to the
                // constructor.
                return *m_iter;
            }
            else
            {
                // output position is start of block: create buffered writer

                m_iter.flush(); // flush container

                // create buffered write stream for blocks
                m_bufout = new buf_ostream_type(m_iter.bid(), m_nbuffers);
                m_prevblk = m_iter;

                // drop through to normal output into buffered writer
            }
        }

        // if the pointer has finished a block, then we inform the vector that
        // this block has been updated.
        if (TLX_UNLIKELY(m_iter.block_offset() == 0)) {
            if (m_prevblk != m_iter) {
                m_prevblk.block_externally_updated();
                m_prevblk = m_iter;
            }
        }

        return m_bufout->operator * ();
    }

    //! Advance internal iterator.
    vector_bufwriter& operator ++ ()
    {
        // always advance internal iterator
        ++m_iter;

        // if buf_ostream active, advance that too
        if (TLX_LIKELY(m_bufout != nullptr)) m_bufout->operator ++ ();

        return *this;
    }

    //! Write value to the current position and advance the internal iterator.
    vector_bufwriter& operator << (const value_type& v)
    {
        operator * () = v;
        operator ++ ();

        return *this;
    }

    //! Finish writing and flush output back to vector.
    void finish()
    {
        if (m_bufout)
        {
            // must finish the block started in the buffered writer: fill it with
            // the data in the vector
            vector_const_iterator const_out = m_iter;

            while (const_out.block_offset() != 0)
            {
                m_bufout->operator * () = *const_out;
                m_bufout->operator ++ ();
                ++const_out;
            }

            // inform the vector that the block has been updated.
            if (m_prevblk != m_iter) {
                m_prevblk.block_externally_updated();
                m_prevblk = m_iter;
            }

            delete m_bufout;
            m_bufout = nullptr;
        }

        if (m_grown)
        {
            vector_type& v = *m_iter.parent_vector();
            v.resize(m_iter - v.begin());

            m_grown = false;
        }
    }
};

//! \}

} // namespace stxxl

namespace std {

template <
    typename ValueType,
    unsigned PageSize,
    typename PagerType,
    size_t BlockSize,
    typename AllocStr>
void swap(stxxl::vector<ValueType, PageSize, PagerType, BlockSize, AllocStr>& a,
          stxxl::vector<ValueType, PageSize, PagerType, BlockSize, AllocStr>& b)
{
    a.swap(b);
}

} // namespace std

#endif // !STXXL_CONTAINERS_VECTOR_HEADER
// vim: et:ts=4:sw=4
