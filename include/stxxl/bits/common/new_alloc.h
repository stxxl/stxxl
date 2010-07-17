/***************************************************************************
 *  include/stxxl/bits/common/new_alloc.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2006 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_NEW_ALLOC_HEADER
#define STXXL_NEW_ALLOC_HEADER

#include <memory>
#include <limits>
#include <stxxl/bits/namespace.h>


__STXXL_BEGIN_NAMESPACE

template <class T>
class new_alloc;

template <typename T, typename U>
struct new_alloc_rebind;

template <typename T>
struct new_alloc_rebind<T, T>{
    typedef new_alloc<T> other;
};

template <typename T, typename U>
struct new_alloc_rebind {
    typedef std::allocator<U> other;
};


// designed for typed_block (to use with std::vector )
template <class T>
class new_alloc {
public:
    // type definitions
    typedef T value_type;
    typedef T * pointer;
    typedef const T * const_pointer;
    typedef T & reference;
    typedef const T & const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // rebind allocator to type U, use new_alloc only if U == T
    template <class U>
    struct rebind {
        typedef typename new_alloc_rebind<T, U>::other other;
    };

    // return address of values
    pointer address(reference value) const
    {
        return &value;
    }
    const_pointer address(const_reference value) const
    {
        return &value;
    }

    new_alloc() throw () { }
    new_alloc(const new_alloc &) throw () { }
    template <class U>
    new_alloc(const new_alloc<U> &) throw () { }
    ~new_alloc() throw () { }

    template <class U>
    operator std::allocator<U>()
    {
        static std::allocator<U> helper_allocator;
        return helper_allocator;
    }

    // return maximum number of elements that can be allocated
    size_type max_size() const throw ()
    {
        return (std::numeric_limits<size_type>::max)() / sizeof(T);
    }

    // allocate but don't initialize num elements of type T
    pointer allocate(size_type num, const void * = 0)
    {
        return static_cast<T *>(T::operator new (num * sizeof(T)));
    }

    // _GLIBCXX_RESOLVE_LIB_DEFECTS
    // 402. wrong new expression in [some_] allocator::construct
    // initialize elements of allocated storage p with value value
    void construct(pointer p, const T & value)
    {
        // initialize memory with placement new
        ::new ((void *)p)T(value);
    }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    template <typename... Args>
    void construct(pointer p, Args &&... args)
    {
        ::new ((void *)p)T(std::forward<Args>(args)...);
    }
#endif

    // destroy elements of initialized storage p
    void destroy(pointer p)
    {
        // destroy objects by calling their destructor
        p->~T();
    }

    // deallocate storage p of deleted elements
    void deallocate(pointer p, size_type /*num*/)
    {
        T::operator delete (p);
    }
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
inline bool operator == (const new_alloc<T1> &,
                         const new_alloc<T2> &) throw ()
{
    return true;
}

template <class T1, class T2>
inline bool operator != (const new_alloc<T1> &,
                         const new_alloc<T2> &) throw ()
{
    return false;
}

__STXXL_END_NAMESPACE

#endif // !STXXL_NEW_ALLOC_HEADER
// vim: et:ts=4:sw=4
