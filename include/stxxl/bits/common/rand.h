/***************************************************************************
 *  include/stxxl/bits/common/rand.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002, 2003, 2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_RAND_HEADER
#define STXXL_COMMON_RAND_HEADER

#include <cmath>
#include <random>

#include <foxxll/common/types.hpp>

#include <stxxl/bits/common/seed.h>
#include <stxxl/bits/config.h>

// Recommended seeding procedure:
// by default, the global seed is initialized from a high resolution timer and the process id
// 1. stxxl::set_seed(seed); // optionally, do this if you wan't to us a specific seed to replay a certain program run
// 2. seed = stxxl::get_next_seed(); // store/print/... this value can be used for step 1 to replay the program with a specific seed
// 3. stxxl::srandom_number32(); // seed the global state of stxxl::random_number32
// 4. create all the other prngs used.

namespace stxxl {

extern unsigned ran32State;

//! \addtogroup support
//! \{

//! Fast uniform [0, 2^32) pseudo-random generator with period 2^32, random
//! bits: 32.
//! \warning Uses a global state and is not reentrant or thread-safe!
struct random_number32
{
    using value_type = unsigned;

    //! Returns a random number from [0, 2^32)
    inline value_type operator () () const
    {
        return (ran32State = 1664525 * ran32State + 1013904223);
    }

    //! Returns a random number from [0, N)
    inline value_type operator () (const value_type& N) const
    {
        return operator () () % N;
    }
};

//! Set a seed value for \c random_number32.
inline void srandom_number32(unsigned seed = get_next_seed())
{
    ran32State = seed;
}

//! Fast uniform [0, 2^32) pseudo-random generator with period 2^32, random
//! bits: 32.
//! Reentrant variant of random_number32 that keeps it's private state.
struct random_number32_r
{
    using value_type = unsigned;
    mutable unsigned state;

    explicit random_number32_r(unsigned seed = get_next_seed())
    {
        state = seed;
    }

    //! Change the current seed
    void set_seed(unsigned seed)
    {
        state = seed;
    }

    //! Returns a random number from [0, 2^32)
    inline value_type operator () () const
    {
        return (state = 1664525 * state + 1013904223);
    }
};

//! Fast uniform [0, 255] pseudo-random generator with period 2^8, random bits:
//! 8 (one byte).
class random_number8_r
{
    random_number32_r m_rnd32;
    uint32_t m_value;
    unsigned int m_pos;

public:
    using value_type = uint8_t;

    explicit random_number8_r(unsigned seed = get_next_seed())
        : m_rnd32(seed), m_pos(4)
    { }

    //! Returns a random byte from [0, 255]
    inline value_type operator () ()
    {
        if (++m_pos >= 4) {
            m_value = m_rnd32();
            m_pos = 0;
        }
        return ((uint8_t*)&m_value)[m_pos];
    }
};

//! Fast uniform [0.0, 1.0) pseudo-random generator
//! \warning Uses a global state and is not reentrant or thread-safe!
struct random_uniform_fast
{
    using value_type = double;
    random_number32 rnd32;

    explicit random_uniform_fast(unsigned /*seed*/ = get_next_seed())
    { }

    //! Returns a random number from [0.0, 1.0)
    inline value_type operator () () const
    {
        return (double(rnd32()) * (0.5 / 0x80000000));
    }
};

#if STXXL_MSVC
#pragma warning(push)
#pragma warning(disable:4512) // assignment operator could not be generated
#endif

//! Slow and precise uniform [0.0, 1.0) pseudo-random generator
//! period: at least 2^48, random bits: at least 31
//!
//! \warning Seed is not the same as in the fast generator \c random_uniform_fast
struct random_uniform_slow
{
    using value_type = double;

    using gen_type = std::default_random_engine;
    mutable gen_type gen;
    using uni_type = std::uniform_real_distribution<>;
    mutable uni_type uni;

    explicit random_uniform_slow(unsigned seed = get_next_seed())
        : gen(seed), uni(0.0, 1.0)
    { }

    //! Returns a random number from [0.0, 1.0)
    inline value_type operator () () const
    {
        return uni(gen);
    }
};

//! Uniform [0, N) pseudo-random generator
template <class UniformRGen = random_uniform_fast>
struct random_number
{
    using value_type = unsigned;
    UniformRGen uniform;

    explicit random_number(unsigned seed = get_next_seed())
        : uniform(seed)
    { }

    //! Returns a random number from [0, N)
    inline value_type operator () (value_type N) const
    {
        return static_cast<value_type>(uniform() * double(N));
    }
};

//! Slow and precise uniform [0, 2^64) pseudo-random generator
struct random_number64
{
    using value_type = uint64_t;
    random_uniform_slow uniform;

    explicit random_number64(unsigned seed = get_next_seed())
        : uniform(seed)
    { }

    //! Returns a random number from [0, 2^64)
    inline value_type operator () () const
    {
        return static_cast<value_type>(uniform() * (18446744073709551616.));
    }

    //! Returns a random number from [0, N)
    inline value_type operator () (value_type N) const
    {
        return static_cast<value_type>(uniform() * double(N));
    }
};

#if STXXL_MSVC
#pragma warning(pop) // assignment operator could not be generated
#endif

//! \}

} // namespace stxxl

#endif // !STXXL_COMMON_RAND_HEADER
