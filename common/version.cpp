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

#define stringify_(x) #x
#define stringify(x) stringify_(x)
#define STXXL_VERSION_STRING_MA_MI_PL stringify(STXXL_VERSION_MAJOR) "." stringify(STXXL_VERSION_MINOR) "." stringify(STXXL_VERSION_PATCHLEVEL)

// version.defs gets created if a snapshot/beta/rc/release is done
#ifdef HAVE_VERSION_DEFS
#include "version.defs"
#endif

// version_svn.defs gets created if stxxl is built from SVN
#ifdef HAVE_VERSION_SVN_DEFS
#include "version_svn.defs"
#endif


__STXXL_BEGIN_NAMESPACE

int version_major()
{
    return STXXL_VERSION_MAJOR;
};

int version_minor()
{
    return STXXL_VERSION_MINOR;
}

int version_patchlevel()
{
    return STXXL_VERSION_PATCHLEVEL;
}

// FIXME: this currently only works for GNU-like systems,
//        there are no details available on windows platform

const char * get_version_string()
{
    return "STXXL"
#ifdef STXXL_VERSION_STRING_SVN_BRANCH
           " (branch: " STXXL_VERSION_STRING_SVN_BRANCH ")"
#endif
           " v"
           STXXL_VERSION_STRING_MA_MI_PL
#ifdef STXXL_VERSION_STRING_DATE
           "-" STXXL_VERSION_STRING_DATE
#endif
#ifdef STXXL_VERSION_STRING_SVN_REVISION
           " (SVN r" STXXL_VERSION_STRING_SVN_REVISION ")"
#endif
#ifdef STXXL_VERSION_STRING_PHASE
           " (" STXXL_VERSION_STRING_PHASE ")"
#else
           " (prerelease)"
#endif
#ifdef STXXL_VERSION_STRING_COMMENT
           " (" STXXL_VERSION_STRING_COMMENT ")"
#endif
#ifdef __MCSTL__
           " + MCSTL"
#ifdef MCSTL_VERSION_STRING_DATE
           " " MCSTL_VERSION_STRING_DATE
#endif
#ifdef MCSTL_VERSION_STRING_SVN_REVISION
           " (SVN r" MCSTL_VERSION_STRING_SVN_REVISION ")"
#endif
#endif
#ifdef STXXL_BOOST_CONFIG
           " + Boost "
#define Y(x) # x
#define X(x) Y(x)
           X(BOOST_VERSION)
#undef X
#undef Y
#endif
    ;
}

__STXXL_END_NAMESPACE

// vim: et:ts=4:sw=4
