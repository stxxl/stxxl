/***************************************************************************
 *  include/stxxl/bits/compat/shared_ptr.h
 *
 *  compatibility interface to shared_ptr (C++0x, TR1 or boost)
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_HEADER__COMPAT__SHARED_PTR_H_
#define STXXL_HEADER__COMPAT__SHARED_PTR_H_


#include <memory>
#if defined(__GNUG__) && !defined(__GXX_EXPERIMENTAL_CXX0X__) \
    && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40200) \
    && (!defined(__ICC) || (__ICC >= 1100))
#include <tr1/memory>
#endif
#if defined(STXXL_BOOST_CONFIG)
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#endif

#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

#ifndef STXXL_HAVE_SHARED_PTR
    #define STXXL_HAVE_SHARED_PTR 1
#endif

#ifndef STXXL_HAVE_MAKE_SHARED
    #define STXXL_HAVE_MAKE_SHARED 1
#endif

namespace compat
{
#if defined(__GXX_EXPERIMENTAL_CXX0X__)
    using std::shared_ptr;
    using std::make_shared;
    using std::allocate_shared;
#elif defined(STXXL_BOOST_CONFIG)
    using boost::shared_ptr;
    using boost::make_shared;
    using boost::allocate_shared;
#elif defined(__GNUG__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40200) \
    && (!defined(__ICC) || (__ICC >= 1100))
    using std::tr1::shared_ptr;
    #undef STXXL_HAVE_MAKE_SHARED
    #define STXXL_HAVE_MAKE_SHARED 0
#else
    // no shared_ptr implementation available
    #undef STXXL_HAVE_SHARED_PTR
    #define STXXL_HAVE_SHARED_PTR 0
    #undef STXXL_HAVE_MAKE_SHARED
    #define STXXL_HAVE_MAKE_SHARED 0
#endif
}  // namespace compat

__STXXL_END_NAMESPACE

#endif // !STXXL_HEADER__COMPAT__SHARED_PTR_H_
// vim: et:ts=4:sw=4
