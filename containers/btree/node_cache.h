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
				  		size_t result = 
							longhash1(bid.offset + uint64(bid.storage)); 
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
			std::vector<request_ptr> reqs_;
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
			
			int64 n_found;
			int64 n_not_found;
			int64 n_created;
			int64 n_deleted;
			int64 n_read;
			int64 n_written;
			int64 n_clean_forced;
			
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
								key_compare comp__
								): 
					btree_(btree__),
					comp_(comp__),
					bm(block_manager::get_instance()),
					n_found(0),
					n_not_found(0),
					n_created(0),
					n_deleted(0),
					n_read(0),
					n_written(0),
					n_clean_forced(0)
			{
				const unsigned nnodes = cache_size_in_bytes/block_type::raw_size;
				STXXL_VERBOSE1("btree::node_cache constructor nodes="<<nnodes)
				if(nnodes < 3)
				{
					STXXL_ERRMSG("btree: Too few memory for a node cache (<3)")
					STXXL_ERRMSG("aborting.")
					abort();
				}
				nodes_.reserve(nnodes);
				reqs_.resize(nnodes);
				free_nodes_.reserve(nnodes);
				fixed_.resize(nnodes,false);
				dirty_.resize(nnodes,true);
				for(unsigned i= 0; i<nnodes;++i)
				{
					nodes_.push_back(new node_type(btree_,comp_));
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
					if(reqs_[p].valid()) reqs_[p]->wait();
					if(dirty_[p]) nodes_[p]->save();
				}
				for(int i=0;i<size();++i)
					delete nodes_[i];
			}
		
			node_type * get_new_node(bid_type & new_bid)
			{
				++n_created;
				
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
					
					
					if(reqs_[node2kick].valid()) reqs_[node2kick]->wait();
						
					node_type & Node = *(nodes_[node2kick]);
					
					if(dirty_[node2kick]) 
					{
						Node.save();
						++n_written;
					}
					else
						++n_clean_forced;
						
					//reqs_[node2kick] = request_ptr(); // reset request
						
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
				
				// assert(!(reqs_[free_node].valid()));
								
				pager_.hit(free_node);
				
				dirty_[free_node] = true;
				
				assert(size() == BID2node_.size() + free_nodes_.size());
				
				STXXL_VERBOSE1("btree::node_cache get_new_node, free node "<<free_node<<"available")
				
				return &Node;
			}
			
			
			
			node_type * get_node(const bid_type & bid, bool fix = false)
			{
				typename BID2node_type::const_iterator it = BID2node_.find(bid);
				++n_read;
				
				if(it != BID2node_.end())
				{
					// the node is in cache
					const int nodeindex = it->second;
					STXXL_VERBOSE1("btree::node_cache get_node, the node "<<nodeindex<<"is in cache , fix="<<fix)
					fixed_[nodeindex] = fix;
					pager_.hit(nodeindex);
					dirty_[nodeindex] = true;
				
					if(reqs_[nodeindex].valid() && !reqs_[nodeindex]->poll()) 
							reqs_[nodeindex]->wait();
					
					++n_found;
					return nodes_[nodeindex];
				}
				
				++n_not_found;
					
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
					
					if(reqs_[node2kick].valid()) reqs_[node2kick]->wait();
					
					node_type & Node = *(nodes_[node2kick]);
					
					if(dirty_[node2kick])
					{
						Node.save();
						++n_written;
					}
					else
						++n_clean_forced;
					
						
					BID2node_.erase(Node.my_bid());
					
					reqs_[node2kick] = Node.load(bid);
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
				reqs_[free_node] = Node.load(bid);
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
				++n_read;
				
				if(it != BID2node_.end())
				{
					// the node is in cache
					const int nodeindex = it->second;
					STXXL_VERBOSE1("btree::node_cache get_node, the node "<<nodeindex<<"is in cache , fix="<<fix)
					fixed_[nodeindex] = fix;
					pager_.hit(nodeindex);
				
					if(reqs_[nodeindex].valid() && !reqs_[nodeindex]->poll()) 
							reqs_[nodeindex]->wait();
					
					++n_found;
					return nodes_[nodeindex];
				}
				
				++n_not_found;
					
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
					
					if(reqs_[node2kick].valid()) reqs_[node2kick]->wait();
					
					node_type & Node = *(nodes_[node2kick]);
					if(dirty_[node2kick])
					{
						Node.save();
						++n_written;
					}
					else
						++n_clean_forced;
						
					BID2node_.erase(Node.my_bid());
					
					reqs_[node2kick] = Node.load(bid);
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
				reqs_[free_node] = Node.load(bid);
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
					// the node is in the cach
					const int nodeindex = it->second;
					STXXL_VERBOSE1("btree::node_cache delete_node "<< nodeindex<<" from cache.")
					if(reqs_[nodeindex].valid()) reqs_[nodeindex]->wait();
					//reqs_[nodeindex] = request_ptr(); // reset request
					free_nodes_.push_back(nodeindex);
					BID2node_.erase(bid);
					fixed_[nodeindex] = false;
				}
				++n_deleted;
				bm->delete_block(bid);
			}
			
			
			void prefetch_node(const bid_type & bid)
			{
				if(BID2node_.find(bid) != BID2node_.end()) return;
					
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
							return;
						}
						pager_.hit(node2kick);
							
					} while (fixed_[node2kick]);
					
					if(reqs_[node2kick].valid()) reqs_[node2kick]->wait();
					
					node_type & Node = *(nodes_[node2kick]);
					
					if(dirty_[node2kick])
					{
						Node.save();
						++n_written;
					}
					else
						++n_clean_forced;
					
						
					BID2node_.erase(Node.my_bid());
					
					reqs_[node2kick] = Node.prefetch(bid);
					BID2node_[bid] = node2kick;
					
					fixed_[node2kick] = false;
					
					dirty_[node2kick] = false;
					
					assert(size() == BID2node_.size() + free_nodes_.size());
					
					STXXL_VERBOSE1("btree::node_cache prefetch_node, need to kick node"<<node2kick<<" ")
					
					return;
				}
				
				int free_node = free_nodes_.back();
				free_nodes_.pop_back();
				assert(fixed_[free_node] == false);
				
				node_type & Node = *(nodes_[free_node]);
				reqs_[free_node] = Node.prefetch(bid);
				BID2node_[bid] = free_node;
				
				pager_.hit(free_node);
				
				fixed_[free_node] = false;
				
				dirty_[free_node] = false;
				
				assert(size() == BID2node_.size() + free_nodes_.size());
				
				STXXL_VERBOSE1("btree::node_cache prefetch_node, free node "<< free_node<<"available")
				
				return;
			}
			
			void unfix_node(const bid_type & bid)
			{
				assert(BID2node_.find(bid) != BID2node_.end());
				fixed_[BID2node_[bid] ] = false;
				STXXL_VERBOSE1("btree::node_cache unfix_node,  node "<< BID2node_[bid] )
			}			
			
			void swap(node_cache & obj)
			{
				std::swap(comp_,obj.comp_);
				std::swap(nodes_,obj.nodes_);
				std::swap(reqs_,obj.reqs_);
				change_btree_pointers(btree_);
				obj.change_btree_pointers(obj.btree_);
				std::swap(fixed_,obj.fixed_);
				std::swap(free_nodes_,obj.free_nodes_);
				std::swap(BID2node_,obj.BID2node_);
				std::swap(pager_,obj.pager_);
				std::swap(alloc_strategy_,obj.alloc_strategy_);
				std::swap(n_found,obj.n_found);
				std::swap(n_not_found,obj.n_found);
				std::swap(n_created,obj.n_created);
				std::swap(n_deleted,obj.n_deleted);
				std::swap(n_read,obj.n_read);
				std::swap(n_written,obj.n_written);
				std::swap(n_clean_forced,obj.n_clean_forced);
			}
			
			void print_statistics(std::ostream & o) const
			{
				if(n_read)
				o << "Found blocks                      : " << n_found<<" ("<< 
					100.*double(n_found)/double(n_read)<<"%)"<<std::endl;
				else
				o << "Found blocks                      : " << n_found<<" ("<< 
					100<<"%)"<<std::endl;
				o << "Not found blocks                  : " << n_not_found<<std::endl;
				o << "Created in the cache blocks       : " << n_created<<std::endl;
				o << "Deleted blocks                    : " << n_deleted<<std::endl;
				o << "Read blocks                       : " << n_read<<std::endl;
				o << "Written blocks                    : " << n_written<<std::endl;
				o << "Clean blocks forced from the cache: " << n_clean_forced<<std::endl;
			}
			void reset_statistics()
			{
				n_found=0;
				n_not_found=0;
				n_created =0;
				n_deleted=0;
				n_read =0;
				n_written =0;
				n_clean_forced =0;
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
