 #ifndef PREFETCH_POOL_HEADER
 #define PREFETCH_POOL_HEADER
 /***************************************************************************
 *            prefetch_pool.h
 *
 *  Thu Jul  3 11:08:09 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../common/utils.h"
#include "../mng/mng.h"
#include "write_pool.h"
#include <list>

#ifdef BOOST_MSVC
#include <hash_map>
#else
#include <ext/hash_map>
#endif
 

__STXXL_BEGIN_NAMESPACE

//! \addtogroup schedlayer
//! \{
  
//! \brief Implements dynamically resizable prefetching pool
template <class BlockType>
class prefetch_pool
{
public:
  typedef BlockType block_type;
  typedef typename block_type::bid_type bid_type;
  
protected:
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
	{	// parameters for hash table
		bucket_size = 4,	// 0 < bucket_size
		min_buckets = 8
	};	// min_buckets = 2 ^^ N, 0 < N
	#endif
  };
  typedef std::pair<block_type *,request_ptr> busy_entry;
  #ifdef BOOST_MSVC
  typedef stdext::hash_map < bid_type, busy_entry , bid_hash > hash_map_type;
  #else
  typedef __gnu_cxx::hash_map < bid_type, busy_entry , bid_hash > hash_map_type;
  #endif
  typedef typename std::list<block_type *>::iterator free_blocks_iterator;
  typedef typename hash_map_type::iterator busy_blocks_iterator;
  
  // contains free prefetch blocks
  std::list<block_type *> free_blocks;
  // blocks that are in reading or already read but not retrieved by user
  hash_map_type busy_blocks;
  
  unsigned_type free_blocks_size;
private:
	prefetch_pool(const prefetch_pool & pool); // forbidden
	prefetch_pool & operator =(const prefetch_pool & pool); // forbidden
public:
  
  //! \brief Constructs pool
  //! \param init_size initial number of blocks in the pool
  explicit prefetch_pool(unsigned_type init_size=1): free_blocks_size(init_size)
  {
    unsigned_type i = 0;
    for(;i<init_size;++i)
      free_blocks.push_back(new block_type);
  }
  
  void swap(prefetch_pool & obj)
  {
	  std::swap(free_blocks,obj.free_blocks);
	  std::swap(busy_blocks,obj.busy_blocks);
	  std::swap(free_blocks_size,obj.free_blocks_size);
  }
  
  //! \brief Waits for completion of all ongoing read requests and frees memory
  virtual ~prefetch_pool()
  {
    while(!free_blocks.empty())
    {
      delete free_blocks.back();
      free_blocks.pop_back();
    }
    
    try
    {
      busy_blocks_iterator i2 = busy_blocks.begin();
      for(;i2 != busy_blocks.end();++i2)
      {
        i2->second.second->wait();
        delete i2->second.first;
      }
    }
    catch(...)
    {
    }
  }
  //! \brief Returns number of owned blocks
  unsigned_type size() const { return free_blocks_size + busy_blocks.size(); }
  
  //! \brief Gives a hint for prefetching a block
  //! \param bid address of a block to be prefetched
  //! \return \c true if there was a free block to do prefetch and prefetching 
  //! was scheduled, \c false otherwise
  //! \note If there are no free blocks available (all blocks 
  //! are already in reading or read but not retrieved by user calling \c read
  //! method) calling \c hint function has no effect
  bool hint(bid_type bid)
  {
    // if block is already hinted, no need to hint it again
    if(busy_blocks.find(bid) != busy_blocks.end())
      return true;
    
    if(free_blocks_size) //  only if we have a free block
    {
      STXXL_VERBOSE2("prefetch_pool::hint bid= " << bid<<" => prefetching")
      
      --free_blocks_size;
      block_type * block = free_blocks.back();
      free_blocks.pop_back();
      request_ptr req = block->read(bid);
      busy_blocks[bid] = busy_entry(block,req);
      return true;
    }
    STXXL_VERBOSE2("prefetch_pool::hint bid=" << bid<<" => no free blocks for prefetching")
    return false;
  }
  
  bool hint(bid_type bid, write_pool<block_type> & w_pool)
  {
    // if block is already hinted, no need to hint it again
    if(busy_blocks.find(bid) != busy_blocks.end())
      return true;
    
    if(free_blocks_size) //  only if we have a free block
    {
      STXXL_VERBOSE2("prefetch_pool::hint2 bid= " << bid<<" => prefetching")
      --free_blocks_size;
      block_type * block = free_blocks.back();
      free_blocks.pop_back();
      request_ptr req = w_pool.get_request(bid);
      if(req.valid())
      {
         STXXL_VERBOSE2("prefetch_pool::hint2 bid= " << bid<<" was in write cache")
         block_type * w_block = w_pool.steal(bid);
         assert(w_block != 0);
         w_pool.add(block);
         busy_blocks[bid] = busy_entry(w_block,req);
         return true;
      }
      req = block->read(bid);
      busy_blocks[bid] = busy_entry(block,req);
      return true;
    }
    STXXL_VERBOSE2("prefetch_pool::hint2 bid=" << bid<<" => no free blocks for prefetching")
    return false;
  }
  
  bool in_prefetching(bid_type bid)
  {
    return (busy_blocks.find(bid) != busy_blocks.end());
  }
  
  //! \brief Reads block. If this block is cached block is not read but passed from the cache
  //! \param block block object, where data to be read to. If block was cached \c block 's
  //! ownership goes to the pool and block from cache is returned in \c block value.
  //! \param bid address of the block
  //! \warning \c block parameter must be allocated dynamically using \c new .
  //! \return request pointer object of read operation
  request_ptr read(block_type * & block, bid_type bid)
  {
    busy_blocks_iterator cache_el = busy_blocks.find(bid);
    if(cache_el == busy_blocks.end())
    {
      // not cached
      STXXL_VERBOSE2("prefetch_pool::read bid="<<bid<<" => no copy in cache, retrieving")
      return block->read(bid);
    }
    
    // cached
    STXXL_VERBOSE2("prefetch_pool::read bid="<<bid<<" => copy in cache exists")
    ++free_blocks_size;
    free_blocks.push_back(block);
    block = cache_el->second.first;
    request_ptr result = cache_el->second.second;
    busy_blocks.erase(cache_el);
    return result;
  }
  
  //! \brief Resizes size of the pool
  //! \param new_size desired size of the pool. If some 
  //! blocks are used for prefetching, these blocks can't be freed. 
  //! Only free blocks (not in prefetching) can be freed by reducing
  //! the size of the pool calling this method.
  //! \return new size of the pool
  unsigned_type resize(unsigned_type new_size)
  {
    int_type diff = int_type(new_size) - int_type(size());
    if(diff > 0)
    {
      free_blocks_size += diff;
      while(--diff >= 0)
        free_blocks.push_back(new block_type);
      
      return size();
    } 
    
    while(diff < 0 && free_blocks_size > 0)
    {
      ++diff;
      --free_blocks_size;
      delete free_blocks.back();
      free_blocks.pop_back();
    }
    return size();
  }
};

//! \}

__STXXL_END_NAMESPACE

namespace std
{
	template <class BlockType>
	void swap(	stxxl::prefetch_pool<BlockType> & a,
				stxxl::prefetch_pool<BlockType> & b)
	{
		a.swap(b);
	}
}

#endif
