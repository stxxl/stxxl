#ifndef MYUTILS_HEADER
#define MYUTILS_HEADER

/***************************************************************************
 *            utils.h
 *
 *  Sat Aug 24 23:54:29 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string>

#define __STXXL_BEGIN_NAMESPACE namespace stxxl {
#define __STXXL_END_NAMESPACE }



__STXXL_BEGIN_NAMESPACE

//#define assert(x)  

#define __STXXL_STRING(x) #x

#define STXXL_MSG(x) { std::cout << x << std::endl; std::cout.flush(); };
#define STXXL_ERRMSG(x) { std::cerr << x << std::endl; std::cerr.flush(); };
  
#ifdef STXXL_VERBOSE_ON
#define STXXL_VERBOSE(x) { std::cout << x << std::endl; std::cerr.flush(); };
#else
#define STXXL_VERBOSE(x) (0);
#endif
  
	inline void
stxxl_perror (const char *errmsg, int errcode)
{
	std::cerr << errmsg << " error code " << errcode;
	std::cerr << " : " << strerror (errcode) << std::endl;

	exit (errcode);
}

#define STXXL_DEBUG_ON

#ifdef STXXL_DEBUG_ON

#define stxxl_error(errmsg) { perror(errmsg); exit(errno); }

#define stxxl_function_error stxxl_error(__PRETTY_FUNCTION__)

#define stxxl_nassert(expr) { int ass_res=expr; if(ass_res) {  std::cerr << "Error in function: " << __PRETTY_FUNCTION__ << " ";  stxxl_perror(__STXXL_STRING(expr),ass_res); }}

#define stxxl_ifcheck(expr) if((expr)<0) { std::cerr<<"Error in function "<<__PRETTY_FUNCTION__<<" "; stxxl_error(__STXXL_STRING(expr));}

#define stxxl_debug(expr) expr

#else

#define stxxl_error(errmsg) ;

#define stxxl_function_error ;

#define stxxl_nassert(expr) expr;

#define stxxl_ifcheck(expr) expr; if(0) {}

#define stxxl_debug(expr) ;

#endif

// returns number of seconds from ...
inline double
stxxl_timestamp ()
{
	struct timeval tp;
	gettimeofday (&tp, NULL);
	return double (tp.tv_sec) + tp.tv_usec / 1000000.;
}


inline
	std::string
stxxl_tmpfilename (std::string dir, std::string prefix)
{
	//stxxl_debug(cerr <<" TMP:"<< dir.c_str() <<":"<< prefix.c_str()<< endl);
	int rnd;
	char buffer[1024];
	std::string result;
	struct stat st;

	do
	{

		rnd = rand ();
		sprintf (buffer, "%u", rnd);
		result = dir + prefix + buffer;

	}
	while (!lstat (result.c_str (), &st));

	if (errno != ENOENT)
		stxxl_function_error
			//  char * temp_name = tempnam(dir.c_str(),prefix.c_str());
			//  if(!temp_name)
			//    stxxl_function_error
			//  std::string result(temp_name);
			//  free(temp_name);
			//  stxxl_debug(cerr << __PRETTY_FUNCTION__ << ":" <<result << endl);
			return result;
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

#define STXXL_MIN(a,b) ( ((a)<(b))?(a):(b)  )
#define STXXL_MAX(a,b) ( ((a)>(b))?(a):(b)  )
#define STXXL_L2_SIZE  (512*1024)

#define div_and_round_up(a,b) ( (a)/(b) + !(!((a)%(b))))

#define log2(x) (log(x)/log(2.))


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
#define START_COUNT_WAIT_TIME	double count_wait_begin = stxxl_timestamp();
#define END_COUNT_WAIT_TIME		stxxl::wait_time_counter+= (stxxl_timestamp() - count_wait_begin);

#define reset_io_wait_time() stxxl::wait_time_counter = 0.0;

#define io_wait_time() (stxxl::wait_time_counter)


#else
#define START_COUNT_WAIT_TIME
#define END_COUNT_WAIT_TIME
inline void reset_io_wait_time()
{
};

inline double io_wait_time()
{
	return -1.0;
};

#endif


typedef long long int int64;


/* since STD library lacks following functions in opposite to STL */	
template <class _Tp>
inline void swap(_Tp& __a, _Tp& __b)
{ 
    _Tp __tmp = __a;
    __a = __b;
    __b = __tmp;
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

__STXXL_END_NAMESPACE
#endif
