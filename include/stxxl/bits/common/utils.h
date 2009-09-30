/***************************************************************************
 *  include/stxxl/bits/common/utils.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2006 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_UTILS_HEADER
#define STXXL_UTILS_HEADER

#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>
#include <limits>

#include <cassert>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef STXXL_BOOST_FILESYSTEM
 #include <boost/filesystem/operations.hpp>
#endif

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/common/is_sorted.h>
#include <stxxl/bits/common/error_handling.h>


__STXXL_BEGIN_NAMESPACE

template <typename U>
inline void STXXL_UNUSED(const U &)
{ }

#ifdef BOOST_MSVC
  #define __STXXL_DEPRECATED(x) __declspec(deprecated) x
#elif defined(__GNUG__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) < 30400)
  // no __attribute__ ((__deprecated__)) in GCC 3.3
  #define __STXXL_DEPRECATED(x) x
#else
  #define __STXXL_DEPRECATED(x) x __attribute__ ((__deprecated__))
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X__)
#define STXXL_STATIC_ASSERT(x) static_assert(x, #x)
#else
#define STXXL_STATIC_ASSERT(x) { typedef int static_assert_dummy_type[(x) ? 1 : -1]; }
#endif

////////////////////////////////////////////////////////////////////////////

inline std::vector<std::string>
split(const std::string & str, const std::string & sep)
{
    std::vector<std::string> result;
    if (str.empty())
        return result;

    std::string::size_type CurPos(0), LastPos(0);
    while (1)
    {
        CurPos = str.find(sep, LastPos);
        if (CurPos == std::string::npos)
            break;

        std::string sub =
            str.substr(LastPos,
                       std::string::size_type(CurPos -
                                              LastPos));
        if (sub.size())
            result.push_back(sub);

        LastPos = CurPos + sep.size();
    }

    std::string sub = str.substr(LastPos);
    if (sub.size())
        result.push_back(sub);

    return result;
}

////////////////////////////////////////////////////////////////////////////

inline stxxl::int64 atoint64(const char * s)
{
#ifdef BOOST_MSVC
    return _atoi64(s);
#else
    return atoll(s);
#endif
}

////////////////////////////////////////////////////////////////////////////

template <typename Tp>
inline const Tp &
STXXL_MIN(const Tp & a, const Tp & b)
{
    return std::min<Tp>(a, b);
}

template <typename Tp>
inline const Tp &
STXXL_MAX(const Tp & a, const Tp & b)
{
    return std::max<Tp>(a, b);
}

#define STXXL_DIVRU(a, b) ((a) / (b) + !(!((a) % (b))))

////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#define HAVE_BUILTIN_EXPECT
#endif

#ifdef HAVE_BUILTIN_EXPECT
 #define LIKELY(c)   __builtin_expect((c), 1)
#else
 #define LIKELY(c)   c
#endif

#ifdef HAVE_BUILTIN_EXPECT
 #define UNLIKELY(c)   __builtin_expect((c), 0)
#else
 #define UNLIKELY(c)   c
#endif

////////////////////////////////////////////////////////////////////////////

inline uint64 longhash1(uint64 key_)
{
    key_ += ~(key_ << 32);
    key_ ^= (key_ >> 22);
    key_ += ~(key_ << 13);
    key_ ^= (key_ >> 8);
    key_ += (key_ << 3);
    key_ ^= (key_ >> 15);
    key_ += ~(key_ << 27);
    key_ ^= (key_ >> 31);
    return key_;
}

////////////////////////////////////////////////////////////////////////////

template <class T>
inline void swap_1D_arrays(T * a, T * b, unsigned_type size)
{
    for (unsigned_type i = 0; i < size; ++i)
        std::swap(a[i], b[i]);
}

////////////////////////////////////////////////////////////////////////////

__STXXL_END_NAMESPACE

#endif // !STXXL_UTILS_HEADER
// vim: et:ts=4:sw=4
