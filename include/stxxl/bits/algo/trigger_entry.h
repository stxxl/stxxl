/***************************************************************************
 *  include/stxxl/bits/algo/trigger_entry.h
 *
 *  include/stxxl/bits/algo/adaptor.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALGO_TRIGGER_ENTRY_HEADER
#define STXXL_ALGO_TRIGGER_ENTRY_HEADER

#include <foxxll/mng/bid.hpp>

#include <stxxl/bits/algo/bid_adapter.h>
#include <stxxl/types>

namespace stxxl {

template <size_t BlockSize, typename RunType, class PosType = std::make_signed<size_t>::type>
struct runs2bid_array_adaptor : public two2one_dim_array_adapter_base<RunType*, foxxll::BID<BlockSize>, PosType>
{
    using self_type = runs2bid_array_adaptor<BlockSize, RunType, PosType>;
    using data_type = foxxll::BID<BlockSize>;

    enum {
        block_size = BlockSize
    };

    size_t dim_size;

    using parent_type = two2one_dim_array_adapter_base<RunType*, data_type, PosType>;
    using parent_type::array;
    using parent_type::pos;

    runs2bid_array_adaptor(RunType** a, PosType p, size_t d)
        : two2one_dim_array_adapter_base<RunType*, data_type, PosType>(a, p), dim_size(d)
    { }
    explicit runs2bid_array_adaptor(const self_type& a)
        : two2one_dim_array_adapter_base<RunType*, data_type, PosType>(a), dim_size(a.dim_size)
    { }

    const self_type& operator = (const self_type& a)
    {
        array = a.array;
        pos = a.pos;
        dim_size = a.dim_size;
        return *this;
    }

    data_type& operator * ()
    {
        CHECK_RUN_BOUNDS(pos);
        return (data_type&)((*(array[(pos) % dim_size]))[(pos) / dim_size].bid);
    }

    const data_type* operator -> () const
    {
        CHECK_RUN_BOUNDS(pos);
        return &((*(array[(pos) % dim_size])[(pos) / dim_size].bid));
    }

    data_type& operator [] (PosType n) const
    {
        n += pos;
        CHECK_RUN_BOUNDS(n);
        return (data_type&)((*(array[(n) % dim_size]))[(n) / dim_size].bid);
    }
};

BLOCK_ADAPTOR_OPERATORS(runs2bid_array_adaptor)

template <size_t BlockSize, typename RunType, class PosType = std::make_signed<size_t>::type>
struct runs2bid_array_adaptor2
    : public two2one_dim_array_adapter_base<RunType*, foxxll::BID<BlockSize>, PosType>
{
    using self_type = runs2bid_array_adaptor2<BlockSize, RunType, PosType>;
    using data_type = foxxll::BID<BlockSize>;

    using base_type = two2one_dim_array_adapter_base<RunType*, data_type, PosType>;

    using base_type::pos;
    using base_type::array;

    enum {
        block_size = BlockSize
    };

    PosType w, h, K;

    runs2bid_array_adaptor2(RunType** a, PosType p, PosType _w, PosType _h)
        : two2one_dim_array_adapter_base<RunType*, data_type, PosType>(a, p),
          w(_w), h(_h), K(_w * _h)
    { }

    runs2bid_array_adaptor2(const runs2bid_array_adaptor2& a)
        : two2one_dim_array_adapter_base<RunType*, data_type, PosType>(a),
          w(a.w), h(a.h), K(a.K)
    { }

    const self_type& operator = (const self_type& a)
    {
        array = a.array;
        pos = a.pos;
        w = a.w;
        h = a.h;
        K = a.K;
        return *this;
    }

    data_type& operator * ()
    {
        PosType i = pos - K;
        if (i < 0)
            return (data_type&)((*(array[(pos) % w]))[(pos) / w].bid);

        PosType _w = w;
        _w--;
        return (data_type&)((*(array[(i) % _w]))[h + (i / _w)].bid);
    }

    const data_type& operator * () const
    {
        PosType i = pos - K;
        if (i < 0)
            return (data_type&)((*(array[(pos) % w]))[(pos) / w].bid);

        PosType _w = w;
        _w--;
        return (data_type&)((*(array[(i) % _w]))[h + (i / _w)].bid);
    }

    data_type* operator -> ()
    { return &operator * (); }

    const data_type* operator -> () const
    { return &operator * (); }

    data_type& operator [] (PosType n) const
    {
        n += pos;
        PosType i = n - K;
        if (i < 0)
            return (data_type&)((*(array[(n) % w]))[(n) / w].bid);

        PosType _w = w;
        _w--;
        return (data_type&)((*(array[(i) % _w]))[h + (i / _w)].bid);
    }
};

BLOCK_ADAPTOR_OPERATORS(runs2bid_array_adaptor2)

template <typename trigger_iterator_type>
struct trigger_entry_iterator
{
    using self_type = trigger_entry_iterator<trigger_iterator_type>;
    using bid_type = typename std::iterator_traits<trigger_iterator_type>::value_type::bid_type;

    // STL typedefs
    using value_type = bid_type;
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = int64_t;
    using pointer = value_type *;
    using reference = value_type &;

    trigger_iterator_type value;

    explicit trigger_entry_iterator(trigger_iterator_type v) : value(v) { }

    bid_type& operator * ()
    {
        return value->bid;
    }
    bid_type* operator -> () const
    {
        return &(value->bid);
    }
    const bid_type& operator [] (size_t n) const
    {
        return (value + n)->bid;
    }
    bid_type& operator [] (size_t n)
    {
        return (value + n)->bid;
    }

    self_type& operator ++ ()
    {
        value++;
        return *this;
    }
    self_type operator ++ (int)
    {
        self_type tmp = *this;
        value++;
        return tmp;
    }
    self_type& operator -- ()
    {
        value--;
        return *this;
    }
    self_type operator -- (int)
    {
        self_type tmp = *this;
        value--;
        return tmp;
    }
    bool operator == (const self_type& a) const
    {
        return value == a.value;
    }
    bool operator != (const self_type& a) const
    {
        return value != a.value;
    }
    self_type operator += (size_t n)
    {
        value += n;
        return *this;
    }
    self_type operator -= (size_t n)
    {
        value -= n;
        return *this;
    }
    int64_t operator - (const self_type& a) const
    {
        return value - a.value;
    }
    trigger_iterator_type operator + (const self_type& a) const
    {
        return value + a.value;
    }
};

template <typename Iterator>
inline
trigger_entry_iterator<Iterator>
make_bid_iterator(Iterator iter)
{
    return trigger_entry_iterator<Iterator>(iter);
}

} // namespace stxxl

#endif // !STXXL_ALGO_TRIGGER_ENTRY_HEADER
