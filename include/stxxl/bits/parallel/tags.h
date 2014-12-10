/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler and Felix Putze                *
 *   singler@ira.uka.de, kontakt@felix-putze.de                            *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/**
 * @file mcstl_tags.h
 * @brief Tags for compile-time options.
 */

#ifndef _MCSTL_TAGS_H
#define _MCSTL_TAGS_H 1

namespace mcstl
{

/** @brief Makes the parametrized class actually do work, i. e. actives it. */
class active_tag { };
/** @brief Makes the parametrized class do nothing, i. e. deactives it. */
class inactive_tag { };


/** @brief Forces sequential execution at compile time. */
struct sequential_tag { };
/** @brief Recommends parallel execution at compile time. */
struct parallel_tag { };
/** @brief Recommends parallel execution using dynamic load-balancing, at compile time. */
struct balanced_tag { };
/** @brief Recommends parallel execution using static load-balancing, at compile time. */
struct unbalanced_tag { };
/** @brief Recommends parallel execution using OpenMP dynamic load-balancing, at compile time. */
struct omp_loop_tag { };
/** @brief Recommends parallel execution using OpenMP static load-balancing, at compile time. */
struct omp_loop_static_tag { };

/** @brief Selects the growing block size variant for std::find().
 *  @see MCSTL_FIND_GROWING_BLOCKS */
struct growing_blocks_tag {};
/** @brief Selects the constant block size variant for std::find().
 *  @see MCSTL_FIND_CONSTANT_SIZE_BLOCKS */
struct constant_size_blocks_tag {};
/** @brief Selects the equal splitting variant for std::find().
 *  @see MCSTL_FIND_EQUAL_SPLIT */
struct equal_split_tag {};
	
}

#endif /* _MCSTL_TAGS_H */
