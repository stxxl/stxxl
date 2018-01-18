/***************************************************************************
 *  include/stxxl/bits/containers/btree/node.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE_NODE_HEADER
#define STXXL_CONTAINERS_BTREE_NODE_HEADER

#include <algorithm>
#include <functional>
#include <utility>

#include <stxxl/bits/containers/btree/iterator.h>
#include <stxxl/bits/containers/btree/node_cache.h>

namespace stxxl {
namespace btree {

template <class NodeType, class BTreeType>
class node_cache;

template <class KeyType, class KeyCmp, unsigned RawSize, class BTreeType>
class normal_node
{
    static constexpr bool debug = false;

public:
    using self_type = normal_node<KeyType, KeyCmp, RawSize, BTreeType>;

    friend class node_cache<self_type, BTreeType>;

    using key_type = KeyType;
    using key_compare = KeyCmp;

    enum {
        raw_size = RawSize
    };
    using bid_type = foxxll::BID<raw_size>;
    using node_bid_type = bid_type;
    using node_type = self_type;
    using value_type = std::pair<key_type, bid_type>;
    using reference = value_type &;
    using const_reference = const value_type &;

    struct metainfo_type
    {
        bid_type me;
        unsigned cur_size;
    };
    using block_type = foxxll::typed_block<raw_size, value_type, 0, metainfo_type>;

    enum {
        nelements = block_type::size - 1,
        max_size = nelements,
        min_size = nelements / 2
    };
    using block_iterator = typename block_type::iterator;
    using block_const_iterator = typename block_type::const_iterator;

    using btree_type = BTreeType;
    using size_type = typename btree_type::size_type;
    using iterator = typename btree_type::iterator;
    using const_iterator = typename btree_type::const_iterator;

    using btree_value_type = typename btree_type::value_type;
    using leaf_bid_type = typename btree_type::leaf_bid_type;
    using leaf_type = typename btree_type::leaf_type;

    using node_cache_type = node_cache<normal_node, btree_type>;

private:
    struct value_compare : public std::binary_function<value_type, value_type, bool>
    {
        key_compare comp;

        explicit value_compare(key_compare c) : comp(c) { }

        bool operator () (const value_type& x, const value_type& y) const
        {
            return comp(x.first, y.first);
        }
    };

    block_type* m_block;
    btree_type* m_btree;
    key_compare m_cmp;
    value_compare m_vcmp;

    std::pair<key_type, bid_type> insert(const std::pair<key_type, bid_type>& splitter,
                                         const block_iterator& place2insert)
    {
        std::pair<key_type, bid_type> result(m_cmp.max_value(), bid_type());

        // splitter != *place2insert
        assert(m_vcmp(*place2insert, splitter) || m_vcmp(splitter, *place2insert));

        block_iterator cur = m_block->begin() + size() - 1;
        for ( ; cur >= place2insert; --cur)
            *(cur + 1) = *cur;
        // copy elements to make space for the new element

        *place2insert = splitter;               // insert

        ++(m_block->info.cur_size);

        if (size() > max_nelements())           // overflow! need to split
        {
            LOG << "btree::normal_node::insert overflow happened, splitting";

            bid_type new_bid;
            m_btree->m_node_cache.get_new_node(new_bid);                             // new (left) node
            normal_node* new_node = m_btree->m_node_cache.get_node(new_bid, true);
            assert(new_node);

            const unsigned end_of_smaller_part = size() / 2;

            result.first = ((*m_block)[end_of_smaller_part - 1]).first;
            result.second = new_bid;

            const unsigned old_size = size();
            // copy the smaller part
            std::copy(m_block->begin(), m_block->begin() + end_of_smaller_part, new_node->m_block->begin());
            new_node->m_block->info.cur_size = end_of_smaller_part;
            // copy the larger part
            std::copy(m_block->begin() + end_of_smaller_part,
                      m_block->begin() + old_size, m_block->begin());
            m_block->info.cur_size = old_size - end_of_smaller_part;
            assert(size() + new_node->size() == old_size);

            m_btree->m_node_cache.unfix_node(new_bid);

            //req-ostream LOG << "btree::normal_node split leaf " << this << " splitter: " << result.first;
        }

        return result;
    }

    template <class CacheType>
    void fuse_or_balance(block_iterator UIt, CacheType& cache)
    {
        using local_node_type = typename CacheType::node_type;
        using local_bid_type = typename local_node_type::bid_type;

        block_iterator leftIt, rightIt;
        if (UIt == (m_block->begin() + size() - 1))                      // UIt is the last entry in the root
        {
            assert(UIt != m_block->begin());
            rightIt = UIt;
            leftIt = --UIt;
        }
        else
        {
            leftIt = UIt;
            rightIt = ++UIt;
            assert(rightIt != (m_block->begin() + size()));
        }

        // now fuse or balance nodes pointed by leftIt and rightIt
        local_bid_type left_bid = (local_bid_type)leftIt->second;
        local_bid_type right_bid = (local_bid_type)rightIt->second;
        local_node_type* left_node = cache.get_node(left_bid, true);
        local_node_type* right_node = cache.get_node(right_bid, true);

        const unsigned total_size = left_node->size() + right_node->size();
        if (total_size <= right_node->max_nelements())
        {
            // --- fuse ---

            // add the content of left_node to right_node
            right_node->fuse(*left_node);

            cache.unfix_node(right_bid);
            // 'delete_node' unfixes left-bid also
            cache.delete_node(left_bid);

            // delete left BID from the root
            std::copy(leftIt + 1, m_block->begin() + size(), leftIt);
            --(m_block->info.cur_size);
        }
        else
        {
            // --- balance ---

            key_type new_splitter = right_node->balance(*left_node);

            // change key
            leftIt->first = new_splitter;
            assert(m_vcmp(*leftIt, *rightIt));

            cache.unfix_node(left_bid);
            cache.unfix_node(right_bid);
        }
    }

public:
    virtual ~normal_node()
    {
        delete m_block;
    }

    normal_node(btree_type* btree,
                key_compare cmp)
        : m_block(new block_type),
          m_btree(btree),
          m_cmp(cmp),
          m_vcmp(cmp)
    {
        assert(min_nelements() >= 2);
        assert(2 * min_nelements() - 1 <= max_nelements());
        assert(max_nelements() <= nelements);
        // extra space for an overflow
        assert(unsigned(block_type::size) >= nelements + 1);
    }

    //! non-copyable: delete copy-constructor
    normal_node(const normal_node&) = delete;
    //! non-copyable: delete assignment operator
    normal_node& operator = (const normal_node&) = delete;

    block_type & block()
    {
        return *m_block;
    }

    bool overflows() const { return m_block->info.cur_size > max_nelements(); }
    bool underflows() const { return m_block->info.cur_size < min_nelements(); }

    static unsigned max_nelements() { return max_size; }
    static unsigned min_nelements() { return min_size; }

    /*
       template <class InputIterator>
       normal_node(InputIterator begin_, InputIterator end_,
            btree_type * btree,
            key_compare cmp):
            m_block(new block_type),
            m_btree(btree),
            m_cmp(cmp),
            m_vcmp(cmp)
       {
            assert(min_nelements() >=2);
            assert(2*min_nelements() - 1 <= max_nelements());
            assert(max_nelements() <= nelements);
            assert(unsigned(block_type::size) >= nelements +1); // extra space for an overflow

            unsigned new_size = end_ - begin_;
            assert(new_size <= max_nelements());
            assert(new_size >= min_nelements());

            std::copy(begin_,end_,m_block->begin());
            assert(stxxl::is_sorted(m_block->cbegin(),m_block->cbegin() + new_size, m_vcmp));
            m_block->info.cur_size = new_size;
       }*/

    unsigned size() const
    {
        return m_block->info.cur_size;
    }

    bid_type my_bid() const
    {
        return m_block->info.me;
    }

    void save()
    {
        foxxll::request_ptr req = m_block->write(my_bid());
        req->wait();
    }

    foxxll::request_ptr load(const bid_type& bid)
    {
        foxxll::request_ptr req = m_block->read(bid);
        req->wait();
        assert(bid == my_bid());
        return req;
    }

    foxxll::request_ptr prefetch(const bid_type& bid)
    {
        return m_block->read(bid);
    }

    void init(const bid_type& my_bid_)
    {
        m_block->info.me = my_bid_;
        m_block->info.cur_size = 0;
    }

    reference operator [] (int i)
    {
        return (*m_block)[i];
    }

    const_reference operator [] (int i) const
    {
        return (*m_block)[i];
    }

    reference back()
    {
        return (*m_block)[size() - 1];
    }

    reference front()
    {
        return *(m_block->begin());
    }

    const_reference back() const
    {
        return (*m_block)[size() - 1];
    }

    const_reference front() const
    {
        return *(m_block->begin());
    }

    std::pair<iterator, bool>
    insert(const btree_value_type& x, unsigned height,
           std::pair<key_type, bid_type>& splitter)
    {
        assert(size() <= max_nelements());
        splitter.first = m_cmp.max_value();

        value_type key2search(x.first, bid_type());
        block_iterator it =
            std::lower_bound(m_block->begin(), m_block->begin() + size(), key2search, m_vcmp);

        assert(it != (m_block->begin() + size()));

        //bid_type found_bid = it->second;

        if (height == 2)                        // found_bid points to a leaf
        {
            LOG << "btree::normal_node Inserting new value into a leaf";
            leaf_type* leaf = m_btree->m_leaf_cache.get_node((leaf_bid_type)it->second, true);
            assert(leaf);
            std::pair<key_type, leaf_bid_type> bot_splitter;
            std::pair<iterator, bool> result = leaf->insert(x, bot_splitter);
            m_btree->m_leaf_cache.unfix_node((leaf_bid_type)it->second);
            //if(key_compare::max_value() == BotSplitter.first)
            if (!(m_cmp(m_cmp.max_value(), bot_splitter.first) ||
                  m_cmp(bot_splitter.first, m_cmp.max_value())))
                return result;
            // no overflow/splitting happened

            LOG << "btree::normal_node Inserting new value in *this";

            splitter = insert(std::make_pair(bot_splitter.first, bid_type(bot_splitter.second)), it);

            return result;
        }
        else
        {                               // found_bid points to a node
            LOG << "btree::normal_node Inserting new value into a node";
            node_type* node = m_btree->m_node_cache.get_node((node_bid_type)it->second, true);
            assert(node);
            std::pair<key_type, node_bid_type> bot_splitter;
            std::pair<iterator, bool> result = node->insert(x, height - 1, bot_splitter);
            m_btree->m_node_cache.unfix_node((node_bid_type)it->second);
            //if(key_compare::max_value() == BotSplitter.first)
            if (!(m_cmp(m_cmp.max_value(), bot_splitter.first) ||
                  m_cmp(bot_splitter.first, m_cmp.max_value())))
                return result;
            // no overflow/splitting happened

            LOG << "btree::normal_node Inserting new value in *this";

            splitter = insert(bot_splitter, it);

            return result;
        }
    }

    iterator begin(unsigned height)
    {
        bid_type first_bid = m_block->begin()->second;
        if (height == 2)                        // FirstBid points to a leaf
        {
            assert(size() > 1);
            LOG << "btree::node retrieveing begin() from the first leaf";
            leaf_type* leaf = m_btree->m_leaf_cache.get_node((leaf_bid_type)first_bid);
            assert(leaf);
            return leaf->begin();
        }
        else
        {                         // FirstBid points to a node
            LOG << "btree: retrieveing begin() from the first node";
            node_type* node = m_btree->m_node_cache.get_node((node_bid_type)first_bid, true);
            assert(node);
            iterator result = node->begin(height - 1);
            m_btree->m_node_cache.unfix_node((node_bid_type)first_bid);
            return result;
        }
    }

    const_iterator begin(unsigned height) const
    {
        bid_type FirstBid = m_block->begin()->second;
        if (height == 2)                        // FirstBid points to a leaf
        {
            assert(size() > 1);
            LOG << "btree::node retrieveing begin() from the first leaf";
            const leaf_type* leaf = m_btree->m_leaf_cache.get_const_node((leaf_bid_type)FirstBid);
            assert(leaf);
            return leaf->begin();
        }
        else
        {                         // FirstBid points to a node
            LOG << "btree: retrieveing begin() from the first node";
            const node_type* node = m_btree->m_node_cache.get_const_node((node_bid_type)FirstBid, true);
            assert(node);
            const_iterator result = node->begin(height - 1);
            m_btree->m_node_cache.unfix_node((node_bid_type)FirstBid);
            return result;
        }
    }

    iterator find(const key_type& k, unsigned height)
    {
        value_type key2search(k, bid_type());

        block_iterator it =
            std::lower_bound(m_block->begin(), m_block->begin() + size(), key2search, m_vcmp);

        assert(it != (m_block->begin() + size()));

        bid_type found_bid = it->second;

        if (height == 2)                // found_bid points to a leaf
        {
            LOG << "Searching in a leaf";
            leaf_type* leaf = m_btree->m_leaf_cache.get_node((leaf_bid_type)found_bid, true);
            assert(leaf);
            iterator result = leaf->find(k);
            m_btree->m_leaf_cache.unfix_node((leaf_bid_type)found_bid);

            return result;
        }

        // found_bid points to a node
        LOG << "Searching in a node";
        node_type* node = m_btree->m_node_cache.get_node((node_bid_type)found_bid, true);
        assert(node);
        iterator result = node->find(k, height - 1);
        m_btree->m_node_cache.unfix_node((node_bid_type)found_bid);

        return result;
    }

    const_iterator find(const key_type& k, unsigned height) const
    {
        value_type key2search(k, bid_type());

        block_iterator it =
            std::lower_bound(m_block->begin(), m_block->begin() + size(), key2search, m_vcmp);

        assert(it != (m_block->begin() + size()));

        bid_type found_bid = it->second;

        if (height == 2)                // found_bid points to a leaf
        {
            LOG << "Searching in a leaf";
            const leaf_type* leaf = m_btree->m_leaf_cache.get_const_node((leaf_bid_type)found_bid, true);
            assert(leaf);
            const_iterator result = leaf->find(k);
            m_btree->m_leaf_cache.unfix_node((leaf_bid_type)found_bid);

            return result;
        }

        // found_bid points to a node
        LOG << "Searching in a node";
        const node_type* node = m_btree->m_node_cache.get_const_node((node_bid_type)found_bid, true);
        assert(node);
        const_iterator result = node->find(k, height - 1);
        m_btree->m_node_cache.unfix_node((node_bid_type)found_bid);

        return result;
    }

    iterator lower_bound(const key_type& k, unsigned height)
    {
        value_type key2search(k, bid_type());
        assert(!m_vcmp(back(), key2search));
        block_iterator it =
            std::lower_bound(m_block->begin(), m_block->begin() + size(), key2search, m_vcmp);

        assert(it != (m_block->begin() + size()));

        bid_type found_bid = it->second;

        if (height == 2)                // found_bid points to a leaf
        {
            LOG << "Searching lower bound in a leaf";
            leaf_type* leaf = m_btree->m_leaf_cache.get_node((leaf_bid_type)found_bid, true);
            assert(leaf);
            iterator result = leaf->lower_bound(k);
            m_btree->m_leaf_cache.unfix_node((leaf_bid_type)found_bid);

            return result;
        }

        // found_bid points to a node
        LOG << "Searching lower bound in a node";
        node_type* node = m_btree->m_node_cache.get_node((node_bid_type)found_bid, true);
        assert(node);
        iterator result = node->lower_bound(k, height - 1);
        m_btree->m_node_cache.unfix_node((node_bid_type)found_bid);

        return result;
    }

    const_iterator lower_bound(const key_type& k, unsigned height) const
    {
        value_type key2search(k, bid_type());
        assert(!m_vcmp(back(), key2search));
        block_iterator it =
            std::lower_bound(m_block->begin(), m_block->begin() + size(), key2search, m_vcmp);

        assert(it != (m_block->begin() + size()));

        bid_type found_bid = it->second;

        if (height == 2)                // found_bid points to a leaf
        {
            LOG << "Searching lower bound in a leaf";
            const leaf_type* leaf = m_btree->m_leaf_cache.get_const_node((leaf_bid_type)found_bid, true);
            assert(leaf);
            const_iterator result = leaf->lower_bound(k);
            m_btree->m_leaf_cache.unfix_node((leaf_bid_type)found_bid);

            return result;
        }

        // found_bid points to a node
        LOG << "Searching lower bound in a node";
        const node_type* node = m_btree->m_node_cache.get_const_node((node_bid_type)found_bid, true);
        assert(node);
        const_iterator result = node->lower_bound(k, height - 1);
        m_btree->m_node_cache.unfix_node((node_bid_type)found_bid);

        return result;
    }

    iterator upper_bound(const key_type& k, unsigned height)
    {
        value_type key2search(k, bid_type());
        assert(m_vcmp(key2search, back()));
        block_iterator it =
            std::upper_bound(m_block->begin(), m_block->begin() + size(), key2search, m_vcmp);

        assert(it != (m_block->begin() + size()));

        bid_type found_bid = it->second;

        if (height == 2)                // found_bid points to a leaf
        {
            LOG << "Searching upper bound in a leaf";
            leaf_type* leaf = m_btree->m_leaf_cache.get_node((leaf_bid_type)found_bid, true);
            assert(leaf);
            iterator result = leaf->upper_bound(k);
            m_btree->m_leaf_cache.unfix_node((leaf_bid_type)found_bid);

            return result;
        }

        // found_bid points to a node
        LOG << "Searching upper bound in a node";
        node_type* node = m_btree->m_node_cache.get_node((node_bid_type)found_bid, true);
        assert(node);
        iterator result = node->upper_bound(k, height - 1);
        m_btree->m_node_cache.unfix_node((node_bid_type)found_bid);

        return result;
    }

    const_iterator upper_bound(const key_type& k, unsigned height) const
    {
        value_type key2search(k, bid_type());
        assert(m_vcmp(key2search, back()));
        block_iterator it =
            std::upper_bound(m_block->begin(), m_block->begin() + size(), key2search, m_vcmp);

        assert(it != (m_block->begin() + size()));

        bid_type found_bid = it->second;

        if (height == 2)                // found_bid points to a leaf
        {
            LOG << "Searching upper bound in a leaf";
            const leaf_type* leaf = m_btree->m_leaf_cache.get_const_node((leaf_bid_type)found_bid, true);
            assert(leaf);
            const_iterator result = leaf->upper_bound(k);
            m_btree->m_leaf_cache.unfix_node((leaf_bid_type)found_bid);

            return result;
        }

        // found_bid points to a node
        LOG << "Searching upper bound in a node";
        const node_type* node = m_btree->m_node_cache.get_const_node((node_bid_type)found_bid, true);
        assert(node);
        const_iterator result = node->upper_bound(k, height - 1);
        m_btree->m_node_cache.unfix_node((node_bid_type)found_bid);

        return result;
    }

    void fuse(const normal_node& src)
    {
        assert(m_vcmp(src.back(), front()));
        const unsigned src_size = src.size();

        block_iterator cur = m_block->begin() + size() - 1;
        block_const_iterator begin = m_block->begin();

        for ( ; cur >= begin; --cur)
            *(cur + src_size) = *cur;
        // move elements to make space for Src elements

        // copy Src to *this leaf
        std::copy(src.m_block->begin(), src.m_block->begin() + src_size, m_block->begin());

        m_block->info.cur_size += src_size;
    }

    key_type balance(normal_node& left, bool check_constraints = true)
    {
        const unsigned total_size = left.size() + size();
        unsigned new_left_size = total_size / 2;
        unsigned new_right_size = total_size - new_left_size;

        assert(!check_constraints || new_left_size <= left.max_nelements());
        assert(!check_constraints || new_left_size >= left.min_nelements());
        assert(!check_constraints || new_right_size <= max_nelements());
        assert(!check_constraints || new_right_size >= min_nelements());
        tlx::unused(check_constraints);

        assert(m_vcmp(left.back(), front()) || size() == 0);

        if (new_left_size < left.size())
        {
            // #elements to move from left to *this
            const unsigned nEl2Move = left.size() - new_left_size;

            block_iterator cur = m_block->begin() + size() - 1;
            block_const_iterator begin = m_block->begin();

            for ( ; cur >= begin; --cur)
                *(cur + nEl2Move) = *cur;
            // move elements to make space for Src elements

            // copy left to *this leaf
            std::copy(left.m_block->begin() + new_left_size,
                      left.m_block->begin() + left.size(), m_block->begin());
        }
        else
        {
            assert(new_right_size < size());

            // #elements to move from *this to left
            const unsigned nEl2Move = size() - new_right_size;

            // copy *this to left
            std::copy(m_block->begin(),
                      m_block->begin() + nEl2Move, left.m_block->begin() + left.size());
            // move elements in *this
            std::copy(m_block->begin() + nEl2Move,
                      m_block->begin() + size(), m_block->begin());
        }

        m_block->info.cur_size = new_right_size;                           // update size
        left.m_block->info.cur_size = new_left_size;                       // update size

        return left.back().first;
    }

    size_type erase(const key_type& k, unsigned height)
    {
        value_type key2search(k, bid_type());

        block_iterator it =
            std::lower_bound(m_block->begin(), m_block->begin() + size(), key2search, m_vcmp);

        assert(it != (m_block->begin() + size()));

        bid_type found_bid = it->second;

        assert(size() >= 2);

        if (height == 2)                        // 'found_bid' points to a leaf
        {
            LOG << "btree::normal_node Deleting key from a leaf";
            leaf_type* leaf = m_btree->m_leaf_cache.get_node((leaf_bid_type)found_bid, true);
            assert(leaf);
            size_type result = leaf->erase(k);
            m_btree->m_leaf_cache.unfix_node((leaf_bid_type)it->second);
            if (!leaf->underflows())
                return result;
            // no underflow or root has a special degree 1 (too few elements)

            LOG << "btree::normal_node Fusing or rebalancing a leaf";
            fuse_or_balance(it, m_btree->m_leaf_cache);

            return result;
        }

        // 'found_bid' points to a node
        LOG << "btree::normal_node Deleting key from a node";
        node_type* node = m_btree->m_node_cache.get_node((node_bid_type)found_bid, true);
        assert(node);
        size_type result = node->erase(k, height - 1);
        m_btree->m_node_cache.unfix_node((node_bid_type)found_bid);
        if (!node->underflows())
            return result;
        // no underflow happened

        LOG << "btree::normal_node Fusing or rebalancing a node";
        fuse_or_balance(it, m_btree->m_node_cache);

        return result;
    }

    void deallocate_children(unsigned height)
    {
        if (height == 2)
        {
            // we have children leaves here
            for (block_const_iterator it = block().begin();
                 it != block().begin() + size(); ++it)
            {
                // delete from leaf cache and deallocate bid
                m_btree->m_leaf_cache.delete_node((leaf_bid_type)it->second);
            }
        }
        else
        {
            for (block_const_iterator it = block().begin();
                 it != block().begin() + size(); ++it)
            {
                node_type* node = m_btree->m_node_cache.get_node((node_bid_type)it->second);
                assert(node);
                node->deallocate_children(height - 1);
                // delete from node cache and deallocate bid
                m_btree->m_node_cache.delete_node((node_bid_type)it->second);
            }
        }
    }

    void push_back(const value_type& x)
    {
        (*this)[size()] = x;
        ++(m_block->info.cur_size);
    }
};

} // namespace btree
} // namespace stxxl

#endif // !STXXL_CONTAINERS_BTREE_NODE_HEADER
