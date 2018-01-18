/***************************************************************************
 *  include/stxxl/bits/parallel/compiletime_settings.h
 *
 *  Defines on options concerning debugging and performance, at compile-time.
 *  Extracted from MCSTL - http://algo2.iti.uni-karlsruhe.de/singler/mcstl/
 *
 *  Part of the STXXL. See http://stxxl.org
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

#include <tlx/logger.hpp>

#include <stxxl/bits/config.h>
#include <stxxl/bits/parallel/settings.h>

namespace stxxl {
namespace parallel {

/** STXXL_PARALLEL_PCALL Macro to produce log message when entering a
 *  function.
 *  \param n Input size.
 */

constexpr bool debug_pcalls = false;

#if STXXL_PARALLEL
#define STXXL_PARALLEL_PCALL(n)                          \
    LOGC(debug_pcalls) << "   " << __FUNCTION__ << ":\n" \
        "iam = " << omp_get_thread_num() << ", "         \
        "n = " << (n) << ", "                            \
        "num_threads = " << int(SETTINGS::num_threads);
#else
#define STXXL_PARALLEL_PCALL(n)                          \
    LOGC(debug_pcalls) << "   " << __FUNCTION__ << ":\n" \
        "iam = single-threaded, "                        \
        "n = " << (n) << ", "                            \
        "num_threads = " << int(SETTINGS::num_threads);
#endif

/** First copy the data, sort it locally, and merge it back (0); or copy it
 * back after everyting is done (1).
 *
 *  Recommendation: 0 */
#define STXXL_MULTIWAY_MERGESORT_COPY_LAST 0

} // namespace parallel
} // namespace stxxl

#endif // !STXXL_PARALLEL_COMPILETIME_SETTINGS_HEADER
