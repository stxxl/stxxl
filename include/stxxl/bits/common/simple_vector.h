/***************************************************************************
 *  include/stxxl/bits/common/simple_vector.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SIMPLE_VECTOR_HEADER
#define STXXL_SIMPLE_VECTOR_HEADER

#include <algorithm>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/types.h>


__STXXL_BEGIN_NAMESPACE

/*!
 * Simpler non-growing vector without initialization.
 *
 * simple_vector can be used a replacement for std::vector when only a
 * non-growing array is needed. The advantages of simple_vector are that it
 * does not initilize memory for POD types (faster), allows simpler inlines and
 * is less error prone to copying and other problems..
 */
template <class Type /*, class Alloc=__STL_DEFAULT_ALLOCATOR(Type) */>
class simple_vector : private noncopyable
{
public:
    typedef unsigned_type size_type;
    typedef Type value_type;
//  typedef simple_alloc<Type, Alloc> _data_allocator;

protected:
    //! size of allocated memory
    size_type m_size;

    //! pointer to allocated memory area
    value_type* m_array;

public:
    // *** simple pointer iterators

    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef value_type& reference;
    typedef const value_type& const_reference;

public:
    //! allocate vector's memory
    simple_vector(size_type sz)
        : m_size(sz), m_array(NULL)
    {
        // m_array = _data_allocator.allocate(sz);
        if (m_size > 0)
            m_array = new Type[m_size];
    }
    void swap(simple_vector & obj)
    {
        std::swap(m_size, obj.m_size);
        std::swap(m_array, obj.m_array);
    }
    ~simple_vector()
    {
        // _data_allocator.deallocate(m_array,m_size);
        delete[] m_array;
    }
    iterator data()
    {
        return m_array;
    }
    const_iterator data() const
    {
        return m_array;
    }
    iterator begin()
    {
        return m_array;
    }
    const_iterator begin() const
    {
        return m_array;
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    iterator end()
    {
        return m_array + m_size;
    }
    const_iterator end() const
    {
        return m_array + m_size;
    }
    const_iterator cend() const
    {
        return end();
    }
    size_type size() const
    {
        return m_size;
    }
    reference operator [] (size_type i)
    {
        return *(begin() + i);
    }
    const_reference operator [] (size_type i) const
    {
        return *(begin() + i);
    }
};
__STXXL_END_NAMESPACE

namespace std
{
    template <class Tp_>
    void swap(stxxl::simple_vector<Tp_> & a,
              stxxl::simple_vector<Tp_> & b)
    {
        a.swap(b);
    }
}

#endif // !STXXL_SIMPLE_VECTOR_HEADER
