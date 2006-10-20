/***************************************************************************
 *            test_mstack.cpp
 *
 *  Fri May  2 13:34:25 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 **************************************************************************/


#include "stack.h"

//! \example containers/test_mstack.cpp
//! This is an example of how to use \c stxxl::STACK_GENERATOR class
//! to generate an \b migrating stack with critical size \c critical_size ,
//! external implementation \c normal_stack , \b four blocks per page,
//! block size \b 4096 bytes, and internal implementation
//! \c std::stack<int>
using namespace stxxl;

int main()
{
  const unsigned critical_size = 8*4096;
  typedef STACK_GENERATOR<int, \
    migrating,normal,4,4096,std::stack<int>,critical_size>::result migrating_stack_type;
  
  
  migrating_stack_type my_stack;
  int test_size = 1*1024*1024/sizeof(int),i;
  
  for(i = 0; i < test_size;i++)
  {
    my_stack.push(i);
    assert(my_stack.top() == i);
    assert(my_stack.size() == i+1);
    assert((my_stack.size() >= critical_size) == my_stack.external() );
  }
  
  // test swap
  migrating_stack_type my_stack2;
  std::swap(my_stack2,my_stack);
  std::swap(my_stack2,my_stack);
  
  for(i=test_size-1;i>=0;i--)
  { 
    assert(my_stack.top() == i);
    my_stack.pop();
    assert(my_stack.size() == i);
    assert(my_stack.external() == (test_size >= int(critical_size)));
  };
  
  STXXL_MSG("Test passed.")
  
  return 0;
}
