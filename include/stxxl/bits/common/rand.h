#ifndef STXXL_RAND_HEADER
#define STXXL_RAND_HEADER

/***************************************************************************
 *            rand.h
 *
 *  Sat Aug 24 23:53:36 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include <cstdlib>

#include "stxxl/bits/common/types.h"
#include "stxxl/bits/common/seed.h"

#ifdef STXXL_BOOST_RANDOM
 #include <boost/random.hpp>
#endif

// Recommended seeding procedure:
// by default, the global seed is initialized from a high resolution timer and the process id
// 1. stxxl::set_seed(seed); // optionally, do this if you wan't to us a specific seed to replay a certain program run
// 2. seed = stxxl::get_next_seed(); // store/print/... this value can be used for step 1 to replay the program with a specific seed
// 3. stxxl::srandom_number32(); // seed the global state of stxxl::random_number32
// 4. create all the other prngs used.

__STXXL_BEGIN_NAMESPACE

extern unsigned ran32State;

//! \brief Fast uniform [0, 2^32) pseudo-random generator
//!        with period 2^32, random bits: 32.
//! \warning Uses a global state and is not reentrant or thread-safe!
struct random_number32
{
    typedef unsigned value_type;

    //! \brief Returns a random number from [0, 2^32)
    inline value_type operator () () const
    {
        return (ran32State = 1664525 * ran32State + 1013904223);
    }
};

//! \brief Set a seed value for \c random_number32.
inline void srandom_number32(unsigned seed = 0)
{
    if (!seed)
        seed = get_next_seed();
    ran32State = seed;
}

//! \brief Fast uniform [0, 2^32) pseudo-random generator
//!        with period 2^32, random bits: 32.
//! Reentrant variant of random_number32 that keeps it's private state.
struct random_number32_r
{
    typedef unsigned value_type;
    mutable unsigned state;

    random_number32_r(unsigned seed = 0)
    {
        if (!seed)
            seed = get_next_seed();
        state = seed;
    }

    //! \brief Returns a random number from [0, 2^32)
    inline value_type operator () () const
    {
        return (state = 1664525 * state + 1013904223);
    }
};

//! \brief Fast uniform [0.0, 1.0) pseudo-random generator
//! \warning Uses a global state and is not reentrant or thread-safe!
struct random_uniform_fast
{
    typedef double value_type;
    random_number32 rnd32;

    random_uniform_fast(unsigned /*seed*/ = 0)
    { }

    //! \brief Returns a random number from [0.0, 1.0)
    inline value_type operator () () const
    {
        return (double(rnd32()) * (0.5 / 0x80000000));
    }
};

//! \brief Slow and precise uniform [0.0, 1.0) pseudo-random generator
//!        period: at least 2^48, random bits: at least 31
//!
//! \warning Seed is not the same as in the fast generator \c random_uniform_fast
struct random_uniform_slow
{
    typedef double value_type;
#ifdef STXXL_BOOST_RANDOM
    typedef boost::minstd_rand base_generator_type;
    base_generator_type generator;
    boost::uniform_real<> uni_dist;
    mutable boost::variate_generator < base_generator_type &, boost::uniform_real<> > uni;

    random_uniform_slow(unsigned seed = 0) : uni(generator, uni_dist)
    {
        if (!seed)
            seed = get_next_seed();
        uni.engine().seed(seed);
    }
#else
    mutable unsigned short state48[3];

    random_uniform_slow(unsigned seed = 0)
    {
        if (!seed)
            seed = get_next_seed();
        state48[0] = seed & 0xffff;
        state48[1] = seed >> 16;
        state48[2] = 42;
        erand48(state48);
    }
#endif

    //! \brief Returns a random number from [0.0, 1.0)
    inline value_type operator () () const
    {
#ifdef STXXL_BOOST_RANDOM
        return uni();
#else
        return erand48(state48);
#endif
    }
};

//! \brief Uniform [0, N) pseudo-random generator
template <class UniformRGen_ = random_uniform_fast>
struct random_number
{
    typedef unsigned value_type;
    UniformRGen_ uniform;

    random_number(unsigned seed = 0) : uniform(seed)
    { }

    //! \brief Returns a random number from [0, N)
    inline value_type operator () (value_type N) const
    {
        return static_cast<value_type>(uniform() * double(N));
    }
};

//! \brief Slow and precise uniform [0, 2^64) pseudo-random generator
struct random_number64
{
    typedef stxxl::uint64 value_type;
    random_uniform_slow uniform;

    random_number64(unsigned seed = 0) : uniform(seed)
    { }

    //! \brief Returns a random number from [0, 2^64)
    inline value_type operator () () const
    {
        return static_cast<value_type>(uniform() * (18446744073709551616.));
    }
};

__STXXL_END_NAMESPACE

#endif
