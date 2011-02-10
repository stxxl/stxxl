/***************************************************************************
 *  include/stxxl/bits/libstxxl.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_LIBSTXXL_H
#define STXXL_LIBSTXXL_H

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef BOOST_MSVC
 #ifndef STXXL_LIBNAME
  #define STXXL_LIBNAME "stxxl"
 #endif
 #pragma comment (lib, "lib" STXXL_LIBNAME ".lib")
#endif

#endif // !STXXL_IO_HEADER
