#ifndef STXXL_STACK_HEADER
#define STXXL_STACK_HEADER

/***************************************************************************
 *            stack.h
 *
 *  Tue May 27 14:12:24 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "../common/utils.h"
#include "../mng/mng.h"
#include "../common/simple_vector.h"
#include <stack>
#include <vector>
 
 
 __STXXL_BEGIN_NAMESPACE
 
 template <class ValTp,
           unsigned BlocksPerPage = 4,
           class AllocStr = STXXL_DEFAULT_ALLOC_STRATEGY,
           unsigned BlkSz = STXXL_DEFAULT_BLOCK_SIZE(ValTp),
           class SzTp = off_t>
 struct stack_config_generator
 {
   typedef ValTp value_type;
   enum { blocks_per_page = BlocksPerPage };
   typedef AllocStr alloc_strategy;
   enum { block_size = BlkSz };
   typedef SzTp size_type;
 };
 
template <class Config_>
class normal_stack
{
public:
  typedef Config_ cfg;
  typedef typename cfg::value_type value_type;
  typedef typename cfg::alloc_strategy alloc_strategy;
  typedef typename cfg::size_type size_type;
  enum {  blocks_per_page = cfg::blocks_per_page,
          block_size = cfg::block_size,
          cache_size = blocks_per_page };
          
  typedef typed_block<block_size,value_type> block_type;
  typedef BID<block_size> bid_type;

private:
  size_type size_;
  unsigned cache_offset_;
  value_type * current_element_;
  simple_vector<block_type> cache_;
  std::vector<bid_type> bids_;
public:
  normal_stack(): size_(0),
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
  normal_stack(const stack_type & stack_): size_(0),
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
  ~normal_stack()
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

//! \brief Efficient implementation for ...
template <class Config_>
class grow_shrink_stack
{
public:
  typedef Config_ cfg;
  typedef typename cfg::value_type value_type;
  typedef typename cfg::alloc_strategy alloc_strategy;
  typedef typename cfg::size_type size_type;
  enum {  blocks_per_page = cfg::blocks_per_page,
          block_size = cfg::block_size,
       };
          
  typedef typed_block<block_size,value_type> block_type;
  typedef BID<block_size> bid_type;

private:
  size_type size_;
  unsigned cache_offset;
  value_type * current_element;
  simple_vector<block_type> cache;
  simple_vector<block_type>::iterator cache_buffers;
  simple_vector<block_type>::iterator overlap_buffers;
  simple_vector<request_ptr> requests;
  std::vector<bid_type> bids;
public:
  grow_shrink_stack(): 
           size_(0),
           cache_offset(0),
           current_element(NULL),
           cache(blocks_per_page*2),
           cache_buffers(cache.begin()),
           overlap_buffers(cache.begin() + blocks_per_page),
           requests(blocks_per_page),
           bids(0)
  {
    bids.reserve(blocks_per_page);
  }
  //! \brief Construction from a stack
  //! \param stack_ stack object (could be external or internal, important is that it must
  //! have a copy constructor, \c top() and \c pop() methods )
  template <class stack_type>
  grow_shrink_stack(const stack_type & stack_): 
           size_(0),
           cache_offset(0),
           current_element(NULL),
           cache(blocks_per_page*2),
           cache_buffers(cache.begin()),
           overlap_buffers(cache.begin() + blocks_per_page),
           requests(blocks_per_page),
           bids(0)
  {
    bids.reserve(blocks_per_page);
    
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
  ~grow_shrink_stack()
  {
    block_manager::get_instance()->delete_blocks(bids.begin(),bids.end());
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
    return (*current_element);
  }
  const value_type & top() const
  {
    assert(size_ > 0);
    return (*current_element);
  }
  void push(const value_type & val)
  {
    assert(cache_offset <= blocks_per_page*block_type::size);
    assert(cache_offset >= 0);
    
    if(cache_offset == blocks_per_page*block_type::size) // cache overflow
    {
      STXXL_VERBOSE2("growing, size: "<<size_)
      
      bids.resize(bids.size() + blocks_per_page);
      std::vector<bid_type>::iterator cur_bid = bids.end() - blocks_per_page;
      block_manager::get_instance()->new_blocks(alloc_strategy(),cur_bid,bids.end());
      
      for(int i=0;i<blocks_per_page;i++,cur_bid++)
      {
        if(requests[i].get()) requests[i]->wait();
        requests[i] = (cache_buffers + i)->write(*cur_bid);
      }
      
      std::swap(cache_buffers,overlap_buffers);
      
      bids.reserve(bids.size() + blocks_per_page);
      
      cache_offset = 1;
      current_element = &((*cache_buffers)[0]);
      ++size_;
 
      *current_element=val;
      
      return;
    }
    
    current_element = &((*(cache_buffers + cache_offset/block_type::size))[cache_offset%block_type::size]);
    *current_element=val;
    ++size_;
    ++cache_offset;
  }
  void pop()
  {
    assert(cache_offset <= blocks_per_page*block_type::size);
    assert(cache_offset > 0);
    assert(size_ > 0);
    
    if(cache_offset == 1 && bids.size() >= blocks_per_page)
    { 
      STXXL_VERBOSE2("shrinking, size: "<<size_)
      
      if(requests[0].get())
        wait_all(requests.begin(),blocks_per_page);
      
      std::swap(cache_buffers,overlap_buffers);
      
      if(bids.size() > blocks_per_page)
      {
        STXXL_VERBOSE2("prefetching, size: "<<size_)
        std::vector<bid_type>::const_iterator cur_bid = bids.end() - blocks_per_page - 1;
        for(int i=blocks_per_page-1 ;i>=0;i--,cur_bid--)
          requests[i] = (overlap_buffers+i)->read(*cur_bid);
        
      }
            
      block_manager::get_instance()->delete_blocks(bids.end() - blocks_per_page,bids.end());
      bids.resize(bids.size() - blocks_per_page);
      
      cache_offset = blocks_per_page*block_type::size;
      --size_;
      current_element = &((*(cache_buffers+(blocks_per_page - 1)))[block_type::size - 1]);
      
      return;
    }
       
    --size_;
    unsigned cur_offset = (--cache_offset) - 1;
    current_element = &((*(cache_buffers + cur_offset/block_type::size))[cur_offset%block_type::size]);
    
  }
  
};

template <unsigned CritSize, class ExternalStack, class InternalStack>
class migrating_stack
{
public:
  typedef typename ExternalStack::cfg cfg;
  typedef typename cfg::value_type value_type;
  typedef typename cfg::alloc_strategy alloc_strategy;
  typedef typename cfg::size_type size_type;
  enum {  blocks_per_page = cfg::blocks_per_page,
          block_size = cfg::block_size };
          
          
  typedef InternalStack int_stack_type;
  typedef ExternalStack ext_stack_type;
  
private:
  enum { critical_size = CritSize };
    
  int_stack_type * int_impl;
  ext_stack_type * ext_impl;

public:
  migrating_stack(): int_impl(new int_stack_type()),ext_impl(NULL) {}
  template <class stack_type>
  migrating_stack(const stack_type & stack_)
  {
    STXXL_ERRMSG(__PRETTY_FUNCTION__ << " is NOT IMPLEMENTED, aborting.")
    abort();
  } 
  //! \brief Returns true if current implementation is internal, otherwise false
  bool internal() 
  { 
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    return int_impl; 
  }
  //! \brief Returns true if current implementation is external, otherwise false
  bool external() 
  {
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    return ext_impl; 
  }
  
  bool empty() const
  {
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    
    if(int_impl)
      return int_impl->empty();
    
    return ext_impl->empty();
  }
  size_type size() const
  {
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    
    if(int_impl)
      return int_impl->size();
    
    return ext_impl->size();
  }
  value_type & top()
  {
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    
    if(int_impl)
      return int_impl->top();
    
    return ext_impl->top();
  }
  void push(const value_type & val)
  {
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    
    if(int_impl)
    {
      int_impl->push(val);
      if(int_impl->size() == critical_size)
      {
        // migrate to external stack
        ext_impl = new ext_stack_type(*int_impl);
        delete int_impl;
        int_impl = NULL;
      }
    }
    else
      ext_impl->push(val);
  }
  void pop()
  {
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    
    if(int_impl)
      int_impl->pop();
    else
      ext_impl->pop();
  }
  ~migrating_stack()
  {
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    
    if(int_impl)
      delete int_impl;
    else
      delete ext_impl;
  }
};

template <class BaseStack>
class stack: public BaseStack
{
public:
  typedef typename BaseStack::cfg cfg;
  typedef typename cfg::value_type value_type;
  typedef typename cfg::alloc_strategy alloc_strategy;
  typedef typename cfg::size_type size_type;
  enum {  blocks_per_page = cfg::blocks_per_page,
          block_size = cfg::block_size };
  
  stack() : BaseStack() {}
    
  //! \brief Construction from a stack
  //! \param stack_ stack object (could be external or internal, important is that it must
  //! have a copy constructor, \c top() and \c pop() methods )
  template <class stack_type>
  stack(const stack_type & stack_) : BaseStack(stack_) {} 
};



__STXXL_END_NAMESPACE
 
 #endif
