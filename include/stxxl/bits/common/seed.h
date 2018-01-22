/***************************************************************************
 *  include/stxxl/bits/common/seed.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *  Copyright (C) 2018 Manuel Penschuck <stxxl@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_SEED_HEADER
#define STXXL_COMMON_SEED_HEADER

#include <mutex>
#include <random>

#include <foxxll/common/timer.hpp>
#include <foxxll/singleton.hpp>

namespace stxxl {

class seed_sequence : public foxxll::singleton<seed_sequence>
{
public:
    using value_type = unsigned;

    //! By default, the seed generator behaves deterministically
    explicit seed_sequence(value_type seed = 0xbadc0ffe) : prng_(seed) { }
    seed_sequence(const seed_sequence&) = delete;

    //! Derives a random seed sequence based on current time
    void random_seed()
    {
        auto seed = static_cast<value_type>(foxxll::timestamp() * 0xc01ddead);
        set_seed(seed);
    }

    //! Reset seed sequence
    void set_seed(value_type seed)
    {
        std::unique_lock<std::mutex> mtx_;
        prng_.seed(seed);
    }

    //! Get a new seed value (thread-safe)
    value_type get_next_seed()
    {
        std::unique_lock<std::mutex> mtx_;
        std::uniform_int_distribution<value_type> distr;
        return distr(prng_);
    }

private:
    std::mutex mtx_;

    //! Random number generator for seed sequence.
    std::mt19937 prng_;
};

} // namespace stxxl

#endif // !STXXL_COMMON_SEED_HEADER
