/***************************************************************************
 *  include/stxxl/bits/msvc_compatibility.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MSVC_COMPATIBILITY_H
#define STXXL_MSVC_COMPATIBILITY_H

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef BOOST_MSVC

#include <cmath>

inline double log2(double x)
{
    return (log(x) / log(2.));
}

#endif

#endif // !STXXL_MSVC_COMPATIBILITY_H
// vim: et:ts=4:sw=4
