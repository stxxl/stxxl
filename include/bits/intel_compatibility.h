#ifndef STXXL_MCSTL_INTEL_COMPATIBILITY_HEADER_INCLUDED
#define STXXL_MCSTL_INTEL_COMPATIBILITY_HEADER_INCLUDED
/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler  <singler@ira.uka.de>          *
 *   Copyright (C) 2008 by Andreas Beckmann  <beckmann@mpi-inf.mpg.de>     *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the STXXL and MCSTL.                                          *
 ***************************************************************************/

/** @file intel_compatibility.h
 *  @brief Intel compiler compatibility work-around. */

#if defined(__ICC) && (__ICC < 1100)

/** @brief Fake function that will cause linking failure if used. */
template <typename T1, typename T2>
T1 __in_icc_there_is_no__sync_fetch_and_add(T1 *, T2);

#if 0
// replacing the overloaded builtin function 'T __sync_fetch_and_add(T*, T)'
// with 'int32 _InterlockedExchangeAdd(int32, int32)' is not a good idea
#define __sync_fetch_and_add(ptr,addend) _InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)
#endif

/** @brief Replacement of unknown atomic builtin function.
 *         Should work as long as it isn't instantiated. */
#undef __sync_fetch_and_add
#define __sync_fetch_and_add __in_icc_there_is_no__sync_fetch_and_add

#endif

#endif // !STXXL_MCSTL_INTEL_COMPATIBILITY_HEADER_INCLUDED
