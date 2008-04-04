/***************************************************************************
 *            iterator_map.h
 *
 *  Tue Feb 14 20:35:33 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef STXXL_CONTAINERS_BTREE__ITERATOR_MAP_H
#define STXXL_CONTAINERS_BTREE__ITERATOR_MAP_H

#include "stxxl/bits/containers/btree/iterator.h"

#include <map>


__STXXL_BEGIN_NAMESPACE

namespace btree
{
    template <class BTreeType>
    class iterator_map
    {
    public:
        typedef BTreeType btree_type;
        typedef typename btree_type::leaf_bid_type bid_type;
        typedef btree_iterator_base<btree_type> iterator_base;

    private:
        struct Key
        {
            bid_type bid;
            unsigned pos;
            Key() { }
            Key(const bid_type &b, unsigned p) : bid(b), pos(p) { }
        };

        struct bid_comp
        {
            bool operator ()  (const bid_type & a, const bid_type & b) const
            {
                return (a.storage < b.storage) || ( a.storage == b.storage && a.offset < b.offset);
            }
        };
        struct KeyCmp
        {
            bid_comp BIDComp;
            bool operator() (const Key & a, const Key & b) const
            {
                return BIDComp(a.bid, b.bid) || (a.bid == b.bid && a.pos < b.pos);
            }
        };

        typedef std::multimap<Key, iterator_base *, KeyCmp> multimap_type;

        multimap_type It2Addr_;
        btree_type * btree_;

        typedef typename multimap_type::value_type pair_type;
        typedef typename multimap_type::iterator mmiterator_type;
        typedef typename multimap_type::const_iterator mmconst_iterator_type;

        iterator_map();         // forbidden
        iterator_map(const iterator_map &);         // forbidden
        iterator_map & operator = (const iterator_map &);         // forbidden


        // changes btree pointer in all contained iterators
        void change_btree_pointers(btree_type * b)
        {
            mmconst_iterator_type it = It2Addr_.begin();
            for ( ; it != It2Addr_.end(); ++it)
            {
                (it->second)->btree_ = b;
            }
        }

    public:

        iterator_map(btree_type * b) : btree_(b)
        { }

        void register_iterator(iterator_base & it)
        {
            STXXL_VERBOSE2("btree::iterator_map register_iterator addr=" << &it <<
                           " BID: " << it.bid << " POS: " << it.pos);
            It2Addr_.insert(pair_type(Key(it.bid, it.pos), &it));
        }
        void unregister_iterator(iterator_base & it)
        {
            STXXL_VERBOSE2("btree::iterator_map unregister_iterator addr=" << &it <<
                           " BID: " << it.bid << " POS: " << it.pos);
            assert(!It2Addr_.empty());
            Key key(it.bid, it.pos);
            std::pair<mmiterator_type, mmiterator_type> range =
                It2Addr_.equal_range(key);

            assert(range.first != range.second);

            mmiterator_type i = range.first;
            for ( ; i != range.second; ++i)
            {
                assert(it.bid == (*i).first.bid );
                assert(it.pos == (*i).first.pos );

                if ((*i).second == &it)
                {
                    // found it
                    It2Addr_.erase(i);
                    return;
                }
            }

            STXXL_THROW(std::runtime_error, "unregister_iterator", "Panic in btree::iterator_map, can not find and unregister iterator");
        }
        template <class OutputContainer>
        void find(      const bid_type & bid,
                        unsigned first_pos,
                        unsigned last_pos,
                        OutputContainer & out
        )
        {
            Key firstkey(bid, first_pos);
            Key lastkey(bid, last_pos);
            mmconst_iterator_type begin = It2Addr_.lower_bound(firstkey);
            mmconst_iterator_type end = It2Addr_.upper_bound(lastkey);

            mmconst_iterator_type i = begin;
            for ( ; i != end; ++i)
            {
                assert(bid == (*i).first.bid);
                out.push_back((*i).second);
            }
        }

        virtual ~iterator_map()
        {
            mmconst_iterator_type it = It2Addr_.begin();
            for ( ; it != It2Addr_.end(); ++it)
                (it->second)->make_invalid();
        }

        void swap(iterator_map & obj)
        {
            std::swap(It2Addr_, obj.It2Addr_);
            change_btree_pointers(btree_);
            obj.change_btree_pointers(obj.btree_);
        }
    };
}

__STXXL_END_NAMESPACE


namespace std
{
    template <class BTreeType>
    void swap( stxxl::btree::iterator_map < BTreeType > & a,
               stxxl::btree::iterator_map<BTreeType> & b)
    {
        a.swap(b);
    }
}

#endif /* STXXL_CONTAINERS_BTREE__ITERATOR_MAP_H */
