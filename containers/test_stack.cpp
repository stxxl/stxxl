 /***************************************************************************
 *            test_stack.cpp
 *
 *  Thu May  1 22:02:00 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stack.h"

//! \example test_stack.cpp
//! This is an example of how to use \c stxxl::STACK_GENERATOR class
//! to generate an \b external stack type 
//! with \c stxxl::grow_shrink_stack implementation, \b four blocks per page,
//! block size \b 4096 bytes

using namespace stxxl;

int main(int argc, char * argv[])
{
  typedef STACK_GENERATOR<int,external,grow_shrink,4,4096>::result ext_stack_type;
  typedef STACK_GENERATOR<int,external,grow_shrink2,1,4096>::result ext_stack_type2;
  
  if(argc<2)
  {
    STXXL_MSG("Usage: "<<argv[0]<<" test_size_in_pages")
    abort();
  }
  {
    /*
    ext_stack_type my_stack;
    int test_size = atoi(argv[1])*4*4096/sizeof(int),i;
    
    for(i = 0; i < test_size;i++)
    {
      my_stack.push(i);
      assert(my_stack.top() == i);
      assert(my_stack.size() == i+1);
    }
    
    for(i=test_size-1;i>=0;i--)
    { 
      assert(my_stack.top() == i);
      my_stack.pop();
      assert(my_stack.size() == i);
    };
    
    for(i = 0; i < test_size;i++)
    {
      my_stack.push(i);
      assert(my_stack.top() == i);
      assert(my_stack.size() == i+1);
    }
    
    for(i=test_size-1;i>=0;i--)
    { 
      assert(my_stack.top() == i);
      my_stack.pop();
      assert(my_stack.size() == i);
    };
    
    std::stack<int> int_stack;
    
    for(i = 0; i < test_size;i++)
    {
      int_stack.push(i);
      assert(int_stack.top() == i);
      assert(int(int_stack.size()) == i+1);
    }
    
    
    ext_stack_type my_stack1(int_stack);
    
    for(i=test_size-1;i>=0;i--)
    { 
      assert(my_stack1.top() == i);
      my_stack1.pop();
      assert(my_stack1.size() == i);
    };
    */
    STXXL_MSG("Test 1 passed.")
  }
  {
    prefetch_pool<ext_stack_type2::block_type> p_pool(10);
    write_pool<ext_stack_type2::block_type> w_pool(10);
    ext_stack_type2 my_stack(p_pool,w_pool,2);
    int test_size = atoi(argv[1])*4*4096/sizeof(int),i;
    
    for(i = 0; i < test_size;i++)
    {
      my_stack.push(i);
      assert(my_stack.top() == i);
      assert(my_stack.size() == i+1);
    }
    
    for(i=test_size-1;i>=0;i--)
    { 
      assert(my_stack.top() == i);
      my_stack.pop();
      assert(my_stack.size() == i);
    };
    
    for(i = 0; i < test_size;i++)
    {
      my_stack.push(i);
      assert(my_stack.top() == i);
      assert(my_stack.size() == i+1);
    }
    
    for(i=test_size-1;i>=0;i--)
    { 
      assert(my_stack.top() == i);
      my_stack.pop();
      assert(my_stack.size() == i);
    };
    
   
    STXXL_MSG("Test 2 passed.")
  }
  
  return 0;
}
