/***************************************************************************
 *            btree.h
 *
 *  Tue Feb 14 19:02:35 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE__BTREE_H
#define STXXL_CONTAINERS_BTREE__BTREE_H

#include "stxxl/bits/namespace.h"
#include "stxxl/bits/containers/btree/iterator.h"
#include "stxxl/bits/containers/btree/iterator_map.h"
#include "stxxl/bits/containers/btree/leaf.h"
#include "stxxl/bits/containers/btree/node_cache.h"
#include "stxxl/bits/containers/btree/root_node.h"
#include "stxxl/bits/containers/btree/node.h"
#include "stxxl/vector"


__STXXL_BEGIN_NAMESPACE

namespace btree
{
    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned RawNodeSize,
              unsigned RawLeafSize,
              class PDAllocStrategy
    >
    class btree
    {
    public:
        typedef KeyType key_type;
        typedef DataType data_type;
        typedef CompareType key_compare;

        typedef btree<KeyType, DataType, CompareType, RawNodeSize, RawLeafSize, PDAllocStrategy> SelfType;

        typedef PDAllocStrategy alloc_strategy_type;

        typedef stxxl::uint64 size_type;
        typedef stxxl::int64 difference_type;
        typedef std::pair < const key_type, data_type > value_type;
        typedef value_type & reference;
        typedef const value_type & const_reference;
        typedef value_type * pointer;
        typedef value_type const * const_pointer;


        // leaf type declarations
        typedef normal_leaf<key_type, data_type, key_compare, RawLeafSize, SelfType> leaf_type;
        friend class normal_leaf<key_type, data_type, key_compare, RawLeafSize, SelfType>;
        typedef typename leaf_type::block_type leaf_block_type;
        typedef typename leaf_type::bid_type leaf_bid_type;
        typedef node_cache<leaf_type, SelfType> leaf_cache_type;
        friend class node_cache<leaf_type, SelfType>;
        // iterator types
        typedef btree_iterator<SelfType> iterator;
        typedef btree_const_iterator<SelfType> const_iterator;
        friend class btree_iterator_base<SelfType>;
        // iterator map type
        typedef iterator_map<SelfType> iterator_map_type;
        // node type declarations
        typedef normal_node<key_type, key_compare, RawNodeSize, SelfType> node_type;
        typedef typename node_type::block_type node_block_type;
        friend class normal_node<key_type, key_compare, RawNodeSize, SelfType>;
        typedef typename node_type::bid_type node_bid_type;
        typedef node_cache<node_type, SelfType> node_cache_type;
        friend class node_cache<node_type, SelfType>;

        typedef typename leaf_type::value_compare value_compare;

        enum {
            min_node_size = node_type::min_size,
            max_node_size = node_type::max_size,
            min_leaf_size = leaf_type::min_size,
            max_leaf_size = leaf_type::max_size
        };

    private:

        key_compare key_compare_;
        mutable node_cache_type node_cache_;
        mutable leaf_cache_type leaf_cache_;
        iterator_map_type iterator_map_;
        size_type size_;
        unsigned_type height_;
        bool prefetching_enabled_;
        block_manager * bm_;
        alloc_strategy_type alloc_strategy_;

        typedef std::map<key_type, node_bid_type, key_compare> root_node_type;
        typedef typename root_node_type::iterator root_node_iterator_type;
        typedef typename root_node_type::const_iterator root_node_const_iterator_type;
        typedef std::pair<key_type, node_bid_type> root_node_pair_type;


        root_node_type root_node_;
        iterator end_iterator;


        btree();         // forbidden
        btree(const btree &);         // forbidden
        btree & operator = (const btree &);         // forbidden

        template <class BIDType>
        void insert_into_root(const std::pair<key_type, BIDType> & splitter)
        {
            std::pair<root_node_iterator_type, bool> result =
                root_node_.insert(splitter);
            assert(result.second == true);
            if (root_node_.size() > max_node_size)            // root overflow
            {
                STXXL_VERBOSE1("btree::insert_into_root, overlow happened, splitting");

                node_bid_type LeftBid;
                node_type * LeftNode = node_cache_.get_new_node(LeftBid);
                assert(LeftNode);
                node_bid_type RightBid;
                node_type * RightNode = node_cache_.get_new_node(RightBid);
                assert(RightNode);

                const unsigned_type old_size = root_node_.size();
                const unsigned_type half = root_node_.size() / 2;
                unsigned_type i = 0;
                root_node_iterator_type it = root_node_.begin();
                typename node_block_type::iterator block_it = LeftNode->block().begin();
                while (i < half)              // copy smaller part
                {
                    *block_it = *it;
                    ++i;
                    ++block_it;
                    ++it;
                }
                LeftNode->block().info.cur_size = half;
                key_type LeftKey = (LeftNode->block()[half - 1]).first;

                block_it = RightNode->block().begin();
                while (i < old_size)              // copy larger part
                {
                    *block_it = *it;
                    ++i;
                    ++block_it;
                    ++it;
                }
                unsigned_type right_size = RightNode->block().info.cur_size = old_size - half;
                key_type RightKey = (RightNode->block()[right_size - 1]).first;

                assert(old_size == RightNode->size() + LeftNode->size());

                // create new root node
                root_node_.clear();
                root_node_.insert(root_node_pair_type(LeftKey, LeftBid));
                root_node_.insert(root_node_pair_type(RightKey, RightBid));


                ++height_;
                STXXL_VERBOSE1("btree Increasing height to " << height_);
                if (node_cache_.size() < (height_ - 1))
                {
                    STXXL_FORMAT_ERROR_MSG(msg, "btree::bulk_construction The height of the tree (" << height_ << ") has exceeded the required capacity ("
                                                                                                    << (node_cache_.size() + 1) << ") of the node cache. " <<
                                           "Increase the node cache size.")
                    throw std::runtime_error(msg.str());
                }
            }
        }

        template <class CacheType>
        void fuse_or_balance(root_node_iterator_type UIt, CacheType & cache_)
        {
            typedef typename CacheType::node_type local_node_type;
            typedef typename local_node_type::bid_type local_bid_type;

            root_node_iterator_type leftIt, rightIt;
            if (UIt->first == key_compare::max_value())            // UIt is the last entry in the root
            {
                assert(UIt != root_node_.begin());
                rightIt = UIt;
                leftIt = --UIt;
            }
            else
            {
                leftIt = UIt;
                rightIt = ++UIt;
                assert(rightIt != root_node_.end());
            }

            // now fuse or balance nodes pointed by leftIt and rightIt
            local_bid_type LeftBid = (local_bid_type) leftIt->second;
            local_bid_type RightBid = (local_bid_type) rightIt->second;
            local_node_type * LeftNode = cache_.get_node(LeftBid, true);
            local_node_type * RightNode = cache_.get_node(RightBid, true);

            const unsigned_type TotalSize = LeftNode->size() + RightNode->size();
            if (TotalSize <= RightNode->max_nelements())
            {
                // fuse
                RightNode->fuse(*LeftNode);                 // add the content of LeftNode to RightNode

                cache_.unfix_node(RightBid);
                cache_.delete_node(LeftBid);                 // 'delete_node' unfixes LeftBid also

                root_node_.erase(leftIt);                 // delete left BID from the root
            }
            else
            {
                // balance

                key_type NewSplitter = RightNode->balance(*LeftNode);

                root_node_.erase(leftIt);                 // delete left BID from the root
                // reinsert with the new key
                root_node_.insert(root_node_pair_type(NewSplitter, (node_bid_type)LeftBid));

                cache_.unfix_node(LeftBid);
                cache_.unfix_node(RightBid);
            }
        }

        void create_empty_leaf()
        {
            leaf_bid_type NewBid;
            leaf_type * NewLeaf = leaf_cache_.get_new_node(NewBid);
            assert(NewLeaf);
            end_iterator = NewLeaf->end();             // initialize end() iterator
            root_node_.insert(root_node_pair_type(key_compare::max_value(), (node_bid_type)NewBid));
        }

        void deallocate_children()
        {
            if (height_ == 2)
            {
                // we have children leaves here
                root_node_const_iterator_type it = root_node_.begin();
                for ( ; it != root_node_.end(); ++it)
                {
                    // delete from leaf cache and deallocate bid
                    leaf_cache_.delete_node((leaf_bid_type)it->second);
                }
            }
            else
            {
                root_node_const_iterator_type it = root_node_.begin();
                for ( ; it != root_node_.end(); ++it)
                {
                    node_type * Node = node_cache_.get_node((node_bid_type)it->second);
                    assert(Node);
                    Node->deallocate_children(height_ - 1);
                    // delete from node cache and deallocate bid
                    node_cache_.delete_node((node_bid_type)it->second);
                }
            }
        }

        template <class InputIterator>
        void bulk_construction(InputIterator b, InputIterator e, double node_fill_factor, double leaf_fill_factor)
        {
            assert(node_fill_factor >= 0.5);
            assert(leaf_fill_factor >= 0.5);
            key_type lastKey = key_compare::max_value();

            typedef std::pair<key_type, node_bid_type> key_bid_pair;
            typedef typename stxxl::VECTOR_GENERATOR < key_bid_pair, 1, 1,
            node_block_type::raw_size > ::result key_bid_vector_type;

            key_bid_vector_type Bids;

            leaf_bid_type NewBid;
            leaf_type * Leaf = leaf_cache_.get_new_node(NewBid);
            const unsigned_type max_leaf_elements = unsigned_type(double (Leaf->max_nelements()) * leaf_fill_factor);

            while (b != e)
            {
                // write data in leaves

                // if *b not equal to the last element
                if (key_compare_(b->first, lastKey) || key_compare_(lastKey, b->first))
                {
                    ++size_;
                    if (Leaf->size() == max_leaf_elements)
                    {
                        // overflow, need a new block
                        Bids.push_back(key_bid_pair(Leaf->back().first, (node_bid_type)NewBid));

                        leaf_type * NewLeaf = leaf_cache_.get_new_node(NewBid);
                        assert(NewLeaf);
                        // Setting links
                        Leaf->succ() = NewLeaf->my_bid();
                        NewLeaf->pred() = Leaf->my_bid();

                        Leaf = NewLeaf;
                    }
                    Leaf->push_back(*b);
                    lastKey = b->first;
                }
                ++b;
            }

            // balance the last leaf
            if (Leaf->underflows() && !Bids.empty())
            {
                leaf_type * LeftLeaf = leaf_cache_.get_node((leaf_bid_type)(Bids.back().second));
                assert(LeftLeaf);
                const key_type NewSplitter = Leaf->balance(*LeftLeaf);
                Bids.back().first = NewSplitter;
                assert(!LeftLeaf->overflows() && !LeftLeaf->underflows());
            }

            assert(!Leaf->overflows() && (!Leaf->underflows() || size_ <= max_leaf_size));

            end_iterator = Leaf->end();             // initialize end() iterator

            Bids.push_back(key_bid_pair(key_compare::max_value(), (node_bid_type)NewBid));

            const unsigned_type max_node_elements = unsigned_type(double (max_node_size) * node_fill_factor);

            while (Bids.size() > max_node_elements)
            {
                key_bid_vector_type ParentBids;

                stxxl::uint64 nparents = div_and_round_up( Bids.size(), stxxl::uint64(max_node_elements));
                assert(nparents >= 2);
                STXXL_VERBOSE1("btree bulk constructBids.size() " << Bids.size() << " nparents: " << nparents << " max_ns: "
                                                                  << max_node_elements);
                typename key_bid_vector_type::const_iterator it = Bids.begin();

                do
                {
                    node_bid_type NewBid;
                    node_type * Node = node_cache_.get_new_node(NewBid);
                    assert(Node);
                    unsigned_type cnt = 0;
                    for ( ; cnt < max_node_elements && it != Bids.end(); ++cnt, ++it)
                    {
                        Node->push_back(*it);
                    }
                    STXXL_VERBOSE1("btree bulk construct Node size : " << Node->size() << " limits: " <<
                                   Node->min_nelements() << " " << Node->max_nelements() << " max_node_elements: " << max_node_elements);

                    if (Node->underflows())
                    {
                        assert(it == Bids.end());                       // this can happen only at the end
                        assert(!ParentBids.empty());
                        // TODO
                        node_type * LeftNode = node_cache_.get_node(ParentBids.back().second);
                        assert(LeftNode);
                        const key_type NewSplitter = Node->balance(*LeftNode);
                        ParentBids.back().first = NewSplitter;
                        assert(!LeftNode->overflows() && !LeftNode->underflows());
                    }
                    assert(!Node->overflows() && !Node->underflows());

                    ParentBids.push_back(key_bid_pair(Node->back().first, NewBid));
                } while (it != Bids.end());

                std::swap(ParentBids, Bids);

                assert(nparents == Bids.size());

                ++height_;
                STXXL_VERBOSE1("Increasing height to " << height_);
                if (node_cache_.size() < (height_ - 1))
                {
                    STXXL_FORMAT_ERROR_MSG(msg, "btree::bulk_construction The height of the tree (" << height_ << ") has exceeded the required capacity ("
                                                                                                    << (node_cache_.size() + 1) << ") of the node cache. " <<
                                           "Increase the node cache size.")

                    throw std::runtime_error(msg.str());
                }
            }

            root_node_.insert(Bids.begin(), Bids.end());
        }

    public:
        btree(  unsigned_type node_cache_size_in_bytes,
                unsigned_type leaf_cache_size_in_bytes
        ) :
            node_cache_(node_cache_size_in_bytes, this, key_compare_),
            leaf_cache_(leaf_cache_size_in_bytes, this, key_compare_),
            iterator_map_(this),
            size_(0),
            height_(2),
            prefetching_enabled_(true),
            bm_(block_manager::get_instance())
        {
            STXXL_VERBOSE1("Creating a btree, addr=" << this);
            STXXL_VERBOSE1(" bytes in a node: " << node_bid_type::size);
            STXXL_VERBOSE1(" bytes in a leaf: " << leaf_bid_type::size);
            STXXL_VERBOSE1(" elements in a node: " << node_block_type::size);
            STXXL_VERBOSE1(" elements in a leaf: " << leaf_block_type::size);
            STXXL_VERBOSE1(" size of a node element: " << sizeof(typename node_block_type::value_type));
            STXXL_VERBOSE1(" size of a leaf element: " << sizeof(typename leaf_block_type::value_type));


            create_empty_leaf();
        }

        btree(  const key_compare & c_,
                unsigned_type node_cache_size_in_bytes,
                unsigned_type leaf_cache_size_in_bytes
        ) :
            key_compare_(c_),
            node_cache_(node_cache_size_in_bytes, this, key_compare_),
            leaf_cache_(leaf_cache_size_in_bytes, this, key_compare_),
            iterator_map_(this),
            size_(0),
            height_(2),
            prefetching_enabled_(true),
            bm_(block_manager::get_instance())
        {
            STXXL_VERBOSE1("Creating a btree, addr=" << this);
            STXXL_VERBOSE1(" bytes in a node: " << node_bid_type::size);
            STXXL_VERBOSE1(" bytes in a leaf: " << leaf_bid_type::size);

            create_empty_leaf();
        }

        virtual ~btree()
        {
            try
            {
                deallocate_children();
            } catch (...)
            {
                // no exceptions in deconstructor
            }
        }

        size_type size() const
        {
            return size_;
        }

        size_type max_size() const
        {
            return (std::numeric_limits < size_type > ::max)();
        }

        bool empty() const
        {
            return !size_;
        }

        std::pair<iterator, bool> insert(const value_type & x)
        {
            root_node_iterator_type it = root_node_.lower_bound(x.first);
            assert(!root_node_.empty());
            assert(it != root_node_.end());
            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("Inserting new value into a leaf");
                leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second, true);
                assert(Leaf);
                std::pair<key_type, leaf_bid_type> Splitter;
                std::pair<iterator, bool> result = Leaf->insert(x, Splitter);
                if (result.second) ++size_;

                leaf_cache_.unfix_node((leaf_bid_type)it->second);
                //if(key_compare::max_value() == Splitter.first)
                if (!(key_compare_(key_compare::max_value(), Splitter.first) ||
                      key_compare_(Splitter.first, key_compare::max_value()) ))
                    return result;
                // no overflow/splitting happened

                STXXL_VERBOSE1("Inserting new value into root node");

                insert_into_root(Splitter);

                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return result;
            }

            // 'it' points to a node
            STXXL_VERBOSE1("Inserting new value into a node");
            node_type * Node = node_cache_.get_node((node_bid_type)it->second, true);
            assert(Node);
            std::pair<key_type, node_bid_type> Splitter;
            std::pair<iterator, bool> result = Node->insert(x, height_ - 1, Splitter);
            if (result.second) ++size_;

            node_cache_.unfix_node((node_bid_type)it->second);
            //if(key_compare::max_value() == Splitter.first)
            if (!(key_compare_(key_compare::max_value(), Splitter.first) ||
                  key_compare_(Splitter.first, key_compare::max_value()) ))
                return result;
            // no overflow/splitting happened

            STXXL_VERBOSE1("Inserting new value into root node");

            insert_into_root(Splitter);

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);

            return result;
        }

        iterator begin()
        {
            root_node_iterator_type it = root_node_.begin();
            assert(it != root_node_.end() );

            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("btree: retrieveing begin() from the first leaf");
                leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second);
                assert(Leaf);

                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return Leaf->begin();
            }

            // 'it' points to a node
            STXXL_VERBOSE1("btree: retrieveing begin() from the first node");
            node_type * Node = node_cache_.get_node((node_bid_type)it->second, true);
            assert(Node);
            iterator result = Node->begin(height_ - 1);
            node_cache_.unfix_node((node_bid_type)it->second);

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);

            return result;
        }

        const_iterator begin() const
        {
            root_node_const_iterator_type it = root_node_.begin();
            assert(it != root_node_.end() );

            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("btree: retrieveing begin() from the first leaf");
                leaf_type const * Leaf = leaf_cache_.get_const_node((leaf_bid_type)it->second);
                assert(Leaf);
                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return Leaf->begin();
            }

            // 'it' points to a node
            STXXL_VERBOSE1("btree: retrieveing begin() from the first node");
            node_type const * Node = node_cache_.get_const_node((node_bid_type)it->second, true);
            assert(Node);
            const_iterator result = Node->begin(height_ - 1);
            node_cache_.unfix_node((node_bid_type)it->second);
            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            return result;
        }

        iterator end()
        {
            return end_iterator;
        }

        const_iterator end() const
        {
            return end_iterator;
        }

        data_type & operator []  (const key_type & k)
        {
            return (*((insert(value_type(k, data_type()))).first)).second;
        }

        iterator find(const key_type & k)
        {
            root_node_iterator_type it = root_node_.lower_bound(k);
            assert(it != root_node_.end());

            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("Searching in a leaf");
                leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second, true);
                assert(Leaf);
                iterator result = Leaf->find(k);
                leaf_cache_.unfix_node((leaf_bid_type)it->second);
                assert(result == end() || result->first == k);
                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return result;
            }

            // 'it' points to a node
            STXXL_VERBOSE1("Searching in a node");
            node_type * Node = node_cache_.get_node((node_bid_type)it->second, true);
            assert(Node);
            iterator result = Node->find(k, height_ - 1);
            node_cache_.unfix_node((node_bid_type)it->second);

            assert(result == end() || result->first == k);
            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            return result;
        }

        const_iterator find(const key_type & k) const
        {
            root_node_const_iterator_type it = root_node_.lower_bound(k);
            assert(it != root_node_.end());

            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("Searching in a leaf");
                leaf_type const * Leaf = leaf_cache_.get_const_node((leaf_bid_type)it->second, true);
                assert(Leaf);
                const_iterator result = Leaf->find(k);
                leaf_cache_.unfix_node((leaf_bid_type)it->second);
                assert(result == end() || result->first == k);
                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return result;
            }

            // 'it' points to a node
            STXXL_VERBOSE1("Searching in a node");
            node_type const * Node = node_cache_.get_const_node((node_bid_type)it->second, true);
            assert(Node);
            const_iterator result = Node->find(k, height_ - 1);
            node_cache_.unfix_node((node_bid_type)it->second);

            assert(result == end() || result->first == k);
            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            return result;
        }

        iterator lower_bound(const key_type & k)
        {
            root_node_iterator_type it = root_node_.lower_bound(k);
            assert(it != root_node_.end());

            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("Searching lower bound in a leaf");
                leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second, true);
                assert(Leaf);
                iterator result = Leaf->lower_bound(k);
                leaf_cache_.unfix_node((leaf_bid_type)it->second);
                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return result;
            }

            // 'it' points to a node
            STXXL_VERBOSE1("Searching lower bound in a node");
            node_type * Node = node_cache_.get_node((node_bid_type)it->second, true);
            assert(Node);
            iterator result = Node->lower_bound(k, height_ - 1);
            node_cache_.unfix_node((node_bid_type)it->second);

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            return result;
        }

        const_iterator lower_bound(const key_type & k) const
        {
            root_node_const_iterator_type it = root_node_.lower_bound(k);
            assert(it != root_node_.end());

            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("Searching lower bound in a leaf");
                leaf_type const * Leaf = leaf_cache_.get_const_node((leaf_bid_type)it->second, true);
                assert(Leaf);
                const_iterator result = Leaf->lower_bound(k);
                leaf_cache_.unfix_node((leaf_bid_type)it->second);

                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return result;
            }

            // 'it' points to a node
            STXXL_VERBOSE1("Searching lower bound in a node");
            node_type const * Node = node_cache_.get_const_node((node_bid_type)it->second, true);
            assert(Node);
            const_iterator result = Node->lower_bound(k, height_ - 1);
            node_cache_.unfix_node((node_bid_type)it->second);

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            return result;
        }

        iterator upper_bound(const key_type & k)
        {
            root_node_iterator_type it = root_node_.upper_bound(k);
            assert(it != root_node_.end());

            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("Searching upper bound in a leaf");
                leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second, true);
                assert(Leaf);
                iterator result = Leaf->upper_bound(k);
                leaf_cache_.unfix_node((leaf_bid_type)it->second);

                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return result;
            }

            // 'it' points to a node
            STXXL_VERBOSE1("Searching upper bound in a node");
            node_type * Node = node_cache_.get_node((node_bid_type)it->second, true);
            assert(Node);
            iterator result = Node->upper_bound(k, height_ - 1);
            node_cache_.unfix_node((node_bid_type)it->second);

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            return result;
        }

        const_iterator upper_bound(const key_type & k) const
        {
            root_node_const_iterator_type it = root_node_.upper_bound(k);
            assert(it != root_node_.end());

            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("Searching upper bound in a leaf");
                leaf_type const * Leaf = leaf_cache_.get_const_node((leaf_bid_type)it->second, true);
                assert(Leaf);
                const_iterator result = Leaf->upper_bound(k);
                leaf_cache_.unfix_node((leaf_bid_type)it->second);

                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return result;
            }

            // 'it' points to a node
            STXXL_VERBOSE1("Searching upper bound in a node");
            node_type const * Node = node_cache_.get_const_node((node_bid_type)it->second, true);
            assert(Node);
            const_iterator result = Node->upper_bound(k, height_ - 1);
            node_cache_.unfix_node((node_bid_type)it->second);

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            return result;
        }

        std::pair<iterator, iterator> equal_range(const key_type & k)
        {
            iterator l = lower_bound(k);             // l->first >= k

            if (l == end() || key_compare_(k, l->first))           // if (k < l->first)
                return std::pair<iterator, iterator>(l, l);
            // then upper_bound == lower_bound

            iterator u = l;
            ++u;             // only one element ==k can exist

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);

            return std::pair<iterator, iterator>(l, u);           // then upper_bound == (lower_bound+1)
        }

        std::pair<const_iterator, const_iterator> equal_range(const key_type & k) const
        {
            const_iterator l = lower_bound(k);             // l->first >= k

            if (l == end() || key_compare_(k, l->first))           // if (k < l->first)
                return std::pair<const_iterator, const_iterator>(l, l);
            // then upper_bound == lower_bound

            const_iterator u = l;
            ++u;             // only one element ==k can exist

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            return std::pair<const_iterator, const_iterator>(l, u);           // then upper_bound == (lower_bound+1)
        }

        size_type erase(const key_type & k)
        {
            root_node_iterator_type it = root_node_.lower_bound(k);
            assert(it != root_node_.end());
            if (height_ == 2)            // 'it' points to a leaf
            {
                STXXL_VERBOSE1("Deleting key from a leaf");
                leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second, true);
                assert(Leaf);
                size_type result = Leaf->erase(k);
                size_ -= result;
                leaf_cache_.unfix_node((leaf_bid_type)it->second);
                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);

                if ((!Leaf->underflows()) || root_node_.size() == 1)
                    return result;
                // no underflow or root has a special degree 1 (too few elements)

                STXXL_VERBOSE1("btree: Fusing or rebalancing a leaf");
                fuse_or_balance(it, leaf_cache_);

                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);

                return result;
            }

            // 'it' points to a node
            STXXL_VERBOSE1("Deleting key from a node");
            assert(root_node_.size() >= 2);
            node_type * Node = node_cache_.get_node((node_bid_type)it->second, true);
            assert(Node);
            size_type result = Node->erase(k, height_ - 1);
            size_ -= result;
            node_cache_.unfix_node((node_bid_type)it->second);
            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
            if (!Node->underflows())
                return result;
            // no underflow happened

            STXXL_VERBOSE1("Fusing or rebalancing a node");
            fuse_or_balance(it, node_cache_);

            if (root_node_.size() == 1)
            {
                STXXL_VERBOSE1("btree Root has size 1 and height > 2");
                STXXL_VERBOSE1("btree Deallocate root and decrease height");
                it = root_node_.begin();
                node_bid_type RootBid = it->second;
                assert(it->first == key_compare::max_value());
                node_type * RootNode = node_cache_.get_node(RootBid);
                assert(RootNode);
                assert(RootNode->back().first == key_compare::max_value());
                root_node_.clear();
                root_node_.insert(      RootNode->block().begin(),
                                        RootNode->block().begin() + RootNode->size());

                node_cache_.delete_node(RootBid);
                --height_;
                STXXL_VERBOSE1("btree Decreasing height to " << height_);
            }

            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);

            return result;
        }

        size_type count(const key_type & k)
        {
            if (find(k) == end())
                return 0;

            return 1;
        }

        void erase(iterator pos)
        {
            assert(pos != end());
#ifndef NDEBUG
            size_type old_size = size();
#endif

            erase(pos->first);

            assert(size() == old_size - 1);
        }

        iterator insert(iterator /*pos*/, const value_type & x)
        {
            return insert(x).first;             // pos ignored in the current version
        }

        void clear()
        {
            deallocate_children();

            root_node_.clear();

            size_ = 0;
            height_ = 2,

            create_empty_leaf();
            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
        }

        template <class InputIterator>
        void insert(InputIterator b, InputIterator e)
        {
            while (b != e)
            {
                insert(*(b++));
            }
        }

        template <class InputIterator>
        btree(  InputIterator b,
                InputIterator e,
                const key_compare & c_,
                unsigned_type node_cache_size_in_bytes,
                unsigned_type leaf_cache_size_in_bytes,
                bool range_sorted = false,
                double node_fill_factor = 0.75,
                double leaf_fill_factor = 0.6
        ) :
            key_compare_(c_),
            node_cache_(node_cache_size_in_bytes, this, key_compare_),
            leaf_cache_(leaf_cache_size_in_bytes, this, key_compare_),
            iterator_map_(this),
            size_(0),
            height_(2),
            prefetching_enabled_(true),
            bm_(block_manager::get_instance())
        {
            STXXL_VERBOSE1("Creating a btree, addr=" << this);
            STXXL_VERBOSE1(" bytes in a node: " << node_bid_type::size);
            STXXL_VERBOSE1(" bytes in a leaf: " << leaf_bid_type::size);

            if (range_sorted == false)
            {
                create_empty_leaf();
                insert(b, e);
                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return;
            }

            bulk_construction(b, e, node_fill_factor, leaf_fill_factor);
            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
        }


        template <class InputIterator>
        btree(  InputIterator b,
                InputIterator e,
                unsigned_type node_cache_size_in_bytes,
                unsigned_type leaf_cache_size_in_bytes,
                bool range_sorted = false,
                double node_fill_factor = 0.75,
                double leaf_fill_factor = 0.6
        ) :
            node_cache_(node_cache_size_in_bytes, this, key_compare_),
            leaf_cache_(leaf_cache_size_in_bytes, this, key_compare_),
            iterator_map_(this),
            size_(0),
            height_(2),
            prefetching_enabled_(true),
            bm_(block_manager::get_instance())
        {
            STXXL_VERBOSE1("Creating a btree, addr=" << this);
            STXXL_VERBOSE1(" bytes in a node: " << node_bid_type::size);
            STXXL_VERBOSE1(" bytes in a leaf: " << leaf_bid_type::size);

            if (range_sorted == false)
            {
                create_empty_leaf();
                insert(b, e);
                assert(leaf_cache_.nfixed() == 0);
                assert(node_cache_.nfixed() == 0);
                return;
            }

            bulk_construction(b, e, node_fill_factor, leaf_fill_factor);
            assert(leaf_cache_.nfixed() == 0);
            assert(node_cache_.nfixed() == 0);
        }

        void erase(iterator first, iterator last)
        {
            if (first == begin() && last == end())
                clear();

            else
                while (first != last)
                    erase(first++);

        }

        key_compare key_comp() const
        {
            return key_compare_;
        }
        value_compare value_comp() const
        {
            return value_compare(key_compare_);
        }

        void swap(btree & obj)
        {
            std::swap(key_compare_, obj.key_compare_);            // OK

            std::swap(node_cache_, obj.node_cache_);             // OK
            std::swap(leaf_cache_, obj.leaf_cache_);             // OK


            std::swap(iterator_map_, obj.iterator_map_);            // must update all iterators

            std::swap(end_iterator, obj.end_iterator);
            std::swap(size_, obj.size_);
            std::swap(height_, obj.height_);
            std::swap(alloc_strategy_, obj.alloc_strategy_);
            std::swap(root_node_, obj.root_node_);
        }

        void enable_prefetching()
        {
            prefetching_enabled_ = true;
        }
        void disable_prefetching()
        {
            prefetching_enabled_ = false;
        }
        bool prefetching_enabled()
        {
            return prefetching_enabled_;
        }

        void print_statistics(std::ostream & o) const
        {
            o << "Node cache statistics:" << std::endl;
            node_cache_.print_statistics(o);
            o << "Leaf cache statistics:" << std::endl;
            leaf_cache_.print_statistics(o);
        }
        void reset_statistics()
        {
            node_cache_.reset_statistics();
            leaf_cache_.reset_statistics();
        }
    };

    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned LogNodeSize,
              unsigned LogLeafSize,
              class PDAllocStrategy
    >
    inline bool operator == (const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & a,
                             const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & b)
    {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
    }

    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned LogNodeSize,
              unsigned LogLeafSize,
              class PDAllocStrategy
    >
    inline bool operator != (const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & a,
                             const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & b)
    {
        return !(a == b);
    }


    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned LogNodeSize,
              unsigned LogLeafSize,
              class PDAllocStrategy
    >
    inline bool operator < (const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & a,
                            const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & b)
    {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }


    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned LogNodeSize,
              unsigned LogLeafSize,
              class PDAllocStrategy
    >
    inline bool operator > (const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & a,
                            const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & b)
    {
        return b < a;
    }


    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned LogNodeSize,
              unsigned LogLeafSize,
              class PDAllocStrategy
    >
    inline bool operator <= (const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & a,
                             const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & b)
    {
        return !(b < a);
    }

    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned LogNodeSize,
              unsigned LogLeafSize,
              class PDAllocStrategy
    >
    inline bool operator >= (const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & a,
                             const btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & b)
    {
        return !(a < b);
    }
}

__STXXL_END_NAMESPACE


namespace std
{
    template <      class KeyType,
              class DataType,
              class CompareType,
              unsigned LogNodeSize,
              unsigned LogLeafSize,
              class PDAllocStrategy
    >
    void swap(stxxl::btree::btree < KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy > & a,
              stxxl::btree::btree<KeyType, DataType, CompareType, LogNodeSize, LogLeafSize, PDAllocStrategy> & b)
    {
        if (&a != &b) a.swap(b);

    }
}

#endif /* STXXL_CONTAINERS_BTREE__BTREE_H */
