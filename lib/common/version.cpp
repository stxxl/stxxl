/***************************************************************************
 *  common/version.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007, 2008, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/version.h>

#ifdef STXXL_BOOST_CONFIG
#include <boost/version.hpp>
#endif


__STXXL_BEGIN_NAMESPACE

int version_major()
{
    return STXXL_VERSION_MAJOR;
}

int version_minor()
{
    return STXXL_VERSION_MINOR;
}

int version_patch()
{
    return STXXL_VERSION_PATCH;
}

#define stringify_(x) #x
#define stringify(x) stringify_(x)

const char * get_version_string()
{
    return "STXXL"
           " v" STXXL_VERSION_STRING
#ifdef STXXL_VERSION_PHASE
           " (" STXXL_VERSION_PHASE ")"
#endif
#ifdef STXXL_VERSION_GIT_SHA1
           " (git " STXXL_VERSION_GIT_SHA1 ")"
#endif // STXXL_VERSION_GIT_SHA1
#ifdef STXXL_PARALLEL
           " + gnu parallel(" stringify(__GLIBCXX__) ")"
#endif // STXXL_PARALLEL
#ifdef STXXL_BOOST_CONFIG
           " + Boost " stringify(BOOST_VERSION)
#endif
    ;
}

#undef stringify
#undef stringify_

__STXXL_END_NAMESPACE

// vim: et:ts=4:sw=4
