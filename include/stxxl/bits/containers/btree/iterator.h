/***************************************************************************
 *  include/stxxl/bits/containers/btree/iterator.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE__ITERATOR_H
#define STXXL_CONTAINERS_BTREE__ITERATOR_H

#include "stxxl/bits/common/utils.h"


__STXXL_BEGIN_NAMESPACE


namespace btree
{
    template <class BTreeType>
    class iterator_map;
    template <class BTreeType>
    class btree_iterator;
    template <class BTreeType>
    class btree_const_iterator;
    template <class KeyType_, class DataType_, class KeyCmp_, unsigned LogNElem_, class BTreeType>
    class normal_leaf;

    template <class BTreeType>
    class btree_iterator_base
    {
    public:
        typedef BTreeType btree_type;
        typedef typename btree_type::leaf_bid_type bid_type;
        typedef typename btree_type::value_type value_type;
        typedef typename btree_type::reference reference;
        typedef typename btree_type::const_reference const_reference;
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef typename btree_type::difference_type difference_type;

        friend class iterator_map<btree_type>;
        template <class KeyType_, class DataType_,
                  class KeyCmp_, unsigned LogNElem_, class BTreeType__>
        friend class normal_leaf;

        template <class BTreeType_>
        friend bool operator == (const btree_iterator<BTreeType_> & a,
                                 const btree_const_iterator<BTreeType_> & b);
        template <class BTreeType_>
        friend bool operator != (const btree_iterator<BTreeType_> & a,
                                 const btree_const_iterator<BTreeType_> & b);

    protected:
        btree_type * btree_;
        bid_type bid;
        unsigned pos;

        btree_iterator_base()
        {
            STXXL_VERBOSE3("btree_iterator_base def construct addr=" << this);
            make_invalid();
        }

        btree_iterator_base(
            btree_type * btree__,
            const bid_type & b,
            unsigned p
            ) : btree_(btree__), bid(b), pos(p)
        {
            STXXL_VERBOSE3("btree_iterator_base parameter construct addr=" << this);
            btree_->iterator_map_.register_iterator(*this);
        }

        void make_invalid()
        {
            btree_ = NULL;
        }

        btree_iterator_base(const btree_iterator_base & obj)
        {
            STXXL_VERBOSE3("btree_iterator_base constr from" << (&obj) << " to " << this);
            btree_ = obj.btree_;
            bid = obj.bid;
            pos = obj.pos;
            if (btree_)
                btree_->iterator_map_.register_iterator(*this);
        }

        btree_iterator_base & operator = (const btree_iterator_base & obj)
        {
            STXXL_VERBOSE3("btree_iterator_base copy from" << (&obj) << " to " << this);
            if (&obj != this)
            {
                if (btree_)
                    btree_->iterator_map_.unregister_iterator(*this);

                btree_ = obj.btree_;
                bid = obj.bid;
                pos = obj.pos;
                if (btree_)
                    btree_->iterator_map_.register_iterator(*this);
            }
            return *this;
        }

        reference non_const_access()
        {
            assert(btree_);
            typename btree_type::leaf_type * Leaf = btree_->leaf_cache_.get_node(bid);
            assert(Leaf);
            return (reference)((*Leaf)[pos]);
        }

        const_reference const_access() const
        {
            assert(btree_);
            typename btree_type::leaf_type const * Leaf = btree_->leaf_cache_.get_const_node(bid);
            assert(Leaf);
            return (reference)((*Leaf)[pos]);
        }

        bool operator == (const btree_iterator_base & obj) const
        {
            return bid == obj.bid && pos == obj.pos && btree_ == obj.btree_;
        }

        bool operator != (const btree_iterator_base & obj) const
        {
            return bid != obj.bid || pos != obj.pos || btree_ != obj.btree_;
        }

        btree_iterator_base & increment ()
        {
            assert(btree_);
            bid_type cur_bid = bid;
            typename btree_type::leaf_type const * Leaf = btree_->leaf_cache_.get_const_node(bid, true);
            assert(Leaf);
            Leaf->increment_iterator(*this);
            btree_->leaf_cache_.unfix_node(cur_bid);
            return *this;
        }

        btree_iterator_base & decrement ()
        {
            assert(btree_);
            bid_type cur_bid = bid;
            typename btree_type::leaf_type const * Leaf = btree_->leaf_cache_.get_const_node(bid, true);
            assert(Leaf);
            Leaf->decrement_iterator(*this);
            btree_->leaf_cache_.unfix_node(cur_bid);
            return *this;
        }

    public:
        virtual ~btree_iterator_base()
        {
            STXXL_VERBOSE3("btree_iterator_base deconst " << this);
            if (btree_)
                btree_->iterator_map_.unregister_iterator(*this);
        }
    };

    template <class BTreeType>
    class btree_iterator : public btree_iterator_base<BTreeType>
    {
    public:
        typedef BTreeType btree_type;
        typedef typename btree_type::leaf_bid_type bid_type;
        typedef typename btree_type::value_type value_type;
        typedef typename btree_type::reference reference;
        typedef typename btree_type::const_reference const_reference;
        typedef typename btree_type::pointer pointer;

        template <class KeyType_, class DataType_,
                  class KeyCmp_, unsigned LogNElem_, class BTreeType__>
        friend class normal_leaf;

        using btree_iterator_base<btree_type>::non_const_access;

        btree_iterator() : btree_iterator_base<btree_type>()
        { }

        btree_iterator(const btree_iterator & obj) :
            btree_iterator_base<btree_type>(obj)
        { }

        btree_iterator & operator = (const btree_iterator & obj)
        {
            btree_iterator_base<btree_type>::operator =(obj);
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

        bool operator == (const btree_iterator & obj) const
        {
            return btree_iterator_base<btree_type>::operator ==(obj);
        }

        bool operator != (const btree_iterator & obj) const
        {
            return btree_iterator_base<btree_type>::operator !=(obj);
        }

        btree_iterator & operator ++ ()
        {
            assert(*this != btree_iterator_base<btree_type>::btree_->end());
            btree_iterator_base<btree_type>::increment();
            return *this;
        }

        btree_iterator & operator -- ()
        {
            btree_iterator_base<btree_type>::decrement();
            return *this;
        }

        btree_iterator operator ++ (int)
        {
            assert(*this != btree_iterator_base<btree_type>::btree_->end());
            btree_iterator result(*this);
            btree_iterator_base<btree_type>::increment();
            return result;
        }

        btree_iterator operator -- (int)
        {
            btree_iterator result(*this);
            btree_iterator_base<btree_type>::decrement();
            return result;
        }

    private:
        btree_iterator(
            btree_type * btree__,
            const bid_type & b,
            unsigned p
            ) : btree_iterator_base<btree_type>(btree__, b, p)
        { }
    };

    template <class BTreeType>
    class btree_const_iterator : public btree_iterator_base<BTreeType>
    {
    public:
        typedef btree_iterator<BTreeType> iterator;

        typedef BTreeType btree_type;
        typedef typename btree_type::leaf_bid_type bid_type;
        typedef typename btree_type::value_type value_type;
        typedef typename btree_type::const_reference reference;
        typedef typename btree_type::const_pointer pointer;

        template <class KeyType_, class DataType_,
                  class KeyCmp_, unsigned LogNElem_, class BTreeType__>
        friend class normal_leaf;

        using btree_iterator_base<btree_type>::const_access;

        btree_const_iterator() : btree_iterator_base<btree_type>()
        { }

        btree_const_iterator(const btree_const_iterator & obj) :
            btree_iterator_base<btree_type>(obj)
        { }

        btree_const_iterator(const iterator & obj) :
            btree_iterator_base<btree_type>(obj)
        { }

        btree_const_iterator & operator = (const btree_const_iterator & obj)
        {
            btree_iterator_base<btree_type>::operator =(obj);
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


        bool operator == (const iterator & obj) const
        {
            return btree_iterator_base<btree_type>::operator ==(obj);
        }

        bool operator != (const iterator & obj) const
        {
            return btree_iterator_base<btree_type>::operator !=(obj);
        }

        bool operator == (const btree_const_iterator & obj) const
        {
            return btree_iterator_base<btree_type>::operator ==(obj);
        }

        bool operator != (const btree_const_iterator & obj) const
        {
            return btree_iterator_base<btree_type>::operator !=(obj);
        }

        btree_const_iterator & operator ++ ()
        {
            assert(*this != btree_iterator_base<btree_type>::btree_->end());
            btree_iterator_base<btree_type>::increment();
            return *this;
        }

        btree_const_iterator & operator -- ()
        {
            btree_iterator_base<btree_type>::decrement();
            return *this;
        }

        btree_const_iterator operator ++ (int)
        {
            assert(*this != btree_iterator_base<btree_type>::btree_->end());
            btree_const_iterator result(*this);
            btree_iterator_base<btree_type>::increment();
            return result;
        }

        btree_const_iterator operator -- (int)
        {
            btree_const_iterator result(*this);
            btree_iterator_base<btree_type>::decrement();
            return result;
        }

    private:
        btree_const_iterator(
            btree_type * btree__,
            const bid_type & b,
            unsigned p
            ) : btree_iterator_base<btree_type>(btree__, b, p)
        { }
    };

    template <class BTreeType>
    inline bool operator == (const btree_iterator<BTreeType> & a,
                             const btree_const_iterator<BTreeType> & b)
    {
        return a.btree_iterator_base<BTreeType>::operator ==(b);
    }

    template <class BTreeType>
    inline bool operator != (const btree_iterator<BTreeType> & a,
                             const btree_const_iterator<BTreeType> & b)
    {
        return a.btree_iterator_base<BTreeType>::operator !=(b);
    }
}

__STXXL_END_NAMESPACE

#endif /* STXXL_CONTAINERS_BTREE__ITERATOR_H */
