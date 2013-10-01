/***************************************************************************
 *  include/stxxl/bits/common/types.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_TYPES_HEADER
#define STXXL_TYPES_HEADER

#include <stxxl/bits/config.h>
#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE


#ifdef STXXL_BOOST_CONFIG
 #ifdef BOOST_MSVC
typedef __int32 int32;
typedef unsigned __int32 uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;
 #else
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;
 #endif
#else
typedef int int32;
typedef unsigned int uint32;
typedef long long int int64;
typedef unsigned long long int uint64;
#endif


// integer types declarations
enum { my_pointer_size = sizeof(void *) };

template <int PtrSize>
struct choose_int_types
{ };

template <>
struct choose_int_types<4>  // for 32-bit processors/compilers
{
    typedef int int_type;
    typedef unsigned unsigned_type;
};

template <>
struct choose_int_types<8>  // for 64-bit processors/compilers
{
    typedef int64 int_type;
    typedef uint64 unsigned_type;
};

typedef choose_int_types<my_pointer_size>::int_type int_type;
typedef choose_int_types<my_pointer_size>::unsigned_type unsigned_type;

typedef unsigned_type internal_size_type;  // fits in internal memory
typedef uint64 external_size_type;         // may require external memory

__STXXL_END_NAMESPACE

#endif // !STXXL_TYPES_HEADER
