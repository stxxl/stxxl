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
};

template <class _ForwardIter, class _StrictWeakOrdering>
bool is_sorted(_ForwardIter __first, _ForwardIter __last,
               _StrictWeakOrdering __comp)
{
  if (__first == __last)
    return true;

  _ForwardIter __next = __first;
  for (++__next; __next != __last; __first = __next, ++__next) {
    if (__comp(*__next, *__first))
      return false;
  }

  return true;
}

int main(int argc,char * argv[])
{
  int n = atoi(argv[1]);
  int iterations=atoi(argv[2]);
  srand(atoi(argv[3]));
  
  int * str = new int[n+4];
  int * SA = new int[n+4];
  my_tuple * tuples = new my_tuple[n];
  my_triple * triples = new my_triple[n];
  int i=0;
  bool result;
  
  for(i=0;i<n;i++)
    str[i] = 1 + (rand()%ALPHABET_SIZE);
  str[n] = str [n+1] = str [n+2] = 0;
  
  suffixArray(str,SA,n,ALPHABET_SIZE+2);
  
  stxxl::timer timer_;
  timer_.start();
  while(iterations--)
  {
    my_tuple *p=tuples;
    for(i=0;p<tuples+n;p++,i++)
    {
      p->i1 = i;
      p->i2 = SA[i];
    }
    std::sort(tuples,tuples+n,my_tuple::cmp());
    my_triple * p1=triples;
    int *p2=str;
    my_tuple *p3=tuples;
    for(;p1<triples + n-1; p1++,p2++,p3++)
    {
      p1->i1 = p3->i1;
      p1->i2 = *p2;
      p1->i3 = (p3+1)->i1;
    }
    p1->i1 = p3->i1;
    p1->i2 = *p2;
    p1->i3 = -1;
    
    std::sort(triples,triples+n,my_triple::cmp1());
    result = is_sorted(triples,triples+n,my_triple::cmp2());
    assert(result);
  }
  timer_.stop();
  std::cout << "Elapsed time: " << timer_.mseconds() << " ms"<<std::endl;
 
  
  delete [] tuples;
  delete [] triples;
  delete [] str;
  delete [] SA;
  
  return !result;
}
