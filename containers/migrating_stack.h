/***************************************************************************
 *            migrating_stack.h
 *
 *  Fri May  2 12:27:10 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stack.h"
#include <stack>

__STXXL_BEGIN_NAMESPACE

//! \brief A stack that migrates from internal memory to external when its size exceeds a certain threshold

//! Template parameters:
//!  - \c Tp_ type of contained objects
//!  - \c CriticalSize_ threshold value for number of elements when stack migrates to the external memory
//!  - \c AllocStr_ one of allocation strategies: \c striping , \c RC , \c SR , or \c FR
//!  default is RC 
//!  - \c BlkSize_ external block size in bytes, default is 2 Mbytes
//!  - \c BlkCacheSize_ how many blocks use for internal cache, default is four
template <
    typename Tp_,
    unsigned CriticalSize_,
		typename AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY,
		unsigned BlkSize_ = STXXL_DEFAULT_BLOCK_SIZE (_Tp),
		unsigned BlkCacheSize_ = 4 >
class migrating_stack
{
public:
  // stl typedefs
  typedef Tp_ value_type;
  typedef off_t size_type;
  
  typedef std::stack<value_type> int_stack_type;
  typedef stack<Tp_,AllocStr_,BlkSize_,BlkCacheSize_> ext_stack_type;
  
private:
  enum { critical_size = CriticalSize_ };
    
  int_stack_type * int_impl;
  ext_stack_type * ext_impl;

public:
  migrating_stack(): int_impl(new int_stack_type()),ext_impl(NULL) {}
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

__STXXL_END_NAMESPACE
