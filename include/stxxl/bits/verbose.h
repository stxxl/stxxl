/***************************************************************************
 *  include/stxxl/bits/verbose.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2005-2006 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_VERBOSE_HEADER
#define STXXL_VERBOSE_HEADER

#include <iostream>
#include <sstream>
#include <string>
#include <stxxl/bits/unused.h>


#define _STXXL_PRNT_COUT        (1 << 0)
#define _STXXL_PRNT_CERR        (1 << 1)
#define _STXXL_PRNT_LOG         (1 << 2)
#define _STXXL_PRNT_ERRLOG      (1 << 3)
#define _STXXL_PRNT_ADDNEWLINE  (1 << 16)
#define _STXXL_PRNT_TIMESTAMP   (1 << 17)

#define _STXXL_PRINT_FLAGS_DEFAULT  (_STXXL_PRNT_COUT | _STXXL_PRNT_LOG)
#define _STXXL_PRINT_FLAGS_ERROR    (_STXXL_PRNT_CERR | _STXXL_PRNT_ERRLOG)
#define _STXXL_PRINT_FLAGS_VERBOSE  (_STXXL_PRINT_FLAGS_DEFAULT | _STXXL_PRNT_TIMESTAMP)


__STXXL_BEGIN_NAMESPACE

void print_msg(const char * label, const std::string & msg, unsigned flags);

__STXXL_END_NAMESPACE


#define _STXXL_ENFORCE_SEMICOLON stxxl::STXXL_UNUSED("expecting the next token to be a ';'")

#define _STXXL_PRINT(label, message, flags) \
    { std::ostringstream str_; \
      str_ << message; \
      stxxl::print_msg(label, str_.str(), flags | _STXXL_PRNT_ADDNEWLINE); \
    } _STXXL_ENFORCE_SEMICOLON

#define _STXXL_NOT_VERBOSE { } _STXXL_ENFORCE_SEMICOLON


#ifdef STXXL_FORCE_VERBOSE_LEVEL
#undef STXXL_VERBOSE_LEVEL
#define STXXL_VERBOSE_LEVEL STXXL_FORCE_VERBOSE_LEVEL
#endif

#ifdef STXXL_DEFAULT_VERBOSE_LEVEL
#ifndef STXXL_VERBOSE_LEVEL
#define STXXL_VERBOSE_LEVEL STXXL_DEFAULT_VERBOSE_LEVEL
#endif
#endif

#ifndef STXXL_VERBOSE_LEVEL
#define STXXL_VERBOSE_LEVEL -1
#endif


#if STXXL_VERBOSE_LEVEL > -10
 #define STXXL_MSG(x) _STXXL_PRINT("STXXL-MSG", x, _STXXL_PRINT_FLAGS_DEFAULT)
#else
 // Please do not report STXXL problems with STXXL_MSG disabled!
 #define STXXL_MSG(x) _STXXL_NOT_VERBOSE
#endif

#if STXXL_VERBOSE_LEVEL > -100
 #define STXXL_ERRMSG(x) _STXXL_PRINT("STXXL-ERRMSG", x, _STXXL_PRINT_FLAGS_ERROR)
#else
 // Please do not report STXXL problems with STXXL_ERRMSG disabled!
 #define STXXL_ERRMSG(x) _STXXL_NOT_VERBOSE
#endif


// STXXL_VERBOSE0 should be used for current debugging activity only,
// and afterwards be replaced by STXXL_VERBOSE1 or higher.
// Code that actively uses STXXL_VERBOSE0 should never get into a release.

#if STXXL_VERBOSE_LEVEL > -1
 #define STXXL_VERBOSE0(x) _STXXL_PRINT("STXXL-VERBOSE0", x, _STXXL_PRINT_FLAGS_VERBOSE)
#else
 #define STXXL_VERBOSE0(x) _STXXL_NOT_VERBOSE
#endif

#if STXXL_VERBOSE_LEVEL > 0
 #define STXXL_VERBOSE1(x) _STXXL_PRINT("STXXL-VERBOSE1", x, _STXXL_PRINT_FLAGS_VERBOSE)
#else
 #define STXXL_VERBOSE1(x) _STXXL_NOT_VERBOSE
#endif

#define STXXL_VERBOSE(x) STXXL_VERBOSE1(x)

#if STXXL_VERBOSE_LEVEL > 1
 #define STXXL_VERBOSE2(x) _STXXL_PRINT("STXXL-VERBOSE2", x, _STXXL_PRINT_FLAGS_VERBOSE)
#else
 #define STXXL_VERBOSE2(x) _STXXL_NOT_VERBOSE
#endif

#if STXXL_VERBOSE_LEVEL > 2
 #define STXXL_VERBOSE3(x) _STXXL_PRINT("STXXL-VERBOSE3", x, _STXXL_PRINT_FLAGS_VERBOSE)
#else
 #define STXXL_VERBOSE3(x) _STXXL_NOT_VERBOSE
#endif

////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_MSVC

#define stxxl_win_lasterror_exit(errmsg, exception_type) \
    { \
        LPVOID lpMsgBuf; \
        DWORD dw = GetLastError(); \
        FormatMessage( \
            FORMAT_MESSAGE_ALLOCATE_BUFFER | \
            FORMAT_MESSAGE_FROM_SYSTEM, \
            NULL, \
            dw, \
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
            (LPTSTR)&lpMsgBuf, \
            0, NULL); \
        std::ostringstream str_; \
        str_ << "Error in " << errmsg << ", error code " << dw << ": " << ((char *)lpMsgBuf); \
        LocalFree(lpMsgBuf); \
        throw exception_type(str_.str()); \
    } _STXXL_ENFORCE_SEMICOLON

#endif


#endif // !STXXL_VERBOSE_HEADER
// vim: et:ts=4:sw=4
