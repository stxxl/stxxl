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


void printArray(Data *a, int n)
{
  for (int i = 0;  i < n;  i++) cout << a[i] << " ";
  cout << endl;
}


bool checkSorted(Data *a, int n)
{
  for (int i = 0;  i < n-1;  i++) {
    if (a[i+1] < a[i]) return false;
  }
  return true;
}
 
int main(int argc, char **argv)
{
  int n        = atoi(argv[1]);  // number of elements
  int logK     = atoi(argv[2]);  // log_2(number of buckets)
  Assert0(logK <= BITS);
  int K        = 1<<logK;
  int repeat   = atoi(argv[3]);
  int seed     = atoi(argv[4]);
  sgenrand(seed);

  Data *a = new Data[n];
  Data *b = new Data[n];
  int *bucket = new int[K];

  for (int i = 0; i < n;  i++) {
    b[i].k = rand0K(1<<BITS);
  }
  Debug2(printArray(b, n));
  double start = cpuTime();
  for (int j = 0;  j<repeat;  j++) {
    memcpy(a, b, n*sizeof(Data));  
    l1sort(b, b + n, a, bucket, K, 0, BITS-logK);
    //sort(a, a + n);
    Assert1(checkSorted(a,n));
    Debug2(printArray(a, n));
  }
  double sumTime = cpuTime() - start;
  cout << checkSorted(a, n) 
       << " time:" << sumTime
       << endl;
  Debug2(printArray(a, n));
  delete [] a;
  delete [] b;
  delete [] bucket;
}
