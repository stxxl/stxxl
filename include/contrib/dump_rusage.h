/***************************************************************************
 *  contrib/dump_rusage.h
 *
 *  Print the rusage values in a simple call:
 *      std::cout << dump_rusage() << std::endl;
 *
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <sys/time.h>
#include <sys/resource.h>

#include <ostream>


std::ostream & operator << (std::ostream & o, const struct rusage & ru)
{
    o << "utime=" << (double(ru.ru_utime.tv_sec) + double(ru.ru_utime.tv_usec) / 1000000.)
      << " stime=" << (double(ru.ru_stime.tv_sec) + double(ru.ru_stime.tv_usec) / 1000000.)
      << " maxrss=" << ru.ru_maxrss
      << " minflt=" << ru.ru_minflt
      << " majflt=" << ru.ru_majflt
      << " inblock=" << ru.ru_inblock
      << " oublock=" << ru.ru_oublock
      << " nvcsw=" << ru.ru_nvcsw
      << " nivcsw=" << ru.ru_nivcsw;
    return o;
}

struct _DumpRusage
{ };

inline _DumpRusage dump_rusage()
{
    return _DumpRusage();
}

std::ostream & operator << (std::ostream & o, const _DumpRusage &)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    o << ru;
    return o;
}
