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
#include "../common/tmeta.h"
#include <stack>
#include <vector>
 
 
 __STXXL_BEGIN_NAMESPACE
 
//! \addtogroup stlcont
//! \{
 
 template <class ValTp,
           unsigned BlocksPerPage = 4,
           unsigned BlkSz = STXXL_DEFAULT_BLOCK_SIZE(ValTp),
           class AllocStr = STXXL_DEFAULT_ALLOC_STRATEGY,
           class SzTp = off_t>
 struct stack_config_generator
 {
   typedef ValTp value_type;
   enum { blocks_per_page = BlocksPerPage };
   typedef AllocStr alloc_strategy;
   enum { block_size = BlkSz };
   typedef SzTp size_type;
 };

 
//! \brief External vector container
 
//! Conservative implementation. Fits best if you access pattern consists of irregulary mixed
//! push'es and pop's.
//! For semantics of the methods see documentation of the STL \c std::vector. <BR>
//! To gain full bandwidth of disks \c Config_::BlocksPerPage must >= number of disks <BR>
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
       };
          
  typedef typed_block<block_size,value_type> block_type;
  typedef BID<block_size> bid_type;

private:
  size_type size_;
  unsigned cache_offset;
  value_type * current_element;
  simple_vector<block_type> cache;
  simple_vector<block_type>::iterator front_page;
  simple_vector<block_type>::iterator back_page;
  std::vector<bid_type> bids;
  alloc_strategy alloc_strategy_;
public:
  normal_stack(): 
           size_(0),
           cache_offset(0),
           current_element(NULL),
           cache(blocks_per_page*2),
           back_page(cache.begin()),
           front_page(cache.begin() + blocks_per_page),
           bids(0)
  {
    bids.reserve(blocks_per_page);
  }
  //! \brief Construction from a stack
  //! \param stack_ stack object (could be external or internal, important is that it must
  //! have a copy constructor, \c top() and \c pop() methods )
  template <class stack_type>
  normal_stack(const stack_type & stack_): 
           size_(0),
           cache_offset(0),
           current_element(NULL),
           cache(blocks_per_page*2),
           back_page(cache.begin()),
           front_page(cache.begin() + blocks_per_page),
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
  ~normal_stack()
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
    assert(cache_offset <= 2*blocks_per_page*block_type::size);
    assert(cache_offset >= 0);
    
    if(cache_offset == 2*blocks_per_page*block_type::size) // cache overflow
    {
      STXXL_VERBOSE2("growing, size: "<<size_)
      
      bids.resize(bids.size() + blocks_per_page);
      std::vector<bid_type>::iterator cur_bid = bids.end() - blocks_per_page;
      block_manager::get_instance()->new_blocks(
          offset_allocator<alloc_strategy>(cur_bid-bids.begin(),alloc_strategy_),cur_bid,bids.end());
      simple_vector<request_ptr> requests(blocks_per_page);
      
      for(int i=0;i<blocks_per_page;i++,cur_bid++)
        requests[i] = (back_page + i)->write(*cur_bid);
      
      
      std::swap(back_page,front_page);
      
      bids.reserve(bids.size() + blocks_per_page);
      
      cache_offset = blocks_per_page*block_type::size + 1;
      current_element = &((*front_page)[0]);
      ++size_;
      
      wait_all(requests.begin(),blocks_per_page);
      
      *current_element=val;
      
      return;
    }
    
    current_element = element(cache_offset);
    *current_element=val;
    ++size_;
    ++cache_offset;
  }
  void pop()
  {
    assert(cache_offset <= 2*blocks_per_page*block_type::size);
    assert(cache_offset > 0);
    assert(size_ > 0);
    
    if(cache_offset == 1 && bids.size() >= blocks_per_page)
    { 
      STXXL_VERBOSE2("shrinking, size: "<<size_)
     
      simple_vector<request_ptr> requests(blocks_per_page);
      
      std::vector<bid_type>::const_iterator cur_bid = bids.end() - 1;
      for(int i=blocks_per_page-1 ;i>=0;i--,cur_bid--)
        requests[i] = (front_page+i)->read(*cur_bid);
        
      std::swap(front_page,back_page);
            
      cache_offset = blocks_per_page*block_type::size;
      --size_;
      current_element = &((*(back_page+(blocks_per_page - 1)))[block_type::size - 1]);
      
      wait_all(requests.begin(),blocks_per_page);
      
      block_manager::get_instance()->delete_blocks(bids.end() - blocks_per_page,bids.end());
      bids.resize(bids.size() - blocks_per_page);
      
      return;
    }
       
    --size_;
    
    current_element = element((--cache_offset) - 1);
  }
private:
  value_type * element(unsigned offset)
  {
    if(offset < blocks_per_page*block_type::size)
      return &((*(back_page + offset/block_type::size))[offset%block_type::size]);
    
    unsigned unbiased_offset = offset - blocks_per_page*block_type::size;
    return &((*(front_page + unbiased_offset/block_type::size))[unbiased_offset%block_type::size]);
  }
};


//! \brief Efficient implementation that uses prefetching and overlapping.

//! Use it if your access patttern consists of many repeated push'es and pop's
//! For semantics of the methods see documentation of the STL \c std::vector.
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
  alloc_strategy alloc_strategy_;
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
      block_manager::get_instance()->new_blocks(
          offset_allocator<alloc_strategy>(cur_bid-bids.begin(),alloc_strategy_),cur_bid,bids.end());
      
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

//! \brief A stack that migrates from internal memory to external when its size exceeds a certain threshold

//! For semantics of the methods see documentation of the STL \c std::vector.
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
/*
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
*/

enum stack_externality { external, migrating, internal };
enum stack_behaviour { normal, grow_shrink };

//! \brief Stack type generator

//! Template parameters:
//!  - \c ValTp type of contained objects
//!  - \c Externality one of
//!    - \c external , \b external container, implementation is chosen according 
//!      to \c Behaviour parameter, is default
//!    - \c migrating , migrates from internal implementation given by \c IntStackTp parameter
//!      to external implementation given by \c Behaviour parameter when size exceeds \c MigrCritSize
//!    - \c internal , choses \c IntStackTp implementation
//!  - \c Behaviour ,  choses \b external implementation, one of:
//!    - \c normal , conservative version, implemented in \c stxxl::normal_stack , is default
//!    - \c grow_shrink , efficient version, implemented in \c stxxl::grow_shrink_stack
//!  - \c BlocksPerPage defines how many blocks has one page of internal cache of an 
//!       \b external implementation, default is four. All \b external implementations have 
//!       \b two pages.
//!  - \c BlkSz external block size in bytes, default is 2 Mbytes
//!  - \c IntStackTp type of internal stack used for some implementations
//!  - \c MigrCritSize threshold value for number of elements when 
//!    \c stxxl::migrating_stack migrates to the external memory
//!  - \c AllocStr one of allocation strategies: \c striping , \c RC , \c SR , or \c FR
//!    default is RC
//!  - \c SzTp size type, default is \c off_t
//!
//! Configured stack type is available as \c STACK_GENERATOR<>::result. <BR> <BR>
//! Examples:
//!    - \c STACK_GENERATOR<double>::result external stack of \c double's ,
//!    - \c STACK_GENERATOR<double,internal>::result internal stack of \c double's ,
//!    - \c STACK_GENERATOR<double,external,grow_shrink>::result external 
//!      grow-shrink stack of \c double's ,
//!    - \c STACK_GENERATOR<double,migrating,grow_shrink>::result migrating 
//!      grow-shrink stack of \c double's, internal implementation is \c std::stack<double> ,
//!    - \c STACK_GENERATOR<double,migrating,grow_shrink,1,512*1024>::result migrating 
//!      grow-shrink stack of \c double's with 1 block per page and block size 512 KB
//!      (total memory occupied = 1 MB).
//! For configured stack method semantics see documentation of the STL \c std::vector.
template  <
            class ValTp,
            stack_externality  Externality = external,
            stack_behaviour    Behaviour = normal,
            unsigned BlocksPerPage = 4,
            unsigned BlkSz = STXXL_DEFAULT_BLOCK_SIZE(ValTp),

            class IntStackTp = std::stack<ValTp>,
            unsigned MigrCritSize = (2*BlocksPerPage*BlkSz),

            class AllocStr = STXXL_DEFAULT_ALLOC_STRATEGY,
            class SzTp = off_t
          >
class STACK_GENERATOR
{
  typedef stack_config_generator<ValTp,BlocksPerPage,BlkSz,AllocStr,SzTp> cfg;
  
  typedef IF<Behaviour==normal, normal_stack<cfg>,grow_shrink_stack<cfg> >::result ExtStackTp;
  typedef IF<Externality==migrating, 
    migrating_stack<MigrCritSize,ExtStackTp,IntStackTp>,ExtStackTp>::result MigrOrNotStackTp;
  
public:
  
  typedef IF<Externality==internal,IntStackTp,MigrOrNotStackTp>::result result;

};

//! \}

__STXXL_END_NAMESPACE
 
 #endif
