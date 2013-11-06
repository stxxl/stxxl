/***************************************************************************
 *  include/stxxl/bits/version.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_VERSION_HEADER
#define STXXL_VERSION_HEADER

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/verbose.h>

__STXXL_BEGIN_NAMESPACE

// STXXL_VERSION_MAJOR and more are defined in cmake generated config.h

#define stringify_(x) #x
#define stringify(x) stringify_(x)

//! Return "X.Y.Z" version string (of headers)
inline const char * get_version_string()
{
    return STXXL_VERSION_STRING;
}

//! Return longer "X.Y.Z (feature) (version)" version string (of headers)
inline const char * get_version_string_long()
{
    return "STXXL"
           " v" STXXL_VERSION_STRING
#ifdef STXXL_VERSION_PHASE
           " (" STXXL_VERSION_PHASE ")"
#endif
#ifdef STXXL_VERSION_GIT_SHA1
           " (git " STXXL_VERSION_GIT_SHA1 ")"
#endif // STXXL_VERSION_GIT_SHA1
#if STXXL_PARALLEL
           " + gnu parallel(" stringify(__GLIBCXX__) ")"
#endif // STXXL_PARALLEL
#if STXXL_BOOST_CONFIG
           " + Boost " stringify(BOOST_VERSION)
#endif
    ;
}

#undef stringify
#undef stringify_

int version_major();
int version_minor();
int version_patch();

//! returns "X.Y.Z" version string of library
const char * get_library_version_string();

//! returns longer "X.Y.Z (feature) (version)" string of library
const char * get_library_version_string_long();

//! Check for a mismatch between library and headers
inline int check_library_version()
{
    if (version_major() != STXXL_VERSION_MAJOR)
        return 1;
    if (version_minor() != STXXL_VERSION_MINOR)
        return 2;
    if (version_patch() != STXXL_VERSION_PATCH)
        return 3;
    return 0;
}

//! Check and print mismatch between header and library versions
inline void print_library_version_mismatch()
{
    if (stxxl::check_library_version() != 0)
    {
        STXXL_ERRMSG("version mismatch between headers" <<
                     " (" << STXXL_VERSION_STRING ") and library" <<
                     " (" << get_library_version_string() << ")");
    }
}

__STXXL_END_NAMESPACE

#endif // !STXXL_VERSION_HEADER
