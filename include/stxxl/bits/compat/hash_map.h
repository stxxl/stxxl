/***************************************************************************
 *  include/stxxl/bits/compat/hash_map.h
 *
 *  compatibility interface to C++ standard extension hash_map
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008, 2010, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009, 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMPAT_HASH_MAP_HEADER
#define STXXL_COMPAT_HASH_MAP_HEADER

#include <stxxl/bits/config.h>
#include <stxxl/bits/namespace.h>

#if __cplusplus >= 201103L
 #include <unordered_map>
#elif STXXL_MSVC
 #include <hash_map>
#elif defined(__GNUG__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40200) && \
    (!defined(__ICC) || (__ICC > 1110))
 #include <tr1/unordered_map>
#else
 #include <ext/hash_map>
#endif

STXXL_BEGIN_NAMESPACE

template <class _Tp>
struct compat_hash {
#if __cplusplus >= 201103L
    typedef std::hash<_Tp> result;
#elif STXXL_MSVC
    typedef stdext::hash_compare<_Tp> result;
#elif defined(__GNUG__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40200) && \
    (!defined(__ICC) || (__ICC > 1110))
    typedef std::tr1::hash<_Tp> result;
#else
    typedef __gnu_cxx::hash<_Tp> result;
#endif
};

template <class _Key, class _Tp, class _Hash = typename compat_hash<_Key>::result>
struct compat_hash_map {
#if __cplusplus >= 201103L
    typedef std::unordered_map<_Key, _Tp, _Hash> result;
#elif STXXL_MSVC
    typedef stdext::hash_map<_Key, _Tp, _Hash> result;
#elif defined(__GNUG__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40200) && \
    (!defined(__ICC) || (__ICC > 1110))
    typedef std::tr1::unordered_map<_Key, _Tp, _Hash> result;
#else
    typedef __gnu_cxx::hash_map<_Key, _Tp, _Hash> result;
#endif
};

STXXL_END_NAMESPACE

#endif // !STXXL_COMPAT_HASH_MAP_HEADER
