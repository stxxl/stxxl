/***************************************************************************
 *  include/stxxl/bits/common/utils.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2006 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007-2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2008 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_UTILS_HEADER
#define STXXL_UTILS_HEADER

#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>

#include <stxxl/bits/config.h>
#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/compat/type_traits.h>
#include <stxxl/bits/msvc_compatibility.h>


__STXXL_BEGIN_NAMESPACE

////////////////////////////////////////////////////////////////////////////

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)))
#  define STXXL_ATTRIBUTE_UNUSED __attribute__((unused))
#else
#  define STXXL_ATTRIBUTE_UNUSED
#endif

////////////////////////////////////////////////////////////////////////////

#if defined(__GXX_EXPERIMENTAL_CXX0X__)
#define STXXL_STATIC_ASSERT(x) static_assert(x, #x)
#else
#define STXXL_STATIC_ASSERT(x) { typedef int static_assert_dummy_type[(x) ? 1 : -1] STXXL_ATTRIBUTE_UNUSED; }
#endif

////////////////////////////////////////////////////////////////////////////

//! Split a string by given separator string. Returns a vector of strings with
//! at least min_fields.
static inline std::vector<std::string>
split(const std::string & str, const std::string & sep, unsigned int min_fields)
{
    std::vector<std::string> result;
    if (str.empty()) {
        result.resize(min_fields);
        return result;
    }

    std::string::size_type CurPos(0), LastPos(0);
    while (1)
    {
        CurPos = str.find(sep, LastPos);
        if (CurPos == std::string::npos)
            break;

        result.push_back(
            str.substr(LastPos,
                       std::string::size_type(CurPos - LastPos))
            );

        LastPos = CurPos + sep.size();
    }

    std::string sub = str.substr(LastPos);
    result.push_back(sub);

    if (result.size() < min_fields)
        result.resize(min_fields);

    return result;
}

////////////////////////////////////////////////////////////////////////////

//! Parse a string like "343KB" or "44 GiB" into the corresponding size in
//! bytes. Returns the number of bytes and sets ok = true if the string could
//! be parsed correctly.
static inline int64 parse_filesize(const std::string& str, bool& ok)
{
    ok = false;

    char* endptr;
    int64 size = strtoul(str.c_str(), &endptr, 10);
    if (!endptr) return 0; // parse failed, no number

    while (endptr[0] == ' ') ++endptr; // skip over spaces

    if ( endptr[0] == 0 ) // number parsed, no suffix defaults to MiB
        size *= 1024 * 1024;
    else if ( (endptr[0] == 'b' || endptr[0] == 'B') && endptr[1] == 0 ) // bytes
        size *= 1;
    else if (endptr[0] == 'k' || endptr[0] == 'K')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= 1000; // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= 1024; // power of two
        else
            return 0;
    }
    else if (endptr[0] == 'm' || endptr[0] == 'M')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= 1000 * 1000; // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= 1024 * 1024; // power of two
        else
            return 0;
    }
    else if (endptr[0] == 'g' || endptr[0] == 'G')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= 1000 * 1000 * 1000; // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= 1024 * 1024 * 1024; // power of two
        else
            return 0;
    }
    else if (endptr[0] == 't' || endptr[0] == 'T')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= int64(1000) * int64(1000) * int64(1000) * int64(1000); // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= int64(1024) * int64(1024) * int64(1024) * int64(1024); // power of two
        else
            return 0;
    }
    else if (endptr[0] == 'p' || endptr[0] == 'P')
    {
        if ( endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) )
            size *= int64(1000) * int64(1000) * int64(1000) * int64(1000) * int64(1000); // power of ten
        else if ( (endptr[1] == 'i' || endptr[0] == 'I') &&
                  (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
            size *= int64(1024) * int64(1024) * int64(1024) * int64(1024) * int64(1024); // power of two
        else
            return 0;
    }
    else
        return 0;

    ok = true;
    return size;
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

////////////////////////////////////////////////////////////////////////////

template <typename Integral>
inline Integral log2_ceil(Integral i)
{
    return Integral(ceil(log2(i)));
}

template <typename Integral>
inline Integral log2_floor(Integral i)
{
    return Integral(log2(i));
}

////////////////////////////////////////////////////////////////////////////

template <typename Integral, typename Integral2>
inline
typename compat::remove_const<Integral>::type
div_ceil(Integral __n, Integral2 __d)
{
#if 0  // ambiguous overload for std::div(unsigned_anything, unsigned_anything)
    typedef __typeof__(std::div(__n, __d)) div_type;
    div_type result = std::div(__n, __d);
    return result.quot + (result.rem != 0);
#else
    return __n / __d + ((__n % __d) != 0);
#endif
}

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

template <typename Integral>
inline Integral round_up_to_power_of_two(Integral n, unsigned_type power)
{
    Integral pot = 1 << power, // = 0..0 1 0^power
             mask = pot - 1;   // = 0..0 0 1^power
    if (n & mask)  // n not divisible by pot
        return (n & ~mask) + pot;
    else
        return n;
}

////////////////////////////////////////////////////////////////////////////

template <class Container>
inline typename Container::value_type pop(Container & c)
{
    typename Container::value_type r = c.top();
    c.pop();
    return r;
}

template <class Container>
inline typename Container::value_type pop_front(Container & c)
{
    typename Container::value_type r = c.front();
    c.pop_front();
    return r;
}

template <class Container>
inline typename Container::value_type pop_back(Container & c)
{
    typename Container::value_type r = c.back();
    c.pop_back();
    return r;
}

template <class Container>
inline typename Container::value_type pop_begin(Container & c)
{
    typename Container::value_type r = *c.begin();
    c.erase(c.begin());
    return r;
}

////////////////////////////////////////////////////////////////////////////

__STXXL_END_NAMESPACE

#endif // !STXXL_UTILS_HEADER
// vim: et:ts=4:sw=4
