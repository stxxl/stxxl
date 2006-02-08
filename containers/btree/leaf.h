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

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	template <class KeyType_, class DataType_, unsigned NElemPower_, class KeyCmp_>
	class normal_leaf
	{
			
		public:
			enum {
				nelements = 1<<NElemPower_,
				magic_block_size = 4096
			};
			typedef KeyType_ key_type;
			typedef DataType_ data_type;
			typedef KeyCmp_ key_compare;
			typedef std::pair<key_type,data_type>  value_type;
			
			enum {
				raw_size_not_4Krounded = sizeof(value_type [nelements])
					+ 3*sizeof(BID<magic_block_size>) // two siblings and parent pointers
					+ sizeof(unsigned),	// current size
				raw_size = (raw_size_not_4Krounded/4096 + 1)*4096
			};
			typedef BID<raw_size> bid_type;
			struct InfoType
			{
				bid_type pred, succ, parent ;
				unsigned cur_size;
			};
			typedef typed_block<raw_size,value_type,0,InfoType> block_type;
			
			typedef typename block_type::iterator iterator;
			typedef typename block_type::const_iterator const_iterator;
private:
			normal_leaf();

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
			const unsigned min_nelements_;
			const unsigned max_nelements_;
			key_compare cmp_;
			value_compare vcmp_;

public:
			virtual ~normal_leaf()
			{
				delete block_;
			}
			
			normal_leaf(unsigned min_nelements, unsigned max_nelements, key_compare cmp): 
				block_(new block_type),
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
			
			iterator begin() { return block_->begin(); };
			const_iterator begin() const { return block_->begin(); };
			iterator end() { return block_->begin() + block_->info.cur_size; };
			const_iterator end() const { return block_->begin() + block_->info.cur_size; };
			
			
			template <class InputIterator>
			normal_leaf(InputIterator begin_, InputIterator end_,
				unsigned min_nelements, unsigned max_nelements,
				key_compare cmp): 
				block_(new block_type),
				min_nelements_(min_nelements),
				max_nelements_(max_nelements),
				cmp_(cmp),
				vcmp_(cmp)
			{
				assert(min_nelements < max_nelements);
				assert(max_nelements <= nelements);
				assert(unsigned(block_type::size) >= nelements +1); // extra space for an overflow
				
				unsigned new_size = end_ - begin_;
				assert(new_size <= max_nelements_);
				assert(new_size >= min_nelements_);
				
				std::copy(begin_,end_,block_->begin());
				assert(stxxl::is_sorted(block_->begin(),block_->begin() + new_size, vcmp_));
				block_->info.cur_size = new_size;
			}
			
			
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
			
			unsigned size() const
			{
				return block_->info.cur_size;
			}
			
			
			
			
	};
};


__STXXL_END_NAMESPACE

#endif /* _LEAF_H */
