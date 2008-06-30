/***************************************************************************
 *  include/stxxl/bits/common/utils_ledasm.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2006 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_UTILS_HEADER
#define STXXL_UTILS_HEADER

/*
 * Abgespeckte Version von common/utils.h für LEDA-SM/TPIE Tests
 * (containers/leda_sm_pq_benchmark.cpp und andere). Mit originalem
 * common/utils.h gibt es name collisions mit Stxxl (LEDA-SM/TPIE
 * benutzen keine namespaces) und auch andere Probleme.
 */

#include <iostream>
#include <algorithm>
#include <vector>

#include <cassert>
#include <cstdio>
#include <cerrno>
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
#include <stxxl/bits/common/log.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/common/timer.h>


__STXXL_BEGIN_NAMESPACE

//#define assert(x)

#define __STXXL_STRING(x) # x

#define STXXL_MSG(x) \
    { std::cout << "[STXXL-MSG] " << x << std::endl; std::cout.flush(); \
    };

#define STXXL_ERRMSG(x) \
    { std::cerr << "[STXXL-ERRMSG] " << x << std::endl; std::cerr.flush(); \
    };

#define STXXL_DEBUGMSG(x) STXXL_MSG(x)


#ifndef STXXL_VERBOSE_LEVEL
#define STXXL_VERBOSE_LEVEL 0
#endif

#if STXXL_VERBOSE_LEVEL > 0
 #define STXXL_VERBOSE1(x) \
    { std::cout << "[STXXL-VERBOSE1] " << x << std::endl; std::cerr.flush(); \
    };
#else
 #define STXXL_VERBOSE1(x) ;
#endif

#define STXXL_VERBOSE(x) STXXL_VERBOSE1(x)

#if STXXL_VERBOSE_LEVEL > 1
 #define STXXL_VERBOSE2(x) \
    { std::cout << "[STXXL-VERBOSE2] " << x << std::endl; std::cerr.flush(); \
    };
#else
 #define STXXL_VERBOSE2(x) ;
#endif

#if STXXL_VERBOSE_LEVEL > 2
 #define STXXL_VERBOSE3(x) \
    { std::cout << "[STXXL-VERBOSE3] " << x << std::endl; std::cerr.flush(); \
    };
#else
 #define STXXL_VERBOSE3(x) ;
#endif


inline void
stxxl_perror(const char * /*errmsg*/, int errcode)
{
//	STXXL_ERRMSG(errmsg << " error code " << errcode << " : " << strerror (errcode) );
    exit(errcode);
}

#ifndef STXXL_DEBUG_ON
 #define STXXL_DEBUG_ON
#endif

#ifdef STXXL_DEBUG_ON

 #define stxxl_error(errmsg) { perror(errmsg); exit(errno); }

 #ifdef BOOST_MSVC
  #define STXXL_PRETTY_FUNCTION_NAME __FUNCTION__
 #else
  #define STXXL_PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
 #endif

 #define stxxl_function_error stxxl_error(STXXL_PRETTY_FUNCTION_NAME)


 #define stxxl_nassert(expr) { int ass_res = expr; if (ass_res) { std::cerr << "Error in function: " << STXXL_PRETTY_FUNCTION_NAME << " ";  stxxl_perror(__STXXL_STRING(expr), ass_res); } }

 #define stxxl_ifcheck(expr) if ((expr) < 0) { std::cerr << "Error in function " << STXXL_PRETTY_FUNCTION_NAME << " "; stxxl_error(__STXXL_STRING(expr)); }
 #define stxxl_ifcheck_win(expr) if ((expr) == 0) { std::cerr << "Error in function " << STXXL_PRETTY_FUNCTION_NAME << " "; stxxl_error(__STXXL_STRING(expr)); }

 #define stxxl_ifcheck_i(expr, info) if ((expr) < 0) { std::cerr << "Error in function " << STXXL_PRETTY_FUNCTION_NAME << " Info: " << info << " "; stxxl_error(__STXXL_STRING(expr)); }

 #ifdef BOOST_MSVC

  #define stxxl_win_lasterror_exit(errmsg) \
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
        STXXL_ERRMSG("Error in " << errmsg << ", error code " << dw << ": " << ((char *)lpMsgBuf)); \
        LocalFree(lpMsgBuf); \
        ExitProcess(dw); \
    }

 #endif

#else

 #define stxxl_error(errmsg) ;

 #define stxxl_function_error ;

 #define stxxl_nassert(expr) expr;

 #define stxxl_ifcheck(expr) expr; if (0) { }

 #ifdef BOOST_MSVC
  #define stxxl_win_lasterror_exit(errmsg) ;
 #endif

#endif

inline double
stxxl_timestamp()
{
    return timestamp();
}


inline stxxl::int64 atoint64(const char * s)
{
#ifdef BOOST_MSVC
    return _atoi64(s);
#else
    return atoll(s);
#endif
}


#ifndef BOOST_MSVC
 #define STXXL_MIN(a, b) (std::min(a, b))
 #define STXXL_MAX(a, b) (std::max(a, b))
#else
 #define STXXL_MIN(a, b) ((std::min)(a, b))
 #define STXXL_MAX(a, b) ((std::max)(a, b))
#endif

#define STXXL_L2_SIZE  (512 * 1024)

#define div_and_round_up(a, b) ((a) / (b) + !(!((a) % (b))))

#define log2(x) (log(x) / log(2.))


//#define HAVE_BUILTIN_EXPECT


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

//#define COUNT_WAIT_TIME

#ifdef COUNT_WAIT_TIME
 #define START_COUNT_WAIT_TIME   double count_wait_begin = stxxl_timestamp();
 #define END_COUNT_WAIT_TIME             stxxl::wait_time_counter += (stxxl_timestamp() - count_wait_begin);

 #define reset_io_wait_time() stxxl::wait_time_counter = 0.0;

 #define io_wait_time() (stxxl::wait_time_counter)


#else
 #define START_COUNT_WAIT_TIME
 #define END_COUNT_WAIT_TIME
inline void reset_io_wait_time()
{ }

inline double io_wait_time()
{
    return -1.0;
}

#endif


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


template <class _ForwardIter>
bool is_sorted(_ForwardIter __first, _ForwardIter __last)
{
    if (__first == __last)
        return true;


    _ForwardIter __next = __first;
    for (++__next; __next != __last; __first = __next, ++__next) {
        if (*__next < *__first)
            return false;
    }

    return true;
}

template <class _ForwardIter, class _StrictWeakOrdering>
bool is_sorted(_ForwardIter __first, _ForwardIter __last,
               _StrictWeakOrdering __comp)
{
    if (__first == __last)
        return true;


    _ForwardIter __next = __first;
    for (++__next; __next != __last; __first = __next, ++__next) {
        if (__comp(*__next, *__first))
            return false;
    }
    return true;
}

template <class T>
void swap_1D_arrays(T * a, T * b, unsigned size)
{
    for (unsigned i = 0; i < size; ++i)
        std::swap(a[i], b[i]);
}


// designed for typed_block (to use with std::vector )
template <class T>
class new_alloc {
public:
    // type definitions
    typedef T value_type;
    typedef T * pointer;
    typedef const T * const_pointer;
    typedef T & reference;
    typedef const T & const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // rebind allocator to type U
    template <class U>
    struct rebind {
        typedef new_alloc<U> other;
    };

    // return address of values
    pointer address(reference value) const
    {
        return &value;
    }
    const_pointer address(const_reference value) const
    {
        return &value;
    }

    new_alloc() throw () { }
    new_alloc(const new_alloc &) throw () { }
    template <class U>
    new_alloc(const new_alloc<U> &) throw () { }
    ~new_alloc() throw () { }

    // allocate but don't initialize num elements of type T
    pointer allocate(size_type num, const void * = 0)
    {
        pointer ret = (pointer)(T::operator new(num * sizeof(T)));
        return ret;
    }

    // initialize elements of allocated storage p with value value
    void construct(pointer p, const T & value)
    {
        // initialize memory with placement new
        new ((void *)p)T(value);
    }

    // destroy elements of initialized storage p
    void destroy(pointer p)
    {
        // destroy objects by calling their destructor
        p->~T();
    }

    // deallocate storage p of deleted elements
    void deallocate(pointer p, size_type num)
    {
        T::operator delete((void *)p);
    }
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator == (const new_alloc<T1> &,
                  const new_alloc<T2> &) throw ()
{
    return true;
}
template <class T1, class T2>
bool operator != (const new_alloc<T1> &,
                  const new_alloc<T2> &) throw ()
{
    return false;
}

__STXXL_END_NAMESPACE

#endif // !STXXL_UTILS_HEADER
