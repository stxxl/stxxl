#ifndef STXXL_HEADER__COMPAT_HASH_SET_H_
#define STXXL_HEADER__COMPAT_HASH_SET_H_


#if defined(__GXX_EXPERIMENTAL_CXX0X__)
 #include <unordered_set>
#elif defined(BOOST_MSVC)
 #include <hash_set>
#else
 #include <ext/hash_set>
#endif

#include <stxxl/bits/namespace.h>

__STXXL_BEGIN_NAMESPACE

template<class _Key>
struct compat_hash_set {
#if defined(__GXX_EXPERIMENTAL_CXX0X__)
    typedef std::unordered_set<_Key> result;
#elif defined(BOOST_MSVC)
    typedef stdext::hash_set<_Key> result;
#else
    typedef __gnu_cxx::hash_set<_Key> result;
#endif
};

__STXXL_END_NAMESPACE

#endif // STXXL_HEADER__COMPAT_HASH_SET_H_

