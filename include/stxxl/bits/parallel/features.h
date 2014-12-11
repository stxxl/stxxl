/***************************************************************************
 *  include/stxxl/bits/parallel/features.h
 *
 *  Defines on whether to include algorithm variants.
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

#ifndef STXXL_PARALLEL_FEATURES_HEADER
#define STXXL_PARALLEL_FEATURES_HEADER

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/parallel/compiletime_settings.h>

STXXL_BEGIN_NAMESPACE

namespace parallel {

#ifndef MCSTL_MERGESORT
/** \def MCSTL_MERGESORT
 *  Include parallel multi-way mergesort.
 *  \see mcstl::Settings::sort_algorithm */
#define MCSTL_MERGESORT 1
#endif

#ifndef MCSTL_LOSER_TREE
/** \def MCSTL_LOSER_TREE
 *  Include guarded (sequences may run empty) loser tree, moving objects.
 *  \see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE 1
#endif

#ifndef MCSTL_LOSER_TREE_EXPLICIT
/** \def MCSTL_LOSER_TREE_EXPLICIT
 *  Include standard loser tree, storing two flags for infimum and supremum.
 *  \see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_EXPLICIT 0
#endif

#ifndef MCSTL_LOSER_TREE_REFERENCE
/** \def MCSTL_LOSER_TREE_REFERENCE
 *  Include some loser tree variant.
 *  \see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_REFERENCE 0
#endif

#ifndef MCSTL_LOSER_TREE_POINTER
/** \def MCSTL_LOSER_TREE_POINTER
 *  Include some loser tree variant.
 *  \see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_POINTER 0
#endif

#ifndef MCSTL_LOSER_TREE_UNGUARDED
/** \def MCSTL_LOSER_TREE_UNGUARDED
 *  Include unguarded (sequences must not run empty) loser tree, moving objects.
 *  \see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_UNGUARDED 1
#endif

#ifndef MCSTL_LOSER_TREE_POINTER_UNGUARDED
/** \def MCSTL_LOSER_TREE_POINTER_UNGUARDED
 *  Include some loser tree variant.
 *  \see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_POINTER_UNGUARDED 0
#endif

#ifndef MCSTL_LOSER_TREE_COMBINED
/** \def MCSTL_LOSER_TREE_COMBINED
 *  Include some loser tree variant.
 *  \see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_COMBINED 0
#endif

#ifndef MCSTL_LOSER_TREE_SENTINEL
/** \def MCSTL_LOSER_TREE_SENTINEL
 *  Include some loser tree variant.
 *  \see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_SENTINEL 0
#endif

} // namespace parallel

STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_FEATURES_HEADER
