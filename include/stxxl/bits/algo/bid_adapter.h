/***************************************************************************
 *  include/stxxl/bits/algo/bid_adapter.h
 *
 *  include/stxxl/bits/mng/adaptor.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2009-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALGO_BID_ADAPTER_HEADER
#define STXXL_ALGO_BID_ADAPTER_HEADER

#include <foxxll/common/types.hpp>

#include <iterator>

namespace stxxl {

//! \addtogroup foxxll_mnglayer
//!
//! \{

template <size_t modulo>
class blocked_index
{
    size_t pos;
    size_t block;
    size_t offset;

    //! \invariant block * modulo + offset = pos

    void set(size_t pos)
    {
        this->pos = pos;
        block = pos / modulo;
        offset = pos % modulo;
    }

public:
    blocked_index()
    {
        set(0);
    }

    explicit blocked_index(size_t pos)
    {
        set(pos);
    }

    blocked_index(size_t block, size_t offset)
    {
        this->block = block;
        this->offset = offset;
        pos = block * modulo + offset;
    }

    void operator = (size_t pos)
    {
        set(pos);
    }

    //pre-increment operator
    blocked_index& operator ++ ()
    {
        ++pos;
        ++offset;
        if (offset == modulo)
        {
            offset = 0;
            ++block;
        }
        return *this;
    }

    //post-increment operator
    blocked_index operator ++ (int)
    {
        blocked_index former(*this);
        operator ++ ();
        return former;
    }

    //pre-increment operator
    blocked_index& operator -- ()
    {
        --pos;
        if (offset == 0)
        {
            offset = modulo;
            --block;
        }
        --offset;
        return *this;
    }

    //post-increment operator
    blocked_index operator -- (int)
    {
        blocked_index former(*this);
        operator -- ();
        return former;
    }

    blocked_index& operator += (size_t addend)
    {
        set(pos + addend);
        return *this;
    }

    blocked_index& operator >>= (size_t shift)
    {
        set(pos >> shift);
        return *this;
    }

    operator size_t () const
    {
        return pos;
    }

    const size_t & get_block() const
    {
        return block;
    }

    const size_t & get_offset() const
    {
        return offset;
    }
};

#define STXXL_ADAPTOR_ARITHMETICS(pos)             \
    bool operator == (const self_type &a) const    \
    {                                              \
        return (a.pos == pos);                     \
    }                                              \
    bool operator != (const self_type& a) const    \
    {                                              \
        return (a.pos != pos);                     \
    }                                              \
    bool operator < (const self_type& a) const     \
    {                                              \
        return (pos < a.pos);                      \
    }                                              \
    bool operator > (const self_type& a) const     \
    {                                              \
        return (pos > a.pos);                      \
    }                                              \
    bool operator <= (const self_type& a) const    \
    {                                              \
        return (pos <= a.pos);                     \
    }                                              \
    bool operator >= (const self_type& a) const    \
    {                                              \
        return (pos >= a.pos);                     \
    }                                              \
    self_type operator + (pos_type off) const      \
    {                                              \
        return self_type(array, pos + off);        \
    }                                              \
    self_type operator - (pos_type off) const      \
    {                                              \
        return self_type(array, pos - off);        \
    }                                              \
    self_type& operator ++ ()                      \
    {                                              \
        pos++;                                     \
        return *this;                              \
    }                                              \
    self_type operator ++ (int)                    \
    {                                              \
        self_type tmp = *this;                     \
        pos++;                                     \
        return tmp;                                \
    }                                              \
    self_type& operator -- ()                      \
    {                                              \
        pos--;                                     \
        return *this;                              \
    }                                              \
    self_type operator -- (int)                    \
    {                                              \
        self_type tmp = *this;                     \
        pos--;                                     \
        return tmp;                                \
    }                                              \
    pos_type operator - (const self_type& a) const \
    {                                              \
        return pos - a.pos;                        \
    }                                              \
    self_type& operator -= (pos_type off)          \
    {                                              \
        pos -= off;                                \
        return *this;                              \
    }                                              \
    self_type& operator += (pos_type off)          \
    {                                              \
        pos += off;                                \
        return *this;                              \
    }

template <class OneDimArrayType, class DataType, class PosType>
struct two2one_dim_array_adapter_base
{
    using one_dim_array_type = OneDimArrayType;
    using data_type = DataType;
    using pos_type = PosType;

    using self_type = two2one_dim_array_adapter_base<one_dim_array_type,
                                                     data_type, pos_type>;

    one_dim_array_type* array;
    pos_type pos;

    two2one_dim_array_adapter_base()
    { }

    two2one_dim_array_adapter_base(one_dim_array_type* a, pos_type p)
        : array(a), pos(p)
    { }
    two2one_dim_array_adapter_base(const two2one_dim_array_adapter_base& a)
        : array(a.array), pos(a.pos)
    { }

    STXXL_ADAPTOR_ARITHMETICS(pos)
};

//////////////////////////////

#define BLOCK_ADAPTOR_OPERATORS(two_to_one_dim_array_adaptor_base)                                   \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& operator ++ (             \
        two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a)                           \
    {                                                                                                \
        a.pos++;                                                                                     \
        return a;                                                                                    \
    }                                                                                                \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType> operator ++ (              \
        two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a, int)                      \
    {                                                                                                \
        two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType> tmp = a;                      \
        a.pos++;                                                                                     \
        return tmp;                                                                                  \
    }                                                                                                \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& operator -- (             \
        two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a)                           \
    {                                                                                                \
        a.pos--;                                                                                     \
        return a;                                                                                    \
    }                                                                                                \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType> operator -- (              \
        two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a, int)                      \
    {                                                                                                \
        two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType> tmp = a;                      \
        a.pos--;                                                                                     \
        return tmp;                                                                                  \
    }                                                                                                \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& operator -= (             \
        two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a,                           \
        typename two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>::_pos_type off)      \
    {                                                                                                \
        a.pos -= off;                                                                                \
        return a;                                                                                    \
    }                                                                                                \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& operator += (             \
        two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a,                           \
        typename two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>::_pos_type off)      \
    {                                                                                                \
        a.pos += off;                                                                                \
        return a;                                                                                    \
    }                                                                                                \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType> operator + (               \
        const two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a,                     \
        typename two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>::_pos_type off)      \
    {                                                                                                \
        return two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>(a.array, a.pos + off); \
    }                                                                                                \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType> operator + (               \
        typename two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>::_pos_type off,      \
        const two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a)                     \
    {                                                                                                \
        return two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>(a.array, a.pos + off); \
    }                                                                                                \
                                                                                                     \
    template <size_t BlockSize, typename RunType, class PosType>                                     \
    inline two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType> operator - (               \
        const two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>& a,                     \
        typename two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>::_pos_type off)      \
    {                                                                                                \
        return two_to_one_dim_array_adaptor_base<BlockSize, RunType, PosType>(a.array, a.pos - off); \
    }

#if 0
//////////////////////////
template <class OneDimArrayType, class DataType,
          unsigned DimSize, class PosType = blocked_index<DimSize> >
struct two2one_dim_array_row_adapter
    : public two2one_dim_array_adapter_base<OneDimArrayType, DataType, PosType>
{
    using one_dim_array_type = OneDimArrayType;
    using data_type = DataType;
    using dim_type = DimSize;
    using pos_type = PosType;
    using self_type = two2one_dim_array_row_adapter<one_dim_array_type,
                                                    data_type, dim_size, pos_type>;
    using base_type = two2one_dim_array_adapter_base<one_dim_array_type,
                                                     data_type, pos_type>;
    using base_type::array;
    using base_type::pos;

    two2one_dim_array_row_adapter()
    { }
    two2one_dim_array_row_adapter(one_dim_array_type* a, pos_type p)
        : two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>(a, p)
    { }
    two2one_dim_array_row_adapter(const two2one_dim_array_row_adapter& a)
        : two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>(a)
    { }

    data_type& operator * ()
    {
        return array[(pos).get_block()][(pos).get_offset()];
    }

    data_type* operator -> () const
    {
        return &(array[(pos).get_block()][(pos).get_offset()]);
    }

    data_type& operator [] (pos_type n)
    {
        n += pos;
        return array[(n) / dim_size][(n) % dim_size];
    }

    const data_type& operator [] (pos_type n) const
    {
        n += pos;
        return array[(n) / dim_size][(n) % dim_size];
    }
    STXXL_ADAPTOR_ARITHMETICS(pos)
};

template <class OneDimArrayType, class DataType,
          unsigned DimSize, class PosType = blocked_index<DimSize> >
struct two2one_dim_array_column_adapter
    : public two2one_dim_array_adapter_base<OneDimArrayType, DataType, PosType>
{
    using self_type = two2one_dim_array_column_adapter<one_dim_array_type,
                                                       data_type, dim_size, pos_type>;

    using two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>::pos;
    using two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>::array;

    two2one_dim_array_column_adapter(one_dim_array_type* a, pos_type p)
        : two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>(a, p)
    { }

    explicit two2one_dim_array_column_adapter(const self_type& a)
        : two2one_dim_array_adapter_base<one_dim_array_type, data_type, pos_type>(a)
    { }

    data_type& operator * ()
    {
        return array[(pos).get_offset()][(pos).get_block()];
    }

    data_type* operator -> () const
    {
        return &(array[(pos).get_offset()][(pos).get_block()]);
    }

    const data_type& operator [] (pos_type n) const
    {
        n += pos;
        return array[(n) % dim_size][(n) / dim_size];
    }

    data_type& operator [] (pos_type n)
    {
        n += pos;
        return array[(n) % dim_size][(n) / dim_size];
    }
    STXXL_ADAPTOR_ARITHMETICS(pos)
};
#endif

template <typename ArrayType, typename ValueType, size_t modulo>
class array_of_sequences_iterator
{
public:
    using array_type = ArrayType;
    using value_type = ValueType;

protected:
    size_t pos;
    size_t offset;
    array_type* arrays;
    array_type* base;
    value_type* base_element;

    //! \invariant block * modulo + offset = pos

    void set(size_t pos)
    {
        this->pos = pos;
        offset = pos % modulo;
        base = arrays + pos / modulo;
        base_element = base->elem;
    }

public:
    array_of_sequences_iterator()
    {
        this->arrays = nullptr;
        set(0);
    }

    explicit array_of_sequences_iterator(array_type* arrays)
    {
        this->arrays = arrays;
        set(0);
    }

    array_of_sequences_iterator(array_type* arrays, size_t pos)
    {
        this->arrays = arrays;
        set(pos);
    }

    void operator = (size_t pos)
    {
        set(pos);
    }

    //pre-increment operator
    array_of_sequences_iterator& operator ++ ()
    {
        ++pos;
        ++offset;
        if (offset == modulo)
        {
            offset = 0;
            ++base;
            base_element = base->elem;
        }
        return *this;
    }

    //post-increment operator
    array_of_sequences_iterator operator ++ (int)
    {
        array_of_sequences_iterator former(*this);
        operator ++ ();
        return former;
    }

    //pre-increment operator
    array_of_sequences_iterator& operator -- ()
    {
        --pos;
        if (offset == 0)
        {
            offset = modulo;
            --base;
            base_element = base->elem;
        }
        --offset;
        return *this;
    }

    //post-increment operator
    array_of_sequences_iterator operator -- (int)
    {
        array_of_sequences_iterator former(*this);
        operator -- ();
        return former;
    }

    array_of_sequences_iterator& operator += (size_t addend)
    {
        set(pos + addend);
        return *this;
    }

    array_of_sequences_iterator& operator -= (size_t addend)
    {
        set(pos - addend);
        return *this;
    }

    array_of_sequences_iterator operator + (size_t addend) const
    {
        return array_of_sequences_iterator(arrays, pos + addend);
    }

    array_of_sequences_iterator operator - (size_t subtrahend) const
    {
        return array_of_sequences_iterator(arrays, pos - subtrahend);
    }

    size_t operator - (const array_of_sequences_iterator& subtrahend) const
    {
        return pos - subtrahend.pos;
    }

    bool operator == (const array_of_sequences_iterator& aoai) const
    {
        return pos == aoai.pos;
    }

    bool operator != (const array_of_sequences_iterator& aoai) const
    {
        return pos != aoai.pos;
    }

    bool operator < (const array_of_sequences_iterator& aoai) const
    {
        return pos < aoai.pos;
    }

    bool operator <= (const array_of_sequences_iterator& aoai) const
    {
        return pos <= aoai.pos;
    }

    bool operator > (const array_of_sequences_iterator& aoai) const
    {
        return pos > aoai.pos;
    }

    bool operator >= (const array_of_sequences_iterator& aoai) const
    {
        return pos >= aoai.pos;
    }

    const value_type& operator * () const
    {
        return base_element[offset];
    }

    value_type& operator * ()
    {
        return base_element[offset];
    }

    const value_type& operator -> () const
    {
        return &(base_element[offset]);
    }

    value_type& operator -> ()
    {
        return &(base_element[offset]);
    }

    const value_type& operator [] (size_t index) const
    {
        return arrays[index / modulo][index % modulo];
    }

    value_type& operator [] (size_t index)
    {
        return arrays[index / modulo][index % modulo];
    }
};

namespace helper {

template <typename BlockType, typename SizeType, bool CanUseTrivialPointer>
class element_iterator_generator
{ };

// default case for blocks with fillers or other data: use array_of_sequences_iterator
template <typename BlockType, typename SizeType>
class element_iterator_generator<BlockType, SizeType, false>
{
    using block_type = BlockType;
    using value_type = typename block_type::value_type;

    using size_type = SizeType;

public:
    using iterator = array_of_sequences_iterator<block_type, value_type, block_type::size>;

    iterator operator () (block_type* blocks, SizeType offset) const
    {
        return iterator(blocks, offset);
    }
};

// special case for completely filled blocks: use trivial pointers
template <typename BlockType, typename SizeType>
class element_iterator_generator<BlockType, SizeType, true>
{
    using block_type = BlockType;
    using value_type = typename block_type::value_type;

    using size_type = SizeType;

public:
    using iterator = value_type *;

    iterator operator () (block_type* blocks, SizeType offset) const
    {
        // blocks[0].elem is a c-style array (value_type[blockSize]) and the
        // elements of the following blocks are directly adjacent, so the
        // "out-of-bounds" access (offset > blockSize, because the range
        // spans multiple blocks) is ok concerning accessing valid memories,
        // but we have to explicitly cast the array to a pointer to make
        // the undefined behavior sanitizer happy.
        return static_cast<iterator>(blocks[0].elem) + offset;
    }
};

} // namespace helper

template <typename BlockType, typename SizeType>
struct element_iterator_traits
{
    using element_iterator = typename helper::element_iterator_generator<
              BlockType, SizeType, BlockType::has_only_data
              >::iterator;
};

template <typename BlockType, typename SizeType>
inline
typename element_iterator_traits<BlockType, SizeType>::element_iterator
make_element_iterator(BlockType* blocks, SizeType offset)
{
    helper::element_iterator_generator<
        BlockType, SizeType, BlockType::has_only_data
        > iter_gen;
    return iter_gen(blocks, offset);
}

//! \}

} // namespace stxxl

#endif // !STXXL_ALGO_BID_ADAPTER_HEADER
