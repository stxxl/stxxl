#include "util.h"

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
  int i; 

  // reset buckets
  for (int i = 0;  i < K;  i++) { bucket[i] = 0; }

  // count occupancies
  for (T* p = a;  p < aEnd;  p++) {
    bucket[(key(*p) - offset) >> shift]++;
  }
}

// distribute input using bucket for the starting indices
template <class T> static void
classify(T* a, T* aEnd, T* b, int *bucket, int offset, int shift) {
  for (T* p = a;  p < aEnd;  p++) {
    i         = (key(*p) - offset) >> shift;
    int bi    = bucket[i];
    b[bi]     = *p;
    bucket[i] = bi + 1;
  }
}


template <class T> inline void 
swap(T& a, T& b) {
  T temp = a;
  a      = b;
  b      = temp;
}


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
      temp=a; a=c;  b=a;  c=b;
    } else {                             // a <=b , c
      if (c < b) {                       // a <=c < b
        swap(b,c);
      }
    }
  }
  Assert2(!(b < a) && !(c < b));
}


template <class T> inline void
sort4(T& a, T& b, T& c, T& d) {
  sort2(a, b);
  sort2(c, d);    // a < b ; c < d
  sort2(c, a);    // a minimal ; c < d
  if (c < b) {
    swap(b, c);
    sort2(c, d);
  }
  Assert2(!(b < a) && !(c < b) & !(d < c));
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
  if (b < d) {        // {bc} < d < c
    sort2(b, c);
  } else {            // c < d < {bc}
    T oldB = b; 
    T oldC = c; 
    b = c;  c = d;
    if (oldB < oldC) { d = oldB;  e = oldC; }
    else             { d = oldC;  e = oldB; }
  }
  Assert2(!(b < a) && !(c < b) & !(d < c) & !(e < d));
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
    case  3: //sort3(c[0], c[1], c[2]);              break;
    case  4: //sort4(c[0], c[1], c[2], c[3]);        break;
    case  5: //sort5(c[0], c[1], c[2], c[3], c[4]);  break;
    case  6:
    case  7:
    case  8:
    case  9:
    case 10:
    case 11:
    case 12:
    case 13: //insertionSort(c, cEnd);        break;
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
template <class T> void 
l1sort(T* a, T* aEnd, T* b, int *bucket, int K, int offset, int shift) {
  int i;

  count(a, aEnd, bucket, K, offset, shift);
  exclusivePrefixSum(bucket, K);
  classify(a, aEnd, bucket, b, offset, shift);
  cleanup(b, bucket, K);
}


