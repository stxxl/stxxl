/***************************************************************************
 *            leaf.h
 *
 *  Mon Feb  6 17:04:11 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE__LEAF_H
#define STXXL_CONTAINERS_BTREE__LEAF_H

#include "stxxl/bits/containers/btree/iterator.h"
#include "stxxl/bits/containers/btree/node_cache.h"


__STXXL_BEGIN_NAMESPACE

namespace btree
{
    template <class NodeType, class BTreeType>
    class node_cache;

    template <class KeyType_, class DataType_, class KeyCmp_, unsigned RawSize_, class BTreeType>
    class normal_leaf
    {
    public:
        typedef normal_leaf<KeyType_, DataType_, KeyCmp_, RawSize_, BTreeType> SelfType;

        friend class node_cache<SelfType, BTreeType>;

        typedef KeyType_ key_type;
        typedef DataType_ data_type;
        typedef KeyCmp_ key_compare;
        typedef std::pair<key_type, data_type>  value_type;
        typedef value_type  & reference;
        typedef const value_type  & const_reference;

        enum {
            raw_size = RawSize_
        };
        typedef BID<raw_size> bid_type;
        struct InfoType
        {
            bid_type me, pred, succ;
            unsigned cur_size;
        };

        typedef typed_block < raw_size, value_type, 0, InfoType > block_type;
        enum {
            nelements = block_type::size - 1,
            max_size = nelements,
            min_size = nelements / 2
        };

        typedef BTreeType btree_type;
        typedef typename btree_type::size_type size_type;
        typedef btree_iterator_base<btree_type> iterator_base;
        typedef btree_iterator<btree_type>  iterator;
        typedef btree_const_iterator<btree_type> const_iterator;

        typedef node_cache<normal_leaf, btree_type> leaf_cache_type;


    private:
        normal_leaf();
        normal_leaf(const normal_leaf &);
        normal_leaf & operator = (const normal_leaf &);
    public:
        struct value_compare : public std::binary_function<value_type, value_type, bool>
        {
            key_compare comp;

            value_compare(key_compare c) : comp(c) { }

            bool operator() (const value_type & x, const value_type & y) const
            {
                return comp(x.first, y.first);
            }
        };
    private:
        block_type * block_;
        btree_type * btree_;

        key_compare cmp_;
        value_compare vcmp_;

        void split(std::pair < key_type, bid_type > & splitter)
        {
            bid_type NewBid;
            btree_->leaf_cache_.get_new_node(NewBid);                     // new (left) leaf
            normal_leaf * NewLeaf = btree_->leaf_cache_.get_node(NewBid, true);
            assert(NewLeaf);

            // update links
            NewLeaf->succ() = my_bid();
            normal_leaf * PredLeaf = NULL;
            if (pred().valid())
            {
                NewLeaf->pred() = pred();
                PredLeaf = btree_->leaf_cache_.get_node(pred());
                assert(PredLeaf);
                assert(vcmp_(PredLeaf->back(), front()));
                PredLeaf->succ() = NewBid;
            }
            pred() = NewBid;

            std::vector<iterator_base *> Iterators2Fix;
            btree_->iterator_map_.find(my_bid(), 0, size(), Iterators2Fix);

            const unsigned end_of_smaller_part = size() / 2;

            splitter.first = ((*block_)[end_of_smaller_part - 1]).first;
            splitter.second = NewBid;

            const unsigned old_size = size();
            // copy the smaller part
            std::copy(block_->begin(), block_->begin() + end_of_smaller_part, NewLeaf->block_->begin());
            NewLeaf->block_->info.cur_size = end_of_smaller_part;
            // copy the larger part
            std::copy(      block_->begin() + end_of_smaller_part,
                            block_->begin() + old_size, block_->begin());
            block_->info.cur_size = old_size - end_of_smaller_part;
            assert(size() + NewLeaf->size() == old_size);

            // fix iterators
            typename std::vector<iterator_base *>::iterator it2fix = Iterators2Fix.begin();
            for ( ; it2fix != Iterators2Fix.end(); ++it2fix)
            {
                btree_->iterator_map_.unregister_iterator(**it2fix);

                if ((*it2fix)->pos < end_of_smaller_part)                        // belongs to the smaller part
                    (*it2fix)->bid = NewBid;

                else
                    (*it2fix)->pos -= end_of_smaller_part;


                btree_->iterator_map_.register_iterator(**it2fix);
            }



            STXXL_VERBOSE1("btree::normal_leaf split leaf " << this
                                                            << " splitter: " << splitter.first);

#if STXXL_VERBOSE_LEVEL >= 1
            if (PredLeaf)
            {
                STXXL_VERBOSE1("btree::normal_leaf pred_part.smallest    = " << PredLeaf->front().first);
                STXXL_VERBOSE1("btree::normal_leaf pred_part.largest     = " << PredLeaf->back().first);
            }
#endif
            STXXL_VERBOSE1("btree::normal_leaf smaller_part.smallest = " << NewLeaf->front().first);
            STXXL_VERBOSE1("btree::normal_leaf smaller_part.largest  = " << NewLeaf->back().first);
            STXXL_VERBOSE1("btree::normal_leaf larger_part.smallest  = " << front().first);
            STXXL_VERBOSE1("btree::normal_leaf larger_part.largest   = " << back().first);

            btree_->leaf_cache_.unfix_node(NewBid);
        }

    public:
        virtual ~normal_leaf()
        {
            delete block_;
        }

        normal_leaf(    btree_type * btree__,
                        key_compare cmp) :
            block_(new block_type),
            btree_(btree__),
            cmp_(cmp),
            vcmp_(cmp)
        {
            assert(min_nelements() >= 2);
            assert(2 * min_nelements() - 1 <= max_nelements());
            assert(max_nelements() <= nelements);
            assert(unsigned (block_type::size) >= nelements + 1);                   // extra space for an overflow
        }

        bool overflows () const { return block_->info.cur_size > max_nelements(); }
        bool underflows () const { return block_->info.cur_size < min_nelements(); }

        unsigned max_nelements() const { return max_size; }
        unsigned min_nelements() const { return min_size; }


        bid_type & succ()
        {
            return block_->info.succ;
        }
        bid_type & pred()
        {
            return block_->info.pred;
        }

        const bid_type & succ() const
        {
            return block_->info.succ;
        }
        const bid_type & pred() const
        {
            return block_->info.pred;
        }

        /*
           template <class InputIterator>
           normal_leaf(InputIterator begin_, InputIterator end_,
                btree_type * btree__,
                key_compare cmp):
                block_(new block_type),
                btree_(btree__),
                cmp_(cmp),
                vcmp_(cmp)
           {
                assert(min_nelements() >=2);
                assert(2*min_nelements() - 1 <= max_nelements());
                assert(max_nelements() <= nelements);
                assert(unsigned(block_type::size) >= nelements +1); // extra space for an overflow element

                unsigned new_size = end_ - begin_;
                assert(new_size <= max_nelements());
                assert(new_size >= min_nelements());

                std::copy(begin_,end_,block_->begin());
                assert(stxxl::is_sorted(block_->begin(),block_->begin() + new_size, vcmp_));
                block_->info.cur_size = new_size;
           }*/

        unsigned size() const
        {
            return block_->info.cur_size;
        }

        const bid_type & my_bid() const
        {
            return block_->info.me;
        }

        void save()
        {
            request_ptr req = block_->write(my_bid());
            req->wait();
        }

        request_ptr load(const bid_type & bid)
        {
            request_ptr req = block_->read(bid);
            req->wait();
            assert(bid == my_bid());
            return req;
        }

        request_ptr prefetch(const bid_type & bid)
        {
            return block_->read(bid);
        }

        void init(const bid_type & my_bid_)
        {
            block_->info.me = my_bid_;
            block_->info.succ = bid_type();
            block_->info.pred = bid_type();
            block_->info.cur_size = 0;
        }

        reference operator [] (int i)
        {
            return (*block_)[i];
        }

        const_reference operator [] (int i) const
        {
            return (*block_)[i];
        }

        reference back()
        {
            return (*block_)[size() - 1];
        }

        reference front()
        {
            return *(block_->begin());
        }

        const_reference back() const
        {
            return (*block_)[size() - 1];
        }

        const_reference front() const
        {
            return *(block_->begin());
        }

        void dump()
        {
            STXXL_VERBOSE2("Dump of leaf " << this);
            for (unsigned i = 0; i < size(); ++i)
                STXXL_VERBOSE2((*this)[i].first << " " << (*this)[i].second);

        }

        std::pair<iterator, bool> insert(
            const value_type & x,
            std::pair<key_type, bid_type> &splitter)
        {
            assert(size() <= max_nelements());
            splitter.first = key_compare::max_value();

            typename block_type::iterator it =
                std::lower_bound(block_->begin(), block_->begin() + size(), x, vcmp_);

            if (!(vcmp_(*it, x) || vcmp_(x, *it)) && it != (block_->begin() + size()))                // *it == x
            {
                // already exists
                return std::pair<iterator, bool>(
                           iterator (btree_, my_bid(), it - block_->begin())
                           , false);
            }

            typename block_type::iterator cur = block_->begin() + size() - 1;

            for ( ; cur >= it; --cur)
                *(cur + 1) = *cur;
            // move elements to make space for the new element

            *it = x;

            std::vector<iterator_base *> Iterators2Fix;
            btree_->iterator_map_.find(my_bid(), it - block_->begin(), size(), Iterators2Fix);
            typename std::vector<iterator_base *>::iterator it2fix = Iterators2Fix.begin();
            for ( ; it2fix != Iterators2Fix.end(); ++it2fix)
            {
                btree_->iterator_map_.unregister_iterator(**it2fix);
                ++ ((*it2fix)->pos);                        // fixing iterators
                btree_->iterator_map_.register_iterator(**it2fix);
            }

            ++ (block_->info.cur_size);

            std::pair<iterator, bool> result(iterator(btree_, my_bid(), it - block_->begin()), true);

            if (size() <= max_nelements())
            {
                // no overflow
                dump();
                return result;
            }

            // overflow

            split(splitter);

            return result;
        }

        iterator begin()
        {
            return iterator(btree_, my_bid(), 0);
        }

        const_iterator begin() const
        {
            return const_iterator(btree_, my_bid(), 0);
        }

        iterator end()
        {
            return iterator(btree_, my_bid(), size());
        }

        void increment_iterator(iterator_base & it) const
        {
            assert(it.bid == my_bid());
            assert(it.pos != size());

            btree_->iterator_map_.unregister_iterator(it);

            ++ (it.pos);
            if (it.pos == size() && succ().valid())
            {
                // run to the end of the leaf
                STXXL_VERBOSE1("btree::normal_leaf jumping to the next block");
                it.pos = 0;
                it.bid = succ();
            } else if (it.pos == 1 && btree_->prefetching_enabled_)                    // increment of pos from 0 to 1
            {
                // prefetch the succ leaf
                if (succ().valid()) btree_->leaf_cache_.prefetch_node(succ());

            }
            btree_->iterator_map_.register_iterator(it);
        }

        void decrement_iterator(iterator_base & it) const
        {
            assert(it.bid == my_bid());

            btree_->iterator_map_.unregister_iterator(it);

            if (it.pos == 0)
            {
                assert(pred().valid());

                it.bid = pred();
                normal_leaf const * PredLeaf = btree_->leaf_cache_.get_const_node(pred(), true);
                assert(PredLeaf);
                it.pos = PredLeaf->size() - 1;

                // prefetch the pred leaf of PredLeaf
                if (btree_->prefetching_enabled_ && PredLeaf->pred().valid())
                    btree_->leaf_cache_.prefetch_node(PredLeaf->pred());


                btree_->leaf_cache_.unfix_node(pred());
            }
            else
                --it.pos;


            btree_->iterator_map_.register_iterator(it);
        }

        iterator find(const key_type & k)
        {
            value_type searchVal(k, data_type());
            typename block_type::iterator lb =
                std::lower_bound(block_->begin(), block_->begin() + size(), searchVal, vcmp_);
            if (lb == block_->begin() + size() || lb->first != k)
                return btree_->end();


            return iterator(btree_, my_bid(), lb - block_->begin());
        }

        const_iterator find(const key_type & k) const
        {
            value_type searchVal(k, data_type());
            typename block_type::iterator lb =
                std::lower_bound(block_->begin(), block_->begin() + size(), searchVal, vcmp_);
            if (lb == block_->begin() + size() || lb->first != k)
                return btree_->end();


            return const_iterator(btree_, my_bid(), lb - block_->begin());
        }

        iterator lower_bound(const key_type & k)
        {
            value_type searchVal(k, data_type());

            typename block_type::iterator lb =
                std::lower_bound(block_->begin(), block_->begin() + size(), searchVal, vcmp_);

            // lower_bound is in the succ block
            if (lb == block_->begin() + size() && succ().valid())
            {
                return iterator(btree_, succ(), 0);
            }

            return iterator(btree_, my_bid(), lb - block_->begin());
        }

        const_iterator lower_bound(const key_type & k) const
        {
            value_type searchVal(k, data_type());
            typename block_type::iterator lb =
                std::lower_bound(block_->begin(), block_->begin() + size(), searchVal, vcmp_);

            // lower_bound is in the succ block
            if (lb == block_->begin() + size() && succ().valid())
            {
                return iterator(btree_, succ(), 0);
            }

            return const_iterator(btree_, my_bid(), lb - block_->begin());
        }

        iterator upper_bound(const key_type & k)
        {
            value_type searchVal(k, data_type());
            typename block_type::iterator lb =
                std::upper_bound(block_->begin(), block_->begin() + size(), searchVal, vcmp_);

            // upper_bound is in the succ block
            if (lb == block_->begin() + size() && succ().valid())
            {
                return iterator(btree_, succ(), 0);
            }

            return iterator(btree_, my_bid(), lb - block_->begin());
        }

        const_iterator upper_bound(const key_type & k) const
        {
            value_type searchVal(k, data_type());
            typename block_type::iterator lb =
                std::upper_bound(block_->begin(), block_->begin() + size(), searchVal, vcmp_);

            // upper_bound is in the succ block
            if (lb == block_->begin() + size() && succ().valid())
            {
                return const_iterator(btree_, succ(), 0);
            }

            return const_iterator(btree_, my_bid(), lb - block_->begin());
        }

        size_type erase(const key_type & k)
        {
            value_type searchVal(k, data_type());
            typename block_type::iterator it =
                std::lower_bound(block_->begin(), block_->begin() + size(), searchVal, vcmp_);

            if (it == block_->begin() + size() || it->first != k)
                return 0;
            // no such element

            // move elements one position left
            std::copy(it + 1, block_->begin() + size(), it);

            std::vector<iterator_base *> Iterators2Fix;
            btree_->iterator_map_.find(my_bid(), it + 1 - block_->begin(), size(), Iterators2Fix);
            typename std::vector<iterator_base *>::iterator it2fix = Iterators2Fix.begin();
            for ( ; it2fix != Iterators2Fix.end(); ++it2fix)
            {
                STXXL_VERBOSE2("btree::normal_leaf updating iterator " << (*it2fix) << " (pos--)");
                btree_->iterator_map_.unregister_iterator(**it2fix);
                -- ((*it2fix)->pos);                        // fixing iterators
                btree_->iterator_map_.register_iterator(**it2fix);
            }

            -- (block_->info.cur_size);

            return 1;
        }

        void fuse(const normal_leaf & Src)
        {
            STXXL_VERBOSE1("btree::normal_leaf Fusing");
            assert(vcmp_(Src.back(), front()));
            const unsigned SrcSize = Src.size();

            typename block_type::iterator cur = block_->begin() + size() - 1;
            typename block_type::const_iterator begin = block_->begin();

            for ( ; cur >= begin; --cur)
                *(cur + SrcSize) = *cur;
            // move elements to make space for Src elements

            // copy Src to *this leaf
            std::copy(Src.block_->begin(), Src.block_->begin() + SrcSize, block_->begin());


            std::vector<iterator_base *> Iterators2Fix;
            btree_->iterator_map_.find(my_bid(), 0, size(), Iterators2Fix);
            typename std::vector<iterator_base *>::iterator it2fix = Iterators2Fix.begin();
            for ( ; it2fix != Iterators2Fix.end(); ++it2fix)
            {
                STXXL_VERBOSE2("btree::normal_leaf updating iterator " << (*it2fix) <<
                               " (pos+" << SrcSize << ")");
                btree_->iterator_map_.unregister_iterator(**it2fix);
                ((*it2fix)->pos) += SrcSize;                       // fixing iterators
                btree_->iterator_map_.register_iterator(**it2fix);
            }

            Iterators2Fix.clear();
            btree_->iterator_map_.find(Src.my_bid(), 0, SrcSize, Iterators2Fix);
            it2fix = Iterators2Fix.begin();
            for ( ; it2fix != Iterators2Fix.end(); ++it2fix)
            {
                STXXL_VERBOSE2("btree::normal_leaf updating iterator " << (*it2fix) <<
                               " (bid=" << my_bid() << ")");
                btree_->iterator_map_.unregister_iterator(**it2fix);
                ((*it2fix)->bid) = my_bid();                         // fixing iterators
                btree_->iterator_map_.register_iterator(**it2fix);
            }

            block_->info.cur_size += SrcSize;

            // update links
            pred() = Src.pred();
            if (pred().valid())
            {                     // update successor link
                normal_leaf * NewPred = btree_->leaf_cache_.get_node(pred());
                assert(NewPred);
                NewPred->succ() = my_bid();
            }
        }

        key_type balance(normal_leaf & Left)
        {
            STXXL_VERBOSE1("btree::normal_leaf Balancing leaves with bids " <<
                           Left.my_bid() << " and " << my_bid());
            const unsigned TotalSize = Left.size() + size();
            unsigned newLeftSize = TotalSize / 2;
            assert(newLeftSize <= Left.max_nelements());
            assert(newLeftSize >= Left.min_nelements());
            unsigned newRightSize = TotalSize - newLeftSize;
            assert(newRightSize <= max_nelements());
            assert(newRightSize >= min_nelements());

            assert(vcmp_(Left.back(), front()) || size() == 0);

            if (newLeftSize < Left.size())
            {
                const unsigned nEl2Move = Left.size() - newLeftSize;                        // #elements to move from Left to *this

                typename block_type::iterator cur = block_->begin() + size() - 1;
                typename block_type::const_iterator begin = block_->begin();

                for ( ; cur >= begin; --cur)
                    *(cur + nEl2Move) = *cur;
                // move elements to make space for Src elements

                // copy Left to *this leaf
                std::copy(      Left.block_->begin() + newLeftSize,
                                Left.block_->begin() + Left.size(), block_->begin());

                std::vector<iterator_base *> Iterators2Fix1;
                std::vector<iterator_base *> Iterators2Fix2;
                btree_->iterator_map_.find(my_bid(), 0, size(), Iterators2Fix1);
                btree_->iterator_map_.find(Left.my_bid(), newLeftSize, Left.size(), Iterators2Fix2);

                typename std::vector<iterator_base *>::iterator it2fix = Iterators2Fix1.begin();
                for ( ; it2fix != Iterators2Fix1.end(); ++it2fix)
                {
                    STXXL_VERBOSE2("btree::normal_leaf updating iterator " << (*it2fix) <<
                                   " (pos+" << nEl2Move << ")");
                    btree_->iterator_map_.unregister_iterator(**it2fix);
                    ((*it2fix)->pos) += nEl2Move;                           // fixing iterators
                    btree_->iterator_map_.register_iterator(**it2fix);
                }


                it2fix = Iterators2Fix2.begin();
                for ( ; it2fix != Iterators2Fix2.end(); ++it2fix)
                {
                    STXXL_VERBOSE2("btree::normal_leaf updating iterator " << (*it2fix) <<
                                   " (pos-" << newLeftSize << " bid=" << my_bid() << ")");
                    btree_->iterator_map_.unregister_iterator(**it2fix);
                    ((*it2fix)->bid) = my_bid();                             // fixing iterators
                    ((*it2fix)->pos) -= newLeftSize;                             // fixing iterators
                    btree_->iterator_map_.register_iterator(**it2fix);
                }
            }
            else
            {
                assert(newRightSize < size());

                const unsigned nEl2Move = size() - newRightSize;                        // #elements to move from *this to Left

                // copy *this to Left
                std::copy(      block_->begin(),
                                block_->begin() + nEl2Move, Left.block_->begin() + Left.size());
                // move elements in *this
                std::copy(      block_->begin() + nEl2Move,
                                block_->begin() + size(), block_->begin() );

                std::vector<iterator_base *> Iterators2Fix1;
                std::vector<iterator_base *> Iterators2Fix2;
                btree_->iterator_map_.find(my_bid(), nEl2Move, size(), Iterators2Fix1);
                btree_->iterator_map_.find(my_bid(), 0, nEl2Move - 1, Iterators2Fix2);

                typename std::vector<iterator_base *>::iterator it2fix = Iterators2Fix1.begin();
                for ( ; it2fix != Iterators2Fix1.end(); ++it2fix)
                {
                    STXXL_VERBOSE2("btree::normal_leaf updating iterator " << (*it2fix) <<
                                   " (pos-" << nEl2Move << ")");
                    btree_->iterator_map_.unregister_iterator(**it2fix);
                    ((*it2fix)->pos) -= nEl2Move;                             // fixing iterators
                    btree_->iterator_map_.register_iterator(**it2fix);
                }


                it2fix = Iterators2Fix2.begin();
                for ( ; it2fix != Iterators2Fix2.end(); ++it2fix)
                {
                    STXXL_VERBOSE2("btree::normal_leaf updating iterator " << (*it2fix) <<
                                   " (pos+" << Left.size() << " bid=" << Left.my_bid() << ")");
                    btree_->iterator_map_.unregister_iterator(**it2fix);
                    ((*it2fix)->bid) = Left.my_bid();                             // fixing iterators
                    ((*it2fix)->pos) += Left.size();                             // fixing iterators
                    btree_->iterator_map_.register_iterator(**it2fix);
                }
            }

            block_->info.cur_size = newRightSize;                     // update size
            Left.block_->info.cur_size = newLeftSize;                     // update size

            return Left.back().first;
        }

        void push_back(const value_type & x)
        {
            (*this)[size()] = x;
            ++ (block_->info.cur_size);
        }
    };
}


__STXXL_END_NAMESPACE

#endif /* STXXL_CONTAINERS_BTREE__LEAF_H */
