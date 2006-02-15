/***************************************************************************
 *            iterator_map.h
 *
 *  Tue Feb 14 20:35:33 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _ITERATOR_MAP_H
#define _ITERATOR_MAP_H

#include <stxxl>
#include "iterator.h"

#include <map>

__STXXL_BEGIN_NAMESPACE

namespace btree
{

	template <class BIDType>
	class iterator_map
	{	
		
	public:
		typedef BIDType	bid_type;
		typedef btree_iterator_base<bid_type> iterator_base;
	
	private:
		struct Key
		{
			bid_type bid;
			unsigned pos;
			Key() {}
			Key(const bid_type & b, unsigned p): bid(b),pos(p){}
		};
	
		struct bid_comp
		{
  				bool operator () (const bid_type & a, const bid_type & b) const
  				{
      				return (a.storage < b.storage) || ( a.storage == b.storage && a.offset < b.offset);
 				}
		};
		struct KeyCmp
		{
			bid_comp BIDComp;
  			bool operator()(const Key & a, const Key & b) const
  			{
				return BIDComp(a.bid,b.bid) || (a.bid == b.bid && a.pos < b.pos);
  			}
		};
		
		typedef std::multimap<Key,iterator_base *,KeyCmp> multimap_type;
		
		multimap_type It2Addr_;
		typedef typename multimap_type::value_type pair_type;
		typedef typename multimap_type::iterator mmiterator_type;
		
	public:
			
		void register_iterator(const iterator_base & it)
		{
			It2Addr_.insert(pair_type(Key(it.bid,it.pos),&it));
		}
		void unregister_iterator(const iterator_base & it)
		{
			Key key(it.bid,it.pos);
			std::pair<mmiterator_type,mmiterator_type> range = 
				It2Addr_.equal_range(key);
			
			assert(range.first != range.second);
			
			mmiterator_type i = range.first;
			for(;i!=range.second;++i)
			{
				assert(it.bid == (*i).first.bid );
				assert(it.pos == (*i).first.pos );
				
				if((*i).second == &it)
				{
					// found it
					It2Addr_.erase(i);
					return;
				}
			}
			STXXL_ERRMSG("Panic in btree::iterator_map, can not find and unregister iterator")
			STXXL_ERRMSG("Aborting")
			abort();
		}
		template <class OutputContainer>
		void find(	const bid_type & bid, 
						unsigned first_pos,
					 	unsigned last_pos,
						OutputContainer & out
						)
		{
			Key firstkey(bid,first_pos);
			Key lastkey(bid,last_pos);
			mmiterator_type begin = It2Addr_.lower_bound(firstkey);
			mmiterator_type end = It2Addr_.upper_bound(lastkey);
			
			mmiterator_type i = begin;
			for(;i!=end;++i)
			{
				assert(bid == (*i).first.bid);
				out.push_back((*i).second);
			}
		}
	};
		
}

__STXXL_END_NAMESPACE

#endif /* _ITERATOR_MAP_H */
