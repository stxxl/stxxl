/***************************************************************************
 *  include/stxxl/bits/verbose.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2005-2006 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_VERBOSE_HEADER
#define STXXL_VERBOSE_HEADER

#include <cstdlib>
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
#define _STXXL_PRNT_THREAD_ID   (1 << 18)

#define _STXXL_PRINT_FLAGS_DEFAULT  (_STXXL_PRNT_COUT | _STXXL_PRNT_LOG)
#define _STXXL_PRINT_FLAGS_ERROR    (_STXXL_PRNT_CERR | _STXXL_PRNT_ERRLOG)
#define _STXXL_PRINT_FLAGS_VERBOSE  (_STXXL_PRINT_FLAGS_DEFAULT | _STXXL_PRNT_TIMESTAMP | _STXXL_PRNT_THREAD_ID)

STXXL_BEGIN_NAMESPACE

void print_msg(const char* label, const std::string& msg, unsigned flags);

STXXL_END_NAMESPACE

#define _STXXL_PRINT(label, message, flags)                                  \
    do {                                                                     \
        std::ostringstream str_;                                             \
        str_ << message;                                                     \
        stxxl::print_msg(label, str_.str(), flags | _STXXL_PRNT_ADDNEWLINE); \
    } while (false)

#define _STXXL_NOT_VERBOSE(message)  \
    do {                             \
        if (0) {                     \
            std::ostringstream str_; \
            str_ << message;         \
        }                            \
    } while (false)

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
 #define STXXL_MSG(x) _STXXL_NOT_VERBOSE(x)
#endif

#if STXXL_VERBOSE_LEVEL > -100
 #define STXXL_ERRMSG(x) _STXXL_PRINT("STXXL-ERRMSG", x, _STXXL_PRINT_FLAGS_ERROR)
#else
// Please do not report STXXL problems with STXXL_ERRMSG disabled!
 #define STXXL_ERRMSG(x) _STXXL_NOT_VERBOSE(x)
#endif

// STXXL_VERBOSE0 should be used for current debugging activity only,
// and afterwards be replaced by STXXL_VERBOSE1 or higher.
// Code that actively uses STXXL_VERBOSE0 should never get into a release.

#if STXXL_VERBOSE_LEVEL > -1
 #define STXXL_VERBOSE0(x) _STXXL_PRINT("STXXL-VERBOSE0", x, _STXXL_PRINT_FLAGS_VERBOSE)
#else
 #define STXXL_VERBOSE0(x) _STXXL_NOT_VERBOSE(x)
#endif

#if STXXL_VERBOSE_LEVEL > 0
 #define STXXL_VERBOSE1(x) _STXXL_PRINT("STXXL-VERBOSE1", x, _STXXL_PRINT_FLAGS_VERBOSE)
#else
 #define STXXL_VERBOSE1(x) _STXXL_NOT_VERBOSE(x)
#endif

#define STXXL_VERBOSE(x) STXXL_VERBOSE1(x)

#if STXXL_VERBOSE_LEVEL > 1
 #define STXXL_VERBOSE2(x) _STXXL_PRINT("STXXL-VERBOSE2", x, _STXXL_PRINT_FLAGS_VERBOSE)
#else
 #define STXXL_VERBOSE2(x) _STXXL_NOT_VERBOSE(x)
#endif

#if STXXL_VERBOSE_LEVEL > 2
 #define STXXL_VERBOSE3(x) _STXXL_PRINT("STXXL-VERBOSE3", x, _STXXL_PRINT_FLAGS_VERBOSE)
#else
 #define STXXL_VERBOSE3(x) _STXXL_NOT_VERBOSE(x)
#endif

// STXXL_VERBOSE[0123]_THIS prefixes "[0xaddress]" and then calls the version
// without _THIS.

#define STXXL_VERBOSE0_THIS(x) \
    STXXL_VERBOSE0("[" << static_cast<void*>(this) << "] " << x)

#define STXXL_VERBOSE1_THIS(x) \
    STXXL_VERBOSE1("[" << static_cast<void*>(this) << "] " << x)

#define STXXL_VERBOSE2_THIS(x) \
    STXXL_VERBOSE2("[" << static_cast<void*>(this) << "] " << x)

#define STXXL_VERBOSE3_THIS(x) \
    STXXL_VERBOSE3("[" << static_cast<void*>(this) << "] " << x)

// STXXL_CHECK is an assertion macro for unit tests, which contrarily to
// assert() also works in release builds. These macros should ONLY be used in
// UNIT TESTS, not in the library source. Use usual assert() there.

#define STXXL_CHECK(condition)                                               \
    do {                                                                     \
        if (!(condition)) {                                                  \
            _STXXL_PRINT("STXXL-CHECK",                                      \
                         #condition " - FAILED @ " __FILE__ ":" << __LINE__, \
                         _STXXL_PRINT_FLAGS_ERROR); abort();                 \
        }                                                                    \
    } while (0)

#define STXXL_CHECK2(condition, text)                                                      \
    do {                                                                                   \
        if (!(condition)) {                                                                \
            _STXXL_PRINT("STXXL-CHECK",                                                    \
                         text << " - " #condition " - FAILED @ " __FILE__ ":" << __LINE__, \
                         _STXXL_PRINT_FLAGS_ERROR); abort();                               \
        }                                                                                  \
    } while (0)

// STXXL_ASSERT is an assertion macro almost identical to assert(). The only
// difference is that with NDEBUG defined, the _code_ itself still exists. This
// silences warnings about unused variables and typedefs in release builds.

#ifdef NDEBUG

#define STXXL_ASSERT(condition)                                                   \
    do { if (0) {                                                                 \
             if (!(condition)) {                                                  \
                 _STXXL_PRINT("STXXL-ASSERT",                                     \
                              #condition " - FAILED @ " __FILE__ ":" << __LINE__, \
                              _STXXL_PRINT_FLAGS_ERROR); abort();                 \
             }                                                                    \
         }                                                                        \
    } while (0)

#else

#define STXXL_ASSERT(condition)                                                   \
    do { if (1) {                                                                 \
             if (!(condition)) {                                                  \
                 _STXXL_PRINT("STXXL-ASSERT",                                     \
                              #condition " - FAILED @ " __FILE__ ":" << __LINE__, \
                              _STXXL_PRINT_FLAGS_ERROR); abort();                 \
             }                                                                    \
         }                                                                        \
    } while (0)

#endif

// STXXL_CHECK_THROW is an assertion macro for unit tests, which checks that
// the enclosed code throws an exception.

#define STXXL_CHECK_THROW(code, exception_type)               \
    do {                                                      \
        bool t_ = false; try { code; }                        \
        catch (const exception_type&) { t_ = true; }          \
        if (t_) break;                                        \
        _STXXL_PRINT("STXXL-CHECK-THROW",                     \
                     #code " - NO EXCEPTION " #exception_type \
                     " @ " __FILE__ ":" << __LINE__,          \
                     _STXXL_PRINT_FLAGS_ERROR);               \
        abort();                                              \
    } while (0)

////////////////////////////////////////////////////////////////////////////

#endif // !STXXL_VERBOSE_HEADER
// vim: et:ts=4:sw=4
