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
		inline unsigned operator () () const
		{
			return (ran32State = 1664525 * ran32State + 1013904223);
		};
	};
	
	//! \brief Fast uniform [0,1] pseudo-random generator
	struct random_uniform_fast
	{
		random_number32 rnd32;
		inline double operator() () const
		{
			return ((double)rnd32() * (0.5 / 0x80000000));
		}
	};
	
	//! \brief Slow and precise uniform [0,1] pseudo-random generator
	
	//! \warning Seed is not the same as in the fast generator \c random_uniform_fast
	struct random_uniform_slow
	{
		inline double operator() () const
		{
			return drand48();
		}
	};
	
	template <class UniformRGen_ = random_uniform_fast>
	struct random_number
	{
		UniformRGen_ uniform;
		inline unsigned operator () (int N) const
		{
			return ((unsigned)(uniform() * (N)));
		};
	};
	
	struct random_number64
	{
		random_uniform_slow uniform;
		inline unsigned long long operator() () const
		{
			return static_cast<unsigned long long>(uniform()*(18446744073709551616.));
		}
	};
	
}

#endif
