#ifndef STXXL_SEED_HEADER
#define STXXL_SEED_HEADER

/***************************************************************************
 *            seed.h
 *
 *  Fri Nov 16 13:33:05 CET 2007
 *  Copyright (C) 2007  Andreas Beckmann <beckmann@mpi-inf.mpg.de>
 ****************************************************************************/

#include "stxxl/bits/namespace.h"


__STXXL_BEGIN_NAMESPACE

//! \brief set the global stxxl seed value
void set_seed(unsigned seed);

//! \brief get a seed value for prng initialization,
//!        subsequent calls return a sequence of different values
unsigned get_next_seed();

__STXXL_END_NAMESPACE

#endif
