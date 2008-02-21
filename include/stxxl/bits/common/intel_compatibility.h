#ifndef STXXL_INTEL_COMPATIBILITY_HEADER_INCLUDED
#define STXXL_INTEL_COMPATIBILITY_HEADER_INCLUDED
/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the STXXL                                                     *
 ***************************************************************************/

/** @file intel_compatibility.h
 *  @brief Intel compiler compatibility work-around. */

#if defined(__ICC) && ! ( \
        (defined(__x86_64) && (__ICC > 1010 || (__ICC == 1010 && __INTEL_COMPILER_BUILD_DATE >= 20080112))) \
	)
/** @brief Replacement of unknown atomic function. Bug fixed for x86_64 in icpc 10.1.012. */
#ifndef __sync_fetch_and_add
#define __sync_fetch_and_add(ptr,addend) _InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend);
#endif
#endif

#endif // !STXXL_INTEL_COMPATIBILITY_HEADER_INCLUDED
