#ifndef MSORT_HEADER
#define MSORT_HEADER

/***************************************************************************
 *            msort.h
 *
 *  Sat Aug 24 23:46:49 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "../mng/mng.h"
#include "../common/rand.h"
#include "../mng/old_adaptor.h"
#include "../io/io.h"

#include <math.h>
#include <iterator>
#include <vector>
#include <queue>
#include <list>

struct Type
{
  int key;
  char filler[128 - sizeof (int)];
};

// #define B (1024*8)
// #define INPUT_SIZE (4*16*16*16)
// #define M (2*1024*1024)



#define B (512*1024)
// #define INPUT_SIZE (1024*1024*1024/(64*B))
// #define M (128*1024*1024/128)
// #define D 1

// #define m (M/B)
// #define _m m
// #define _n (INPUT_SIZE*D)




// #define B 64*1024

// unsigned INPUT_SIZE = (1024*1024*1024/(64*B));
unsigned M = (128 * 1024 * 1024 / 128);
unsigned D = 1;
unsigned m = M / B;

#define _m m
long long unsigned _n;



using namespace stxxl;


bool
operator < (const Type & a, const Type & b)
{
  return (a.key < b.key);
}

typedef typed_block < B, Type > Block;

struct OffsetStriping:public striping
{
  int offset;
    OffsetStriping (int b, int e, int o):striping (b, e), offset (o)
  {
  };
  int operator    () (int i) const
  {
    return begin + (i + offset) % diff;
  };
};

struct OffsetFR:public FR
{
  OffsetFR (int b, int e, int o):FR (b, e)
  {
  };
};

struct OffsetSR:public SR
{
  OffsetSR (int b, int e, int o):SR (b, e)
  {
  };
};

struct OffsetRC:public RC
{
  OffsetRC (int b, int e, int o):RC (b, e)
  {
  };
};



struct TriggerEntry
{
  BID < B > bid;
  Type value;
};

bool
operator < (const TriggerEntry & a, const TriggerEntry & b)
{
  return (a.value < b.value);
};

/*
 * class Run: public std::vector < TriggerEntry > { Run() {}; public:
 * Run(int _size): std::vector < TriggerEntry > (_size) {}; }; 
 */



template < class _Tp		/* , class
				 * _Alloc=__STL_DEFAULT_ALLOCATOR(_Tp) */  >
  class simple_vector
{

  simple_vector ()
  {
  };
public:
  typedef size_t size_type;
  typedef _Tp value_type;
  // typedef simple_alloc<_Tp, _Alloc> _data_allocator;
protected:
  size_type _size;
  value_type *_array;
public:

  typedef value_type *iterator;
  typedef const value_type *const_iterator;
  typedef value_type & reference;
  typedef const value_type & const_reference;

simple_vector (size_type sz):_size (sz)
  {
    // _array = _data_allocator.allocate(sz);
    _array = new _Tp[sz];
  }
  ~simple_vector ()
  {
    // _data_allocator.deallocate(_array,_size);
    delete[]_array;
  }
  iterator begin ()
  {
    return _array;
  }
  const_iterator begin () const
  {
    return _array;
  }
  iterator end ()
  {
    return _array + _size;
  }
  const_iterator end () const
  {
    return _array + _size;
  }
  size_type size ()
  {
    return _size;
  }
  reference operator [] (size_type i)
  {
    return *(begin () + i);
  }
  const_reference operator [] (size_type i) const
  {
    return *(begin () + i);
  }
};


typedef simple_vector < TriggerEntry > Run;



#define ELEM(blocks,i) blocks[(i)/Block::size].elem[ (i) % Block::size ]

/*
 * #define CHECK_RUN_BOUNDS(pos) \ if(array[( pos )%dim_size]->size() <=
 * (( pos ) / dim_size) ) \ { \ STXXL_ERRMSG("Out of "<< (pos)%dim_size
 * <<"th run bounds:" << pos/dim_size <<" >= " <<
 * array[(pos)%dim_size]->size()<<" pos:"<<pos ) \ abort(); \ } 
 */

#define CHECK_RUN_BOUNDS(pos)

struct InterleavedStriping
{
  int nruns, begindisk, diff;

  InterleavedStriping (int _nruns, int _begindisk,
			 int _enddisk):nruns (_nruns),
    begindisk (_begindisk), diff (_enddisk - _begindisk)
  {
  };

  int operator    () (int i) const
  {
    // std::cout << i<<" " << nruns << " " << begindisk<<" "<<diff <<
    // " " << begindisk + (i/nruns)%diff << std::endl;
    return begindisk + (i / nruns) % diff;
  };
};

struct InterleavedFR:public InterleavedStriping
{
  InterleavedFR (int _nruns, int _begindisk,
		 int _enddisk):InterleavedStriping (_nruns, _begindisk,
						    _enddisk)
  {
  };

  int operator    () (int i) const
  {
    return begindisk + rand0K (diff);
  };
};

struct InterleavedSR:public InterleavedStriping
{
  std::vector < int >offsets;
    InterleavedSR (int _nruns, int _begindisk,
		   int _enddisk):InterleavedStriping (_nruns,
						      _begindisk, _enddisk)
  {
    for (int i = 0; i < nruns; i++)
      offsets.push_back (rand0K (diff));
  };

  int operator    () (int i) const
  {
    return begindisk + (i / nruns + offsets[i % nruns]) % diff;
  };
};



struct InterleavedRC:public InterleavedStriping
{
  std::vector < std::vector < int > > perms;

  InterleavedRC (int _nruns, int _begindisk,
		   int _enddisk):InterleavedStriping (_nruns,
						      _begindisk,
						      _enddisk),
    perms (nruns, std::vector < int >(diff))
  {
    for (int i = 0; i < nruns; i++)
      {
	for (int j = 0; j < diff; j++)
	  perms[i][j] = j;

	stxxl::random_number rnd;
	std::random_shuffle (perms[i].begin (), perms[i].end (), rnd);
      }
  };

  int operator    () (int i) const
  {
    return begindisk + perms[i % nruns][(i / nruns) % diff];
  };
};


#endif
