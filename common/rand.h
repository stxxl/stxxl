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

namespace stxxl
{
	unsigned ran32State = time (NULL);

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
		inline value_type operator() () const
		{
			return drand48();
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
    typedef unsigned long long value_type;
		random_uniform_slow uniform;
		inline value_type operator() () const
		{
			return static_cast<value_type>(uniform()*(18446744073709551616.));
		}
	};
	
}

#endif
