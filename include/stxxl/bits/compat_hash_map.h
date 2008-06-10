#ifndef STXXL_HEADER__COMPAT_HASH_MAP_H_
#define STXXL_HEADER__COMPAT_HASH_MAP_H_


#if defined(__GXX_EXPERIMENTAL_CXX0X__)
 #include <unordered_map>
#elif defined(BOOST_MSVC)
 #include <hash_map>
#else
 #include <ext/hash_map>
#endif

#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

template<class _Tp>
struct compat_hash {
#if defined(__GXX_EXPERIMENTAL_CXX0X__)
    typedef std::hash<_Tp> result;
#elif defined(BOOST_MSVC)
    typedef stdext::hash_compare<_Tp> result;
#else
    typedef __gnu_cxx::hash<_Tp> result;
#endif
};

template<class _Key, class _Tp, class _Hash = typename compat_hash<_Key>::result >
struct compat_hash_map {
#if defined(__GXX_EXPERIMENTAL_CXX0X__)
    typedef std::unordered_map<_Key, _Tp, _Hash> result;
#elif defined(BOOST_MSVC)
    typedef stdext::hash_map<_Key, _Tp, _Hash> result;
#else
    typedef __gnu_cxx::hash_map<_Key, _Tp, _Hash> result;
#endif
};

__STXXL_END_NAMESPACE

#endif // STXXL_HEADER__COMPAT_HASH_MAP_H_

