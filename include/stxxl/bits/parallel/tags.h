/***************************************************************************
 *  include/stxxl/bits/parallel/tags.h
 *
 *  Tags for compile-time options.
 *  Extracted from MCSTL http://algo2.iti.uni-karlsruhe.de/singler/mcstl/
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

#ifndef STXXL_PARALLEL_TAGS_HEADER
#define STXXL_PARALLEL_TAGS_HEADER

#include <stxxl/bits/namespace.h>

STXXL_BEGIN_NAMESPACE

namespace parallel {

/** Makes the parametrized class actually do work, i. e. actives it. */
class active_tag
{ };
/** Makes the parametrized class do nothing, i. e. deactives it. */
class inactive_tag
{ };

/** Recommends parallel execution using dynamic load-balancing, at compile time. */
struct balanced_tag { };
/** Recommends parallel execution using static load-balancing, at compile time. */
struct unbalanced_tag { };
/** Recommends parallel execution using OpenMP dynamic load-balancing, at compile time. */
struct omp_loop_tag { };
/** Recommends parallel execution using OpenMP static load-balancing, at compile time. */
struct omp_loop_static_tag { };

/** Selects the growing block size variant for std::find().
 *  \see MCSTL_FIND_GROWING_BLOCKS */
struct growing_blocks_tag { };
/** Selects the constant block size variant for std::find().
 *  \see MCSTL_FIND_CONSTANT_SIZE_BLOCKS */
struct constant_size_blocks_tag { };
/** Selects the equal splitting variant for std::find().
 *  \see MCSTL_FIND_EQUAL_SPLIT */
struct equal_split_tag { };

} // namespace parallel

STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_TAGS_HEADER
