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
#include "btree_pager.h"

__STXXL_BEGIN_NAMESPACE

// TODO:  speedup BID2node_ access using search result iterator in the methods

namespace btree
{
	template <class NodeType, class BTreeType>
	class node_cache
	{
			node_cache();
		
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
			const unsigned min_node_elements_;
			const unsigned max_node_elements_;
			key_compare comp_;
		
			struct bid_comp
			{
  				bool operator () (const bid_type & a, const bid_type & b) const
  				{
      				return (a.storage < b.storage) || ( a.storage == b.storage && a.offset < b.offset);
 				}
			};
			
			std::vector<node_type *> nodes_;
			std::vector<bool> fixed_;
			std::vector<int> free_nodes_;
			typedef std::map<bid_type,int,bid_comp> BID2node_type;
			BID2node_type BID2node_;
			stxxl::btree::lru_pager pager_;	
			block_manager * bm;
			alloc_strategy_type alloc_strategy_;
		
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
				nodes_.reserve(nnodes);
				free_nodes_.reserve(nnodes);
				fixed_.resize(nnodes,false);
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
			
			~node_cache()
			{
				STXXL_VERBOSE1("btree::node_cache deconstructor addr="<<this)
				typename BID2node_type::const_iterator i = BID2node_.begin();
				for(;i!=BID2node_.end();++i)
				{
					nodes_[(*i).second]->save();
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
					do
					{
						++i;
						node2kick = pager_.kick();
						if(i==size()+1)
						{
							STXXL_ERRMSG(
								"The node cache is too small, no node can be kicked out (all nodes are fixed) !");
							STXXL_ERRMSG("Returning NULL node.")
							return NULL;
						}
						pager_.hit(node2kick);
							
					} while (!(fixed_[node2kick]));
					
					nodes_[node2kick]->save();
					BID2node_.erase(nodes_[node2kick]->my_bid());
					bm->new_blocks<block_type>(1,alloc_strategy_,&new_bid);
					
					BID2node_[new_bid] = node2kick;
					
					nodes_[node2kick]->init(new_bid);
					
					assert(size() == BID2node_.size() + free_nodes_.size());
					
					return nodes_[node2kick];
				}
				
				int free_node = free_nodes_.back();
				free_nodes_.pop_back();
				assert(fixed_[free_node] == false);
				
				bm->new_blocks<block_type>(1,alloc_strategy_,&new_bid);
				BID2node_[new_bid] = free_node;
				nodes_[free_node]->init(new_bid);
				
				pager_.hit(free_node);
				
				assert(size() == BID2node_.size() + free_nodes_.size());
				
				return nodes_[free_node];
			}
			
			
			
			node_type * get_node(const bid_type & bid, bool fix = false)
			{
				if(BID2node_.find(bid) != BID2node_.end())
				{
					// the node is in cache
					int nodeindex = BID2node_[bid];
					fixed_[nodeindex] = fix;
					pager_.hit(nodeindex);
					return nodes_[nodeindex];
				}
					
				// the node is not in cache
				if(free_nodes_.empty())
				{
					// need to kick a node
					int node2kick, i = 0;
					do
					{
						++i;
						node2kick = pager_.kick();
						if(i==size()+1)
						{
							STXXL_ERRMSG(
								"The node cache is too small, no node can be kicked out (all nodes are fixed) !");
							STXXL_ERRMSG("Returning NULL node.")
							return NULL;
						}
						pager_.hit(node2kick);
							
					} while (!(fixed_[node2kick]));
					
					nodes_[node2kick]->save();
					BID2node_.erase(nodes_[node2kick]->my_bid());
					
					nodes_[node2kick]->load(bid);
					BID2node_[bid] = node2kick;
					
					fixed_[node2kick] = fix;
					
					assert(size() == BID2node_.size() + free_nodes_.size());
					
					return nodes_[node2kick];
				}
				
				int free_node = free_nodes_.back();
				free_nodes_.pop_back();
				assert(fixed_[free_node] == false);
				
				nodes_[free_node]->load(bid);
				BID2node_[bid] = free_node;
				
				pager_.hit(free_node);
				
				fixed_[free_node] = fix;
				
				assert(size() == BID2node_.size() + free_nodes_.size());
				
				return nodes_[free_node];
			}
			
			void delete_node(const bid_type & bid)
			{
				if(BID2node_.find(bid) != BID2node_.end())
				{
					// the node is in the cache
					int nodeindex = BID2node_[bid];
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
			}			
	};
	
}

__STXXL_END_NAMESPACE

#endif /* _NODE_CACHE_H */
