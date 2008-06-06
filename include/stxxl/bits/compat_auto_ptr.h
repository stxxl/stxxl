#ifndef STXXL_HEADER__COMPAT_AUTO_PTR_H_
#define STXXL_HEADER__COMPAT_AUTO_PTR_H_


#include <memory>
#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

template<class _Tp>
struct compat_auto_ptr {
#if defined(STXXL_CXX0X) && defined(__GNUG__) && ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40300)
    typedef std::shared_ptr<_Tp> result;
#else
    typedef std::auto_ptr<_Tp> result;
#endif
};

__STXXL_END_NAMESPACE

#endif // STXXL_HEADER__COMPAT_AUTO_PTR_H_

