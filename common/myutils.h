#ifndef MYUTILS_HEADER
#define MYUTILS_HEADER


/***************************************************************************
 *            myutils.h
 *
 *  Sat Aug 24 23:41:20 2002
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
#include <vector>
#include <string>

#define __MYSTRING(x) # x

#define MYMSG(x) { std::cout << x << std::endl; std::cout.flush(); };
#define MYERRMSG(x) { std::cerr << x << std::endl; std::cerr.flush(); };


inline void
myperror (char * errmsg, int errcode)
{
    std::cerr << errmsg << " error code " << errcode;
    std::cerr << " : " << strerror (errcode) << std::endl;

    exit (errcode);
}

#define myerror(errmsg) { perror(errmsg); exit(errno); }

#define myfunction_error myerror(__PRETTY_FUNCTION__)

//#define mynassert(expr) assert(!(expr))
//#define mynassert(expr) { if (expr) myfunction_error }
//#define mynassert(expr) { if(expr) perror(__PRETTY_FUNCTION__); }
#define mynassert(expr) { int ass_res = expr; if (ass_res) {  std::cerr << "Error in function: " << __PRETTY_FUNCTION__ << " ";  myperror(__MYSTRING(expr), ass_res); } }

#define myifcheck(expr) if ((expr) < 0) { std::cerr << "Error in function " << __PRETTY_FUNCTION__ << " "; myerror(__MYSTRING(expr)); }

#define mydebug(expr) expr


// returns number of seconds from ...
inline double
mytimestamp ()
{
    struct timeval tp;
    gettimeofday (&tp, NULL);
    return double (tp.tv_sec) + tp.tv_usec / 1000000.;
}


inline
std::string
mytmpfilename (std::string dir, std::string prefix)
{
    //mydebug(cerr <<" TMP:"<< dir.c_str() <<":"<< prefix.c_str()<< endl);
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
        myfunction_error
        //  char * temp_name = tempnam(dir.c_str(),prefix.c_str());
        //  if(!temp_name)
        //    myfunction_error
        //  std::string result(temp_name);
        //  free(temp_name);
        //  mydebug(cerr << __PRETTY_FUNCTION__ << ":" <<result << endl);
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

    sub.return result;
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

#define MYMIN(a, b) ( ((a) < (b)) ? (a) : (b)  )



#endif
