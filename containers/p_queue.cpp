 /***************************************************************************
 *            p_queue.cpp
 *
 *  Fri Jul  4 11:31:34 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "priority_queue.h"

using namespace stxxl;


struct my_type
{
	int key;
	//char data[128 - sizeof(int)];
	my_type(){}
	explicit my_type(int k):key(k) {}
};

std::ostream & operator << (std::ostream & o,const my_type & obj)
{
	o << obj.key;
	return o;
}

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

struct my_cmp // greater
{
  bool operator () (const my_type & a, const my_type & b) const { return a.key > b.key; }
  my_type min_value() const { return my_type(std::numeric_limits<int>::max()); }
  my_type max_value() const { return my_type(std::numeric_limits<int>::min()); }
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
  typedef priority_queue<priority_queue_config<my_type,my_cmp,
    32,512,64,3,(4*1024),0x7fffffff,1> > pq_type;
  //typedef PRIORITY_QUEUE_GENERATOR<my_type,my_cmp,128*1024*1024,128*1024*1024/sizeof(my_type)>::result pq_type;
  typedef pq_type::block_type block_type;
  
  //STXXL_MSG(settings::EConsumption);
  /*
  STXXL_MSG(settings::AE);
  STXXL_MSG(settings::settings::B);
  STXXL_MSG(settings::N); */
  
  prefetch_pool<block_type> p_pool(10);
  write_pool<block_type>    w_pool(10);
  pq_type p(p_pool,w_pool);
  off_t nelements = 5*off_t(1024*1024),i;
  STXXL_MSG("Internal memory consumption of the priority queue: "<<p.mem_cons()<<" bytes")
  for(i = 0;i<nelements ;i++ )
    p.push(my_type(nelements - i));
  
  STXXL_MSG("Internal memory consumption of the priority queue: "<<p.mem_cons()<<" bytes")
  for(i = 0; i<(nelements) ;i++ )
  {
    assert( !p.empty() );
    STXXL_MSG( p.top() )
    assert(p.top().key == i+1);
    p.pop();
  }
  STXXL_MSG("Internal memory consumption of the priority queue: "<<p.mem_cons()<<" bytes")
  
}
