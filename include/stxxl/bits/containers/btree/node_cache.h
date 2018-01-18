/***************************************************************************
 *  include/stxxl/bits/containers/btree/node_cache.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE_NODE_CACHE_HEADER
#define STXXL_CONTAINERS_BTREE_NODE_CACHE_HEADER

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tlx/logger.hpp>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/request.hpp>
#include <foxxll/mng/block_manager.hpp>
#include <foxxll/mng/typed_block.hpp>

#include <stxxl/bits/config.h>
#include <stxxl/bits/containers/pager.h>

namespace stxxl {

// TODO:  speedup BID2node_ access using search result iterator in the methods

namespace btree {

template <class NodeType, class BTreeType>
class node_cache
{
    static constexpr bool debug = false;

public:
    using btree_type = BTreeType;
    using node_type = NodeType;
    using block_type = typename node_type::block_type;
    using bid_type = typename node_type::bid_type;
    using key_compare = typename btree_type::key_compare;

    using alloc_strategy_type = typename btree_type::alloc_strategy_type;
    using pager_type = stxxl::lru_pager<>;

private:
    btree_type* m_btree;
    key_compare m_cmp;

    struct bid_hash
    {
        size_t operator () (const bid_type& bid) const
        {
            size_t result =
                foxxll::longhash1(bid.offset + reinterpret_cast<intptr_t>(bid.storage));
            return result;
        }
#if STXXL_MSVC
        bool operator () (const bid_type& a, const bid_type& b) const
        {
            return (a.storage < b.storage) || (a.storage == b.storage && a.offset < b.offset);
        }
        enum
        {                                       // parameters for hash table
            bucket_size = 4,                    // 0 < bucket_size
            min_buckets = 8                     // min_buckets = 2 ^^ N, 0 < N
        };
#endif
    };

    std::vector<node_type*> m_nodes;
    std::vector<foxxll::request_ptr> m_reqs;
    std::vector<bool> m_fixed;
    std::vector<bool> m_dirty;
    std::vector<size_t> m_free_nodes;
    using bid2node_type = std::unordered_map<bid_type, size_t, bid_hash>;

    bid2node_type m_bid2node;
    pager_type m_pager;
    foxxll::block_manager* m_bm;
    alloc_strategy_type m_alloc_strategy;

    uint64_t n_found { 0 };
    uint64_t n_not_found { 0 };
    uint64_t n_created { 0 };
    uint64_t n_deleted { 0 };
    uint64_t n_read { 0 };
    uint64_t n_written { 0 };
    uint64_t n_clean_forced { 0 };

    // changes btree pointer in all contained iterators
    void change_btree_pointers(btree_type* b)
    {
        for (typename std::vector<node_type*>::const_iterator it = m_nodes.begin();
             it != m_nodes.end(); ++it)
        {
            (*it)->m_btree = b;
        }
    }

public:
    node_cache(const size_t cache_size_in_bytes,
               btree_type* btree,
               key_compare cmp)
        : m_btree(btree),
          m_cmp(cmp),
          m_bm(foxxll::block_manager::get_instance())
    {
        const size_t nnodes = cache_size_in_bytes / block_type::raw_size;
        LOG << "btree::node_cache constructor nodes=" << nnodes;
        if (nnodes < 3)
        {
            FOXXLL_THROW2(std::runtime_error, "btree::node_cache::node_cache", "Too few memory for a node cache (<3)");
        }
        m_nodes.reserve(nnodes);
        m_reqs.resize(nnodes);
        m_free_nodes.reserve(nnodes);
        m_fixed.resize(nnodes, false);
        m_dirty.resize(nnodes, true);
        for (size_t i = 0; i < nnodes; ++i)
        {
            m_nodes.push_back(new node_type(m_btree, m_cmp));
            m_free_nodes.push_back(i);
        }

        pager_type tmp_pager(nnodes);
        std::swap(m_pager, tmp_pager);
    }

    //! non-copyable: delete copy-constructor
    node_cache(const node_cache&) = delete;
    //! non-copyable: delete assignment operator
    node_cache& operator = (const node_cache&) = delete;

    size_t size() const
    {
        return m_nodes.size();
    }

    // returns the number of fixed pages
    size_t nfixed() const
    {
        return std::count_if(m_bid2node.cbegin(), m_bid2node.cend(),
                             [&](const auto& it) { return m_fixed[it.second]; });
    }

    ~node_cache()
    {
        LOG << "btree::node_cache destructor addr=" << this;

        for (const auto& it : m_bid2node)
        {
            const size_t& p = it.second;
            if (m_reqs[p].valid())
                m_reqs[p]->wait();

            if (m_dirty[p])
                m_nodes[p]->save();
        }

        for (size_t i = 0; i < size(); ++i)
            delete m_nodes[i];
    }

    node_type * get_new_node(bid_type& new_bid)
    {
        ++n_created;

        if (m_free_nodes.empty())
        {
            // need to kick a node
            size_t node2kick;
            {
                size_t i = 0;
                const size_t max_tries = size() + 1;
                do {
                    ++i;
                    node2kick = m_pager.kick();
                    if (i == max_tries) {
                        LOG1 <<
                            "The node cache is too small, no node can be kicked out (all nodes are fixed) !\n"
                            "Returning nullptr node.";
                        return nullptr;
                    }
                    m_pager.hit(node2kick);
                } while (m_fixed[node2kick]);
            }
            if (m_reqs[node2kick].valid())
                m_reqs[node2kick]->wait();

            node_type& node = *(m_nodes[node2kick]);

            if (m_dirty[node2kick])
            {
                node.save();
                ++n_written;
            }
            else
                ++n_clean_forced;

            //reqs_[node2kick] = request_ptr(); // reset request

            assert(m_bid2node.find(node.my_bid()) != m_bid2node.end());
            m_bid2node.erase(node.my_bid());
            m_bm->new_block(m_alloc_strategy, new_bid);

            m_bid2node[new_bid] = node2kick;

            node.init(new_bid);

            m_dirty[node2kick] = true;

            assert(size() == m_bid2node.size() + m_free_nodes.size());

            LOG << "btree::node_cache get_new_node, need to kick node " << node2kick;

            return &node;
        }

        const size_t free_node = m_free_nodes.back();
        m_free_nodes.pop_back();
        assert(m_fixed[free_node] == false);

        m_bm->new_block(m_alloc_strategy, new_bid);
        m_bid2node[new_bid] = free_node;
        node_type& node = *(m_nodes[free_node]);
        node.init(new_bid);

        // assert(!(reqs_[free_node].valid()));

        m_pager.hit(free_node);

        m_dirty[free_node] = true;

        assert(size() == m_bid2node.size() + m_free_nodes.size());

        LOG << "btree::node_cache get_new_node, free node " << free_node << "available";

        return &node;
    }

    node_type * get_node(const bid_type& bid, bool fix = false)
    {
        typename bid2node_type::const_iterator it = m_bid2node.find(bid);
        ++n_read;

        if (it != m_bid2node.end())
        {
            // the node is in cache
            const size_t nodeindex = it->second;
            LOG << "btree::node_cache get_node, the node " << nodeindex << "is in cache , fix=" << fix;
            m_fixed[nodeindex] = fix;
            m_pager.hit(nodeindex);
            m_dirty[nodeindex] = true;

            if (m_reqs[nodeindex].valid() && !m_reqs[nodeindex]->poll())
                m_reqs[nodeindex]->wait();

            ++n_found;
            return m_nodes[nodeindex];
        }

        ++n_not_found;

        // the node is not in cache
        if (m_free_nodes.empty())
        {
            // need to kick a node
            size_t node2kick;
            {
                size_t i = 0;
                const size_t max_tries = size() + 1;
                do {
                    ++i;
                    node2kick = m_pager.kick();
                    if (i == max_tries) {
                        LOG1 <<
                            "The node cache is too small, no node can be kicked out (all nodes are fixed) !\n"
                            "Returning nullptr node.";
                        return nullptr;
                    }
                    m_pager.hit(node2kick);
                } while (m_fixed[node2kick]);
            }

            if (m_reqs[node2kick].valid())
                m_reqs[node2kick]->wait();

            node_type& node = *(m_nodes[node2kick]);

            if (m_dirty[node2kick])
            {
                node.save();
                ++n_written;
            }
            else
                ++n_clean_forced;

            m_bid2node.erase(node.my_bid());

            m_reqs[node2kick] = node.load(bid);
            m_bid2node[bid] = node2kick;

            m_fixed[node2kick] = fix;

            m_dirty[node2kick] = true;

            assert(size() == m_bid2node.size() + m_free_nodes.size());

            LOG << "btree::node_cache get_node, need to kick node" << node2kick << " fix=" << fix;

            return &node;
        }

        const size_t free_node = m_free_nodes.back();
        m_free_nodes.pop_back();
        assert(m_fixed[free_node] == false);

        node_type& node = *(m_nodes[free_node]);
        m_reqs[free_node] = node.load(bid);
        m_bid2node[bid] = free_node;

        m_pager.hit(free_node);

        m_fixed[free_node] = fix;

        m_dirty[free_node] = true;

        assert(size() == m_bid2node.size() + m_free_nodes.size());

        LOG << "btree::node_cache get_node, free node " << free_node << "available, fix=" << fix;

        return &node;
    }

    node_type const * get_const_node(const bid_type& bid, bool fix = false)
    {
        typename bid2node_type::const_iterator it = m_bid2node.find(bid);
        ++n_read;

        if (it != m_bid2node.end())
        {
            // the node is in cache
            const size_t nodeindex = it->second;
            LOG << "btree::node_cache get_node, the node " << nodeindex << "is in cache , fix=" << fix;
            m_fixed[nodeindex] = fix;
            m_pager.hit(nodeindex);

            if (m_reqs[nodeindex].valid() && !m_reqs[nodeindex]->poll())
                m_reqs[nodeindex]->wait();

            ++n_found;
            return m_nodes[nodeindex];
        }

        ++n_not_found;

        // the node is not in cache
        if (m_free_nodes.empty())
        {
            // need to kick a node
            size_t node2kick;
            size_t i = 0;
            const size_t max_tries = size() + 1;
            do
            {
                ++i;
                node2kick = m_pager.kick();
                if (i == max_tries)
                {
                    LOG1 <<
                        "The node cache is too small, no node can be kicked out (all nodes are fixed) !\n"
                        "Returning nullptr node.";
                    return nullptr;
                }
                m_pager.hit(node2kick);
            } while (m_fixed[node2kick]);

            if (m_reqs[node2kick].valid())
                m_reqs[node2kick]->wait();

            node_type& node = *(m_nodes[node2kick]);
            if (m_dirty[node2kick])
            {
                node.save();
                ++n_written;
            }
            else
                ++n_clean_forced;

            m_bid2node.erase(node.my_bid());

            m_reqs[node2kick] = node.load(bid);
            m_bid2node[bid] = node2kick;

            m_fixed[node2kick] = fix;

            m_dirty[node2kick] = false;

            assert(size() == m_bid2node.size() + m_free_nodes.size());

            LOG << "btree::node_cache get_node, need to kick node" << node2kick << " fix=" << fix;

            return &node;
        }

        const size_t free_node = m_free_nodes.back();
        m_free_nodes.pop_back();
        assert(m_fixed[free_node] == false);

        node_type& node = *(m_nodes[free_node]);
        m_reqs[free_node] = node.load(bid);
        m_bid2node[bid] = free_node;

        m_pager.hit(free_node);

        m_fixed[free_node] = fix;

        m_dirty[free_node] = false;

        assert(size() == m_bid2node.size() + m_free_nodes.size());

        LOG << "btree::node_cache get_node, free node " << free_node << "available, fix=" << fix;

        return &node;
    }

    void delete_node(const bid_type& bid)
    {
        typename bid2node_type::const_iterator it = m_bid2node.find(bid);
        try
        {
            if (it != m_bid2node.end())
            {
                // the node is in the cache
                const size_t nodeindex = it->second;
                LOG << "btree::node_cache delete_node " << nodeindex << " from cache.";
                if (m_reqs[nodeindex].valid())
                    m_reqs[nodeindex]->wait();

                //reqs_[nodeindex] = request_ptr(); // reset request
                m_free_nodes.push_back(nodeindex);
                m_bid2node.erase(bid);
                m_fixed[nodeindex] = false;
            }
            ++n_deleted;
        } catch (const foxxll::io_error& ex)
        {
            m_bm->delete_block(bid);
            throw foxxll::io_error(ex.what());
        }
        m_bm->delete_block(bid);
    }

    void prefetch_node(const bid_type& bid)
    {
        if (m_bid2node.find(bid) != m_bid2node.end())
            return;

        // the node is not in cache
        if (m_free_nodes.empty())
        {
            // need to kick a node
            size_t node2kick;
            size_t i = 0;
            const size_t max_tries = size() + 1;
            do
            {
                ++i;
                node2kick = m_pager.kick();
                if (i == max_tries)
                {
                    LOG1 <<
                        "The node cache is too small, no node can be kicked out (all nodes are fixed) !\n"
                        "Returning nullptr node.";
                    return;
                }
                m_pager.hit(node2kick);
            } while (m_fixed[node2kick]);

            if (m_reqs[node2kick].valid())
                m_reqs[node2kick]->wait();

            node_type& node = *(m_nodes[node2kick]);

            if (m_dirty[node2kick])
            {
                node.save();
                ++n_written;
            }
            else
                ++n_clean_forced;

            m_bid2node.erase(node.my_bid());

            m_reqs[node2kick] = node.prefetch(bid);
            m_bid2node[bid] = node2kick;

            m_fixed[node2kick] = false;

            m_dirty[node2kick] = false;

            assert(size() == m_bid2node.size() + m_free_nodes.size());

            LOG << "btree::node_cache prefetch_node, need to kick node" << node2kick << " ";

            return;
        }

        const size_t free_node = m_free_nodes.back();
        m_free_nodes.pop_back();
        assert(m_fixed[free_node] == false);

        node_type& node = *(m_nodes[free_node]);
        m_reqs[free_node] = node.prefetch(bid);
        m_bid2node[bid] = free_node;

        m_pager.hit(free_node);

        m_fixed[free_node] = false;

        m_dirty[free_node] = false;

        assert(size() == m_bid2node.size() + m_free_nodes.size());

        LOG << "btree::node_cache prefetch_node, free node " << free_node << "available";

        return;
    }

    void unfix_node(const bid_type& bid)
    {
        assert(m_bid2node.find(bid) != m_bid2node.end());
        m_fixed[m_bid2node[bid]] = false;
        LOG << "btree::node_cache unfix_node,  node " << m_bid2node[bid];
    }

    void swap(node_cache& obj)
    {
        std::swap(m_cmp, obj.m_cmp);
        std::swap(m_nodes, obj.m_nodes);
        std::swap(m_reqs, obj.m_reqs);
        change_btree_pointers(m_btree);
        obj.change_btree_pointers(obj.m_btree);
        std::swap(m_fixed, obj.m_fixed);
        std::swap(m_free_nodes, obj.m_free_nodes);
        std::swap(m_bid2node, obj.m_bid2node);
        std::swap(m_pager, obj.m_pager);
        std::swap(m_alloc_strategy, obj.m_alloc_strategy);
        std::swap(n_found, obj.n_found);
        std::swap(n_not_found, obj.n_found);
        std::swap(n_created, obj.n_created);
        std::swap(n_deleted, obj.n_deleted);
        std::swap(n_read, obj.n_read);
        std::swap(n_written, obj.n_written);
        std::swap(n_clean_forced, obj.n_clean_forced);
    }

    void print_statistics(std::ostream& o) const
    {
        if (n_read)
            o << "Found blocks                      : " << n_found << " (" <<
                100. * double(n_found) / double(n_read) << "%)" << std::endl;
        else
            o << "Found blocks                      : " << n_found << " (" <<
                100 << "%)" << std::endl;

        o << "Not found blocks                  : " << n_not_found << std::endl;
        o << "Created in the cache blocks       : " << n_created << std::endl;
        o << "Deleted blocks                    : " << n_deleted << std::endl;
        o << "Read blocks                       : " << n_read << std::endl;
        o << "Written blocks                    : " << n_written << std::endl;
        o << "Clean blocks forced from the cache: " << n_clean_forced << std::endl;
    }
    void reset_statistics()
    {
        n_found = 0;
        n_not_found = 0;
        n_created = 0;
        n_deleted = 0;
        n_read = 0;
        n_written = 0;
        n_clean_forced = 0;
    }
};

} // namespace btree
} // namespace stxxl

namespace std {

template <class NodeType, class BTreeType>
void swap(stxxl::btree::node_cache<NodeType, BTreeType>& a,
          stxxl::btree::node_cache<NodeType, BTreeType>& b)
{
    a.swap(b);
}

} // namespace std

#endif // !STXXL_CONTAINERS_BTREE_NODE_CACHE_HEADER
