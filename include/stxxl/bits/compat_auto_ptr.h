/***************************************************************************
 *  include/stxxl/bits/compat_auto_ptr.h
 *
 *  compatibility interface to C++ standard extension auto_ptr
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_HEADER__COMPAT_AUTO_PTR_H_
#define STXXL_HEADER__COMPAT_AUTO_PTR_H_


#include <memory>
#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

template<class _Tp>
struct compat_auto_ptr {
#if defined(__GXX_EXPERIMENTAL_CXX0X__)
    typedef std::shared_ptr<_Tp> result;
#else
    typedef std::auto_ptr<_Tp> result;
#endif
};

__STXXL_END_NAMESPACE

#endif // STXXL_HEADER__COMPAT_AUTO_PTR_H_
