/***************************************************************************
 *  include/stxxl/bits/containers/deque.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef _STXXL_DEQUE_H
#define _STXXL_DEQUE_H

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
    DequeType * Deque;
    size_type Offset;

    deque_iterator(DequeType * Deque_, size_type Offset_) :
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

    difference_type operator - (const _Self & a) const
    {
        size_type SelfAbsOffset = (Offset >= Deque->begin_o) ?
                                  Offset : (Deque->Vector.size() + Offset);
        size_type aAbsOffset = (a.Offset >= Deque->begin_o) ?
                               a.Offset : (Deque->Vector.size() + a.Offset);

        return SelfAbsOffset - aAbsOffset;
    }

    difference_type operator - (const const_iterator & a) const
    {
        size_type SelfAbsOffset = (Offset >= Deque->begin_o) ?
                                  Offset : (Deque->Vector.size() + Offset);
        size_type aAbsOffset = (a.Offset >= Deque->begin_o) ?
                               a.Offset : (Deque->Vector.size() + a.Offset);

        return SelfAbsOffset - aAbsOffset;
    }

    _Self operator - (size_type op) const
    {
        return _Self(Deque, (Offset + Deque->Vector.size() - op) % Deque->Vector.size());
    }

    _Self operator + (size_type op) const
    {
        return _Self(Deque, (Offset + op) % Deque->Vector.size());
    }

    _Self & operator -= (size_type op)
    {
        Offset = (Offset + Deque->Vector.size() - op) % Deque->Vector.size();
        return *this;
    }

    _Self & operator += (size_type op)
    {
        Offset = (Offset + op) % Deque->Vector.size();
        return *this;
    }

    reference operator * ()
    {
        return Deque->Vector[Offset];
    }

    pointer operator -> ()
    {
        return &(Deque->Vector[Offset]);
    }

    const_reference operator * () const
    {
        return Deque->Vector[Offset];
    }

    const_pointer operator -> () const
    {
        return &(Deque->Vector[Offset]);
    }

    reference operator [] (size_type op)
    {
        return Deque->Vector[(Offset + op) % Deque->Vector.size()];
    }

    const_reference operator [] (size_type op) const
    {
        return Deque->Vector[(Offset + op) % Deque->Vector.size()];
    }

    _Self & operator ++ ()
    {
        Offset = (Offset + 1) % Deque->Vector.size();
        return *this;
    }
    _Self operator ++ (int)
    {
        _Self __tmp = *this;
        Offset = (Offset + 1) % Deque->Vector.size();
        return __tmp;
    }
    _Self & operator -- ()
    {
        Offset = (Offset + Deque->Vector.size() - 1) % Deque->Vector.size();
        return *this;
    }
    _Self operator -- (int)
    {
        _Self __tmp = *this;
        Offset = (Offset + Deque->Vector.size() - 1) % Deque->Vector.size();
        return __tmp;
    }
    bool operator == (const _Self & a) const
    {
        assert(Deque == a.Deque);
        return Offset == a.Offset;
    }
    bool operator != (const _Self & a) const
    {
        assert(Deque == a.Deque);
        return Offset != a.Offset;
    }

    bool operator < (const _Self & a) const
    {
        assert(Deque == a.Deque);
        return (a - (*this)) > 0;
    }

    bool operator > (const _Self & a) const
    {
        return a < (*this);
    }

    bool operator <= (const _Self & a) const
    {
        return !((*this) > a);
    }

    bool operator >= (const _Self & a) const
    {
        return !((*this) < a);
    }

    bool operator == (const const_iterator & a) const
    {
        assert(Deque == a.Deque);
        return Offset == a.Offset;
    }
    bool operator != (const const_iterator & a) const
    {
        assert(Deque == a.Deque);
        return Offset != a.Offset;
    }

    bool operator < (const const_iterator & a) const
    {
        assert(Deque == a.Deque);
        return (a - (*this)) > 0;
    }

    bool operator > (const const_iterator & a) const
    {
        return a < (*this);
    }

    bool operator <= (const const_iterator & a) const
    {
        return !((*this) > a);
    }

    bool operator >= (const const_iterator & a) const
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
    const DequeType * Deque;
    size_type Offset;

    const_deque_iterator(const DequeType * Deque_, size_type Offset_) :
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
    const_deque_iterator(const deque_iterator<DequeType> & it) :
        Deque(it.Deque), Offset(it.Offset)
    { }

    difference_type operator - (const _Self & a) const
    {
        size_type SelfAbsOffset = (Offset >= Deque->begin_o) ?
                                  Offset : (Deque->Vector.size() + Offset);
        size_type aAbsOffset = (a.Offset >= Deque->begin_o) ?
                               a.Offset : (Deque->Vector.size() + a.Offset);

        return SelfAbsOffset - aAbsOffset;
    }

    difference_type operator - (const iterator & a) const
    {
        size_type SelfAbsOffset = (Offset >= Deque->begin_o) ?
                                  Offset : (Deque->Vector.size() + Offset);
        size_type aAbsOffset = (a.Offset >= Deque->begin_o) ?
                               a.Offset : (Deque->Vector.size() + a.Offset);

        return SelfAbsOffset - aAbsOffset;
    }

    _Self operator - (size_type op) const
    {
        return _Self(Deque, (Offset + Deque->Vector.size() - op) % Deque->Vector.size());
    }

    _Self operator + (size_type op) const
    {
        return _Self(Deque, (Offset + op) % Deque->Vector.size());
    }

    _Self & operator -= (size_type op)
    {
        Offset = (Offset + Deque->Vector.size() - op) % Deque->Vector.size();
        return *this;
    }

    _Self & operator += (size_type op)
    {
        Offset = (Offset + op) % Deque->Vector.size();
        return *this;
    }

    const_reference operator * () const
    {
        return Deque->Vector[Offset];
    }

    const_pointer operator -> () const
    {
        return &(Deque->Vector[Offset]);
    }

    const_reference operator [] (size_type op) const
    {
        return Deque->Vector[(Offset + op) % Deque->Vector.size()];
    }

    _Self & operator ++ ()
    {
        Offset = (Offset + 1) % Deque->Vector.size();
        return *this;
    }
    _Self operator ++ (int)
    {
        _Self __tmp = *this;
        Offset = (Offset + 1) % Deque->Vector.size();
        return __tmp;
    }
    _Self & operator -- ()
    {
        Offset = (Offset + Deque->Vector.size() - 1) % Deque->Vector.size();
        return *this;
    }
    _Self operator -- (int)
    {
        _Self __tmp = *this;
        Offset = (Offset + Deque->Vector.size() - 1) % Deque->Vector.size();
        return __tmp;
    }
    bool operator == (const _Self & a) const
    {
        assert(Deque == a.Deque);
        return Offset == a.Offset;
    }
    bool operator != (const _Self & a) const
    {
        assert(Deque == a.Deque);
        return Offset != a.Offset;
    }

    bool operator < (const _Self & a) const
    {
        assert(Deque == a.Deque);
        return (a - (*this)) > 0;
    }

    bool operator > (const _Self & a) const
    {
        return a < (*this);
    }

    bool operator <= (const _Self & a) const
    {
        return !((*this) > a);
    }

    bool operator >= (const _Self & a) const
    {
        return !((*this) < a);
    }

    bool operator == (const iterator & a) const
    {
        assert(Deque == a.Deque);
        return Offset == a.Offset;
    }
    bool operator != (const iterator & a) const
    {
        assert(Deque == a.Deque);
        return Offset != a.Offset;
    }

    bool operator < (const iterator & a) const
    {
        assert(Deque == a.Deque);
        return (a - (*this)) > 0;
    }

    bool operator > (const iterator & a) const
    {
        return a < (*this);
    }

    bool operator <= (const iterator & a) const
    {
        return !((*this) > a);
    }

    bool operator >= (const iterator & a) const
    {
        return !((*this) < a);
    }
};

//! \addtogroup stlcont
//! \{

//! \brief A deque container
//!
//! It is an adaptor of the \c VectorType.
//! The implementation wraps the elements around
//! the end of the \c VectorType circularly.
//! Template parameters:
//! - \c T the element type
//! - \c VectorType the type of the underlying vector container,
//! the default is \c stxxl::vector<T>
template <class T, class VectorType = stxxl::vector<T> >
class deque : private noncopyable
{
    typedef deque<T, VectorType> Self_;

public:
    typedef typename VectorType::size_type size_type;
    typedef typename VectorType::difference_type difference_type;
    typedef VectorType vector_type;
    typedef T value_type;
    typedef T * pointer;
    typedef const value_type * const_pointer;
    typedef T & reference;
    typedef const T & const_reference;
    typedef deque_iterator<Self_> iterator;
    typedef const_deque_iterator<Self_> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    friend class deque_iterator<Self_>;
    friend class const_deque_iterator<Self_>;

private:
    VectorType Vector;
    size_type begin_o, end_o, size_;

    void double_array()
    {
        const size_type old_size = Vector.size();
        Vector.resize(2 * old_size);
        if (begin_o > end_o)
        {                         // copy data to the new end of the vector
            const size_type new_begin_o = old_size + begin_o;
            std::copy(Vector.begin() + begin_o,
                      Vector.begin() + old_size,
                      Vector.begin() + new_begin_o);
            begin_o = new_begin_o;
        }
    }

public:
    deque() : Vector((STXXL_DEFAULT_BLOCK_SIZE(T)) / sizeof(T)), begin_o(0), end_o(0), size_(0)
    { }

    deque(size_type n)
        : Vector((std::max)((size_type)(STXXL_DEFAULT_BLOCK_SIZE(T)) / sizeof(T), 2 * n)),
          begin_o(0), end_o(n), size_(n)
    { }

    ~deque()      // empty so far
    { }

    iterator begin()
    {
        return iterator(this, begin_o);
    }
    iterator end()
    {
        return iterator(this, end_o);
    }
    const_iterator begin() const
    {
        return const_iterator(this, begin_o);
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    const_iterator end() const
    {
        return const_iterator(this, end_o);
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
        return size_;
    }

    size_type max_size() const
    {
        return (std::numeric_limits<size_type>::max)() / 2 - 1;
    }

    bool empty() const
    {
        return size_ == 0;
    }

    reference operator [] (size_type n)
    {
        assert(n < size());
        return Vector[(begin_o + n) % Vector.size()];
    }

    const_reference operator [] (size_type n) const
    {
        assert(n < size());
        return Vector[(begin_o + n) % Vector.size()];
    }

    reference front()
    {
        assert(!empty());
        return Vector[begin_o];
    }

    const_reference front() const
    {
        assert(!empty());
        return Vector[begin_o];
    }

    reference back()
    {
        assert(!empty());
        return Vector[(end_o + Vector.size() - 1) % Vector.size()];
    }

    const_reference back() const
    {
        assert(!empty());
        return Vector[(end_o + Vector.size() - 1) % Vector.size()];
    }

    void push_front(const T & el)
    {
        if ((begin_o + Vector.size() - 1) % Vector.size() == end_o)
        {
            // an overflow will occur: resize the array
            double_array();
        }

        begin_o = (begin_o + Vector.size() - 1) % Vector.size();
        Vector[begin_o] = el;
        ++size_;
    }

    void push_back(const T & el)
    {
        if ((end_o + 1) % Vector.size() == begin_o)
        {
            // an overflow will occur: resize the array
            double_array();
        }
        Vector[end_o] = el;
        end_o = (end_o + 1) % Vector.size();
        ++size_;
    }

    void pop_front()
    {
        assert(!empty());
        begin_o = (begin_o + 1) % Vector.size();
        --size_;
    }

    void pop_back()
    {
        assert(!empty());
        end_o = (end_o + Vector.size() - 1) % Vector.size();
        --size_;
    }

    void swap(deque & obj)
    {
        std::swap(Vector, obj.Vector);
        std::swap(begin_o, obj.begin_o);
        std::swap(end_o, obj.end_o);
        std::swap(size_, obj.size_);
    }

    void clear()
    {
        Vector.clear();
        Vector.resize((STXXL_DEFAULT_BLOCK_SIZE(T)) / sizeof(T));
        begin_o = 0;
        end_o = 0;
        size_ = 0;
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
            if (n + 1 > Vector.size())
            {                             // need to resize
                const size_type old_size = Vector.size();
                Vector.resize(2 * n);
                if (begin_o > end_o)
                {                         // copy data to the new end of the vector
                    const size_type new_begin_o = Vector.size() - old_size + begin_o;
                    std::copy(Vector.begin() + begin_o,
                              Vector.begin() + old_size,
                              Vector.begin() + new_begin_o);
                    begin_o = new_begin_o;
                }
            }
            end_o = (end_o + n - size()) % Vector.size();
            size_ = n;
        }
    }
};

template <class T, class VectorType>
bool operator == (const deque<T, VectorType> & a, const deque<T, VectorType> & b)
{
    return std::equal(a.begin(), a.end(), b.begin());
}

template <class T, class VectorType>
bool operator < (const deque<T, VectorType> & a, const deque<T, VectorType> & b)
{
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

//! \}

__STXXL_END_NAMESPACE

namespace std
{
    template <typename T, typename VT>
    void swap(stxxl::deque<T, VT> & a,
              stxxl::deque<T, VT> & b)
    {
        a.swap(b);
    }
}

#endif /* _STXXL_DEQUE_H */
