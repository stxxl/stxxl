#ifndef FASTSORT 
#define FASTSORT
#include "util.h"

template <class T> 
void printArray(T* a, int n)
{
  for (int i = 0;  i < n;  i++) cout << a[i] << " ";
  cout << endl;
}


template <class T> 
bool checkSorted(T* a, int n)
{
  for (int i = 0;  i < n-1;  i++) {
    if (a[i+1] < a[i]) return false;
  }
  return true;
}

static void exclusivePrefixSum(int *bucket, int K) {
  int sum = 0;
  for (int i = 0;  i < K;  i++) {
    int current = bucket[i];
    bucket[i]   = sum;
    sum        += current;
  }
} 

template <class T> static void
count(T* a, T* aEnd, int *bucket, int K, int offset, int shift) {
  // reset buckets
  for (int i = 0;  i < K;  i++) { bucket[i] = 0; }

  // count occupancies
  for (T* p = a;  p < aEnd;  p++) {
    int i = (key(*p) - offset) >> shift;
    Assert1(i >= 0 && i < K);
    bucket[i]++;
  }
}

// distribute input a to output b using bucket for the starting indices
template <class T> static void
classify(T* a, T* aEnd, T* b, int *bucket, int offset, int shift) {
  for (T* p = a;  p < aEnd;  p++) {
    int i      = (key(*p) - offset) >> shift;
    int bi    = bucket[i];
    b[bi]     = *p;
    bucket[i] = bi + 1;
  }
}


//template <class T> inline void 
//swap(T& a, T& b) {
//  T temp = a;
//  a      = b;
//  b      = temp;
//}


template <class T> inline void 
sort2(T& a, T& b) { if (b < a) swap(a, b); }


template <class T> inline void 
sort3(T& a, T& b, T& c) {
  T temp;
  if (b < a) { 
    if (c < a) {                         // b , c < a
      if (b < c) {                       // b < c < a
        temp = a; a=b;  b=c;  c=temp;
      } else {                           // c <=b < a
        swap(c,a);
      }
    } else {                             // b < a <=c
      swap(a, b);
    }
  } else {                               // a <=b
    if (c < a) {                         // c < a <=b
      temp=a; a=c;  b=temp;  c=b;
    } else {                             // a <=b , c
      if (c < b) {                       // a <=c < b
        swap(b,c);
      }
    }
  }
  Assert1(!(b < a) && !(c < b));
}


template <class T> inline void
sort4(T& a, T& b, T& c, T& d) {
  sort2(a, b);
  sort2(c, d);          // a < b ; c < d
  if (c < a) {          // c minimal, a < b
    if (d < a) {        // c < d < a < b
      swap(a,c);
      swap(b,d);
    } else {            // c < a < {db}
      if (d < b) {      // c < a < d < b
        T temp = a;  a = c;  c = d;  d = b;  b = temp;
      } else {          // c < a < b < d
        T temp = a;  a = c;  c = b;  b = temp;  
      }
    }
  } else {              // a minimal ; c < d
    if (c < b) {        // c < (bd)
      if (d < b) {      // c < d < b
        T temp = b;  b = c;  c = d;  d = temp;    
      } else {          // a < c < b < d
        swap(b, c);
      }
    } // else sorted
  }
  Assert1(!(b < a) && !(c < b) & !(d < c));
}


template <class T> inline void
sort5(T& a, T& b, T& c, T& d, T& e) {
  sort2(a, b);
  sort2(d, e);
  if (d < a){
    swap(a, d); 
    swap(b, e); 
  }                   // a < d < e, a < b
  if (d < c) {
    swap(c,d);        // a minimal, c < {de}
    sort2(d,e);
  } else {            // a<b, a<d<e, c<d<e
    sort2(a, c);
  }                   // a min, c < d < e
  // insert b int cde by binary search
  if (d < b) {        // c < d < {be}
    if (e < b) {      // c < d < e < b
      T temp = b;  b = c;  c = d;  d = e;  e = temp;
    } else {          // c < d < b < e
      T temp = b;  b = c;  c = d;  d = temp;
    }
  } else {            // {cb} <=d < e 
    sort2(b,c);
  }
  Assert1(!(b < a) && !(c < b) & !(d < c) & !(e < d));
}


template <class T> inline void
insertionSort(T* a, T* aEnd) {
  T* pp;
  for (T* p=a+1;  p < aEnd;  p++) {
    // Invariant a..p-1 is sorted;
    T t = *p;
    if (t < *a) { // new minimum
      // move stuff to the right
      for (pp = p; pp != a;  pp--) {
        *pp = *(pp-1);
      }
      *pp = t;
    } else {
      // now we can use *a as a sentinel
      for (pp = p;  t < *(pp-1);  pp--) {
        *pp = *(pp-1);
      }  
      *pp = t;
    }
  }
}

// sort each bucket
// bucket[i] is an index one off to the right from
// the end of the i-th bucket
template <class T> static void
cleanup(T* b, int *bucket, int K) {
  T* c = b;
  for (int i = 0;  i < K;  i++) {
    T* cEnd = b + bucket[i];
    switch (cEnd - c) {
    case  0:                                       break;
    case  1:                                       break;
    case  2: sort2(c[0], c[1]);                    break;
    case  3: sort3(c[0], c[1], c[2]);              break;
    case  4: sort4(c[0], c[1], c[2], c[3]);        break;
    case  5: //sort5(c[0], c[1], c[2], c[3], c[4]);  break;
    case  6:
    case  7:
    case  8:
    case  9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16: insertionSort(c, cEnd);        break;
    default: sort(c, cEnd);
    }
    c = cEnd;
  }
}


// do a single level MDS radix sort
// using bucket[0..K-1] as a counter array
// and using (key(x) - offset) >> shift to index buckets.
// and using (key(x) - offset) >> shift to index buckets.
// the input comes from a..aEnd-1
// the output goes to b
template <class T> void 
l1sort(T* a, T* aEnd, T* b, int *bucket, int K, int offset, int shift) {
  count(a, aEnd, bucket, K, offset, shift);
  exclusivePrefixSum(bucket, K);
  classify(a, aEnd, b, bucket, offset, shift);
  cleanup(b, bucket, K);
}

// do a two level MDS radix sort
// using bucket1[0..2^logK1-1] as a counter array for the top    level pass
// using bucket2[0..2^logK2-1] as a counter array for the bottom level pass
// and using (key(x) - offset) >> shift to index buckets in the top level
// the input comes from a..aEnd-1 and also goes there
// b is temporary storage of the same size
template <class T> void 
l2sort(T* a, T* aEnd, T* b, 
       int *bucket1, int *bucket2, 
       int logK1, int logK2, 
       int offset, int shift) {
  int K1 = 1<<logK1;
  int K2 = 1<<logK2;
  count(a, aEnd, bucket1, K1, offset, shift);
  exclusivePrefixSum(bucket1, K1);
  classify(a, aEnd, b, bucket1, offset, shift);
  Debug2(printArray(b, aEnd-a));

  // recurse on each bucket
  T* c = b;
  T* d = a;
  for (int i = 0;  i < K1;  i++) {
    T* cEnd = b + bucket1[i];
    T* dEnd = a + bucket1[i];
    l1sort(c, cEnd, d, bucket2, K2, offset + (1<<shift)*i, shift - logK2);
    Debug2(printArray(d, dEnd-d));
    c = cEnd;
    d = dEnd;
  }
}

#endif
