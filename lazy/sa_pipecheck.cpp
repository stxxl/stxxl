 #include "algorithm.h"
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include "../common/timer.h"
 
//! \brief suffix array construction by KJ and PS 
#include "drittel.C"
 
#define ALPHABET_SIZE 128


struct my_tuple
{
  int i1,i2;
  struct cmp
  {
    bool operator () (const my_tuple & a, const my_tuple & b)
    {
      return a.i2 < b.i2;
    }
  };
  my_tuple() {}
  my_tuple(int _1, int _2): i1(_1),i2(_2) {}
};

struct my_triple
{
  int i1,i2,i3;
  struct cmp1
  {
    bool operator () (const my_triple & a, const my_triple & b)
    {
      return a.i1 < b.i1;
    }
  };
  struct cmp2
  {
    bool operator () (const my_triple & a, const my_triple & b)
    {
      return (a.i2 < b.i2) || (a.i2 == b.i2 && a.i3 < b.i3);
    }
  };
  my_triple() {}
  my_triple(int _1, int _2, int _3): i1(_1),i2(_2),i3(_3) {}
};



template <class LazyAlgorithm_>
class tuple_generator
{
  LazyAlgorithm_ & input_;
  int i;
  tuple_generator() {}
public:
  tuple_generator(LazyAlgorithm_ & input): input_(input),i(0) {}
   
  typedef my_tuple result_type;
    
  bool empty() { return input_.empty(); }
  bool next() { return input_.next(); }
  result_type current()
  {
    return result_type(i++,input_.current());
  }
};

template <class LazyAlgorithm1_,class LazyAlgorithm2_>
class triple_generator
{
  LazyAlgorithm1_ & tuples;
  LazyAlgorithm2_ & str;
  bool first;
  my_tuple cur_tuple;
  triple_generator() {}
public:
  triple_generator(LazyAlgorithm1_ & tuples_,LazyAlgorithm2_ & str_): 
    tuples(tuples_),str(str_),first(true) {}
    
  typedef my_triple result_type;
    
  bool empty() { return str.empty(); }
  bool next()
  {
    if(!tuples.empty())
    {
      cur_tuple = tuples.current();
      tuples.next();
    }
    
    return str.next();
  }
  result_type current()
  {
    if(first)
    {
      first = false;
      cur_tuple = tuples.current();
      tuples.next();
    }
    
    if(tuples.empty())
      return my_triple(cur_tuple.i1,str.current(),-1);
    
    return my_triple(cur_tuple.i1,str.current(),tuples.current().i1);
  }
  
};

template <class LazyAlgorithm_, class Cmp_>
bool is_sorted(LazyAlgorithm_ & alg,Cmp_ cmp)
{
  typename LazyAlgorithm_::result_type last_record = alg.current(); alg.next();
  while(!alg.empty())
  {
    typename LazyAlgorithm_::result_type cur_record = alg.current();
    if(!cmp(last_record,cur_record))
      return false;
    last_record = cur_record;
    alg.next();
  }
  return true;
}


template <class LazyAlgorithm_,class Cmp_>
class lazy_sort
{
public:
  typedef typename LazyAlgorithm_::result_type result_type;
private:
  LazyAlgorithm_ & input;
  result_type * storage;
  result_type * cur;
  const result_type * end;
  bool sorted;
  lazy_sort() {}
public:
    
  lazy_sort(LazyAlgorithm_ & input_,int storage_size_): 
    input(input_),
    storage(new result_type[storage_size_]),
    cur(storage),
    end(storage + storage_size_),
    sorted(false)
  {}
  
  bool empty()
  {
    return (cur == end);
  }
  bool next()
  {
    return (++cur != end);
  }
  result_type current()
  {
    if(!sorted)
    {
      // sort
      result_type * p= storage;
      while(!input.empty())
      {
        *p++=input.current();
        input.next();
      }
      std::sort(storage,p,Cmp_());
      sorted = true;
    }
    return *cur;
  }
  ~lazy_sort()
  {
    delete [] storage;
  }
};




int main(int argc,char * argv[])
{
  int n = atoi(argv[1]);
  int iterations=atoi(argv[2]);
  srand(atoi(argv[3]));
  
  int * str = new int[n+4];
  int * SA = new int[n+4];
  
  
  bool result=false;
  
  for(int i=0;i<n;i++)
    str[i] = 1 + (rand()%ALPHABET_SIZE);
  str[n] = str [n+1] = str [n+2] = 0;
  
  suffixArray(str,SA,n,ALPHABET_SIZE+2);
  
  stxxl::timer timer_;
  
  while(iterations--)
  {
    typedef iterator_range_to_la<int *> int_stream_type;
    typedef tuple_generator<int_stream_type> tuple_stream_type;
    typedef lazy_sort<tuple_stream_type,my_tuple::cmp> as_stream_type;
    typedef triple_generator<as_stream_type,int_stream_type> triple_stream_type;
    typedef lazy_sort<triple_stream_type,my_triple::cmp1> sorted_triple_stream_type;
    
    int_stream_type SA_stream = create_lazy_stream(SA,SA+n); 
    int_stream_type str_stream = create_lazy_stream(str,str+n);
    
    tuple_stream_type tuple_stream(SA_stream);
    as_stream_type AS_stream(tuple_stream,n);
    triple_stream_type triple_stream(AS_stream,str_stream);
    sorted_triple_stream_type sorted_triple_stream(triple_stream,n); 
    
    
    timer_.start();
    result = is_sorted(sorted_triple_stream,my_triple::cmp2());
    assert(result);
    timer_.stop();
  }
  std::cout << "Elapsed time: " << timer_.mseconds() << " ms"<<std::endl;
 
  
  
  delete [] str;
  delete [] SA;
  
  return !result;
}
