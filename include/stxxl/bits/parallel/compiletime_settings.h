/***************************************************************************
 *  include/stxxl/bits/parallel/compiletime_settings.h
 *
 *  Defines on options concerning debugging and performance, at compile-time.
 *  Extracted from MCSTL - http://algo2.iti.uni-karlsruhe.de/singler/mcstl/
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_COMPILETIME_SETTINGS_HEADER
#define STXXL_PARALLEL_COMPILETIME_SETTINGS_HEADER

#include <stxxl/bits/namespace.h>
#include <cstdio>

STXXL_BEGIN_NAMESPACE

namespace parallel {

/** Determine verbosity level of MCSTL.
 *  Level 1 prints a message each time when entering a MCSTL function. */
#define MCSTL_VERBOSE_LEVEL 0

/** \def MCSTL_CALL
 *  Macro to produce log message when entering a function.
 *  \param n Input size.
 *  \see MCSTL_VERBOSE_LEVEL */
#if (MCSTL_VERBOSE_LEVEL == 0)
#define MCSTL_CALL(n)
#endif
#if (MCSTL_VERBOSE_LEVEL == 1)
#define MCSTL_CALL(n) printf("   %s:\niam = %d, n = %ld, num_threads = %d\n", __PRETTY_FUNCTION__, omp_get_thread_num(), (n), SETTINGS::num_threads);   //avoid usage of iostream in the MCSTL
#endif

/** Use floating-point scaling instead of modulo for mapping random numbers to a range.
 *  This can be faster on certain CPUs. */
#define MCSTL_SCALE_DOWN_FPU 0

/** Switch on many assertions in MCSTL code.
 *  Should be switched on only locally. */
#define MCSTL_ASSERTIONS 0

/** Switch on many assertions in MCSTL code.
 *  Consider the size of the L1 cache for mcstl::parallel_random_shuffle(). */
#define MCSTL_RANDOM_SHUFFLE_CONSIDER_L1 0
/** Switch on many assertions in MCSTL code.
 *  Consider the size of the TLB for mcstl::parallel_random_shuffle(). */
#define MCSTL_RANDOM_SHUFFLE_CONSIDER_TLB 0

/** First copy the data, sort it locally, and merge it back (0); or copy it back after everyting is done (1).
 *
 *  Recommendation: 0 */
#define MCSTL_MULTIWAY_MERGESORT_COPY_LAST 0

} // namespace parallel

STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_COMPILETIME_SETTINGS_HEADER
