 /***************************************************************************
 *            p_queue.cpp
 *
 *  Fri Jul  4 11:31:34 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "priority_queue.h"

using namespace stxxl;

typedef int my_type;
typedef typed_block<4096,my_type> block_type;

struct dummy_merger
{
  int & cnt;
  dummy_merger(int & c):cnt(c) {}
  template <class OutputIterator>
  void multi_merge(OutputIterator b,OutputIterator e)
  {
    while(b!=e)
    {
      *b = cnt;
      ++b;
      ++cnt;
    }
  }
};

struct my_cmp: public std::greater<my_type>
{
  my_type min_value() const { return std::numeric_limits<my_type>::max(); }
  my_type max_value() const { return std::numeric_limits<my_type>::min(); }
};

int main()
{/*
      unsigned BufferSize1_ = 32, // equalize procedure call overheads etc. 
      unsigned N_ = 512, // bandwidth
      unsigned IntKMAX_ = 64, // maximal arity for internal mergers
      unsigned IntLevels_ = 4, 
      unsigned BlockSize_ = (2*1024*1024),
      unsigned ExtKMAX_ = 64, // maximal arity for external mergers
      unsigned ExtLevels_ = 2,
  */
  priority_queue<priority_queue_config<my_type,my_cmp,2,512,2,3,4096,1024,1> > p;
  off_t nelements = 10*1024,i;
  for(i = 0;i<nelements ;i++ )
    p.push(nelements - i);
  
  for(i = 0; i<(nelements) ;i++ )
  {
    assert( !p.empty() );
    STXXL_MSG( p.top() )
    assert(p.top() == i+1);
    p.pop();
  }
  
}
