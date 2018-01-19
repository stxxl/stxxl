/***************************************************************************
 *  include/stxxl.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002-2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MAIN_HEADER
#define STXXL_MAIN_HEADER

#include <foxxll/common/utils.hpp>
#include <foxxll/io.hpp>
#include <foxxll/mng.hpp>

#include <stxxl/priority_queue>
#include <stxxl/stack>
#include <stxxl/vector>
#if ! defined(__GNUG__) || ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 30400)
// map does not work with g++ 3.3
#include <stxxl/map>
#endif
#include <stxxl/deque>
#include <stxxl/queue>
#include <stxxl/unordered_map>

#include <stxxl/algorithm>

#include <stxxl/stream>

#include <stxxl/timer>

#endif // STXXL_MAIN_HEADER
