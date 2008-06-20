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
#include <stxxl/bits/common/log.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/common/timer.h>

__STXXL_BEGIN_NAMESPACE

template <typename U>
inline void UNUSED(const U &)
{ }

#define __STXXL_STRING(x) # x


#define STXXL_MSG(x) \
    { std::cout << "[STXXL-MSG] " << x << std::endl << std::flush; \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-MSG] " << x << std::endl << std::flush; \
    }

#define STXXL_ERRMSG(x) \
    { std::cerr << "[STXXL-ERRMSG] " << x << std::endl << std::flush; \
      stxxl::logger::get_instance()->errlog_stream() << "[STXXL-ERRMSG] " << x << std::endl << std::flush; \
    }


#ifndef STXXL_VERBOSE_LEVEL
#define STXXL_VERBOSE_LEVEL -1
#endif

// STXXL_VERBOSE0 should be used for current debugging activity only,
// and afterwards be replaced by STXXL_VERBOSE1 or higher.
// Code that actively uses STXXL_VERBOSE0 should never get into a release.

#if STXXL_VERBOSE_LEVEL > -1
 #define STXXL_VERBOSE0(x) \
    { std::cout << "[STXXL-VERBOSE0] " << x << std::endl << std::flush; \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-VERBOSE0] " << x << std::endl << std::flush; \
    }
#else
 #define STXXL_VERBOSE0(x)
#endif

#if STXXL_VERBOSE_LEVEL > 0
 #define STXXL_VERBOSE1(x) \
    { std::cout << "[STXXL-VERBOSE1] " << x << std::endl << std::flush; \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-VERBOSE1] " << x << std::endl << std::flush; \
    }
#else
 #define STXXL_VERBOSE1(x)
#endif

#define STXXL_VERBOSE(x) STXXL_VERBOSE1(x)

#if STXXL_VERBOSE_LEVEL > 1
 #define STXXL_VERBOSE2(x) \
    { std::cout << "[STXXL-VERBOSE2] " << x << std::endl << std::flush; \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-VERBOSE2] " << x << std::endl << std::flush; \
    }
#else
 #define STXXL_VERBOSE2(x)
#endif

#if STXXL_VERBOSE_LEVEL > 2
 #define STXXL_VERBOSE3(x) \
    { std::cout << "[STXXL-VERBOSE3] " << x << std::endl << std::flush; \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-VERBOSE3] " << x << std::endl << std::flush; \
    }
#else
 #define STXXL_VERBOSE3(x)
#endif


#ifdef BOOST_MSVC
 #define STXXL_PRETTY_FUNCTION_NAME __FUNCTION__
#else
 #define STXXL_PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

#define STXXL_FORMAT_ERROR_MSG(str_, errmsg_) \
    std::ostringstream str_; str_ << "Error in " << errmsg_

#define STXXL_THROW(exception_type, location, error_message) \
    { \
        std::ostringstream msg_; \
        msg_ << "Error in " << location << ": " << error_message; \
        throw exception_type(msg_.str()); \
    }

#define STXXL_THROW2(exception_type, error_message) \
    STXXL_THROW(exception_type, "function " << STXXL_PRETTY_FUNCTION_NAME, \
                "Info: " << error_message << " " << strerror(errno))

template <typename E>
inline void stxxl_util_function_error(const char * func_name, const char * expr = 0)
{
    std::ostringstream str_;
    str_ << "Error in function " << func_name << " " << (expr ? expr : strerror(errno));
    throw E(str_.str());
}

 #define stxxl_function_error(exception_type) \
    stxxl::stxxl_util_function_error<exception_type>(STXXL_PRETTY_FUNCTION_NAME)

template <typename E>
inline bool helper_check_success(bool success, const char * func_name, const char * expr = 0)
{
    if (!success)
        stxxl_util_function_error<E>(func_name, expr);
    return success;
}

template <typename E, typename INT>
inline bool helper_check_eq_0(INT res, const char * func_name, const char * expr)
{
    return helper_check_success<E>(res == 0, func_name, expr);
}

#define check_pthread_call(expr) \
    stxxl::helper_check_eq_0<stxxl::resource_error>(expr, STXXL_PRETTY_FUNCTION_NAME, __STXXL_STRING(expr))

template <typename E, typename INT>
inline bool helper_check_ge_0(INT res, const char * func_name)
{
    return helper_check_success<E>(res >= 0, func_name);
}

#define stxxl_check_ge_0(expr, exception_type) \
    stxxl::helper_check_ge_0<exception_type>(expr, STXXL_PRETTY_FUNCTION_NAME)

template <typename E, typename INT>
inline bool helper_check_ne_0(INT res, const char * func_name)
{
    return helper_check_success<E>(res != 0, func_name);
}

#define stxxl_check_ne_0(expr, exception_type) \
    stxxl::helper_check_ne_0<exception_type>(expr, STXXL_PRETTY_FUNCTION_NAME)

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
    }

#endif

inline double
stxxl_timestamp()
{
    return timestamp();
}


inline
std::string
stxxl_tmpfilename(std::string dir, std::string prefix)
{
    //STXXL_VERBOSE0(" TMP:"<< dir.c_str() <<":"<< prefix.c_str());
    int rnd;
    char buffer[1024];
    std::string result;

#ifndef STXXL_BOOST_FILESYSTEM
    struct stat st;
#endif

    do
    {
        rnd = rand();
        sprintf(buffer, "%d", rnd);
        result = dir + prefix + buffer;
    }
#ifdef STXXL_BOOST_FILESYSTEM
    while (boost::filesystem::exists(result));

    return result;
#else
    while (!lstat(result.c_str(), &st));

    if (errno != ENOENT)
        stxxl_function_error(io_error);

    return result;
#endif
}

inline
std::vector<std::string>
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

#define str2int(str) atoi(str.c_str())

inline
std::string
int2str(int i)
{
    char buf[32];
    sprintf(buf, "%d", i);
    return std::string(buf);
}

#define STXXL_MIN(a, b) ((std::min)(a, b))
#define STXXL_MAX(a, b) ((std::max)(a, b))

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

 #define START_COUNT_WAIT_TIME  double count_wait_begin = stxxl_timestamp();
 #define END_COUNT_WAIT_TIME    stxxl::wait_time_counter += (stxxl_timestamp() - count_wait_begin);

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
void swap_1D_arrays(T * a, T * b, unsigned_type size)
{
    for (unsigned_type i = 0; i < size; ++i)
        std::swap(a[i], b[i]);
}

template <class T>
class new_alloc;


template <typename T, typename U>
struct new_alloc_rebind;

template <typename T>
struct new_alloc_rebind<T, T>{
    typedef new_alloc<T> other;
};

template <typename T, typename U>
struct new_alloc_rebind {
    typedef std::allocator<U> other;
};


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

    // rebind allocator to type U, use new_alloc only if U == T
    template <class U>
    struct rebind {
        typedef typename new_alloc_rebind<T, U>::other other;
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

    template <class U>
    operator std::allocator<U>()
    {
        static std::allocator<U> helper_allocator;
        return helper_allocator;
    }

    // return maximum number of elements that can be allocated
    size_type max_size() const throw ()
    {
        return (std::numeric_limits<std::size_t>::max) () / sizeof(T);
    }

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
    void deallocate(pointer p, size_type /*num*/)
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

inline unsigned_type sort_memory_usage_factor()
{
#if defined(__MCSTL__) && !defined(STXXL_NOT_CONSIDER_SORT_MEMORY_OVERHEAD)
    return (mcstl::HEURISTIC::sort_algorithm == mcstl::HEURISTIC::MWMS && mcstl::HEURISTIC::num_threads > 1) ? 2 : 1;   //memory overhead for multiway mergesort
#else
    return 1;                                                                                                           //no overhead
#endif
}

#ifdef __MCSTL__
#define __STXXL_FORCE_SEQUENTIAL , mcstl::sequential_tag()
#else
#define __STXXL_FORCE_SEQUENTIAL
#endif

inline stxxl::int64 atoint64(const char * s)
{
#ifdef BOOST_MSVC
    return _atoi64(s);
#else
    return atoll(s);
#endif
}

__STXXL_END_NAMESPACE

#endif // !STXXL_UTILS_HEADER
