#ifndef STXXL_RAND_HEADER
#define STXXL_RAND_HEADER


/***************************************************************************
 *            rand.h
 *
 *  Sat Aug 24 23:53:36 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include <time.h>
#include <stdlib.h>
#include "utils.h"

#ifdef STXXL_BOOST_RANDOM
#include "boost/random.hpp"
#endif

__STXXL_BEGIN_NAMESPACE

	extern unsigned ran32State;

	struct random_number32
	{
    typedef unsigned value_type;
		inline value_type operator () () const
		{
			return (ran32State = 1664525 * ran32State + 1013904223);
		};
	};
	
	//! \brief Fast uniform [0,1] pseudo-random generator
	struct random_uniform_fast
	{
    typedef double value_type;
		random_number32 rnd32;
		inline value_type operator() () const
		{
			return ((double)rnd32() * (0.5 / 0x80000000));
		}
	};
	
	//! \brief Slow and precise uniform [0,1] pseudo-random generator
	//!
	//! \warning Seed is not the same as in the fast generator \c random_uniform_fast
	struct random_uniform_slow
	{
		typedef double value_type;
		#ifdef STXXL_BOOST_RANDOM
		typedef boost::minstd_rand base_generator_type;
		base_generator_type generator;
		boost::uniform_real<> uni_dist;
		boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni;

		random_uniform_slow() : uni(generator, uni_dist) {}
		#endif

		inline value_type operator() () const
		{
			#ifdef STXXL_BOOST_RANDOM
			return uni();
			#else
			return drand48();
			#endif
		}
	};
	
	template <class UniformRGen_ = random_uniform_fast>
	struct random_number
	{
    typedef unsigned value_type;
		UniformRGen_ uniform;
		inline value_type operator () (int N) const
		{
			return ((value_type)(uniform() * (N)));
		};
	};
	
	struct random_number64
	{
    typedef stxxl::uint64 value_type;
		random_uniform_slow uniform;
		inline value_type operator() ()
		{
			return static_cast<value_type>(uniform()*(18446744073709551616.));
		}
	};
	
__STXXL_END_NAMESPACE

#endif
