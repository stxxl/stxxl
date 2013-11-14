/***************************************************************************
 *  include/stxxl/bits/containers/deque.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_DEQUE_HEADER
#define STXXL_CONTAINERS_DEQUE_HEADER

#include <limits>
#include <stxxl/vector>


__STXXL_BEGIN_NAMESPACE

template <class T, class VectorType>
class deque;

template <class DequeType>
class const_deque_iterator;

template <class DequeType>
class deque_iterator
{
    typedef typename DequeType::size_type size_type;
    typedef typename DequeType::vector_type vector_type;
    typedef deque_iterator<DequeType> _Self;
    DequeType* Deque;
    size_type Offset;

    deque_iterator(DequeType* Deque_, size_type Offset_) :
        Deque(Deque_), Offset(Offset_)
    { }

    friend class const_deque_iterator<DequeType>;

public:
    typedef typename DequeType::value_type value_type;
    typedef typename DequeType::pointer pointer;
    typedef typename DequeType::const_pointer const_pointer;
    typedef typename DequeType::reference reference;
    typedef typename DequeType::const_reference const_reference;
    typedef deque_iterator<DequeType> iterator;
    typedef const_deque_iterator<DequeType> const_iterator;
    friend class deque<value_type, vector_type>;
    typedef std::random_access_iterator_tag iterator_category;
    typedef typename DequeType::difference_type difference_type;

    deque_iterator() : Deque(NULL), Offset(0) { }

    difference_type operator - (const _Self& a) const
    {
        size_type SelfAbsOffset = (Offset >= Deque->m_begin) ?
                                  Offset : (Deque->m_vector.size() + Offset);
        size_type aAbsOffset = (a.Offset >= Deque->m_begin) ?
                               a.Offset : (Deque->m_vector.size() + a.Offset);

        return SelfAbsOffset - aAbsOffset;
    }

    difference_type operator - (const const_iterator& a) const
    {
        size_type SelfAbsOffset = (Offset >= Deque->m_begin) ?
                                  Offset : (Deque->m_vector.size() + Offset);
        size_type aAbsOffset = (a.Offset >= Deque->m_begin) ?
                               a.Offset : (Deque->m_vector.size() + a.Offset);

        return SelfAbsOffset - aAbsOffset;
    }

    _Self operator - (size_type op) const
    {
        return _Self(Deque, (Offset + Deque->m_vector.size() - op) % Deque->m_vector.size());
    }

    _Self operator + (size_type op) const
    {
        return _Self(Deque, (Offset + op) % Deque->m_vector.size());
    }

    _Self& operator -= (size_type op)
    {
        Offset = (Offset + Deque->m_vector.size() - op) % Deque->m_vector.size();
        return *this;
    }

    _Self& operator += (size_type op)
    {
        Offset = (Offset + op) % Deque->m_vector.size();
        return *this;
    }

    reference operator * ()
    {
        return Deque->m_vector[Offset];
    }

    pointer operator -> ()
    {
        return &(Deque->m_vector[Offset]);
    }

    const_reference operator * () const
    {
        return Deque->m_vector[Offset];
    }

    const_pointer operator -> () const
    {
        return &(Deque->m_vector[Offset]);
    }

    reference operator [] (size_type op)
    {
        return Deque->m_vector[(Offset + op) % Deque->m_vector.size()];
    }

    const_reference operator [] (size_type op) const
    {
        return Deque->m_vector[(Offset + op) % Deque->m_vector.size()];
    }

    _Self& operator ++ ()
    {
        Offset = (Offset + 1) % Deque->m_vector.size();
        return *this;
    }
    _Self operator ++ (int)
    {
        _Self __tmp = *this;
        Offset = (Offset + 1) % Deque->m_vector.size();
        return __tmp;
    }
    _Self& operator -- ()
    {
        Offset = (Offset + Deque->m_vector.size() - 1) % Deque->m_vector.size();
        return *this;
    }
    _Self operator -- (int)
    {
        _Self __tmp = *this;
        Offset = (Offset + Deque->m_vector.size() - 1) % Deque->m_vector.size();
        return __tmp;
    }
    bool operator == (const _Self& a) const
    {
        assert(Deque == a.Deque);
        return Offset == a.Offset;
    }
    bool operator != (const _Self& a) const
    {
        assert(Deque == a.Deque);
        return Offset != a.Offset;
    }

    bool operator < (const _Self& a) const
    {
        assert(Deque == a.Deque);
        return (a - (*this)) > 0;
    }

    bool operator > (const _Self& a) const
    {
        return a < (*this);
    }

    bool operator <= (const _Self& a) const
    {
        return !((*this) > a);
    }

    bool operator >= (const _Self& a) const
    {
        return !((*this) < a);
    }

    bool operator == (const const_iterator& a) const
    {
        assert(Deque == a.Deque);
        return Offset == a.Offset;
    }
    bool operator != (const const_iterator& a) const
    {
        assert(Deque == a.Deque);
        return Offset != a.Offset;
    }

    bool operator < (const const_iterator& a) const
    {
        assert(Deque == a.Deque);
        return (a - (*this)) > 0;
    }

    bool operator > (const const_iterator& a) const
    {
        return a < (*this);
    }

    bool operator <= (const const_iterator& a) const
    {
        return !((*this) > a);
    }

    bool operator >= (const const_iterator& a) const
    {
        return !((*this) < a);
    }
};

template <class DequeType>
class const_deque_iterator
{
    typedef const_deque_iterator<DequeType> _Self;
    typedef typename DequeType::size_type size_type;
    typedef typename DequeType::vector_type vector_type;
    const DequeType* Deque;
    size_type Offset;

    const_deque_iterator(const DequeType* Deque_, size_type Offset_) :
        Deque(Deque_), Offset(Offset_)
    { }

public:
    typedef typename DequeType::value_type value_type;
    typedef typename DequeType::const_pointer pointer;
    typedef typename DequeType::const_pointer const_pointer;
    typedef typename DequeType::const_reference reference;
    typedef typename DequeType::const_reference const_reference;
    typedef deque_iterator<DequeType> iterator;
    typedef const_deque_iterator<DequeType> const_iterator;
    friend class deque<value_type, vector_type>;
    friend class deque_iterator<DequeType>;

    typedef std::random_access_iterator_tag iterator_category;
    typedef typename DequeType::difference_type difference_type;

    const_deque_iterator() : Deque(NULL), Offset(0) { }
    const_deque_iterator(const deque_iterator<DequeType>& it) :
        Deque(it.Deque), Offset(it.Offset)
    { }

    difference_type operator - (const _Self& a) const
    {
        size_type SelfAbsOffset = (Offset >= Deque->m_begin) ?
                                  Offset : (Deque->m_vector.size() + Offset);
        size_type aAbsOffset = (a.Offset >= Deque->m_begin) ?
                               a.Offset : (Deque->m_vector.size() + a.Offset);

        return SelfAbsOffset - aAbsOffset;
    }

    difference_type operator - (const iterator& a) const
    {
        size_type SelfAbsOffset = (Offset >= Deque->m_begin) ?
                                  Offset : (Deque->m_vector.size() + Offset);
        size_type aAbsOffset = (a.Offset >= Deque->m_begin) ?
                               a.Offset : (Deque->m_vector.size() + a.Offset);

        return SelfAbsOffset - aAbsOffset;
    }

    _Self operator - (size_type op) const
    {
        return _Self(Deque, (Offset + Deque->m_vector.size() - op) % Deque->m_vector.size());
    }

    _Self operator + (size_type op) const
    {
        return _Self(Deque, (Offset + op) % Deque->m_vector.size());
    }

    _Self& operator -= (size_type op)
    {
        Offset = (Offset + Deque->m_vector.size() - op) % Deque->m_vector.size();
        return *this;
    }

    _Self& operator += (size_type op)
    {
        Offset = (Offset + op) % Deque->m_vector.size();
        return *this;
    }

    const_reference operator * () const
    {
        return Deque->m_vector[Offset];
    }

    const_pointer operator -> () const
    {
        return &(Deque->m_vector[Offset]);
    }

    const_reference operator [] (size_type op) const
    {
        return Deque->m_vector[(Offset + op) % Deque->m_vector.size()];
    }

    _Self& operator ++ ()
    {
        Offset = (Offset + 1) % Deque->m_vector.size();
        return *this;
    }
    _Self operator ++ (int)
    {
        _Self __tmp = *this;
        Offset = (Offset + 1) % Deque->m_vector.size();
        return __tmp;
    }
    _Self& operator -- ()
    {
        Offset = (Offset + Deque->m_vector.size() - 1) % Deque->m_vector.size();
        return *this;
    }
    _Self operator -- (int)
    {
        _Self __tmp = *this;
        Offset = (Offset + Deque->m_vector.size() - 1) % Deque->m_vector.size();
        return __tmp;
    }
    bool operator == (const _Self& a) const
    {
        assert(Deque == a.Deque);
        return Offset == a.Offset;
    }
    bool operator != (const _Self& a) const
    {
        assert(Deque == a.Deque);
        return Offset != a.Offset;
    }

    bool operator < (const _Self& a) const
    {
        assert(Deque == a.Deque);
        return (a - (*this)) > 0;
    }

    bool operator > (const _Self& a) const
    {
        return a < (*this);
    }

    bool operator <= (const _Self& a) const
    {
        return !((*this) > a);
    }

    bool operator >= (const _Self& a) const
    {
        return !((*this) < a);
    }

    bool operator == (const iterator& a) const
    {
        assert(Deque == a.Deque);
        return Offset == a.Offset;
    }
    bool operator != (const iterator& a) const
    {
        assert(Deque == a.Deque);
        return Offset != a.Offset;
    }

    bool operator < (const iterator& a) const
    {
        assert(Deque == a.Deque);
        return (a - (*this)) > 0;
    }

    bool operator > (const iterator& a) const
    {
        return a < (*this);
    }

    bool operator <= (const iterator& a) const
    {
        return !((*this) > a);
    }

    bool operator >= (const iterator& a) const
    {
        return !((*this) < a);
    }
};

//! \addtogroup stlcont
//! \{

//! A deque container. \n
//! <b> Introduction </b> to deque container: see \ref tutorial_deque tutorial. \n
//! <b> Design and Internals </b> of deque container: see \ref design_deque
//!
//! It is an adaptor of the \c VectorType.
//! The implementation wraps the elements around
//! the end of the \c VectorType circularly.
//! \tparam ValueType type of the contained objects (POD with no references to internal memory)
//! \tparam VectorType the type of the underlying vector container,
//! the default is \c stxxl::vector<ValueType>
template <class ValueType, class VectorType = stxxl::vector<ValueType> >
class deque : private noncopyable
{
    typedef deque<ValueType, VectorType> Self_;

public:
    typedef typename VectorType::size_type size_type;
    typedef typename VectorType::difference_type difference_type;
    typedef VectorType vector_type;
    typedef ValueType value_type;
    typedef ValueType* pointer;
    typedef const value_type* const_pointer;
    typedef ValueType& reference;
    typedef const ValueType& const_reference;
    typedef deque_iterator<Self_> iterator;
    typedef const_deque_iterator<Self_> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    friend class deque_iterator<Self_>;
    friend class const_deque_iterator<Self_>;

private:
    vector_type m_vector;
    size_type m_begin, m_end, m_size;

    void double_array()
    {
        const size_type old_size = m_vector.size();
        m_vector.resize(2 * old_size);
        if (m_begin > m_end)
        {                         // copy data to the new end of the vector
            const size_type new_begin = old_size + m_begin;
            std::copy(m_vector.begin() + m_begin,
                      m_vector.begin() + old_size,
                      m_vector.begin() + new_begin);
            m_begin = new_begin;
        }
    }

public:
    deque()
        : m_vector((STXXL_DEFAULT_BLOCK_SIZE(T)) / sizeof(value_type)),
          m_begin(0), m_end(0), m_size(0)
    { }

    deque(size_type n)
        : m_vector(STXXL_MAX<size_type>(STXXL_DEFAULT_BLOCK_SIZE(ValueType) / sizeof(value_type), 2 * n)),
          m_begin(0), m_end(n), m_size(n)
    { }

    ~deque()      // empty so far
    { }

    iterator begin()
    {
        return iterator(this, m_begin);
    }
    iterator end()
    {
        return iterator(this, m_end);
    }
    const_iterator begin() const
    {
        return const_iterator(this, m_begin);
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    const_iterator end() const
    {
        return const_iterator(this, m_end);
    }
    const_iterator cend() const
    {
        return end();
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(end());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(begin());
    }

    size_type size() const
    {
        return m_size;
    }

    size_type max_size() const
    {
        return (std::numeric_limits<size_type>::max)() / 2 - 1;
    }

    bool empty() const
    {
        return m_size == 0;
    }

    reference operator [] (size_type n)
    {
        assert(n < size());
        return m_vector[(m_begin + n) % m_vector.size()];
    }

    const_reference operator [] (size_type n) const
    {
        assert(n < size());
        return m_vector[(m_begin + n) % m_vector.size()];
    }

    reference front()
    {
        assert(!empty());
        return m_vector[m_begin];
    }

    const_reference front() const
    {
        assert(!empty());
        return m_vector[m_begin];
    }

    reference back()
    {
        assert(!empty());
        return m_vector[(m_end + m_vector.size() - 1) % m_vector.size()];
    }

    const_reference back() const
    {
        assert(!empty());
        return m_vector[(m_end + m_vector.size() - 1) % m_vector.size()];
    }

    void push_front(const value_type& el)
    {
        if ((m_begin + m_vector.size() - 1) % m_vector.size() == m_end)
        {
            // an overflow will occur: resize the array
            double_array();
        }

        m_begin = (m_begin + m_vector.size() - 1) % m_vector.size();
        m_vector[m_begin] = el;
        ++m_size;
    }

    void push_back(const value_type& el)
    {
        if ((m_end + 1) % m_vector.size() == m_begin)
        {
            // an overflow will occur: resize the array
            double_array();
        }
        m_vector[m_end] = el;
        m_end = (m_end + 1) % m_vector.size();
        ++m_size;
    }

    void pop_front()
    {
        assert(!empty());
        m_begin = (m_begin + 1) % m_vector.size();
        --m_size;
    }

    void pop_back()
    {
        assert(!empty());
        m_end = (m_end + m_vector.size() - 1) % m_vector.size();
        --m_size;
    }

    void swap(deque& obj)
    {
        std::swap(m_vector, obj.m_vector);
        std::swap(m_begin, obj.m_begin);
        std::swap(m_end, obj.m_end);
        std::swap(m_size, obj.m_size);
    }

    void clear()
    {
        m_vector.clear();
        m_vector.resize((STXXL_DEFAULT_BLOCK_SIZE(T)) / sizeof(value_type));
        m_begin = 0;
        m_end = 0;
        m_size = 0;
    }

    void resize(size_type n)
    {
        if (n < size())
        {
            do
            {
                pop_back();
            } while (n < size());
        }
        else
        {
            if (n + 1 > m_vector.size())
            {                             // need to resize
                const size_type old_size = m_vector.size();
                m_vector.resize(2 * n);
                if (m_begin > m_end)
                {                         // copy data to the new end of the vector
                    const size_type new_begin = m_vector.size() - old_size + m_begin;
                    std::copy(m_vector.begin() + m_begin,
                              m_vector.begin() + old_size,
                              m_vector.begin() + new_begin);
                    m_begin = new_begin;
                }
            }
            m_end = (m_end + n - size()) % m_vector.size();
            m_size = n;
        }
    }
};

template <class ValueType, class VectorType>
bool operator == (const deque<ValueType, VectorType>& a, const deque<ValueType, VectorType>& b)
{
    return std::equal(a.begin(), a.end(), b.begin());
}

template <class ValueType, class VectorType>
bool operator < (const deque<ValueType, VectorType>& a, const deque<ValueType, VectorType>& b)
{
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

//! \}

__STXXL_END_NAMESPACE

namespace std {

template <typename ValueType, typename VectorType>
void swap(stxxl::deque<ValueType, VectorType>& a,
          stxxl::deque<ValueType, VectorType>& b)
{
    a.swap(b);
}

} // namespace std

#endif // !STXXL_CONTAINERS_DEQUE_HEADER
