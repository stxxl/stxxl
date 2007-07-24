#include <stxxl/bits/version.h>

#define STXXL_VERSION_STRING_MA_MI_PL "1.1.0"

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
    return "STXXL v"
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
    ;
}

// vim: et:ts=4:sw=4
