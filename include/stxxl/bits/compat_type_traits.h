/***************************************************************************
 *  include/stxxl/bits/compat_type_traits.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_HEADER__COMPAT_TYPE_TRAITS_H_
#define STXXL_HEADER__COMPAT_TYPE_TRAITS_H_

#if defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <type_traits>
#elif defined(__GNUG__) && (__GNUC__ >= 4) && !defined(__ICC)
#include <tr1/type_traits>
#elif defined(STXXL_BOOST_CONFIG)
#include <boost/type_traits/remove_const.hpp>
#endif

#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

#if defined(__GXX_EXPERIMENTAL_CXX0X__)
using std::remove_const;
#elif defined(__GNUG__) && (__GNUC__ >= 4) && !defined(__ICC)
using std::tr1::remove_const;
#elif defined(STXXL_BOOST_CONFIG)
using boost::remove_const;
#else
template <typename _Tp>
struct remove_const
{
    typedef _Tp type;
};

template <typename _Tp>
struct remove_const<_Tp const>
{
    typedef _Tp type;
};
#endif

#if defined(__GNUG__) && (__GNUC__ == 4) && (__GNUC_MINOR__ < 3)
  // That's a small subset of what GCC 4.3 does.

  // Utility for finding the signed versions of unsigned integral types.
  template<typename _Tp>
    struct __make_signed
    { typedef _Tp __type; };

  template<>
    struct __make_signed<char>
    { typedef signed char __type; };

  template<>
    struct __make_signed<unsigned char>
    { typedef signed char __type; };

  template<>
    struct __make_signed<unsigned short>
    { typedef signed short __type; };

  template<>
    struct __make_signed<unsigned int>
    { typedef signed int __type; };

  template<>
    struct __make_signed<unsigned long>
    { typedef signed long __type; };

  template<>
    struct __make_signed<unsigned long long>
    { typedef signed long long __type; };


  template<typename _Tp>
    struct make_signed 
    { typedef typename __make_signed<_Tp>::__type type; };
#endif

__STXXL_END_NAMESPACE

#endif // !STXXL_HEADER__COMPAT_TYPE_TRAITS_H_
