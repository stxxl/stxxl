/***************************************************************************
 *  include/stxxl/bits/expensive_assert.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2018 Manuel Penschuck <stxxl@manuel.jetzt>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_EXPENSIVE_ASSERT_HEADER
#define STXXL_EXPENSIVE_ASSERT_HEADER

#include <tlx/die.hpp>
#include <tlx/unused.hpp>

#if STXXL_EXPENSIVE_ASSERTIONS
    #define STXXL_EXPENSIVE_ASSERT(condition) die_unless(condition)
#else
    #define STXXL_EXPENSIVE_ASSERT(condition) { }
#endif

#endif // !STXXL_EXPENSIVE_ASSERT_HEADER
// vim: et:ts=4:sw=4
