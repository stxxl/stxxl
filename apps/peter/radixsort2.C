#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <algorithm>
#define DEBUGLEVEL 0
#include "util.h"
#include "mt-real.c"
#include "fastsort.h"
//#include "myqsort.h"
using namespace std;

// data type to be sorted
class Data {
public:
  int k;
  void *stuff;

  bool operator < ( const Data& y) const { return k < y.k;}
};

const int BITS = 30;

inline int key(Data x) { return x.k; }

typedef unsigned int Card32;
inline Card32 rand32() 
{  
  static Card32 state = 42;
  state = 1664525 * state + 1013904223;
  return state;
}


ostream& operator<<(ostream& s, const Data& d) {
  return s << key(d);
}


inline double randUniform() 
{ return ((double)rand32() * (0.5 / 0x80000000));
}

// return random int between 0 and k-1
inline int rand0K(int k) 
{ return (int)(genrand() * k);}

 
int main(int argc, char **argv)
{
  int n        = atoi(argv[1]);  // number of elements
  int logK1     = atoi(argv[2]);  // log_2(number of top level buckets)
  int logK2     = atoi(argv[3]);  // log_2(number of bot level buckets)
  Assert0(logK1 + logK2 <= BITS);
  int K1        = 1<<logK1;
  int K2        = 1<<logK2;
  int repeat   = atoi(argv[4]);
  int seed     = atoi(argv[5]);
  sgenrand(seed);

  Data *a = new Data[n];
  Data *b = new Data[n];
  Data *c = new Data[n];
  int *bucket1 = new int[K1];
  int *bucket2 = new int[K2];

  for (int i = 0; i < n;  i++) {
    c[i].k = rand0K(1<<BITS);
  }
  Debug2(printArray(c, n));
  double start = cpuTime();
  for (int j = 0;  j<repeat;  j++) {
    memcpy(a, c, n*sizeof(Data));  
    l2sort(a, a + n, b, bucket1, bucket2, 
           logK1, logK2, 0, BITS-logK1);
    //sort(a, a + n);
    Assert1(checkSorted(a,n));
    Debug2(printArray(a, n));
    Debug2(printArray(b, n));
  }
  double sumTime = cpuTime() - start;
  cout << checkSorted(a, n) 
       << " time:" << sumTime
       << endl;
  Debug2(printArray(a, n));
  delete [] a;
  delete [] b;
  delete [] c;
  delete [] bucket1;
  delete [] bucket2;
}
