/***************************************************************************
 *  include/stxxl/bits/algo/intksort.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Peter Sanders <sanders@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_INTKSORT_HEADER
#define STXXL_INTKSORT_HEADER

#include <algorithm>
#include <cassert>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/unused.h>


__STXXL_BEGIN_NAMESPACE

template <typename type_key>
static void
count(type_key * a, type_key * aEnd, int_type * bucket, int_type K, typename type_key::key_type offset,
      unsigned shift)
{
    // reset buckets
    std::fill(bucket, bucket + K, 0);

    // count occupancies
    for (type_key * p = a; p < aEnd; p++)
    {
        int_type i = (p->key - offset) >> shift;
        /*
        if (!(i < K && i >= 0))
        {
            STXXL_ERRMSG("i: " << i);
            abort();
        }
        */
        bucket[i]++;
    }
}


static void
exclusive_prefix_sum(int_type * bucket, int_type K)
{
    int_type sum = 0;
    for (int_type i = 0; i < K; i++)
    {
        int_type current = bucket[i];
        bucket[i] = sum;
        sum += current;
    }
}


// distribute input a to output b using bucket for the starting indices
template <typename type_key>
static void
classify(type_key * a, type_key * aEnd, type_key * b, int_type * bucket, typename type_key::key_type offset, unsigned shift)
{
    for (type_key * p = a; p < aEnd; p++)
    {
        int_type i = (p->key - offset) >> shift;
        int_type bi = bucket[i];
        b[bi] = *p;
        bucket[i] = bi + 1;
    }
}


template <class T>
inline void
sort2(T & a, T & b)
{
    if (b < a)
        std::swap(a, b);
}

template <class T>
inline void
sort3(T & a, T & b, T & c)
{
    T temp;
    if (b < a)
    {
        if (c < a)
        {                       // b , c < a
            if (b < c)
            {                   // b < c < a
                temp = a;
                a = b;
                b = c;
                c = temp;
            }
            else
            {                   // c <=b < a
                std::swap(c, a);
            }
        }
        else
        {                       // b < a <=c
            std::swap(a, b);
        }
    }
    else
    {                           // a <=b
        if (c < a)
        {                       // c < a <=b
            temp = a;
            a = c;
            c = b;
            b = temp;
        }
        else
        {                       // a <=b , c
            if (c < b)
            {                   // a <=c < b
                std::swap(b, c);
            }
        }
    }
    // Assert1 (!(b < a) && !(c < b));
}


template <class T>
inline void
sort4(T & a, T & b, T & c, T & d)
{
    sort2(a, b);
    sort2(c, d);                // a < b ; c < d
    if (c < a)
    {                           // c minimal, a < b
        if (d < a)
        {                       // c < d < a < b
            std::swap(a, c);
            std::swap(b, d);
        }
        else
        {                       // c < a < {db}
            if (d < b)
            {                   // c < a < d < b
                T temp = a;
                a = c;
                c = d;
                d = b;
                b = temp;
            }
            else
            {                   // c < a < b < d
                T temp = a;
                a = c;
                c = b;
                b = temp;
            }
        }
    }
    else
    {                           // a minimal ; c < d
        if (c < b)
        {                       // c < (bd)
            if (d < b)
            {                   // c < d < b
                T temp = b;
                b = c;
                c = d;
                d = temp;
            }
            else
            {                   // a < c < b < d
                std::swap(b, c);
            }
        }                       // else sorted
    }
    //Assert1 (!(b < a) && !(c < b) & !(d < c));
}


template <class T>
inline void
sort5(T & a, T & b, T & c, T & d, T & e)
{
    sort2(a, b);
    sort2(d, e);
    if (d < a)
    {
        std::swap(a, d);
        std::swap(b, e);
    }                           // a < d < e, a < b
    if (d < c)
    {
        std::swap(c, d);        // a minimal, c < {de}
        sort2(d, e);
    }
    else
    {                           // a<b, a<d<e, c<d<e
        sort2(a, c);
    }                           // a min, c < d < e
    // insert b int cde by binary search
    if (d < b)
    {                           // c < d < {be}
        if (e < b)
        {                       // c < d < e < b
            T temp = b;
            b = c;
            c = d;
            d = e;
            e = temp;
        }
        else
        {                       // c < d < b < e
            T temp = b;
            b = c;
            c = d;
            d = temp;
        }
    }
    else
    {                           // {cb} <=d < e
        sort2(b, c);
    }
    //Assert1 (!(b < a) && !(c < b) & !(d < c) & !(e < d));
}


template <class T>
inline void
insertion_sort(T * a, T * aEnd)
{
    T * pp;
    for (T * p = a + 1; p < aEnd; p++)
    {
        // Invariant a..p-1 is sorted;
        T t = *p;
        if (t < *a)
        {   // new minimum
            // move stuff to the right
            for (pp = p; pp != a; pp--)
            {
                *pp = *(pp - 1);
            }
            *pp = t;
        }
        else
        {
            // now we can use *a as a sentinel
            for (pp = p; t < *(pp - 1); pp--)
            {
                *pp = *(pp - 1);
            }
            *pp = t;
        }
    }
}

// sort each bucket
// bucket[i] is an index one off to the right from
// the end of the i-th bucket
template <class T>
static void
cleanup(T * b, int_type * bucket, int_type K)
{
    T * c = b;
    for (int_type i = 0; i < K; i++)
    {
        T * cEnd = b + bucket[i];
        switch (cEnd - c)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            sort2(c[0], c[1]);
            break;
        case 3:
            sort3(c[0], c[1], c[2]);
            break;
        case 4:
            sort4(c[0], c[1], c[2], c[3]);
            break;
        case 5:
#if 0
            sort5(c[0], c[1], c[2], c[3], c[4]);
            break;
#endif
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            insertion_sort(c, cEnd);
            break;
        default:
            std::sort(c, cEnd);
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
template <typename type_key>
void
l1sort(type_key * a,
       type_key * aEnd,
       type_key * b, int_type * bucket, int_type K, typename type_key::key_type offset, int shift)
{
    count(a, aEnd, bucket, K, offset, shift);
    exclusive_prefix_sum(bucket, K);
    classify(a, aEnd, b, bucket, offset, shift);
    cleanup(b, bucket, K);
}

template <typename type, typename type_key, typename key_extractor>
void classify_block(type * begin, type * end, type_key * & out,
                    int_type * bucket, typename key_extractor::key_type offset, unsigned shift, key_extractor keyobj)
{
    assert(shift < (sizeof(typename key_extractor::key_type) * 8 + 1));
    for (type * p = begin; p < end; p++, out++) // count & create references
    {
        out->ptr = p;
        typename key_extractor::key_type key = keyobj(*p);
        int_type ibucket = (key - offset) >> shift;
        out->key = key;
        bucket[ibucket]++;
    }
}
template <typename type, typename type_key, typename key_extractor>
void classify_block(type * begin, type * end, type_key * & out, int_type * bucket, typename type::key_type offset, unsigned shift,
                    const int_type K, key_extractor keyobj)
{
    assert(shift < (sizeof(typename type::key_type) * 8 + 1));
    for (type * p = begin; p < end; p++, out++) // count & create references
    {
        out->ptr = p;
        typename type::key_type key = keyobj(*p);
        int_type ibucket = (key - offset) >> shift;
        /*
        if (!(ibucket < K && ibucket >= 0))
        {
            STXXL_ERRMSG("ibucket: " << ibucket << " K:" << K);
            abort();
        }
        */
        out->key = key;
        bucket[ibucket]++;
    }
    STXXL_UNUSED(K);
}

__STXXL_END_NAMESPACE

#endif // !STXXL_INTKSORT_HEADER
