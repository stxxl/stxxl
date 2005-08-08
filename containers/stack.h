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
#include "../mng/write_pool.h"
#include "../mng/prefetch_pool.h"
#include <stack>
#include <vector>

 
 __STXXL_BEGIN_NAMESPACE
 
//! \addtogroup stlcontinternals
//! \{
 
 template <class ValTp,
           unsigned BlocksPerPage = 4,
           unsigned BlkSz = STXXL_DEFAULT_BLOCK_SIZE(ValTp),
           class AllocStr = STXXL_DEFAULT_ALLOC_STRATEGY,
           class SzTp = stxxl::int64>
 struct stack_config_generator
 {
   typedef ValTp value_type;
   enum { blocks_per_page = BlocksPerPage };
   typedef AllocStr alloc_strategy;
   enum { block_size = BlkSz };
   typedef SzTp size_type;
 };

 
//! \brief External stack container
 
//! Conservative implementation. Fits best if you access pattern consists of irregulary mixed
//! push'es and pop's.
//! For semantics of the methods see documentation of the STL \c std::stack. <BR>
//! To gain full bandwidth of disks \c Config_::BlocksPerPage must >= number of disks <BR>
//! \internal
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
  typename simple_vector<block_type>::iterator front_page;
  typename simple_vector<block_type>::iterator back_page;
  std::vector<bid_type> bids;
  alloc_strategy alloc_strategy_;

  normal_stack(const normal_stack & obj); // forbidden
  normal_stack & operator = (const normal_stack & obj); // forbidden
public:
  normal_stack(): 
           size_(0),
           cache_offset(0),
           current_element(NULL),
           cache(blocks_per_page*2),
           front_page(cache.begin() + blocks_per_page),
           back_page(cache.begin()),
           bids(0)
  {
    bids.reserve(blocks_per_page);
  }
  
  void swap(normal_stack & obj)
  {
	  std::swap(size_,obj.size_);
	  std::swap(cache_offset,obj.cache_offset);
	  std::swap(current_element,obj.current_element);
	  std::swap(cache,obj.cache);
	  std::swap(front_page,obj.front_page);
	  std::swap(back_page,obj.back_page);
	  std::swap(bids,obj.bids);
	  std::swap(alloc_strategy_,obj.alloc_strategy_);
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
           front_page(cache.begin() + blocks_per_page),
           back_page(cache.begin()),
           bids(0)
  {
    bids.reserve(blocks_per_page);
    
    stack_type stack_copy = stack_;
    const size_type sz=stack_copy.size();
    size_type i;
    
    std::vector<value_type> tmp(sz);
    
    for(i=0;i<sz;++i)
    {
      tmp[sz - i - 1] = stack_copy.top();
      stack_copy.pop();
    }
    for(i=0;i<sz;++i)
      this->push(tmp[i]);
    
  }
  virtual ~normal_stack()
  {
    STXXL_VERBOSE(STXXL_PRETTY_FUNCTION_NAME);
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
      typename std::vector<bid_type>::iterator cur_bid = bids.end() - blocks_per_page;
      block_manager::get_instance()->new_blocks(
          offset_allocator<alloc_strategy>(cur_bid-bids.begin(),alloc_strategy_),cur_bid,bids.end());
      
      simple_vector<request_ptr> requests(blocks_per_page);
      
      for(int i=0;i<blocks_per_page;++i,++cur_bid)
      {
        requests[i] = (back_page + i)->write(*cur_bid);
      }
      
      
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
      
      typename std::vector<bid_type>::const_iterator cur_bid = bids.end() - 1;
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


//! \brief Efficient implementation that uses prefetching and overlapping using internal buffers

//! Use it if your access patttern consists of many repeated push'es and pop's
//! For semantics of the methods see documentation of the STL \c std::stack.
//! \warning The amortized complexity of operation is not O(1/DB), rather O(DB)
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
  typename simple_vector<block_type>::iterator cache_buffers;
  typename simple_vector<block_type>::iterator overlap_buffers;
  simple_vector<request_ptr> requests;
  std::vector<bid_type> bids;
  alloc_strategy alloc_strategy_;


  grow_shrink_stack & operator = (const grow_shrink_stack & obj); // forbidden
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
  
  void swap(grow_shrink_stack & obj)
  {
	  std::swap(size_,obj.size_);
	  std::swap(cache_offset,obj.cache_offset);
	  std::swap(current_element,obj.current_element);
	  std::swap(cache,obj.cache);
	  std::swap(cache_buffers,obj.cache_buffers);
	  std::swap(overlap_buffers,obj.overlap_buffers);
	  std::swap(requests,obj.requests);
	  std::swap(bids,obj.bids);
	  std::swap(alloc_strategy_,obj.alloc_strategy_);
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
    
    for(i=0;i<sz;++i)
    {
      tmp[sz - i - 1] = stack_copy.top();
      stack_copy.pop();
    }
    for(i=0;i<sz;++i)
      this->push(tmp[i]);
    
  }
  virtual ~grow_shrink_stack()
  {
    STXXL_VERBOSE(STXXL_PRETTY_FUNCTION_NAME);
    if(requests[0].get()) wait_all(requests.begin(),blocks_per_page);  
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
      typename std::vector<bid_type>::iterator cur_bid = bids.end() - blocks_per_page;
      block_manager::get_instance()->new_blocks(
          offset_allocator<alloc_strategy>(cur_bid-bids.begin(),alloc_strategy_),cur_bid,bids.end());
      
      for(int i=0;i<blocks_per_page;++i,++cur_bid)
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
        typename std::vector<bid_type>::const_iterator cur_bid = bids.end() - blocks_per_page - 1;
        for(int i=blocks_per_page-1 ;i>=0;--i,--cur_bid)
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

//! \brief Efficient implementation that uses prefetching and overlapping using (shared) buffers pools
template <class Config_>
class grow_shrink_stack2
{
public:
  typedef Config_ cfg;
  typedef typename cfg::value_type value_type;
  typedef typename cfg::alloc_strategy alloc_strategy;
  typedef typename cfg::size_type size_type;
  enum {  blocks_per_page = cfg::blocks_per_page, // stack of this type has only one page
          block_size = cfg::block_size,
       };
          
  typedef typed_block<block_size,value_type> block_type;
  typedef BID<block_size> bid_type;

private:
  size_type size_;
  unsigned cache_offset;
  block_type * cache;
  value_type current;
  std::vector<bid_type> bids;
  alloc_strategy alloc_strategy_;
  unsigned pref_aggr;
  prefetch_pool<block_type> & p_pool;
  write_pool<block_type>    & w_pool;
  // for a moment forbid default construction
  grow_shrink_stack2();
  grow_shrink_stack2(const grow_shrink_stack2 & obj); // forbidden
  grow_shrink_stack2 & operator = (const grow_shrink_stack2 & obj); // forbidden
public:
  //! \brief Constructs stack
  //! \param p_pool_ prefetch pool, that will be used for block prefetching
  //! \param w_pool_ write pool, that will be used for block writing
  //! \param prefetch_aggressiveness number of blocks that will be used from prefetch pool
  grow_shrink_stack2(
       prefetch_pool<block_type> & p_pool_,
       write_pool<block_type>    & w_pool_,
       unsigned prefetch_aggressiveness = 0):
       size_(0),
       cache_offset(0),
       cache(new block_type),
       pref_aggr(prefetch_aggressiveness),
       p_pool(p_pool_),
       w_pool(w_pool_)
  {
    STXXL_VERBOSE2("grow_shrink_stack2::grow_shrink_stack2(...)")
  }
  
  void swap(grow_shrink_stack2 & obj)
  {
	  std::swap(size_,obj.size_);
	  std::swap(cache_offset,obj.cache_offset);
	  std::swap(cache,obj.cache);
	  std::swap(current,obj.current);
	  std::swap(bids,obj.bids);
	  std::swap(alloc_strategy_,obj.alloc_strategy_);
	  std::swap(pref_aggr,obj.pref_aggr);
	  //std::swap(p_pool,obj.p_pool);
	  //std::swap(w_pool,obj.w_pool);
  }
  
  virtual ~ grow_shrink_stack2()
  {
    STXXL_VERBOSE2("grow_shrink_stack2::~grow_shrink_stack2()")
    const int bids_size = bids.size();
    const int last_pref = STXXL_MAX(int(bids_size) - int(pref_aggr),0);
    int i;
    for(i=bids_size - 1 ; i >= last_pref ; --i )
    {
        if(p_pool.in_prefetching(bids[i]));
          p_pool.read(cache,bids[i])->wait();  // clean the prefetch buffer
    }
    typename std::vector<bid_type>::iterator cur = bids.begin();
    typename std::vector<bid_type>::const_iterator end = bids.end();
    for(;cur!=end;++cur)
    {
       block_type * b = w_pool.steal(*cur);
       if(b)
       {
         w_pool.add(cache); // return buffer
         cache = b;
       }
    }
    delete cache;
    block_manager::get_instance()->delete_blocks(bids.begin(),bids.end());
  }
  size_type size() const { return size_; }
    
  bool empty() const
  {
    return (!size_);
  }
    
  void push(const value_type & val)
  {
    STXXL_VERBOSE3("grow_shrink_stack2::push("<< val <<")")
    assert(cache_offset <= block_type::size);
    
    if(cache_offset == block_type::size)
    {
      STXXL_VERBOSE2("grow_shrink_stack2::push("<< val <<") growing, size: "<<size_)
      
      bids.resize(bids.size() + 1);
      typename std::vector<bid_type>::iterator cur_bid = bids.end() - 1;
      block_manager::get_instance()->new_blocks(
          offset_allocator<alloc_strategy>(cur_bid-bids.begin(),alloc_strategy_),cur_bid,bids.end());
      w_pool.write(cache,bids.back());
      cache = w_pool.steal();
      const int bids_size = bids.size();
      const int last_pref = STXXL_MAX(int(bids_size) - int(pref_aggr) - 1,0);
      for(int i=bids_size - 2 ; i >= last_pref ; --i )
      {
        if(p_pool.in_prefetching(bids[i]))
            p_pool.read(cache,bids[i])->wait(); //  clean prefetch buffers
      }
      cache_offset = 0;
    }
    current = val;
    (*cache)[cache_offset] = val;
    ++size_;
    ++cache_offset;
    
    assert(cache_offset > 0);
    assert(cache_offset <= block_type::size);
  }
  value_type & top()
  {
    assert(size_ > 0);
    assert(cache_offset > 0);
    assert(cache_offset <= block_type::size);
    return current;
  }
  const value_type & top() const
  {
    assert(size_ > 0);
    assert(cache_offset > 0);
    assert(cache_offset <= block_type::size);
    return current;
  }
  void pop()
  {
    STXXL_VERBOSE3("grow_shrink_stack2::pop()")
    assert(size_ > 0);
    assert(cache_offset > 0);
    assert(cache_offset <= block_type::size);
    if(cache_offset == 1 && (!bids.empty()) )
    {
      STXXL_VERBOSE2("grow_shrink_stack2::pop() shrinking, size = "<<size_)
      
      bid_type last_block = bids.back();
      bids.pop_back();
      /*block_type * b = w_pool.steal(last_block);
      if(b)
      {
        STXXL_VERBOSE2("grow_shrink_stack2::pop() block is still in write buffer")
        w_pool.add(cache);
        cache = b;
      }
      else*/
      {
        //STXXL_VERBOSE2("grow_shrink_stack2::pop() block is no longer in write buffer"
        //  ", reading from prefetch/read pool")
        p_pool.read(cache,last_block)->wait();
      }
      block_manager::get_instance()->delete_block(last_block);
      const int bids_size = bids.size();
      const int last_pref = STXXL_MAX(int(bids_size) - int(pref_aggr),0);
      for(int i=bids_size - 1; i >= last_pref ; --i )
      {
        p_pool.hint(bids[i]); // prefetch
      }
      cache_offset = block_type::size + 1;
      
    }
    
    --cache_offset;
    if(cache_offset > 0)
      current = (*cache)[cache_offset - 1];
    --size_;
  }
  //! \brief Sets level of prefetch aggressiveness (number 
  //! of blocks from the prefetch pool used for prefetching)
  //! \param new_p new value for the prefetch aggressiveness
  void set_prefetch_aggr(unsigned new_p)
  {
    if(pref_aggr > new_p)
    {
      const int bids_size = bids.size();
      const int last_pref = STXXL_MAX(int(bids_size) - int(pref_aggr),0);
      for(int i=bids_size - new_p - 1 ; i >= last_pref ; --i )
      {
        if(p_pool.in_prefetching(bids[i]))
            p_pool.read(cache,bids[i])->wait(); //  clean prefetch buffers
      }
    }
    else
    if(pref_aggr < new_p)
    {
      const int bids_size = bids.size();
      const int last_pref = STXXL_MAX(int(bids_size) - int(new_p),0);
      for(int i=bids_size - 1 ; i >= last_pref ; --i )
      {
        p_pool.hint(bids[i]); // prefetch
      }
    }
    pref_aggr = new_p;
  }
  //! \brief Returns number of blocks used for prefetching
  unsigned get_prefetch_aggr() const
  {
    return pref_aggr;
  }
};


//! \brief A stack that migrates from internal memory to external when its size exceeds a certain threshold

//! For semantics of the methods see documentation of the STL \c std::stack.
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

  // not implemented yet
  template <class stack_type> 
  migrating_stack(const stack_type & stack_);  
  
  migrating_stack & operator = (const migrating_stack &); // forbidden
public:
  migrating_stack(): int_impl(new int_stack_type()),ext_impl(NULL) {}
  
  void swap(migrating_stack & obj)
  {
	  std::swap(int_impl,obj.int_impl);
	  std::swap(ext_impl,obj.ext_impl);
  }
	  
  //! \brief Returns true if current implementation is internal, otherwise false
  bool internal() const
  { 
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    return int_impl; 
  }
  //! \brief Returns true if current implementation is external, otherwise false
  bool external() const
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
  const value_type & top() const
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
  virtual ~migrating_stack()
  {
    assert((int_impl && !ext_impl)||(!int_impl && ext_impl));
    
    if(int_impl)
      delete int_impl;
    else
      delete ext_impl;
  }
};

//! \}


//! \addtogroup stlcont
//! \{

enum stack_externality { external, migrating, internal };
enum stack_behaviour { normal, grow_shrink, grow_shrink2 };

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
//!    - \c grow_shrink2 , efficient version, implemented in \c stxxl::grow_shrink_stack2
//!  - \c BlocksPerPage defines how many blocks has one page of internal cache of an 
//!       \b external implementation, default is four. All \b external implementations have 
//!       \b two pages.
//!  - \c BlkSz external block size in bytes, default is 2 Mbytes
//!  - \c IntStackTp type of internal stack used for some implementations
//!  - \c MigrCritSize threshold value for number of elements when 
//!    \c stxxl::migrating_stack migrates to the external memory
//!  - \c AllocStr one of allocation strategies: \c striping , \c RC , \c SR , or \c FR
//!    default is RC
//!  - \c SzTp size type, default is \c stxxl::int64
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
//! For configured stack method semantics see documentation of the STL \c std::stack.
template  <
            class ValTp,
            stack_externality  Externality = external,
            stack_behaviour    Behaviour = normal,
            unsigned BlocksPerPage = 4,
            unsigned BlkSz = STXXL_DEFAULT_BLOCK_SIZE(ValTp),

            class IntStackTp = std::stack<ValTp>,
            unsigned MigrCritSize = (2*BlocksPerPage*BlkSz),

            class AllocStr = STXXL_DEFAULT_ALLOC_STRATEGY,
            class SzTp = stxxl::int64
          >
class STACK_GENERATOR
{
  typedef stack_config_generator<ValTp,BlocksPerPage,BlkSz,AllocStr,SzTp> cfg;
  
  typedef typename IF<Behaviour==grow_shrink,
          grow_shrink_stack<cfg>,
          grow_shrink_stack2<cfg> >::result GrShrTp;
  typedef typename IF<Behaviour==normal, normal_stack<cfg>, GrShrTp >::result ExtStackTp;
  typedef typename IF<Externality==migrating, 
    migrating_stack<MigrCritSize,ExtStackTp,IntStackTp>,ExtStackTp>::result MigrOrNotStackTp;
  
public:
  
  typedef typename IF<Externality==internal,IntStackTp,MigrOrNotStackTp>::result result;

};

//! \}

__STXXL_END_NAMESPACE
 

namespace std
{
	template <class Config_>
	void swap(	stxxl::normal_stack<Config_> & a,
				stxxl::normal_stack<Config_> & b)
	{
		a.swap(b);
	}
	
	template <class Config_>
	void swap(	stxxl::grow_shrink_stack<Config_> & a,
				stxxl::grow_shrink_stack<Config_> & b)
	{
		a.swap(b);
	}
	
	template <class Config_>
	void swap(	stxxl::grow_shrink_stack2<Config_> & a,
				stxxl::grow_shrink_stack2<Config_> & b)
	{
		a.swap(b);
	}
	
	template <unsigned CritSize, class ExternalStack, class InternalStack>
	void swap(	stxxl::migrating_stack<CritSize,ExternalStack,InternalStack> & a,
				stxxl::migrating_stack<CritSize,ExternalStack,InternalStack> & b)
	{
		a.swap(b);
	}
}

 #endif
