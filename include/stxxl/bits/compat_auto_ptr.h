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

template <class _Tp>
struct compat_auto_ptr {
#if defined(__GXX_EXPERIMENTAL_CXX0X__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40400)
    typedef std::unique_ptr<_Tp> result;
#else
    typedef std::auto_ptr<_Tp> result;
#endif
};

__STXXL_END_NAMESPACE

#if defined(__GNUG__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) == 30400)

namespace workaround_gcc_3_4 {

    // std::swap in gcc 3.4 is broken, __tmp is declared const there
    template<typename _Tp>
    inline void
    swap(_Tp& __a, _Tp& __b)
    {
      // concept requirements
      __glibcxx_function_requires(_SGIAssignableConcept<_Tp>)

      _Tp __tmp = __a;
      __a = __b;
      __b = __tmp;
    }

}

namespace std {

    // overload broken std::swap<auto_ptr> to call a working swap()
    template<typename _Tp>
    inline void swap(std::auto_ptr<_Tp>& a, std::auto_ptr<_Tp>& b)
    {
        workaround_gcc_3_4::swap(a, b);
    }
}

#endif

#endif // !STXXL_HEADER__COMPAT_AUTO_PTR_H_
