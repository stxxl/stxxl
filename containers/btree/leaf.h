/***************************************************************************
 *            leaf.h
 *
 *  Mon Feb  6 17:04:11 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _LEAF_H
#define _LEAF_H

#include <stxxl>
#include "iterator.h"
#include "node_cache.h"

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	
	template <class KeyType_, class DataType_, class KeyCmp_, unsigned LogNElem_, class BTreeType>
	class normal_leaf
	{
			
		public:
			enum {
				nelements = 1<<LogNElem_,
				magic_block_size = 4096
			};
			typedef KeyType_ key_type;
			typedef DataType_ data_type;
			typedef KeyCmp_ key_compare;
			typedef std::pair<key_type,data_type>  value_type;
			typedef value_type  & reference;
			
			enum {
				raw_size_not_4Krounded = sizeof(value_type [nelements])
					+ 3*sizeof(BID<magic_block_size>) // succ, pred links and my bid
					+ sizeof(unsigned),	// current size
				raw_size = (raw_size_not_4Krounded/4096 + 1)*4096
			};
			typedef BID<raw_size> bid_type;
			struct InfoType
			{
				bid_type me, pred, succ ;
				unsigned cur_size;
			};

			typedef typed_block<raw_size,value_type,0,InfoType> block_type;			
			typedef BTreeType btree_type;
			typedef btree_iterator_base<btree_type> iterator_base;
			typedef btree_iterator<btree_type>  iterator;
			typedef btree_const_iterator<btree_type> const_iterator;
			
			
			
			typedef node_cache<normal_leaf,btree_type> leaf_cache_type;
			
			
private:
			normal_leaf();
			normal_leaf(const normal_leaf &);
			normal_leaf & operator = (const normal_leaf &);

			struct value_compare : public std::binary_function<value_type, value_type, bool> 
			{
					key_compare comp;
				
					value_compare(key_compare c) : comp(c) {}
					
					bool operator()(const value_type& x, const value_type& y) const
					{
						return comp(x.first, y.first);
					}
			};

			block_type * block_;
			btree_type * btree_;
			const unsigned min_nelements_;
			const unsigned max_nelements_;
			key_compare cmp_;
			value_compare vcmp_;

public:
			virtual ~normal_leaf()
			{
				delete block_;
			}
			
			normal_leaf(	btree_type * btree__,
								unsigned min_nelements, 
								unsigned max_nelements, 
								key_compare cmp): 
				block_(new block_type),
				btree_(btree__),
				min_nelements_(min_nelements),
				max_nelements_(max_nelements),
				cmp_(cmp),
				vcmp_(cmp)
			{
				assert(min_nelements < max_nelements);
				assert(max_nelements <= nelements);
				assert(unsigned(block_type::size) >= nelements +1); // extra space for an overflow
			}
			
			bool overflows () const { return block_->info.cur_size > max_nelements_; }
			bool underflows () const { return block_->info.cur_size < min_nelements_; }
			
			/*
			iterator begin() { return block_->begin(); };
			const_iterator begin() const { return block_->begin(); };
			iterator end() { return block_->begin() + block_->info.cur_size; };
			const_iterator end() const { return block_->begin() + block_->info.cur_size; };
			*/
			
			template <class InputIterator>
			normal_leaf(InputIterator begin_, InputIterator end_,
				btree_type * btree__,
				unsigned min_nelements, unsigned max_nelements,
				key_compare cmp): 
				block_(new block_type),
				btree_(btree__),
				min_nelements_(min_nelements),
				max_nelements_(max_nelements),
				cmp_(cmp),
				vcmp_(cmp)
			{
				assert(min_nelements < max_nelements);
				assert(max_nelements <= nelements);
				assert(unsigned(block_type::size) >= nelements +1); // extra space for an overflow element
				
				unsigned new_size = end_ - begin_;
				assert(new_size <= max_nelements_);
				assert(new_size >= min_nelements_);
				
				std::copy(begin_,end_,block_->begin());
				assert(stxxl::is_sorted(block_->begin(),block_->begin() + new_size, vcmp_));
				block_->info.cur_size = new_size;
			}
			
			/*
			iterator find(const key_type & Key)
			{
				const value_type searchVal(Key,data_type());
				iterator lb = std::lower_bound(begin(),end(),searchVal,vcmp_);
				if(lb == end() || lb->first != Key)
					return end();
				return lb;
			}
			const_iterator find(const key_type & Key) const
			{
				const value_type searchVal(Key,data_type());
				const_iterator lb = std::lower_bound(begin(),end(),searchVal,vcmp_);
				if(lb == end() || lb->first != Key)
					return end();
				return lb;
			}
			
			iterator lower_bound(const key_type & Key)
			{
				return std::lower_bound(begin(),end(),vcmp_);
			}
			const_iterator lower_bound(const key_type & Key) const
			{
				return std::lower_bound(begin(),end(),vcmp_);
			}
			
			iterator upper_bound(const key_type & Key)
			{
				return std::upper_bound(begin(),end(),vcmp_);
			}
			
			const_iterator upper_bound(const key_type & Key) const
			{
				return std::upper_bound(begin(),end(),vcmp_);
			}
			*/
			
			unsigned size() const
			{
				return block_->info.cur_size;
			}
			
			bid_type my_bid() const 
			{
				return block_->info.me;
			}
			
			void save()
			{
				request_ptr req = block_->write(my_bid());
				req->wait();
			}
			
			void load(const bid_type & bid)
			{
				request_ptr req = block_->read(bid);
				req->wait();
			}
			
			void init(const bid_type & my_bid_)
			{
				block_->info.me = my_bid_;
				block_->info.cur_size = 0;
			}
			
			reference operator [] (int i)
			{
				return (*block_)[i];
			}
			
			void dump()
			{
				STXXL_VERBOSE1("Dump og leaf "<<this)
				for(int i=0;i<size();++i)
					STXXL_VERBOSE1((*this)[i].first<<" "<<(*this)[i].second)
			}
			
			std::pair<iterator,bool> insert(
					const value_type & x, 
					std::pair<key_type,bid_type> & splitter)
			{
				assert(size() <= max_nelements_);
				typename block_type::iterator it = 
					std::lower_bound(block_->begin(),block_->begin() + size(), x ,vcmp_);
				
				if(!(vcmp_(*it,x) || vcmp_(x,*it))) // *it == x
				{
					// already exists
					return std::pair<iterator,bool>(
						iterator(btree_,my_bid(),it - block_->begin())
						,false);
				}
				
				typename block_type::iterator cur = block_->begin() + size() - 1;

				for(; cur>=it; --cur)
					*(cur+1) = *cur;  // copy elements to make space for the new element
				
				*it = x;
				
				std::vector<iterator_base *> Iterators2Fix;
				btree_->iterator_map_.find(my_bid(),it - block_->begin(),size() - 1,Iterators2Fix);				
				typename std::vector<iterator_base *>::iterator  it2fix = Iterators2Fix.begin();
				for(;it2fix!=Iterators2Fix.end();++it2fix)
				{
					btree_->iterator_map_.unregister_iterator(**it2fix);
					++((*it2fix)->pos); // fixing iterators
					btree_->iterator_map_.register_iterator(**it2fix);
				}
				
				++(block_->info.cur_size);
				
				if(size() <= max_nelements_)
				{
					// no overflow
					dump();
					
					return std::pair<iterator,bool>(
						iterator(btree_,my_bid(),it - block_->begin())
						,true);
				}
				
				abort();
				
				return std::pair<iterator,bool>();
			}

	};
};


__STXXL_END_NAMESPACE

#endif /* _LEAF_H */
