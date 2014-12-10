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
/** @def MCSTL_MERGESORT 
 *  @brief Include parallel multi-way mergesort.
 *  @see mcstl::Settings::sort_algorithm */
#define MCSTL_MERGESORT 1
#endif

#ifndef MCSTL_QUICKSORT
/** @def MCSTL_QUICKSORT
 *  @brief Include parallel unbalanced quicksort.
 *  @see mcstl::Settings::sort_algorithm */
#define MCSTL_QUICKSORT 1
#endif

#ifndef MCSTL_BAL_QUICKSORT
/** @def MCSTL_BAL_QUICKSORT
 *  @brief Include parallel dynamically load-balanced quicksort.
 *  @see mcstl::Settings::sort_algorithm */
#define MCSTL_BAL_QUICKSORT 1
#endif

#ifndef MCSTL_LOSER_TREE
/** @def MCSTL_LOSER_TREE
 *  @brief Include guarded (sequences may run empty) loser tree, moving objects.
 *  @see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE 1
#endif

#ifndef MCSTL_LOSER_TREE_EXPLICIT
/** @def MCSTL_LOSER_TREE_EXPLICIT
 *  @brief Include standard loser tree, storing two flags for infimum and supremum.
 *  @see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_EXPLICIT 0
#endif

#ifndef MCSTL_LOSER_TREE_REFERENCE
/** @def MCSTL_LOSER_TREE_REFERENCE
 *  @brief Include some loser tree variant.
 *  @see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_REFERENCE 0
#endif

#ifndef MCSTL_LOSER_TREE_POINTER
/** @def MCSTL_LOSER_TREE_POINTER
 *  @brief Include some loser tree variant.
 *  @see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_POINTER 0
#endif

#ifndef MCSTL_LOSER_TREE_UNGUARDED
/** @def MCSTL_LOSER_TREE_UNGUARDED
 *  @brief Include unguarded (sequences must not run empty) loser tree, moving objects.
 *  @see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_UNGUARDED 1
#endif

#ifndef MCSTL_LOSER_TREE_POINTER_UNGUARDED
/** @def MCSTL_LOSER_TREE_POINTER_UNGUARDED
 *  @brief Include some loser tree variant.
 *  @see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_POINTER_UNGUARDED 0
#endif

#ifndef MCSTL_LOSER_TREE_COMBINED
/** @def MCSTL_LOSER_TREE_COMBINED
 *  @brief Include some loser tree variant.
 *  @see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_COMBINED 0
#endif

#ifndef MCSTL_LOSER_TREE_SENTINEL
/** @def MCSTL_LOSER_TREE_SENTINEL
 *  @brief Include some loser tree variant.
 *  @see mcstl::Settings multiway_merge_algorithm */
#define MCSTL_LOSER_TREE_SENTINEL 0
#endif


#ifndef MCSTL_FIND_GROWING_BLOCKS
/** @brief Include the growing blocks variant for std::find. 
 *  @see mcstl::Settings::find_distribution */
#define MCSTL_FIND_GROWING_BLOCKS 1
#endif

#ifndef MCSTL_FIND_CONSTANT_SIZE_BLOCKS
/** @brief Include the equal-sized blocks variant for std::find. 
 *  @see mcstl::Settings::find_distribution */
#define MCSTL_FIND_CONSTANT_SIZE_BLOCKS 1
#endif

#ifndef MCSTL_FIND_EQUAL_SPLIT
/** @def MCSTL_FIND_EQUAL_SPLIT
 *  @brief Include the equal splitting variant for std::find.
 *  @see mcstl::Settings::find_distribution */
#define MCSTL_FIND_EQUAL_SPLIT 1
#endif


#ifndef MCSTL_TREE_INITIAL_SPLITTING
/** @def MCSTL_TREE_INITIAL_SPLITTING
 *  @brief Include the initial splitting variant for _Rb_tree::insert_unique(InputIterator beg, InputIterator end).
 *  @see mcstl::_Rb_tree */
#define MCSTL_TREE_INITIAL_SPLITTING 1
#endif

#ifndef MCSTL_TREE_DYNAMIC_BALANCING
/** @def MCSTL_TREE_DYNAMIC_BALANCING
 *  @brief Include the dynamic balancing variant for _Rb_tree::insert_unique(InputIterator beg, InputIterator end).
 *  @see mcstl::_Rb_tree */
#define MCSTL_TREE_DYNAMIC_BALANCING 1
#endif

#ifndef MCSTL_TREE_FULL_COPY
/** @def MCSTL_TREE_FULL_COPY
 *  @brief In order to sort the input sequence of _Rb_tree::insert_unique(InputIterator beg, InputIterator end) a full copy of the input elements is done.
 *  @see mcstl::_Rb_tree */
#define MCSTL_TREE_FULL_COPY 1
#endif

} // namespace parallel

STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_FEATURES_HEADER
