 /***************************************************************************
 *            stack.h
 *
 *  Thu May  1 21:19:54 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../mng/mng.h"
#include "../common/simple_vector.h"
#include <stack>
#include <vector>

__STXXL_BEGIN_NAMESPACE

//! \brief External vector container
  
//! For semantics of the methods see documentation of the STL std::vector. <BR>
//! Template parameters:
//!  - \c Tp_ type of contained objects
//!  - \c AllocStr_ one of allocation strategies: \c striping , \c RC , \c SR , or \c FR
//!  default is RC 
//!  - \c BlkSize_ external block size in bytes, default is 2 Mbytes
//!  - \c BlkCacheSize_ how many blocks use for internal cache, default is four
template < typename Tp_,
		typename AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY,
		unsigned BlkSize_ = STXXL_DEFAULT_BLOCK_SIZE (_Tp),
		unsigned BlkCacheSize_ = 4 >
class stack
{
public:
  // stl typedefs
  typedef Tp_ value_type;
  typedef off_t size_type;
  
  enum{ 
			block_size = BlkSize_, 
			cache_size = BlkCacheSize_
  };
 
  // stxxl specific typedefs
  typedef AllocStr_ alloc_strategy;
  typedef typed_block<block_size,value_type> block_type;
  typedef BID<block_size> bid_type;
  
private:
  size_type size_;
  unsigned cache_offset_;
  value_type * current_element_;
  simple_vector<block_type> cache_;
  std::vector<bid_type> bids_;
public:
  stack(): size_(0),
           cache_offset_(0),
           current_element_(NULL),
           cache_(cache_size),
           bids_(0)
  {
    bids_.reserve(cache_size);
  }
  //! \brief Construction from a stack
  //! \param stack_ stack object (could be external or internal, important is that it must
  //! have a copy constructor, \c top() and \c pop() methods )
  template <class stack_type>
  stack(const stack_type & stack_): size_(0),
           cache_offset_(0),
           current_element_(NULL),
           cache_(cache_size),
           bids_(0)
  {
    bids_.reserve(cache_size);
    
    stack_type stack_copy = stack_;
    const size_type sz=stack_copy.size();
    size_type i;
    
    std::vector<value_type> tmp(sz);
    
    for(i=0;i<sz;i++)
    {
      tmp[sz - i - 1] = stack_copy.top();
      stack_copy.pop();
    }
    for(i=0;i<sz;i++)
      this->push(tmp[i]);
   
    
  }
  ~stack()
  {
    block_manager::get_instance()->delete_blocks(bids_.begin(),bids_.end());
  }
  size_type size() const
  {
    return size_;
  }
  bool empty() const
  {
    return (!size_);
  }
  value_type & top()
  {
    assert(size_ > 0);
    return (*current_element_);
  }
  const value_type & top() const
  {
    assert(size_ > 0);
    return (*current_element_);
  }
  void push(const value_type & val)
  {
    assert(cache_offset_ <= cache_size*block_type::size);
    assert(cache_offset_ >= 0);
   
    if(cache_offset_ == cache_size*block_type::size) // cache overflow
    {
      // write cache on disk
      bids_.resize(bids_.size() + cache_size);
      std::vector<bid_type>::iterator cur_bid = bids_.end() - cache_size;
      block_manager::get_instance()->new_blocks(alloc_strategy(),cur_bid,bids_.end());
      
      request_ptr reqs[cache_size];
      for(int i=0;i<cache_size;i++,cur_bid++)
        reqs[i] = cache_[i].write(*cur_bid);
      
      bids_.reserve(bids_.size() + cache_size);
      
      cache_offset_ = 1;
      current_element_ = &(cache_[0][0]);
      ++size_;
 
      wait_all(reqs,cache_size);
 
      *current_element_=val;
      
      return;
    }
    
    current_element_ = &(cache_[cache_offset_/block_type::size][cache_offset_%block_type::size]);
    *current_element_=val;
    ++size_;
    ++cache_offset_;
  }
  void pop()
  {
    assert(cache_offset_ <= cache_size*block_type::size);
    assert(cache_offset_ > 0);
    assert(size_ > 0);
    
    if(cache_offset_ == 1 && bids_.size() >= cache_size)
    {
      // need to load from disk
      
      std::vector<bid_type>::const_iterator cur_bid = --bids_.end();
      request_ptr reqs[cache_size];
      for(int i=cache_size-1 ;i>=0;i--,cur_bid--)
        reqs[i] = cache_[i].read(*cur_bid);
      
      cache_offset_ = cache_size*block_type::size;
      --size_;
      current_element_ = &(cache_[cache_size - 1][block_type::size - 1]);
      
      wait_all(reqs,cache_size);
      
      block_manager::get_instance()->delete_blocks(bids_.end() - cache_size,bids_.end());
      bids_.resize(bids_.size() - cache_size);
      
      return;
    }
    
    --size_;
    unsigned cur_offset = (--cache_offset_) - 1;
    current_element_ = &(cache_[cur_offset/block_type::size][cur_offset%block_type::size]);
    
  }
};

__STXXL_END_NAMESPACE
