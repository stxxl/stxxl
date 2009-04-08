/***************************************************************************
 *  include/stxxl/bits/containers/pq_mergers.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 1999 Peter Sanders <sanders@mpi-sb.mpg.de>
 *  Copyright (C) 2003, 2004, 2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PQ_MERGERS_HEADER
#define STXXL_PQ_MERGERS_HEADER

__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
 */
namespace priority_queue_local
{
/////////////////////////////////////////////////////////////////////
// auxiliary functions

// merge length elements from the two sentinel terminated input
// sequences source0 and source1 to target
// advance source0 and source1 accordingly
// require: at least length nonsentinel elements available in source0, source1
// require: target may overwrite one of the sources as long as
//   *(sourcex + length) is before the end of sourcex
    template <class InputIterator, class OutputIterator, class Cmp_>
    void merge_iterator(
        InputIterator & source0,
        InputIterator & source1,
        OutputIterator target, unsigned_type length, Cmp_ cmp)
    {
        OutputIterator done = target + length;

        while (target != done)
        {
            if (cmp(*source0, *source1))
            {
                *target = *source1;
                ++source1;
            }
            else
            {
                *target = *source0;
                ++source0;
            }
            ++target;
        }
    }

// merge length elements from the three sentinel terminated input
// sequences source0, source1 and source2 to target
// advance source0, source1 and source2 accordingly
// require: at least length nonsentinel elements available in source0, source1 and source2
// require: target may overwrite one of the sources as long as
//   *(sourcex + length) is before the end of sourcex
    template <class InputIterator, class OutputIterator, class Cmp_>
    void merge3_iterator(
        InputIterator & source0,
        InputIterator & source1,
        InputIterator & source2,
        OutputIterator target, unsigned_type length, Cmp_ cmp)
    {
        OutputIterator done = target + length;

        if (cmp(*source1, *source0)) {
            if (cmp(*source2, *source1)) {
                goto s012;
            }
            else {
                if (cmp(*source0, *source2)) {
                    goto s201;
                }
                else {
                    goto s021;
                }
            }
        } else {
            if (cmp(*source2, *source1)) {
                if (cmp(*source2, *source0)) {
                    goto s102;
                }
                else {
                    goto s120;
                }
            } else {
                goto s210;
            }
        }

#define Merge3Case(a, b, c) \
    s ## a ## b ## c : \
    if (target == done) \
        return;\
    *target = *source ## a; \
    ++target; \
    ++source ## a; \
    if (cmp(*source ## b, *source ## a)) \
        goto s ## a ## b ## c;\
    if (cmp(*source ## c, *source ## a)) \
        goto s ## b ## a ## c;\
    goto s ## b ## c ## a;

        // the order is chosen in such a way that
        // four of the trailing gotos can be eliminated by the optimizer
        Merge3Case(0, 1, 2);
        Merge3Case(1, 2, 0);
        Merge3Case(2, 0, 1);
        Merge3Case(1, 0, 2);
        Merge3Case(0, 2, 1);
        Merge3Case(2, 1, 0);

#undef Merge3Case
    }


// merge length elements from the four sentinel terminated input
// sequences source0, source1, source2 and source3 to target
// advance source0, source1, source2 and source3 accordingly
// require: at least length nonsentinel elements available in source0, source1, source2 and source3
// require: target may overwrite one of the sources as long as
//   *(sourcex + length) is before the end of sourcex
    template <class InputIterator, class OutputIterator, class Cmp_>
    void merge4_iterator(
        InputIterator & source0,
        InputIterator & source1,
        InputIterator & source2,
        InputIterator & source3,
        OutputIterator target, unsigned_type length, Cmp_ cmp)
    {
        OutputIterator done = target + length;

#define StartMerge4(a, b, c, d) \
    if ((!cmp(*source ## a, *source ## b)) && (!cmp(*source ## b, *source ## c)) && (!cmp(*source ## c, *source ## d))) \
        goto s ## a ## b ## c ## d;

        // b>a c>b d>c
        // a<b b<c c<d
        // a<=b b<=c c<=d
        // !(a>b) !(b>c) !(c>d)

        StartMerge4(0, 1, 2, 3);
        StartMerge4(1, 2, 3, 0);
        StartMerge4(2, 3, 0, 1);
        StartMerge4(3, 0, 1, 2);

        StartMerge4(0, 3, 1, 2);
        StartMerge4(3, 1, 2, 0);
        StartMerge4(1, 2, 0, 3);
        StartMerge4(2, 0, 3, 1);

        StartMerge4(0, 2, 3, 1);
        StartMerge4(2, 3, 1, 0);
        StartMerge4(3, 1, 0, 2);
        StartMerge4(1, 0, 2, 3);

        StartMerge4(2, 0, 1, 3);
        StartMerge4(0, 1, 3, 2);
        StartMerge4(1, 3, 2, 0);
        StartMerge4(3, 2, 0, 1);

        StartMerge4(3, 0, 2, 1);
        StartMerge4(0, 2, 1, 3);
        StartMerge4(2, 1, 3, 0);
        StartMerge4(1, 3, 0, 2);

        StartMerge4(1, 0, 3, 2);
        StartMerge4(0, 3, 2, 1);
        StartMerge4(3, 2, 1, 0);
        StartMerge4(2, 1, 0, 3);

#define Merge4Case(a, b, c, d) \
    s ## a ## b ## c ## d : \
    if (target == done) \
        return;\
    *target = *source ## a; \
    ++target; \
    ++source ## a; \
    if (cmp(*source ## c, *source ## a)) \
    { \
        if (cmp(*source ## b, *source ## a)) \
            goto s ## a ## b ## c ## d;\
        else \
            goto s ## b ## a ## c ## d;\
    } \
    else \
    { \
        if (cmp(*source ## d, *source ## a)) \
            goto s ## b ## c ## a ## d;\
        else \
            goto s ## b ## c ## d ## a;\
    }

        Merge4Case(0, 1, 2, 3);
        Merge4Case(1, 2, 3, 0);
        Merge4Case(2, 3, 0, 1);
        Merge4Case(3, 0, 1, 2);

        Merge4Case(0, 3, 1, 2);
        Merge4Case(3, 1, 2, 0);
        Merge4Case(1, 2, 0, 3);
        Merge4Case(2, 0, 3, 1);

        Merge4Case(0, 2, 3, 1);
        Merge4Case(2, 3, 1, 0);
        Merge4Case(3, 1, 0, 2);
        Merge4Case(1, 0, 2, 3);

        Merge4Case(2, 0, 1, 3);
        Merge4Case(0, 1, 3, 2);
        Merge4Case(1, 3, 2, 0);
        Merge4Case(3, 2, 0, 1);

        Merge4Case(3, 0, 2, 1);
        Merge4Case(0, 2, 1, 3);
        Merge4Case(2, 1, 3, 0);
        Merge4Case(1, 3, 0, 2);

        Merge4Case(1, 0, 3, 2);
        Merge4Case(0, 3, 2, 1);
        Merge4Case(3, 2, 1, 0);
        Merge4Case(2, 1, 0, 3);

#undef StartMerge4
#undef Merge4Case
    }
} //priority_queue_local

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_PQ_MERGERS_HEADER
// vim: et:ts=4:sw=4
