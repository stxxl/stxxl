/***************************************************************************
 *            btree.h
 *
 *  Tue Feb 14 19:02:35 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _BTREE_H
#define _BTREE_H

#include <stxxl>

#include "iterator.h"
#include "iterator_map.h"
#include "leaf.h"
#include "node_cache.h"
#include "root_node.h"
#include "node.h"

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	template <	class KeyType, 
						class DataType, 
						class CompareType, 
						unsigned LogNodeSize,
						unsigned LogLeafSize,
						class PDAllocStrategy
					>
	class btree
	{
		public:
			typedef KeyType key_type;
			typedef DataType	data_type;
			typedef CompareType key_compare;
			enum {
				log_node_size = LogNodeSize,
				log_leaf_size = LogLeafSize
			};
		typedef btree<KeyType,DataType,CompareType,LogNodeSize,LogLeafSize,PDAllocStrategy> SelfType;
			
		typedef PDAllocStrategy alloc_strategy_type;
			
		typedef stxxl::uint64	size_type;
		typedef std::pair<const key_type,data_type> value_type;
		typedef value_type & reference;
		typedef const value_type & const_reference;
		typedef value_type * pointer;
			
		// leaf type declarations
		typedef normal_leaf<key_type,data_type,key_compare,log_leaf_size,SelfType> leaf_type;
		friend class normal_leaf<key_type,data_type,key_compare,log_leaf_size,SelfType>;
		typedef typename leaf_type::bid_type leaf_bid_type;
		typedef node_cache<leaf_type,SelfType> leaf_cache_type;
		friend class node_cache<leaf_type,SelfType>;
		// iterator types
		typedef btree_iterator<SelfType> iterator;
		typedef btree_const_iterator<SelfType> const_iterator;
		friend class btree_iterator_base<SelfType>;
		// iterator map type
		typedef iterator_map<SelfType> iterator_map_type;
		// node type declarations
		typedef normal_node<key_type,key_compare,log_node_size,SelfType> node_type;
		typedef typename node_type::block_type node_block_type;
		friend class normal_node<key_type,key_compare,log_node_size,SelfType>;
		typedef typename node_type::bid_type node_bid_type;
		typedef node_cache<node_type,SelfType> node_cache_type;
		friend class node_cache<node_type,SelfType>;
		
		
	private:
		key_compare key_compare_;
		node_cache_type node_cache_;
		leaf_cache_type leaf_cache_;
		iterator_map_type iterator_map_;
		size_type size_;
		unsigned height_;
		block_manager * bm_;
		alloc_strategy_type alloc_strategy_;
	
		typedef std::map<key_type,node_bid_type,key_compare> root_node_type;
		typedef typename root_node_type::iterator root_node_iterator_type;
		typedef std::pair<key_type,node_bid_type> root_node_pair_type;
		root_node_type root_node_;
	
		iterator end_iterator;
		
	
		btree() {}
			
		template <class BIDType>
		void insert_into_root(const std::pair<key_type,BIDType> & splitter)
		{
			std::pair<root_node_iterator_type,bool> result = 
				root_node_.insert(splitter);
			assert(result.second == true);
			if(root_node_.size() >= (1<<LogLeafSize)) // root overflow
			{
				STXXL_VERBOSE1("btree::insert_into_root, overlow happened, splitting")
				
				node_bid_type LeftBid;
				node_type * LeftNode = node_cache_.get_new_node(LeftBid);
				assert(LeftNode);
				node_bid_type RightBid;
				node_type * RightNode = node_cache_.get_new_node(RightBid);
				assert(RightNode);
				
				const unsigned old_size = root_node_.size();
				const unsigned half = root_node_.size()/2;
				unsigned i = 0;
				root_node_iterator_type it=root_node_.begin();
				typename node_block_type::iterator block_it = LeftNode->block().begin();
				while(i<half) // copy smaller part
				{
					*block_it = *it;
					++i;
					++block_it;
					++it;
				}
				LeftNode->block().info.cur_size = half;
				key_type LeftKey = (LeftNode->block()[half-1]).first;
				
				block_it = RightNode->block().begin();
				while(i<old_size) // copy larger part
				{
					*block_it = *it;
					++i;
					++block_it;
					++it;
				}
				unsigned right_size = RightNode->block().info.cur_size = old_size - half;
				key_type RightKey = (RightNode->block()[right_size-1]).first;
				
				assert(old_size == RightNode->size() + LeftNode->size());
				
				// create new root node
				root_node_.clear(); 
				root_node_.insert(root_node_pair_type(LeftKey,LeftBid));
				root_node_.insert(root_node_pair_type(RightKey,RightBid));
				
				
				++height_;
				STXXL_MSG("btree Increasing height to "<<height_)
				assert(node_cache_.size()>= (height_-1));
			}
		}
		
		template <class CacheType>
		void fuse_or_balance(root_node_iterator_type UIt, CacheType & cache_)
		{
			typedef typename CacheType::node_type local_node_type;
			typedef typename local_node_type::bid_type local_bid_type;
			
			root_node_iterator_type leftIt,rightIt;
			if(UIt->first == key_compare::max_value()) // UIt is the last entry in the root
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
			local_node_type * LeftNode = cache_.get_node(LeftBid,true);
			local_node_type * RightNode = cache_.get_node(RightBid,true);
			
			const unsigned TotalSize = LeftNode->size() + RightNode->size();
			if(TotalSize <= RightNode->max_nelements())
			{
				// fuse
				RightNode->fuse(*LeftNode); // add the content of LeftNode to RightNode
				
				cache_.unfix_node(RightBid);
				cache_.delete_node(LeftBid); // 'delete_node' unfixes LeftBid also
				
				root_node_.erase(leftIt); // delete left BID from the root
			}
			else
			{
				// balance
				
				key_type NewSplitter = RightNode->balance(*LeftNode);
				
				root_node_.erase(leftIt); // delete left BID from the root
				// reinsert with the new key
				root_node_.insert(root_node_pair_type(NewSplitter,(node_bid_type)LeftBid)); 
			
				cache_.unfix_node(LeftBid);
				cache_.unfix_node(RightBid);				
				
			}
		}
		
	public:
		btree(	unsigned node_cache_size_in_bytes,
					unsigned leaf_cache_size_in_bytes
				): 
			node_cache_(node_cache_size_in_bytes,this,1<<(LogNodeSize-1),1<<LogNodeSize,key_compare_),
			leaf_cache_(leaf_cache_size_in_bytes,this,1<<(LogLeafSize-1),1<<LogLeafSize,key_compare_),
			size_(0),
			height_(2),
			bm_(block_manager::get_instance())
		{
			STXXL_VERBOSE1("Creating a btree, addr="<<this)
			STXXL_VERBOSE1(" bytes in a node: "<<node_bid_type::size)
			STXXL_VERBOSE1(" bytes in a leaf: "<<leaf_bid_type::size)
			leaf_bid_type NewBid;
			leaf_type * NewLeaf = leaf_cache_.get_new_node(NewBid);
			assert(NewLeaf);
			
			end_iterator = NewLeaf->end(); // initialize end() iterator
			
			root_node_.insert(root_node_pair_type(key_compare::max_value(),(node_bid_type)NewBid));
		}
		
		size_type size() const
		{
			return size_;
		}
		
		bool empty() const
		{
			return !size_;
		}
		
		std::pair<iterator, bool> insert(const value_type & x)
		{
			root_node_iterator_type it = root_node_.lower_bound(x.first);
			assert(it != root_node_.end());
			if(height_ == 2) // 'it' points to a leaf
			{
				STXXL_VERBOSE1("Inserting new value into a leaf");
				leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second,true);
				assert(Leaf);
				std::pair<key_type,leaf_bid_type> Splitter;
				std::pair<iterator, bool> result = Leaf->insert(x,Splitter);
				if(result.second) ++size_;	
				leaf_cache_.unfix_node((leaf_bid_type)it->second);
				if(key_compare::max_value() == Splitter.first)
					return result;	// no overflow/splitting happened
				
				STXXL_VERBOSE1("Inserting new value into root node");
				
				insert_into_root(Splitter);
				
				return result;
			}
			
			// 'it' points to a node
			STXXL_VERBOSE1("Inserting new value into a node");
			node_type * Node = node_cache_.get_node((node_bid_type)it->second,true);
			assert(Node);
			std::pair<key_type,node_bid_type> Splitter;
			std::pair<iterator, bool> result = Node->insert(x,height_-1,Splitter);
			if(result.second) ++size_;	
			node_cache_.unfix_node((node_bid_type)it->second);
			if(key_compare::max_value() == Splitter.first)
				return result;	// no overflow/splitting happened
			
			STXXL_VERBOSE1("Inserting new value into root node");
				
			insert_into_root(Splitter);
				
			return result;
				
		}
		
		iterator begin()
		{
			root_node_iterator_type it = root_node_.begin();
			assert(it != root_node_.end() );			
			
			if(height_ == 2) // 'it' points to a leaf
			{
				STXXL_VERBOSE1("btree: retrieveing begin() from the first leaf");
				leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second,true);
				assert(Leaf);
				return Leaf->begin();
			}
			
			// 'it' points to a node
			STXXL_VERBOSE1("btree: retrieveing begin() from the first node");
			node_type * Node = node_cache_.get_node((node_bid_type)it->second,true);
			assert(Node);
			iterator result = Node->begin(height_-1);
			node_cache_.unfix_node((node_bid_type)it->second);
			return result;
			
		}
		
		iterator end()
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
			
			if(height_ == 2) // 'it' points to a leaf
			{
				STXXL_VERBOSE1("Searching in a leaf");
				leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second,true);
				assert(Leaf);
				iterator result = Leaf->find(k);
				leaf_cache_.unfix_node((leaf_bid_type)it->second);
				assert(result == end() || result->first == k);
				return result;
			}
			
			// 'it' points to a node
			STXXL_VERBOSE1("Searching in a node");
			node_type * Node = node_cache_.get_node((node_bid_type)it->second,true);
			assert(Node);
			iterator result = Node->find(k,height_-1);
			node_cache_.unfix_node((node_bid_type)it->second);
			
			assert(result == end() || result->first == k);
			return result;
		}
		
		size_type erase(const key_type & k)
		{
			root_node_iterator_type it = root_node_.lower_bound(k);
			assert(it != root_node_.end());
			if(height_ == 2) // 'it' points to a leaf
			{
				STXXL_VERBOSE1("Deleting key from a leaf")
				leaf_type * Leaf = leaf_cache_.get_node((leaf_bid_type)it->second,true);
				assert(Leaf);
				size_type result = Leaf->erase(k);
				size_ -= result;
				leaf_cache_.unfix_node((leaf_bid_type)it->second);
				if((!Leaf->underflows()) || root_node_.size() == 1)
					return result;	// no underflow or root has a special degree 1 (too few elements)
				
				STXXL_VERBOSE1("Fusing or rebalancing a leaf")
				fuse_or_balance(it,leaf_cache_);
				
				return result;
			}
			
			// 'it' points to a node
			STXXL_VERBOSE1("Deleting key from a node")
			assert(root_node_.size()>=2);
			node_type * Node = node_cache_.get_node((node_bid_type)it->second,true);
			assert(Node);
			size_type result = Node->erase(k,height_-1);
			size_ -= result;
			node_cache_.unfix_node((node_bid_type)it->second);
			if(!Node->underflows())
				return result;	// no underflow happened
			
			STXXL_VERBOSE1("Fusing or rebalancing a node")
			fuse_or_balance(it,node_cache_);
				
			return result;
		}
	
	};
	
}

__STXXL_END_NAMESPACE

#endif /* _BTREE_H */
