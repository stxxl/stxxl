#ifndef MYUTILS_HEADER
#define MYUTILS_HEADER

/***************************************************************************
 *            utils.h
 *
 *  Sat Aug 24 23:54:29 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif


#include <iostream>
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sstream>


#ifdef BOOST_MSVC
#else
 #include <sys/time.h>
 #include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string>

#ifdef STXXL_BOOST_TIMESTAMP
 #include <boost/date_time/posix_time/posix_time.hpp>
#endif

#ifdef STXXL_BOOST_FILESYSTEM
 #include "boost/filesystem/operations.hpp"
#endif

#include "namespace.h"
#include "log.h"
#include "exceptions.h"


__STXXL_BEGIN_NAMESPACE

//#define assert(x)

#define __STXXL_STRING(x) # x

#define STXXL_MSG(x) \
    { std::cout << "[STXXL-MSG] " << x << std::endl; std::cout.flush(); \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-MSG] " << x << std::endl; stxxl::logger::get_instance()->log_stream().flush(); \
    };

#define STXXL_ERRMSG(x) \
    { std::cerr << "[STXXL-ERRMSG] " << x << std::endl; std::cerr.flush(); \
      stxxl::logger::get_instance()->errlog_stream() << "[STXXL-ERRMSG] " << x << std::endl; stxxl::logger::get_instance()->errlog_stream().flush(); \
    };


#if STXXL_VERBOSE_LEVEL > 0
 #define STXXL_VERBOSE1(x) \
    { std::cout << "[STXXL-VERBOSE1] " << x << std::endl; std::cerr.flush(); \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-VERBOSE1] " << x << std::endl; stxxl::logger::get_instance()->log_stream().flush(); \
    };
#else
 #define STXXL_VERBOSE1(x) ;
#endif

#define STXXL_VERBOSE(x) STXXL_VERBOSE1(x)

#if STXXL_VERBOSE_LEVEL > 1
 #define STXXL_VERBOSE2(x) \
    { std::cout << "[STXXL-VERBOSE2] " << x << std::endl; std::cerr.flush(); \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-VERBOSE2] " << x << std::endl; stxxl::logger::get_instance()->log_stream().flush(); \
    };
#else
 #define STXXL_VERBOSE2(x) ;
#endif

#if STXXL_VERBOSE_LEVEL > 2
 #define STXXL_VERBOSE3(x) \
    { std::cout << "[STXXL-VERBOSE3] " << x << std::endl; std::cerr.flush(); \
      stxxl::logger::get_instance()->log_stream() << "[STXXL-VERBOSE3] " << x << std::endl; stxxl::logger::get_instance()->log_stream().flush(); \
    };
#else
 #define STXXL_VERBOSE3(x) ;
#endif

/* DEPRICATED
   inline void
   stxxl_perror (const char *errmsg, int errcode)
   {
        exit (errcode);
   }
 */

#ifndef STXXL_DEBUG_ON
 #define STXXL_DEBUG_ON
#endif

#ifdef STXXL_DEBUG_ON

/* DEPRICATED
 #define stxxl_error(errmsg) { perror(errmsg); exit(errno); }
 */

inline std::string perror_string()
{
    return std::string(strerror(errno));
}

 #ifdef BOOST_MSVC
  #define STXXL_PRETTY_FUNCTION_NAME __FUNCTION__
 #else
  #define STXXL_PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
 #endif

 #define stxxl_function_error(exception_type) \
    { \
        std::ostringstream str_; \
        str_ << "Error in function " << \
        STXXL_PRETTY_FUNCTION_NAME << \
        " " << perror_string(); \
        throw exception_type(str_.str()); \
    }

 #define stxxl_nassert(expr, exception_type) \
    { \
        int ass_res = expr; \
        if (ass_res) \
        { \
            std::ostringstream str_; \
            str_ << "Error in function: " << \
            STXXL_PRETTY_FUNCTION_NAME << " " \
                 << __STXXL_STRING(expr); \
            throw exception_type(str_.str()); \
        } \
    }

//#define stxxl_ifcheck(expr) if((expr)<0) { std::cerr<<"Error in function "<<STXXL_PRETTY_FUNCTION_NAME<<" "; stxxl_error(__STXXL_STRING(expr));}
 #define stxxl_ifcheck(expr, exception_type) \
    if ((expr) < 0) \
    { \
        std::ostringstream str_; \
        str_ << "Error in function " << \
        STXXL_PRETTY_FUNCTION_NAME << \
        " " << perror_string(); \
        throw exception_type(str_.str()); \
    }


 #define stxxl_ifcheck_win(expr, exception_type) \
    if ((expr) == 0) \
    { \
        std::ostringstream str_; \
        str_ << "Error in function " << \
        STXXL_PRETTY_FUNCTION_NAME << \
        " " << perror_string(); \
        throw exception_type(str_.str()); \
    }

// #define stxxl_ifcheck_i(expr,info) if((expr)<0) { std::cerr<<"Error in function "<<STXXL_PRETTY_FUNCTION_NAME<<" Info: "<< info<<" "; stxxl_error(__STXXL_STRING(expr)); }

 #define stxxl_ifcheck_i(expr, info, exception_type) \
    if ((expr) < 0) \
    { \
        std::ostringstream str_; \
        str_ << "Error in function " << \
        STXXL_PRETTY_FUNCTION_NAME << " Info: " << \
        info << " " << perror_string(); \
        throw exception_type(str_.str()); \
    }

 #define stxxl_debug(expr) expr

 #define STXXL_FORMAT_ERROR_MSG(str_, errmsg_) \
    std::ostringstream str_; str_ << "Error in " << errmsg_;


 #ifdef BOOST_MSVC

  #define stxxl_win_lasterror_exit(errmsg, exception_type)  \
    { \
        TCHAR szBuf[80];  \
        LPVOID lpMsgBuf; \
        DWORD dw = GetLastError(); \
        FormatMessage( \
            FORMAT_MESSAGE_ALLOCATE_BUFFER | \
            FORMAT_MESSAGE_FROM_SYSTEM, \
            NULL, \
            dw, \
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
            (LPTSTR) &lpMsgBuf, \
            0, NULL ); \
        std::ostringstream str_; \
        str_ << "Error in " << errmsg << ", error code " << dw << ": " << ((char *)lpMsgBuf); \
        LocalFree(lpMsgBuf); \
        throw exception_type(str_.str());  \
    }

 #endif

#else

 #define stxxl_error(errmsg) ;

 #define stxxl_function_error ;

 #define stxxl_nassert(expr) expr;

 #define stxxl_ifcheck(expr) expr; if (0) { }

 #define stxxl_debug(expr) ;

 #ifdef BOOST_MSVC
  #define stxxl_win_lasterror_exit(errmsg) ;
 #endif

#endif

// returns number of seconds from ...
inline double
stxxl_timestamp ()
{
#ifdef STXXL_BOOST_TIMESTAMP
    boost::posix_time::ptime MyTime = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration Duration = MyTime.time_of_day();
    double sec = double (Duration.hours()) * 3600. +
                 double (Duration.minutes()) * 60. +
                 double (Duration.seconds()) +
                 double (Duration.fractional_seconds()) / (pow(10., Duration.num_fractional_digits()));
    return sec;
#else
    struct timeval tp;
    gettimeofday (&tp, NULL);
    return double (tp.tv_sec) + tp.tv_usec / 1000000.;
#endif
}


inline
std::string
stxxl_tmpfilename (std::string dir, std::string prefix)
{
    //stxxl_debug(cerr <<" TMP:"<< dir.c_str() <<":"<< prefix.c_str()<< endl);
    int rnd;
    char buffer[1024];
    std::string result;

#ifndef STXXL_BOOST_FILESYSTEM
    struct stat st;
#endif

    do
    {
        rnd = rand ();
        sprintf (buffer, "%u", rnd);
        result = dir + prefix + buffer;
    }
#ifdef STXXL_BOOST_FILESYSTEM
    while (boost::filesystem::exists(result));

    return result;
#else
    while (!lstat (result.c_str (), &st));

    if (errno != ENOENT)
        stxxl_function_error(io_error)

        return result;

#endif
}

inline
std::vector <
             std::string >
split (const std::string & str, const std::string & sep)
{
    std::vector < std::string > result;
    if (str.empty ())
        return result;


    std::string::size_type CurPos (0), LastPos (0);
    while (1)
    {
        CurPos = str.find (sep, LastPos);
        if (CurPos == std::string::npos)
            break;


        std::string sub =
            str.substr (LastPos,
                        std::string::size_type (CurPos -
                                                LastPos));
        if (sub.size ())
            result.push_back (sub);


        LastPos = CurPos + sep.size ();
    };

    std::string sub = str.substr (LastPos);
    if (sub.size ())
        result.push_back (sub);


    return result;
};

#define str2int(str) atoi(str.c_str())

inline
std::string
int2str (int i)
{
    char buf[32];
    sprintf (buf, "%d", i);
    return std::string (buf);
}

#ifndef BOOST_MSVC
 #define STXXL_MIN(a, b) ( std::min(a, b) )
 #define STXXL_MAX(a, b) ( std::max(a, b) )
#else
 #define STXXL_MIN(a, b) ( (std::min)(a, b) )
 #define STXXL_MAX(a, b) ( (std::max)(a, b) )
#endif

#define STXXL_L2_SIZE  (512 * 1024)

#define div_and_round_up(a, b) ( (a) / (b) + !(!((a) % (b))))

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
{ };

inline double io_wait_time()
{
    return -1.0;
};

#endif

#ifdef STXXL_BOOST_CONFIG
 #ifdef BOOST_MSVC
typedef __int64 int64;
typedef unsigned __int64 uint64;
 #else
typedef long long int int64;
typedef unsigned long long uint64;
 #endif
#else
typedef long long int int64;
typedef unsigned long long uint64;
#endif


// integer types declarations
enum { my_pointer_size = sizeof(void *) };

template <int PtrSize>
struct choose_int_types
{};

template <>
struct choose_int_types < 4 >  // for 32-bit processors/compilers
{
    typedef int int_type;
    typedef unsigned unsigned_type;
};

template <>
struct choose_int_types < 8 > // for 64-bit processors/compilers
{
    typedef long long int int_type;
    typedef long long unsigned unsigned_type;
};

typedef choose_int_types<my_pointer_size>::int_type int_type;
typedef choose_int_types<my_pointer_size>::unsigned_type unsigned_type;


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


// designed for typed_block (to use with std::vector )
template <class T>
class new_alloc {
public:
    // type definitions
    typedef T value_type;
    typedef T *       pointer;
    typedef const T * const_pointer;
    typedef T &       reference;
    typedef const T & const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // rebind allocator to type U
    template <class U>
    struct rebind {
        typedef new_alloc<U> other;
    };

    // return address of values
    pointer address (reference value) const
    {
        return &value;
    }
    const_pointer address (const_reference value) const
    {
        return &value;
    }

    new_alloc() throw () { }
    new_alloc(const new_alloc &) throw () { }
    template <class U>
    new_alloc (const new_alloc<U> &) throw () { }
    ~new_alloc() throw () { }

    // return maximum number of elements that can be allocated
    size_type max_size () const throw ()
    {
        return (std::numeric_limits < std::size_t > ::max)() / sizeof(T);
    }

    // allocate but don't initialize num elements of type T
    pointer allocate (size_type num, const void * = 0)
    {
        pointer ret = (pointer)(T::operator new(num * sizeof(T)));
        return ret;
    }

    // initialize elements of allocated storage p with value value
    void construct (pointer p, const T & value)
    {
        // initialize memory with placement new
        new ((void *)p)T(value);
    }

    // destroy elements of initialized storage p
    void destroy (pointer p)
    {
        // destroy objects by calling their destructor
        p->~T();
    }

    // deallocate storage p of deleted elements
    void deallocate (pointer p, size_type /*num*/)
    {
        T::operator delete((void *)p);
    }
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator== (const new_alloc<T1> &,
                 const new_alloc<T2> &) throw ()
{
    return true;
}
template <class T1, class T2>
bool operator!= (const new_alloc<T1> &,
                 const new_alloc<T2> &) throw ()
{
    return false;
}

inline unsigned_type sort_memory_usage_factor()
{
#ifdef __MCSTL__
    return (mcstl::HEURISTIC::sort_algorithm == mcstl::HEURISTIC::PMWMS) ? 2 : 1;
#else
    return 1;
#endif
}

__STXXL_END_NAMESPACE
#endif
