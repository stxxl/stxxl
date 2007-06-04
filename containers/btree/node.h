/***************************************************************************
 *            node.h
 *
 *  Tue Feb 14 21:02:51 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _NODE_H
#define _NODE_H


#include "iterator.h"
#include "node_cache.h"

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	template <class NodeType, class BTreeType>
	class node_cache;
	
	template <class KeyType_, class KeyCmp_, unsigned RawSize_, class BTreeType>
	class normal_node
	{
			
		public:
			typedef	normal_node<KeyType_,KeyCmp_,RawSize_,BTreeType> SelfType;
		
			friend class node_cache<SelfType,BTreeType>;
		
			typedef KeyType_ key_type;
			typedef KeyCmp_ key_compare;
			
			enum {
					raw_size = RawSize_
			};
			typedef BID<raw_size> bid_type;
			typedef bid_type 	node_bid_type;
			typedef SelfType	node_type;
			typedef std::pair<key_type,bid_type>  value_type;
			typedef value_type  & reference;
			typedef const value_type  & const_reference;
			
			
			struct InfoType
			{
				bid_type me;
				unsigned cur_size;
			};
			typedef typed_block<raw_size,value_type,0,InfoType> block_type;		

			enum {
				nelements = block_type::size -1,
				max_size = nelements,
				min_size = nelements/2
			};				
			typedef typename block_type::iterator block_iterator;
			typedef typename block_type::const_iterator block_const_iterator;
			
			typedef BTreeType btree_type;
			typedef typename btree_type::size_type size_type;
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
				
				if(size() > max_nelements()) // overflow! need to split
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
			
			template <class CacheType>
			void fuse_or_balance(block_iterator UIt, CacheType & cache_)
			{
				typedef typename CacheType::node_type local_node_type;
				typedef typename local_node_type::bid_type local_bid_type;
				
				block_iterator leftIt,rightIt;
				if(UIt == (block_->begin() + size()-1) ) // UIt is the last entry in the root
				{
					assert(UIt != block_->begin());
					rightIt = UIt;
					leftIt = --UIt;
				}
				else
				{
					leftIt = UIt;
					rightIt = ++UIt;
					assert(rightIt != (block_->begin() + size()));
				}
				
				// now fuse or balance nodes pointed by leftIt and rightIt
				local_bid_type LeftBid = (local_bid_type) leftIt->second;
				local_bid_type RightBid = (local_bid_type) rightIt->second;
				local_node_type * LeftNode = cache_.get_node(LeftBid,true);
				local_node_type * RightNode = cache_.get_node(RightBid,true);
				
				const unsigned TotalSize = LeftNode->size() + RightNode->size();
				if(TotalSize <= RightNode->max_nelements())
				{
					// fuse
					RightNode->fuse(*LeftNode); // add the content of LeftNode to RightNode
					
					cache_.unfix_node(RightBid);
					cache_.delete_node(LeftBid); // 'delete_node' unfixes LeftBid also
					
					std::copy(leftIt + 1, block_->begin() + size(),leftIt ); // delete left BID from the root
					--(block_->info.cur_size);
				}
				else
				{
					// balance
					
					key_type NewSplitter = RightNode->balance(*LeftNode);
					
					
					leftIt->first = NewSplitter; // change key
					assert(vcmp_(*leftIt,*rightIt));
				
					cache_.unfix_node(LeftBid);
					cache_.unfix_node(RightBid);				
					
				}
			}

public:
			virtual ~normal_node()
			{
				delete block_;
			}
			
			normal_node(	btree_type * btree__,
								key_compare cmp): 
				block_(new block_type),
				btree_(btree__),
				cmp_(cmp),
				vcmp_(cmp)
			{
				assert(min_nelements() >=2);
				assert(2*min_nelements() - 1 <= max_nelements());
				assert(max_nelements() <= nelements);
				assert(unsigned(block_type::size) >= nelements +1); // extra space for an overflow
			}
			
			block_type & block()
			{
				return *block_;
			}
			
			bool overflows () const { return block_->info.cur_size > max_nelements(); }
			bool underflows () const { return block_->info.cur_size < min_nelements(); }
			
			unsigned max_nelements() const { return max_size; }
			unsigned min_nelements() const { return min_size; }
			
			/*
			template <class InputIterator>
			normal_node(InputIterator begin_, InputIterator end_,
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
				assert(unsigned(block_type::size) >= nelements +1); // extra space for an overflow
				
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
			
			bid_type my_bid() const 
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
				return (*block_)[size()-1];
			}
			
			reference front()
			{
				return *(block_->begin());
			}
			
			const_reference back() const
			{
				return (*block_)[size()-1];
			}
			
			const_reference front() const
			{
				return *(block_->begin());
			}
			
			
			std::pair<iterator,bool> insert(
					const btree_value_type & x,
					unsigned height,
					std::pair<key_type,bid_type> & splitter)
			{
				assert(size() <= max_nelements());
				splitter.first = key_compare::max_value();
				
				value_type Key2Search(x.first,bid_type());
				block_iterator it = 
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
					//if(key_compare::max_value() == BotSplitter.first)
					if(!(cmp_(key_compare::max_value(),BotSplitter.first) ||
						 cmp_(BotSplitter.first,key_compare::max_value()) ))
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
					//if(key_compare::max_value() == BotSplitter.first)
					if(!(cmp_(key_compare::max_value(),BotSplitter.first) ||
						 cmp_(BotSplitter.first,key_compare::max_value()) ))
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
					leaf_type * Leaf = btree_->leaf_cache_.get_node((leaf_bid_type)FirstBid);
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
			
			const_iterator begin(unsigned height) const
			{
				bid_type FirstBid = block_->begin()->second;
				if(height == 2) // FirstBid points to a leaf
				{
					assert(size() > 1);
					STXXL_VERBOSE1("btree::node retrieveing begin() from the first leaf");
					leaf_type const * Leaf = btree_->leaf_cache_.get_const_node((leaf_bid_type)FirstBid);
					assert(Leaf);
					return Leaf->begin();
				}
				else
				{ // FirstBid points to a node
					STXXL_VERBOSE1("btree: retrieveing begin() from the first node");
					node_type const * Node = btree_->node_cache_.get_const_node((node_bid_type)FirstBid,true);
					assert(Node);
					const_iterator result = Node->begin(height-1);
					btree_->node_cache_.unfix_node((node_bid_type)FirstBid);
					return result;
				}
			}
			
		iterator find(const key_type & k, unsigned height)
		{
			value_type Key2Search(k,bid_type());
			
			block_iterator it = 
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
		
		const_iterator find(const key_type & k, unsigned height) const
		{
			value_type Key2Search(k,bid_type());
			
			block_iterator it = 
				std::lower_bound(block_->begin(),block_->begin() + size(), Key2Search ,vcmp_);
				
			assert(it != (block_->begin() + size()));
				
			bid_type found_bid = it->second;
			
			if(height == 2) // found_bid points to a leaf
			{
				STXXL_VERBOSE1("Searching in a leaf");
				leaf_type const * Leaf = btree_->leaf_cache_.get_const_node((leaf_bid_type)found_bid,true);
				assert(Leaf);
				const_iterator result = Leaf->find(k);
				btree_->leaf_cache_.unfix_node((leaf_bid_type)found_bid);
				
				return result;
			}
			
			// found_bid points to a node
			STXXL_VERBOSE1("Searching in a node");
			node_type const * Node = btree_->node_cache_.get_const_node((node_bid_type)found_bid,true);
			assert(Node);
			const_iterator result = Node->find(k,height-1);
			btree_->node_cache_.unfix_node((node_bid_type)found_bid);
			
			return result;

		}
		
		iterator lower_bound(const key_type & k, unsigned height)
		{
			value_type Key2Search(k,bid_type());
			assert(!vcmp_(back(),Key2Search));
			block_iterator it = 
				std::lower_bound(block_->begin(),block_->begin() + size(), Key2Search ,vcmp_);
				
			assert(it != (block_->begin() + size()));
				
			bid_type found_bid = it->second;
			
			if(height == 2) // found_bid points to a leaf
			{
				STXXL_VERBOSE1("Searching lower bound in a leaf");
				leaf_type * Leaf = btree_->leaf_cache_.get_node((leaf_bid_type)found_bid,true);
				assert(Leaf);
				iterator result = Leaf->lower_bound(k);
				btree_->leaf_cache_.unfix_node((leaf_bid_type)found_bid);
				
				return result;
			}
			
			// found_bid points to a node
			STXXL_VERBOSE1("Searching lower bound in a node");
			node_type * Node = btree_->node_cache_.get_node((node_bid_type)found_bid,true);
			assert(Node);
			iterator result = Node->lower_bound(k,height-1);
			btree_->node_cache_.unfix_node((node_bid_type)found_bid);
			
			return result;

		}
		
		const_iterator lower_bound(const key_type & k, unsigned height) const
		{
			value_type Key2Search(k,bid_type());
			assert(!vcmp_(back(),Key2Search));
			block_iterator it = 
				std::lower_bound(block_->begin(),block_->begin() + size(), Key2Search ,vcmp_);
				
			assert(it != (block_->begin() + size()));
				
			bid_type found_bid = it->second;
			
			if(height == 2) // found_bid points to a leaf
			{
				STXXL_VERBOSE1("Searching lower bound in a leaf");
				leaf_type const * Leaf = btree_->leaf_cache_.get_const_node((leaf_bid_type)found_bid,true);
				assert(Leaf);
				const_iterator result = Leaf->lower_bound(k);
				btree_->leaf_cache_.unfix_node((leaf_bid_type)found_bid);
				
				return result;
			}
			
			// found_bid points to a node
			STXXL_VERBOSE1("Searching lower bound in a node");
			node_type const * Node = btree_->node_cache_.get_const_node((node_bid_type)found_bid,true);
			assert(Node);
			const_iterator result = Node->lower_bound(k,height-1);
			btree_->node_cache_.unfix_node((node_bid_type)found_bid);
			
			return result;

		}
		
		iterator upper_bound(const key_type & k, unsigned height)
		{
			value_type Key2Search(k,bid_type());
			assert(vcmp_(Key2Search,back()));
			block_iterator it = 
				std::upper_bound(block_->begin(),block_->begin() + size(), Key2Search ,vcmp_);
				
			assert(it != (block_->begin() + size()));
				
			bid_type found_bid = it->second;
			
			if(height == 2) // found_bid points to a leaf
			{
				STXXL_VERBOSE1("Searching upper bound in a leaf");
				leaf_type * Leaf = btree_->leaf_cache_.get_node((leaf_bid_type)found_bid,true);
				assert(Leaf);
				iterator result = Leaf->upper_bound(k);
				btree_->leaf_cache_.unfix_node((leaf_bid_type)found_bid);
				
				return result;
			}
			
			// found_bid points to a node
			STXXL_VERBOSE1("Searching upper bound in a node");
			node_type * Node = btree_->node_cache_.get_node((node_bid_type)found_bid,true);
			assert(Node);
			iterator result = Node->upper_bound(k,height-1);
			btree_->node_cache_.unfix_node((node_bid_type)found_bid);
			
			return result;

		}
		
		const_iterator upper_bound(const key_type & k, unsigned height) const
		{
			value_type Key2Search(k,bid_type());
			assert(vcmp_(Key2Search,back()));
			block_iterator it = 
				std::upper_bound(block_->begin(),block_->begin() + size(), Key2Search ,vcmp_);
				
			assert(it != (block_->begin() + size()));
				
			bid_type found_bid = it->second;
			
			if(height == 2) // found_bid points to a leaf
			{
				STXXL_VERBOSE1("Searching upper bound in a leaf");
				leaf_type const * Leaf = btree_->leaf_cache_.get_const_node((leaf_bid_type)found_bid,true);
				assert(Leaf);
				const_iterator result = Leaf->upper_bound(k);
				btree_->leaf_cache_.unfix_node((leaf_bid_type)found_bid);
				
				return result;
			}
			
			// found_bid points to a node
			STXXL_VERBOSE1("Searching upper bound in a node");
			node_type const * Node = btree_->node_cache_.get_const_node((node_bid_type)found_bid,true);
			assert(Node);
			const_iterator result = Node->upper_bound(k,height-1);
			btree_->node_cache_.unfix_node((node_bid_type)found_bid);
			
			return result;

		}
		
			void fuse(const normal_node & Src)
			{
				assert(vcmp_(Src.back(),front()));
				const unsigned SrcSize = Src.size();
				
				block_iterator cur = block_->begin() + size() - 1;
				block_const_iterator begin = block_->begin() ;

				for(; cur>= begin; --cur)
					*(cur+SrcSize) = *cur;  // move elements to make space for Src elements
				
				// copy Src to *this leaf
				std::copy(Src.block_->begin(), Src.block_->begin() + SrcSize, block_->begin());
				
				block_->info.cur_size += SrcSize;
			}
			
			
			key_type balance(normal_node & Left)
			{
				const unsigned TotalSize =Left.size() + size();
				unsigned newLeftSize = TotalSize/2;
				assert(newLeftSize <= Left.max_nelements());
				assert(newLeftSize >= Left.min_nelements());
				unsigned newRightSize = TotalSize - newLeftSize;
				assert(newRightSize <= max_nelements());
				assert(newRightSize >= min_nelements());
				
				assert(vcmp_(Left.back(),front()) || size()==0);
				
				if(newLeftSize < Left.size())
				{	
					const unsigned nEl2Move = Left.size() - newLeftSize;// #elements to move from Left to *this
					
					block_iterator cur = block_->begin() + size() - 1;
					block_const_iterator begin = block_->begin() ;
	
					for(; cur>= begin; --cur)
						*(cur+nEl2Move) = *cur;  // move elements to make space for Src elements
					
					// copy Left to *this leaf
					std::copy(	Left.block_->begin() + newLeftSize, 
									Left.block_->begin() + Left.size(), block_->begin());

				}
				else
				{
					assert(newRightSize < size());
					
					const unsigned nEl2Move = size() - newRightSize;// #elements to move from *this to Left
					
					// copy *this to Left
					std::copy(	block_->begin(), 
									block_->begin() + nEl2Move, Left.block_->begin() + Left.size());
					// move elements in *this
					std::copy(	block_->begin() + nEl2Move, 
									block_->begin() + size(), block_->begin() );
										
				}
				
				block_->info.cur_size = newRightSize; // update size
				Left.block_->info.cur_size = newLeftSize; // update size
				
				return Left.back().first;
			}
			
			size_type erase(const key_type & k, unsigned height)
			{
				value_type Key2Search(k,bid_type());
			
				block_iterator it = 
					std::lower_bound(block_->begin(),block_->begin() + size(), Key2Search ,vcmp_);
				
				assert(it != (block_->begin() + size()));
				
				bid_type found_bid = it->second;
				
				assert(size()>=2);
				
				if(height == 2) // 'found_bid' points to a leaf
				{
					STXXL_VERBOSE1("btree::normal_node Deleting key from a leaf")
					leaf_type * Leaf = btree_->leaf_cache_.get_node((leaf_bid_type)found_bid,true);
					assert(Leaf);
					size_type result = Leaf->erase(k);
					btree_->leaf_cache_.unfix_node((leaf_bid_type)it->second);
					if(!Leaf->underflows())
						return result;	// no underflow or root has a special degree 1 (too few elements)
					
					STXXL_VERBOSE1("btree::normal_node Fusing or rebalancing a leaf")
					fuse_or_balance(it,btree_->leaf_cache_);
					
					return result;
				}
				
				// 'found_bid' points to a node
				STXXL_VERBOSE1("btree::normal_node Deleting key from a node")
				node_type * Node = btree_->node_cache_.get_node((node_bid_type)found_bid,true);
				assert(Node);
				size_type result = Node->erase(k,height-1);
				btree_->node_cache_.unfix_node((node_bid_type)found_bid);
				if(!Node->underflows())
					return result;	// no underflow happened
				
				STXXL_VERBOSE1("btree::normal_node Fusing or rebalancing a node")
				fuse_or_balance(it,btree_->node_cache_);
					
				return result;
			}
			
			void deallocate_children(unsigned height)
			{
				if(height == 2)
				{
					// we have children leaves here
					block_const_iterator it = block().begin();
					for(;it!=block().begin() + size();++it)
					{
						// delete from leaf cache and deallocate bid
						btree_->leaf_cache_.delete_node((leaf_bid_type)it->second); 
					}
				}
				else
				{
					block_const_iterator it = block().begin();
					for(;it!=block().begin() + size();++it)
					{
						node_type * Node = btree_->node_cache_.get_node((node_bid_type)it->second);
						assert(Node);
						Node->deallocate_children(height-1);
						// delete from node cache and deallocate bid
						btree_->node_cache_.delete_node((node_bid_type)it->second); 
					}
				}
			}
			
			void push_back(const value_type & x)
			{
				(*this)[size()] = x;
				++(block_->info.cur_size);
			}
			
	};
};


__STXXL_END_NAMESPACE


#endif /* _NODE_H */
