/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler and Felix Putze                *
 *   singler@ira.uka.de, kontakt@felix-putze.de                            *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/**
 * @file mcstl_types.h
 * @brief Basic typedefs.
 */

#ifndef _MCSTL_TYPES_H
#define _MCSTL_TYPES_H 1

#include <cstdlib>

namespace mcstl
{
/** @brief 8-bit signed integer. */
typedef char int8;
/** @brief 8-bit unsigned integer. */
typedef unsigned char uint8;
/** @brief 16-bit signed integer. */
typedef short int16;
/** @brief 16-bit unsigned integer. */
typedef unsigned short uint16;
/** @brief 32-bit signed integer. */
typedef int int32;
/** @brief 32-bit unsigned integer. */
typedef unsigned int uint32;
/** @brief 64-bit signed integer. */
typedef long long int64;
/** @brief 64-bit unsigned integer. */
typedef unsigned long long uint64;

/**
 * @brief Unsigned integer to index elements.
 * The total number of elements for each algorithm must fit into this type.
 */
typedef uint64 sequence_index_t;	//must be able to index that maximum data load
/**
 * @brief Unsigned integer to index a thread number.
 * The maximum thread number must fit into this type.
 */
typedef uint16 thread_index_t;	//must be able to index each processor
/**
 * @brief Longest compare-and-swappable integer type on this platform.
 */
typedef int64 lcas_t;	//longest compare-and-swappable type
/**
 * @brief Number of bits of ::lcas_t.
 */
static const int lcas_t_bits = sizeof(lcas_t) * 8;
/**
 * @brief ::lcas_t with the right half of bits set to 1.
 */
static const lcas_t lcas_t_mask = (((lcas_t)1 << (lcas_t_bits / 2)) - 1);
}

#endif /* _MCSTL_TYPES_H */
