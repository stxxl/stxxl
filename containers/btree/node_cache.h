/***************************************************************************
 *            node_cache.h
 *
 *  Tue Feb 14 20:30:15 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _NODE_CACHE_H
#define _NODE_CACHE_H

#include <stxxl>

#ifdef BOOST_MSVC
#include <hash_map>
#else
#include <ext/hash_map>
#endif

#include "btree_pager.h"

__STXXL_BEGIN_NAMESPACE

// TODO:  speedup BID2node_ access using search result iterator in the methods

namespace btree
{
	template <class NodeType, class BTreeType>
	class node_cache
	{
			node_cache();
			node_cache(const node_cache &);
			node_cache & operator =(const node_cache &);
		
		public:
			typedef BTreeType btree_type;
			typedef NodeType node_type;
			typedef typename node_type::block_type block_type;
			typedef typename node_type::bid_type bid_type;
			typedef typename btree_type::key_compare key_compare;
		
			typedef typename btree_type::alloc_strategy_type alloc_strategy_type;
			typedef stxxl::btree::lru_pager pager_type;
		
		private:
		
			btree_type * btree_;
			unsigned min_node_elements_;
			unsigned max_node_elements_;
			key_compare comp_;
		
			struct bid_comp
			{
  				bool operator () (const bid_type & a, const bid_type & b) const
  				{
      				return (a.storage < b.storage) || ( a.storage == b.storage && a.offset < b.offset);
 				}
			};
			
			struct bid_hash
		  	{
					size_t operator()(const bid_type & bid) const
					{
				  		size_t result = size_t(bid.storage) +
							size_t(bid.offset & 0xffffffff) + size_t(bid.offset>>32);
					  	return result;
					}
					#ifdef BOOST_MSVC
					bool operator () (const bid_type & a, const bid_type & b) const
  					{
	      				return (a.storage < b.storage) || ( a.storage == b.storage && a.offset < b.offset);
 					}
					enum
					{       // parameters for hash table
							bucket_size = 4,        // 0 < bucket_size
							min_buckets = 8
					};      // min_buckets = 2 ^^ N, 0 < N
					#endif
			};
					
			std::vector<node_type *> nodes_;
			std::vector<bool> fixed_;
			std::vector<bool> dirty_;
			std::vector<int> free_nodes_;
			#ifdef BOOST_MSVC
  			typedef stdext::hash_map < bid_type, int , bid_hash > hash_map_type;
  			#else
  			typedef __gnu_cxx::hash_map < bid_type, int , bid_hash > hash_map_type;
  			#endif
			
			//typedef std::map<bid_type,int,bid_comp> BID2node_type;
			typedef hash_map_type BID2node_type;
			
			BID2node_type BID2node_;
			stxxl::btree::lru_pager pager_;	
			block_manager * bm;
			alloc_strategy_type alloc_strategy_;
			
			// changes btree pointer in all contained iterators
			void change_btree_pointers(btree_type * b)
			{
				typename std::vector<node_type *>::const_iterator it = nodes_.begin();
				for(;it!=nodes_.end();++it)
				{
					(*it)->btree_ = b;
				}
			}
		
		public:
			node_cache(	unsigned cache_size_in_bytes,
								btree_type * btree__,
								unsigned min_node_elements__,
								unsigned max_node_elements__,
								key_compare comp__
								): 
					btree_(btree__),
					min_node_elements_(min_node_elements__),
					max_node_elements_(max_node_elements__),
					comp_(comp__),
					bm(block_manager::get_instance())
			{
				const unsigned nnodes = cache_size_in_bytes/block_type::raw_size;
				STXXL_VERBOSE1("btree::node_cache constructor nodes="<<nnodes)
				assert(nnodes >= 3);
				nodes_.reserve(nnodes);
				free_nodes_.reserve(nnodes);
				fixed_.resize(nnodes,false);
				dirty_.resize(nnodes,true);
				for(unsigned i= 0; i<nnodes;++i)
				{
					nodes_.push_back(new node_type(btree_,min_node_elements_,max_node_elements_,comp_));
					free_nodes_.push_back(i);
				}
				
				pager_type tmp_pager(nnodes);
				std::swap(pager_,tmp_pager);
				
			}
			
			unsigned size() const 
			{
				return nodes_.size();
			}
			
			// returns the number of fixed pages
			unsigned nfixed() const 
			{
				typename BID2node_type::const_iterator i = BID2node_.begin();
				typename BID2node_type::const_iterator end = BID2node_.end();
				unsigned cnt = 0;
				for(;i!=end;++i)
				{
					if(fixed_[(*i).second]) ++cnt;
				}
				return cnt;
			}
			
			~node_cache()
			{
				STXXL_VERBOSE1("btree::node_cache deconstructor addr="<<this)
				typename BID2node_type::const_iterator i = BID2node_.begin();
				typename BID2node_type::const_iterator end = BID2node_.end();
				for(;i!=end;++i)
				{
					const unsigned p = (*i).second;
					if(dirty_[p]) nodes_[p]->save();
				}
				for(int i=0;i<size();++i)
					delete nodes_[i];
			}
		
			node_type * get_new_node(bid_type & new_bid)
			{
				if(free_nodes_.empty())
				{
					// need to kick a node
					int node2kick, i = 0;
					unsigned int max_tries = size() +1;
					do
					{
						++i;
						node2kick = pager_.kick();
						if(i == max_tries )
						{
							STXXL_ERRMSG(
								"The node cache is too small, no node can be kicked out (all nodes are fixed) !");
							STXXL_ERRMSG("Returning NULL node.")
							return NULL;
						}
						pager_.hit(node2kick);
							
					} while (fixed_[node2kick]);
					
					node_type & Node = *(nodes_[node2kick]);
					
					if(dirty_[node2kick]) Node.save();
						
					BID2node_.erase(Node.my_bid());
					bm->new_blocks<block_type>(1,alloc_strategy_,&new_bid);
					
					BID2node_[new_bid] = node2kick;
					
					Node.init(new_bid);
					
					dirty_[node2kick] = true;
					
					assert(size() == BID2node_.size() + free_nodes_.size());
					
					
					STXXL_VERBOSE1("btree::node_cache get_new_node, need to kick node "<<node2kick)
					
					return &Node;
				}
				
				
				int free_node = free_nodes_.back();
				free_nodes_.pop_back();
				assert(fixed_[free_node] == false);
				
				bm->new_blocks<block_type>(1,alloc_strategy_,&new_bid);
				BID2node_[new_bid] = free_node;
				node_type & Node = *(nodes_[free_node]);
				Node.init(new_bid);
				
				pager_.hit(free_node);
				
				dirty_[free_node] = true;
				
				assert(size() == BID2node_.size() + free_nodes_.size());
				
				STXXL_VERBOSE1("btree::node_cache get_new_node, free node "<<free_node<<"available")
				
				return &Node;
			}
			
			
			
			node_type * get_node(const bid_type & bid, bool fix = false)
			{
				typename BID2node_type::const_iterator it = BID2node_.find(bid);
				if(it != BID2node_.end())
				{
					// the node is in cache
					const int nodeindex = it->second;
					STXXL_VERBOSE1("btree::node_cache get_node, the node "<<nodeindex<<"is in cache , fix="<<fix)
					fixed_[nodeindex] = fix;
					pager_.hit(nodeindex);
					dirty_[nodeindex] = true;
					return nodes_[nodeindex];
				}
					
				// the node is not in cache
				if(free_nodes_.empty())
				{
					// need to kick a node
					int node2kick, i = 0;
					const unsigned max_tries = size()+1;
					do
					{
						++i;
						node2kick = pager_.kick();
						if(i == max_tries)
						{
							STXXL_ERRMSG(
								"The node cache is too small, no node can be kicked out (all nodes are fixed) !");
							STXXL_ERRMSG("Returning NULL node.")
							return NULL;
						}
						pager_.hit(node2kick);
							
					} while (fixed_[node2kick]);
					
					node_type & Node = *(nodes_[node2kick]);
					
					if(dirty_[node2kick]) Node.save();
						
					BID2node_.erase(Node.my_bid());
					
					Node.load(bid);
					BID2node_[bid] = node2kick;
					
					fixed_[node2kick] = fix;
					
					dirty_[node2kick] = true;
					
					assert(size() == BID2node_.size() + free_nodes_.size());
					
					STXXL_VERBOSE1("btree::node_cache get_node, need to kick node"<<node2kick<<" fix="<<fix)
					
					return &Node;
				}
				
				int free_node = free_nodes_.back();
				free_nodes_.pop_back();
				assert(fixed_[free_node] == false);
				
				node_type & Node = *(nodes_[free_node]);
				Node.load(bid);
				BID2node_[bid] = free_node;
				
				pager_.hit(free_node);
				
				fixed_[free_node] = fix;
				
				dirty_[free_node] = true;
				
				assert(size() == BID2node_.size() + free_nodes_.size());
				
				STXXL_VERBOSE1("btree::node_cache get_node, free node "<< free_node<<"available, fix="<<fix)
				
				return &Node;
			}
			
			node_type const * get_const_node(const bid_type & bid, bool fix = false)
			{
				typename BID2node_type::const_iterator it = BID2node_.find(bid);
				if(it != BID2node_.end())
				{
					// the node is in cache
					const int nodeindex = it->second;
					STXXL_VERBOSE1("btree::node_cache get_node, the node "<<nodeindex<<"is in cache , fix="<<fix)
					fixed_[nodeindex] = fix;
					pager_.hit(nodeindex);
					
					return nodes_[nodeindex];
				}
					
				// the node is not in cache
				if(free_nodes_.empty())
				{
					// need to kick a node
					int node2kick, i = 0;
					const unsigned max_tries = size() +1;
					do
					{
						++i;
						node2kick = pager_.kick();
						if(i==max_tries)
						{
							STXXL_ERRMSG(
								"The node cache is too small, no node can be kicked out (all nodes are fixed) !");
							STXXL_ERRMSG("Returning NULL node.")
							return NULL;
						}
						pager_.hit(node2kick);
							
					} while (fixed_[node2kick]);
					
					node_type & Node = *(nodes_[node2kick]);
					if(dirty_[node2kick]) Node.save();
						
					BID2node_.erase(Node.my_bid());
					
					Node.load(bid);
					BID2node_[bid] = node2kick;
					
					fixed_[node2kick] = fix;
					
					dirty_[node2kick] = false;
					
					assert(size() == BID2node_.size() + free_nodes_.size());
					
					STXXL_VERBOSE1("btree::node_cache get_node, need to kick node"<<node2kick<<" fix="<<fix)
					
					return &Node;
				}
				
				int free_node = free_nodes_.back();
				free_nodes_.pop_back();
				assert(fixed_[free_node] == false);
				
				node_type & Node = *(nodes_[free_node]);
				Node.load(bid);
				BID2node_[bid] = free_node;
				
				pager_.hit(free_node);
				
				fixed_[free_node] = fix;
				
				dirty_[free_node] = false;
				
				assert(size() == BID2node_.size() + free_nodes_.size());
				
				STXXL_VERBOSE1("btree::node_cache get_node, free node "<< free_node<<"available, fix="<<fix)
				
				return &Node;
			}
			
			void delete_node(const bid_type & bid)
			{
				typename BID2node_type::const_iterator it = BID2node_.find(bid);
				if(it != BID2node_.end())
				{
					// the node is in the cache
					const int nodeindex = it->second;
					free_nodes_.push_back(nodeindex);
					BID2node_.erase(bid);
					fixed_[nodeindex] = false;
				}
				
				bm->delete_block(bid);
			}
			
			void unfix_node(const bid_type & bid)
			{
				assert(BID2node_.find(bid) != BID2node_.end());
				fixed_[BID2node_[bid] ] = false;
				STXXL_VERBOSE1("btree::node_cache unfix_node,  node "<< BID2node_[bid] )
			}			
			
			void swap(node_cache & obj)
			{
				//std::swap(btree_,obj.btree_);
				std::swap(min_node_elements_,obj.min_node_elements_);
				std::swap(max_node_elements_,obj.max_node_elements_);
				std::swap(comp_,obj.comp_);
				std::swap(nodes_,obj.nodes_);
				change_btree_pointers(btree_);
				obj.change_btree_pointers(obj.btree_);
				std::swap(fixed_,obj.fixed_);
				std::swap(free_nodes_,obj.free_nodes_);
				std::swap(BID2node_,obj.BID2node_);
				std::swap(pager_,obj.pager_);
				std::swap(alloc_strategy_,obj.alloc_strategy_);		
			}
	};
	
}

__STXXL_END_NAMESPACE


namespace std
{
	template <class NodeType, class BTreeType>
	void swap(stxxl::btree::node_cache<NodeType,BTreeType> & a,
					stxxl::btree::node_cache<NodeType,BTreeType> & b )
	{
		a.swap(b);
	}
}

#endif /* _NODE_CACHE_H */
