#include <stxxl/bits/version.h>

#ifdef STXXL_BOOST_CONFIG
#include<boost/version.hpp>
#endif

#define STXXL_VERSION_STRING_MA_MI_PL "1.1.1"

// version.defs gets created if a snapshot/beta/rc/release is done
#ifdef HAVE_VERSION_DEFS
#include "version.defs"
#endif

// version_svn.defs gets created if stxxl is built from SVN
#ifdef HAVE_VERSION_SVN_DEFS
#include "version_svn.defs"
#endif

// FIXME: this currently only works for GNU-like systems,
//        there are no details available on windows platform


const char * stxxl::get_version_string()
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

// vim: et:ts=4:sw=4
