/***************************************************************************
 *            test_mstack.cpp
 *
 *  Fri May  2 13:34:25 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "stack.h"

//! \example test_mstack.cpp
//! This is an example of how to use \c stxxl::migrating_stack data structure

using namespace stxxl;

int main()
{
  const unsigned critical_size = 8*4096; 
  typedef stack<normal_stack<stack_config_generator<int,4,RC,4096> > > ext_stack_type;
  typedef stack<migrating_stack<(8*4096),ext_stack_type,std::stack<int> > > migrating_stack_type;
  
  migrating_stack_type my_stack;
  int test_size = 1*1024*1024/sizeof(int),i;
  
  for(i = 0; i < test_size;i++)
  {
    my_stack.push(i);
    assert(my_stack.top() == i);
    assert(my_stack.size() == i+1);
    assert((my_stack.size() >= critical_size) == my_stack.external() );
  }
  
  for(i=test_size-1;i>=0;i--)
  { 
    assert(my_stack.top() == i);
    my_stack.pop();
    assert(my_stack.size() == i);
    assert(my_stack.external() == (test_size >= critical_size));
  };
  
  STXXL_MSG("Test passed.")
  
  return 0;
}
