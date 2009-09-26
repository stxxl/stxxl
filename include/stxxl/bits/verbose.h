/***************************************************************************
 *  include/stxxl/bits/verbose.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007-2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_VERBOSE_HEADER
#define STXXL_VERBOSE_HEADER

#include <iostream>
#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/log.h>


__STXXL_BEGIN_NAMESPACE


__STXXL_END_NAMESPACE


#define __STXXL_ENFORCE_SEMICOLON stxxl::UNUSED("expecting the next token to be a ';'")

#define _STXXL_PRINT(label, outstream, log_stream, message) \
    { std::ostringstream str_; \
      str_ << "[" label "] " << message << std::endl; \
      outstream << str_.str() << std::flush; \
      stxxl::logger::get_instance()->log_stream() << str_.str() << std::flush; \
    } __STXXL_ENFORCE_SEMICOLON

#define _STXXL_NOT_VERBOSE { } __STXXL_ENFORCE_SEMICOLON

#define STXXL_MSG(x) _STXXL_PRINT("STXXL-MSG", std::cout, log_stream, x)

#define STXXL_ERRMSG(x) _STXXL_PRINT("STXXL-ERRMSG", std::cerr, errlog_stream, x)


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

// STXXL_VERBOSE0 should be used for current debugging activity only,
// and afterwards be replaced by STXXL_VERBOSE1 or higher.
// Code that actively uses STXXL_VERBOSE0 should never get into a release.

#if STXXL_VERBOSE_LEVEL > -1
 #define STXXL_VERBOSE0(x) _STXXL_PRINT("STXXL-VERBOSE0", std::cout, log_stream, x)
#else
 #define STXXL_VERBOSE0(x) _STXXL_NOT_VERBOSE
#endif

#if STXXL_VERBOSE_LEVEL > 0
 #define STXXL_VERBOSE1(x) _STXXL_PRINT("STXXL-VERBOSE1", std::cout, log_stream, x)
#else
 #define STXXL_VERBOSE1(x) _STXXL_NOT_VERBOSE
#endif

#define STXXL_VERBOSE(x) STXXL_VERBOSE1(x)

#if STXXL_VERBOSE_LEVEL > 1
 #define STXXL_VERBOSE2(x) _STXXL_PRINT("STXXL-VERBOSE2", std::cout, log_stream, x)
#else
 #define STXXL_VERBOSE2(x) _STXXL_NOT_VERBOSE
#endif

#if STXXL_VERBOSE_LEVEL > 2
 #define STXXL_VERBOSE3(x) _STXXL_PRINT("STXXL-VERBOSE3", std::cout, log_stream, x)
#else
 #define STXXL_VERBOSE3(x) _STXXL_NOT_VERBOSE
#endif

////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_MSVC

#define stxxl_win_lasterror_exit(errmsg, exception_type) \
    { \
        TCHAR szBuf[80]; \
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
    } __STXXL_ENFORCE_SEMICOLON

#endif


#endif // !STXXL_VERBOSE_HEADER
// vim: et:ts=4:sw=4
