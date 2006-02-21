/***************************************************************************
 *            node.h
 *
 *  Tue Feb 14 21:02:51 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _NODE_H
#define _NODE_H


#include <stxxl>
#include "iterator.h"
#include "node_cache.h"

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	
	template <class KeyType_, class KeyCmp_, unsigned LogNElem_, class BTreeType>
	class normal_node
	{
			
		public:
			typedef	normal_node<KeyType_,KeyCmp_,LogNElem_,BTreeType> SelfType;
			enum {
				nelements = 1<<LogNElem_,
				magic_block_size = 4096
			};
			typedef KeyType_ key_type;
			typedef KeyCmp_ key_compare;
			
			typedef std::pair<key_type,BID<magic_block_size> >  dummy_value_type;
			
			enum {
				raw_size_not_4Krounded = sizeof(dummy_value_type [nelements])
					+ sizeof(BID<magic_block_size>) //  my bid
					+ sizeof(unsigned),	// current size
				raw_size = (raw_size_not_4Krounded/4096 + 1)*4096
			};
			typedef BID<raw_size> bid_type;
			typedef bid_type 	node_bid_type;
			typedef SelfType	node_type;
			typedef std::pair<key_type,bid_type>  value_type;
			struct InfoType
			{
				bid_type me;
				unsigned cur_size;
			};
			typedef typed_block<raw_size,value_type,0,InfoType> block_type;			
			typedef typename block_type::iterator block_iterator;
			
			typedef BTreeType btree_type;
			typedef typename btree_type::iterator  iterator;
			typedef typename btree_type::const_iterator const_iterator;
			
			typedef typename btree_type::value_type btree_value_type;
			typedef typename btree_type::leaf_bid_type leaf_bid_type;
			typedef typename btree_type::leaf_type leaf_type;
			
			typedef node_cache<normal_node,btree_type> node_cache_type;

			
private:
			normal_node();
			normal_node(const normal_node &);
			normal_node & operator = (const normal_node &);

			struct value_compare : public std::binary_function<value_type, bid_type, bool> 
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
			
			
			
			template <class BIDType>
			std::pair<key_type,bid_type> insert(const std::pair<key_type,BIDType> & splitter, 
				const block_iterator & place2insert)
			{
				std::pair<key_type,bid_type> result(key_compare::max_value(),bid_type());
				
				// splitter != *place2insert
				assert(vcmp_(*place2insert,splitter) || vcmp_(splitter,*place2insert)); 
				
				block_iterator cur = block_->begin() + size() - 1;
				for(; cur>=place2insert; --cur)
					*(cur+1) = *cur;  // copy elements to make space for the new element
				
				*place2insert = splitter; // insert
				
				++(block_->info.cur_size);
				
				if(size() > max_nelements_) // overflow! need to split
				{
					STXXL_VERBOSE1("btree::normal_node::insert overflow happened, splitting")
					
					bid_type NewBid;
					btree_->node_cache_.get_new_node(NewBid); // new (left) node
					normal_node * NewNode = btree_->node_cache_.get_node(NewBid,true);
					assert(NewNode);
					
					const unsigned end_of_smaller_part = size()/2;
					
					result.first = ((*block_)[end_of_smaller_part-1]).first;
					result.second = NewBid;
					
					
					const unsigned old_size = size();
					// copy the smaller part
					std::copy(block_->begin(),block_->begin() + end_of_smaller_part, NewNode->block_->begin());
					NewNode->block_->info.cur_size = end_of_smaller_part;	
					// copy the larger part
					std::copy(	block_->begin() + end_of_smaller_part,
									block_->begin() + old_size, block_->begin());
					block_->info.cur_size = old_size - end_of_smaller_part;
					assert(size() + NewNode->size() == old_size);
					
					btree_->node_cache_.unfix_node(NewBid);
					
					STXXL_VERBOSE1("btree::normal_node split leaf "<<this
						<<" splitter: "<<result.first)
				}
				
				return result;
			}

public:
			virtual ~normal_node()
			{
				delete block_;
			}
			
			normal_node(	btree_type * btree__,
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
			
			block_type & block()
			{
				return *block_;
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
			normal_node(InputIterator begin_, InputIterator end_,
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
				assert(unsigned(block_type::size) >= nelements +1); // extra space for an overflow
				
				unsigned new_size = end_ - begin_;
				assert(new_size <= max_nelements_);
				assert(new_size >= min_nelements_);
				
				std::copy(begin_,end_,block_->begin());
				assert(stxxl::is_sorted(block_->begin(),block_->begin() + new_size, vcmp_));
				block_->info.cur_size = new_size;
			}
			
			/*
			
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
				assert(bid == my_bid());
			}
			
			void init(const bid_type & my_bid_)
			{
				block_->info.me = my_bid_;
				block_->info.cur_size = 0;
			}
			
			std::pair<iterator,bool> insert(
					const btree_value_type & x,
					unsigned height,
					std::pair<key_type,bid_type> & splitter)
			{
				assert(size() <= max_nelements_);
				splitter.first = key_compare::max_value();
				
				value_type Key2Search(x.first,bid_type());
				typename block_type::iterator it = 
					std::lower_bound(block_->begin(),block_->begin() + size(), Key2Search ,vcmp_);
				
				assert(it != (block_->begin() + size()));
				
				bid_type found_bid = it->second;
				
				if(height == 2) // found_bid points to a leaf
				{
					STXXL_VERBOSE1("btree::normal_node Inserting new value into a leaf");
					leaf_type * Leaf = btree_->leaf_cache_.get_node((leaf_bid_type)it->second,true);
					assert(Leaf);
					std::pair<key_type,leaf_bid_type> BotSplitter;
					std::pair<iterator, bool> result = Leaf->insert(x,BotSplitter);
					btree_->leaf_cache_.unfix_node((leaf_bid_type)it->second);
					if(key_compare::max_value() == BotSplitter.first)
						return result;	// no overflow/splitting happened
					
					STXXL_VERBOSE1("btree::normal_node Inserting new value in *this");
					
					splitter = insert(BotSplitter,it);
					
					return result;	
				}
				else
				{	// found_bid points to a node
					
					STXXL_VERBOSE1("btree::normal_node Inserting new value into a node");
					node_type * Node = btree_->node_cache_.get_node((node_bid_type)it->second,true);
					assert(Node);
					std::pair<key_type,node_bid_type> BotSplitter;
					std::pair<iterator, bool> result = Node->insert(x,height-1,BotSplitter);
					btree_->node_cache_.unfix_node((node_bid_type)it->second);
					if(key_compare::max_value() == BotSplitter.first)
						return result;	// no overflow/splitting happened
					
					STXXL_VERBOSE1("btree::normal_node Inserting new value in *this");
					
					splitter = insert(BotSplitter,it);
					
					return result;	
				}
				
			}
			
			iterator begin(unsigned height)
			{
				bid_type FirstBid = block_->begin()->second;
				if(height == 2) // FirstBid points to a leaf
				{
					assert(size() > 1);
					STXXL_VERBOSE1("btree::node retrieveing begin() from the first leaf");
					leaf_type * Leaf = btree_->leaf_cache_.get_node((leaf_bid_type)FirstBid,true);
					assert(Leaf);
					return Leaf->begin();
				}
				else
				{ // FirstBid points to a node
					STXXL_VERBOSE1("btree: retrieveing begin() from the first node");
					node_type * Node = btree_->node_cache_.get_node((node_bid_type)FirstBid,true);
					assert(Node);
					iterator result = Node->begin(height-1);
					btree_->node_cache_.unfix_node((node_bid_type)FirstBid);
					return result;
				}
			}
			
		iterator find(const key_type & k, unsigned height)
		{
			value_type Key2Search(k,bid_type());
			
			typename block_type::iterator it = 
				std::lower_bound(block_->begin(),block_->begin() + size(), Key2Search ,vcmp_);
				
			assert(it != (block_->begin() + size()));
				
			bid_type found_bid = it->second;
			
			if(height == 2) // found_bid points to a leaf
			{
				STXXL_VERBOSE1("Searching in a leaf");
				leaf_type * Leaf = btree_->leaf_cache_.get_node((leaf_bid_type)found_bid,true);
				assert(Leaf);
				iterator result = Leaf->find(k);
				btree_->leaf_cache_.unfix_node((leaf_bid_type)found_bid);
				
				return result;
			}
			
			// found_bid points to a node
			STXXL_VERBOSE1("Searching in a node");
			node_type * Node = btree_->node_cache_.get_node((node_bid_type)found_bid,true);
			assert(Node);
			iterator result = Node->find(k,height-1);
			btree_->node_cache_.unfix_node((node_bid_type)found_bid);
			
			return result;

		}
			
	};
};


__STXXL_END_NAMESPACE


#endif /* _NODE_H */
