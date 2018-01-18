/***************************************************************************
 *  include/stxxl/bits/containers/btree/iterator.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE_ITERATOR_HEADER
#define STXXL_CONTAINERS_BTREE_ITERATOR_HEADER

#include <cassert>
#include <iterator>

#include <tlx/logger.hpp>

#include <foxxll/common/types.hpp>

namespace stxxl {
namespace btree {

template <class BTreeType>
class iterator_map;
template <class BTreeType>
class btree_iterator;
template <class BTreeType>
class btree_const_iterator;
template <class KeyType, class DataType, class KeyCmp,
          unsigned LogNElem, class BTreeType>
class normal_leaf;

template <class BTreeType>
class btree_iterator_base
{
    static constexpr bool debug = false;

public:
    using btree_type = BTreeType;
    using bid_type = typename btree_type::leaf_bid_type;
    using value_type = typename btree_type::value_type;
    using reference = typename btree_type::reference;
    using const_reference = typename btree_type::const_reference;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = typename btree_type::difference_type;

    using leaf_type = typename btree_type::leaf_type;

    friend class iterator_map<btree_type>;
    template <class KeyType, class DataType,
              class KeyCmp, unsigned LogNElem, class AnyBTreeType>
    friend class normal_leaf;

    template <class AnyBTreeType>
    friend bool operator == (const btree_iterator<AnyBTreeType>& a,
                             const btree_const_iterator<AnyBTreeType>& b);
    template <class AnyBTreeType>
    friend bool operator != (const btree_iterator<AnyBTreeType>& a,
                             const btree_const_iterator<AnyBTreeType>& b);

protected:
    btree_type* btree;
    bid_type bid;
    size_t pos;

    btree_iterator_base()
    {
        LOG << "btree_iterator_base def construct addr=" << this;
        make_invalid();
    }

    btree_iterator_base(btree_type* _btree,
                        const bid_type& _bid,
                        const size_t _pos)
        : btree(_btree), bid(_bid), pos(_pos)
    {
        LOG << "btree_iterator_base parameter construct addr=" << this;
        btree->m_iterator_map.register_iterator(*this);
    }

    void make_invalid()
    {
        btree = nullptr;
        pos = 0;
    }

    btree_iterator_base(const btree_iterator_base& obj)
    {
        LOG << "btree_iterator_base constr from" << (&obj) << " to " << this;
        btree = obj.btree;
        bid = obj.bid;
        pos = obj.pos;
        if (btree)
            btree->m_iterator_map.register_iterator(*this);
    }

    btree_iterator_base& operator = (const btree_iterator_base& obj)
    {
        LOG << "btree_iterator_base copy from" << (&obj) << " to " << this;
        if (&obj != this)
        {
            if (btree)
                btree->m_iterator_map.unregister_iterator(*this);

            btree = obj.btree;
            bid = obj.bid;
            pos = obj.pos;
            if (btree)
                btree->m_iterator_map.register_iterator(*this);
        }
        return *this;
    }

    reference non_const_access()
    {
        assert(btree);
        leaf_type* leaf = btree->m_leaf_cache.get_node(bid);
        assert(leaf);
        return (reference)((*leaf)[pos]);
    }

    const_reference const_access() const
    {
        assert(btree);
        leaf_type const* leaf = btree->m_leaf_cache.get_const_node(bid);
        assert(leaf);
        return (reference)((*leaf)[pos]);
    }

    bool operator == (const btree_iterator_base& obj) const
    {
        return bid == obj.bid && pos == obj.pos && btree == obj.btree;
    }

    bool operator != (const btree_iterator_base& obj) const
    {
        return bid != obj.bid || pos != obj.pos || btree != obj.btree;
    }

    btree_iterator_base & increment()
    {
        assert(btree);
        bid_type cur_bid = bid;
        const leaf_type* leaf = btree->m_leaf_cache.get_const_node(bid, true);
        assert(leaf);
        leaf->increment_iterator(*this);
        btree->m_leaf_cache.unfix_node(cur_bid);
        return *this;
    }

    btree_iterator_base & decrement()
    {
        assert(btree);
        bid_type cur_bid = bid;
        const leaf_type* leaf = btree->m_leaf_cache.get_const_node(bid, true);
        assert(leaf);
        leaf->decrement_iterator(*this);
        btree->m_leaf_cache.unfix_node(cur_bid);
        return *this;
    }

public:
    virtual ~btree_iterator_base()
    {
        LOG << "btree_iterator_base deconst " << this;
        if (btree)
            btree->m_iterator_map.unregister_iterator(*this);
    }
};

template <class BTreeType>
class btree_iterator : public btree_iterator_base<BTreeType>
{
public:
    using btree_type = BTreeType;
    using bid_type = typename btree_type::leaf_bid_type;
    using value_type = typename btree_type::value_type;
    using reference = typename btree_type::reference;
    using const_reference = typename btree_type::const_reference;
    using pointer = typename btree_type::pointer;

    using base_type = btree_iterator_base<btree_type>;

    template <class KeyType, class DataType,
              class KeyCmp, unsigned LogNElem, class AnyBTreeType>
    friend class normal_leaf;

    using base_type::non_const_access;

    btree_iterator()
        : base_type()
    { }

    btree_iterator(const btree_iterator& obj)
        : base_type(obj)
    { }

    btree_iterator& operator = (const btree_iterator& obj)
    {
        base_type::operator = (obj);
        return *this;
    }

    reference operator * ()
    {
        return non_const_access();
    }

    pointer operator -> ()
    {
        return &(non_const_access());
    }

    bool operator == (const btree_iterator& obj) const
    {
        return base_type::operator == (obj);
    }

    bool operator != (const btree_iterator& obj) const
    {
        return base_type::operator != (obj);
    }

    btree_iterator& operator ++ ()
    {
        assert(*this != base_type::btree->end());
        base_type::increment();
        return *this;
    }

    btree_iterator& operator -- ()
    {
        base_type::decrement();
        return *this;
    }

    btree_iterator operator ++ (int)
    {
        assert(*this != base_type::btree->end());
        btree_iterator result(*this);
        base_type::increment();
        return result;
    }

    btree_iterator operator -- (int)
    {
        btree_iterator result(*this);
        base_type::decrement();
        return result;
    }

private:
    btree_iterator(btree_type* _btree,
                   const bid_type& _bid,
                   const size_t _pos)
        : base_type(_btree, _bid, _pos)
    { }
};

template <class BTreeType>
class btree_const_iterator : public btree_iterator_base<BTreeType>
{
public:
    using iterator = btree_iterator<BTreeType>;

    using btree_type = BTreeType;
    using bid_type = typename btree_type::leaf_bid_type;
    using value_type = typename btree_type::value_type;
    using reference = typename btree_type::const_reference;
    using pointer = typename btree_type::const_pointer;

    using base_type = btree_iterator_base<btree_type>;

    template <class KeyType, class DataType,
              class KeyCmp, unsigned LogNElem, class AnyBTreeType>
    friend class normal_leaf;

    using base_type::const_access;

    btree_const_iterator()
        : base_type()
    { }

    btree_const_iterator(const btree_const_iterator& obj)
        : base_type(obj)
    { }

    btree_const_iterator(const iterator& obj) // NOLINT
        : base_type(obj)
    { }

    btree_const_iterator& operator = (const btree_const_iterator& obj)
    {
        base_type::operator = (obj);
        return *this;
    }

    reference operator * ()
    {
        return const_access();
    }

    pointer operator -> ()
    {
        return &(const_access());
    }

    bool operator == (const iterator& obj) const
    {
        return base_type::operator == (obj);
    }

    bool operator != (const iterator& obj) const
    {
        return base_type::operator != (obj);
    }

    bool operator == (const btree_const_iterator& obj) const
    {
        return base_type::operator == (obj);
    }

    bool operator != (const btree_const_iterator& obj) const
    {
        return base_type::operator != (obj);
    }

    btree_const_iterator& operator ++ ()
    {
        assert(*this != base_type::btree->end());
        base_type::increment();
        return *this;
    }

    btree_const_iterator& operator -- ()
    {
        base_type::decrement();
        return *this;
    }

    btree_const_iterator operator ++ (int)
    {
        assert(*this != base_type::btree->end());
        btree_const_iterator result(*this);
        base_type::increment();
        return result;
    }

    btree_const_iterator operator -- (int)
    {
        btree_const_iterator result(*this);
        base_type::decrement();
        return result;
    }

private:
    btree_const_iterator(btree_type* _btree,
                         const bid_type& _bid,
                         const size_t _pos)
        : base_type(_btree, _bid, _pos)
    { }
};

template <class BTreeType>
inline bool operator == (const btree_iterator<BTreeType>& a,
                         const btree_const_iterator<BTreeType>& b)
{
    return a.btree_iterator_base<BTreeType>::operator == (b);
}

template <class BTreeType>
inline bool operator != (const btree_iterator<BTreeType>& a,
                         const btree_const_iterator<BTreeType>& b)
{
    return a.btree_iterator_base<BTreeType>::operator != (b);
}

} // namespace btree
} // namespace stxxl

#endif // !STXXL_CONTAINERS_BTREE_ITERATOR_HEADER
