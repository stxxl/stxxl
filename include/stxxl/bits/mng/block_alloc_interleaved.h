/***************************************************************************
 *  include/stxxl/bits/mng/block_alloc_interleaved.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002, 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_INTERLEAVED_ALLOC_HEADER
#define STXXL_INTERLEAVED_ALLOC_HEADER

#include <vector>

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/common/rand.h>


__STXXL_BEGIN_NAMESPACE

#define CHECK_RUN_BOUNDS(pos)

struct interleaved_striping
{
    int_type nruns;
    int begindisk;
    int diff;
    interleaved_striping(int_type _nruns, int _begindisk,
                         int _enddisk) : nruns(_nruns),
                                         begindisk(_begindisk), diff(_enddisk - _begindisk)
    { }

    int operator () (int_type i) const
    {
        return begindisk + (i / nruns) % diff;
    }

    virtual ~interleaved_striping()
    { }
};

struct interleaved_FR : public interleaved_striping
{
    interleaved_FR(int_type _nruns, int _begindisk,
                   int _enddisk) : interleaved_striping(_nruns, _begindisk,
                               _enddisk)
    { }
    random_number<random_uniform_fast> rnd;
    int operator () (int_type /*i*/) const
    {
        return begindisk + rnd(diff);
    }
};

struct interleaved_SR : public interleaved_striping
{
    std::vector<int> offsets;

    interleaved_SR(int_type _nruns, int _begindisk,
                   int _enddisk) : interleaved_striping(_nruns,
                                                        _begindisk,
                                                        _enddisk)
    {
        random_number<random_uniform_fast> rnd;
        for (int_type i = 0; i < nruns; i++)
            offsets.push_back(rnd(diff));
    }

    int operator () (int_type i) const
    {
        return begindisk + (i / nruns + offsets[i % nruns]) % diff;
    }
};


struct interleaved_RC : public interleaved_striping
{
    std::vector<std::vector<int> > perms;

    interleaved_RC(int_type _nruns, int _begindisk,
                   int _enddisk) : interleaved_striping(_nruns,
                                                        _begindisk,
                                                        _enddisk),
                                   perms(nruns, std::vector<int>(diff))
    {
        for (int_type i = 0; i < nruns; i++)
        {
            for (int j = 0; j < diff; j++)
                perms[i][j] = j;


            random_number<random_uniform_fast> rnd;
            std::random_shuffle(perms[i].begin(),
                                perms[i].end(), rnd);
        }
    }

    int operator () (int_type i) const
    {
        return begindisk + perms[i % nruns][(i / nruns) % diff];
    }
};

struct first_disk_only : public interleaved_striping
{
    first_disk_only(int_type _nruns, int _begindisk, int)
        : interleaved_striping(_nruns, _begindisk, _begindisk + 1)
    { }

    int operator () (int_type) const
    {
        return begindisk;
    }
};

template <typename scheme>
struct interleaved_alloc_traits
{ };

template <>
struct interleaved_alloc_traits<striping>
{
    typedef interleaved_striping strategy;
};

template <>
struct interleaved_alloc_traits<FR>
{
    typedef interleaved_FR strategy;
};

template <>
struct interleaved_alloc_traits<SR>
{
    typedef interleaved_SR strategy;
};

template <>
struct interleaved_alloc_traits<RC>
{
    typedef interleaved_RC strategy;
};

template <>
struct interleaved_alloc_traits<RC_disk>
{
    // FIXME! HACK!
    typedef interleaved_RC strategy;
};

template <>
struct interleaved_alloc_traits<RC_flash>
{
    // FIXME! HACK!
    typedef interleaved_RC strategy;
};

template <>
struct interleaved_alloc_traits<single_disk>
{
    typedef first_disk_only strategy;
};

__STXXL_END_NAMESPACE

#endif // !STXXL_INTERLEAVED_ALLOC_HEADER
