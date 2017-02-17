/***************************************************************************
 *  include/stxxl/bits/mng/block_alloc_strategy_interleaved.h
 *
 *  include/stxxl/bits/mng/block_alloc_interleaved.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002, 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2009, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MNG_BLOCK_ALLOC_STRATEGY_INTERLEAVED_HEADER
#define STXXL_MNG_BLOCK_ALLOC_STRATEGY_INTERLEAVED_HEADER

#include <stxxl/bits/mng/block_manager.h>

#include <random>
#include <vector>

namespace stxxl {

#define CHECK_RUN_BOUNDS(pos)

struct interleaved_striping
{
protected:
    int nruns_;
    size_t begin_disk_;
    size_t diff_;

    interleaved_striping(int nruns, size_t begin_disk, size_t diff)
        : nruns_(nruns), begin_disk_(begin_disk), diff_(diff)
    { }

public:
    interleaved_striping(int nruns, const striping& strategy)
        : nruns_(nruns), begin_disk_(strategy.begin_), diff_(strategy.diff_)
    { }

    size_t operator () (size_t i) const
    {
        return begin_disk_ + (i / nruns_) % diff_;
    }
};

struct interleaved_fully_random : public interleaved_striping
{
    mutable std::default_random_engine rng_ { std::random_device { } () };

    interleaved_fully_random(int nruns, const fully_random& strategy)
        : interleaved_striping(nruns, strategy.begin_, strategy.diff_)
    { }

    size_t operator () (size_t /* i */) const
    {
        return begin_disk_ + rng_() % diff_;
    }
};

struct interleaved_simple_random : public interleaved_striping
{
    std::vector<size_t> offsets_;

    interleaved_simple_random(int nruns, const simple_random& strategy)
        : interleaved_striping(nruns, strategy.begin_, strategy.diff_)
    {
        std::default_random_engine rng { std::random_device { } () };
        for (int i = 0; i < nruns; i++)
            offsets_.push_back(rng() % diff_);
    }

    size_t operator () (size_t i) const
    {
        return begin_disk_ + (i / nruns_ + offsets_[i % nruns_]) % diff_;
    }
};

struct interleaved_random_cyclic : public interleaved_striping
{
    std::vector<std::vector<size_t> > perms_;

    interleaved_random_cyclic(int nruns, const random_cyclic& strategy)
        : interleaved_striping(nruns, strategy.begin_, strategy.diff_),
          perms_(nruns, std::vector<size_t>(diff_))
    {
        for (int i = 0; i < nruns; i++)
        {
            for (size_t j = 0; j < diff_; j++)
                perms_[i][j] = j;

            std::random_shuffle(perms_[i].begin(), perms_[i].end());
        }
    }

    size_t operator () (size_t i) const
    {
        return begin_disk_ + perms_[i % nruns_][(i / nruns_) % diff_];
    }
};

struct first_disk_only : public interleaved_striping
{
    first_disk_only(int nruns, const single_disk& strategy)
        : interleaved_striping(nruns, strategy.disk_, 1)
    { }

    size_t operator () (size_t) const
    {
        return begin_disk_;
    }
};

template <typename scheme>
struct interleaved_alloc_traits
{ };

template <>
struct interleaved_alloc_traits<striping>
{
    using strategy = interleaved_striping;
};

template <>
struct interleaved_alloc_traits<fully_random>
{
    using strategy = interleaved_fully_random;
};

template <>
struct interleaved_alloc_traits<simple_random>
{
    using strategy = interleaved_simple_random;
};

template <>
struct interleaved_alloc_traits<random_cyclic>
{
    using strategy = interleaved_random_cyclic;
};

template <>
struct interleaved_alloc_traits<random_cyclic_disk>
{
    // FIXME! HACK!
    using strategy = interleaved_random_cyclic;
};

template <>
struct interleaved_alloc_traits<random_cyclic_flash>
{
    // FIXME! HACK!
    using strategy = interleaved_random_cyclic;
};

template <>
struct interleaved_alloc_traits<single_disk>
{
    using strategy = first_disk_only;
};

} // namespace stxxl

#endif // !STXXL_MNG_BLOCK_ALLOC_STRATEGY_INTERLEAVED_HEADER
// vim: et:ts=4:sw=4
