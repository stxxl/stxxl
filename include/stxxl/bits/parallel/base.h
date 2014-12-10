/***************************************************************************
 *  include/stxxl/bits/parallel/base.h
 *
 *  Sequential helper functions.
 *  Extracted from MCSTL - http://algo2.iti.uni-karlsruhe.de/singler/mcstl/
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_PARALLEL_BASE_HEADER
#define STXXL_PARALLEL_BASE_HEADER

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/parallel/features.h>
#include <stxxl/bits/parallel/types.h>

#include <functional>
#include <iterator>

STXXL_BEGIN_NAMESPACE

namespace parallel {

/*!
 * Calculates the rounded-down logrithm of \c n for base 2.
 *
 * \param n Argument.
 * \return Returns 0 for argument 0.
 */
template <typename Size>
inline Size log2(Size n)
{
    Size k;
    for (k = 0; n != 1; n >>= 1)
        ++k;
    return k;
}

/** Same functionality as std::partial_sum(). */
template <typename InputIterator, typename OutputIterator>
OutputIterator partial_sum(InputIterator begin, InputIterator end,
                           OutputIterator result)
{
    typedef typename std::iterator_traits<OutputIterator>::value_type ValueType;

    if (begin == end)
        return result;
    ValueType value = (*begin);
    *result = value;
    while (++begin != end)
    {
        value = value + (*begin);
        *++result = value;
    }
    return ++result;
}

/** Same functionality as std::partial_sum(). */
template <typename InputIterator, typename OutputIterator,
          typename BinaryOperation>
OutputIterator partial_sum(InputIterator begin, InputIterator end,
                           OutputIterator result, BinaryOperation binary_op)
{
    typedef typename std::iterator_traits<InputIterator>::value_type ValueType;

    if (begin == end)
        return result;
    ValueType value = *begin;
    *result = value;
    while (++begin != end)
    {
        value = binary_op(value, *begin);
        *++result = value;
    }
    return ++result;
}

/*!
 * Alternative to std::not2, typedefs first_argument_type and
 * second_argument_type not needed.
 */
template <class Predicate, typename first_argument_type, typename second_argument_type>
class binary_negate /*: public std::binary_function<first_argument_type, second_argument_type, bool>*/
{
protected:
    Predicate pred;

public:
    explicit
    binary_negate(const Predicate& _pred) : pred(_pred) { }

    bool operator () (const first_argument_type& x,
                      const second_argument_type& y) const
    {
        return !pred(x, y);
    }
};

/*!
 * Encode two integers into one mcstl::lcas_t.
 *
 * \param a First integer, to be encoded in the most-significant \c lcas_t_bits/2 bits.
 * \param b Second integer, to be encoded in the least-significant \c lcas_t_bits/2 bits.
 * \return mcstl::lcas_t value encoding \c a and \c b.
 * \see decode2
 */
static inline lcas_t encode2(int a, int b) // must all be non-negative, actually
{
    return (((lcas_t)a) << (lcas_t_bits / 2)) | (((lcas_t)b) << 0);
}

/*!
 * Decode two integers from one mcstl::lcas_t.
 *
 * \param x mcstl::lcas_t to decode integers from.
 * \param a First integer, to be decoded from the most-significant \c lcas_t_bits/2 bits of \c x.
 * \param b Second integer, to be encoded in the least-significant \c lcas_t_bits/2 bits of \c x.
 * \see encode2
 */
static inline void decode2(lcas_t x, int& a, int& b)
{
    a = (int)((x >> (lcas_t_bits / 2)) & lcas_t_mask);
    b = (int)((x >> 0) & lcas_t_mask);
}

/*!
 * Constructs predicate for equality from strict weak ordering predicate
 */
template <class Comparator, typename T1, typename T2>
class equal_from_less : public std::binary_function<T1, T2, bool>
{
private:
    Comparator& comp;

public:
    equal_from_less(Comparator& _comp) : comp(_comp) { }

    bool operator () (const T1& a, const T2& b)
    {
        //FIXME: wrong in general (T1 != T2)
        return !comp(a, b) && !comp(b, a);
    }
};

template <typename T, typename DiffType>
class pseudo_sequence;

/*!
 * Iterator associated with mcstl::pseudo_sequence.  If features the usual
 * random-access iterator functionality.
 *
 * \param T Sequence value type.
 * \param DiffType Sequence difference type.
 */
template <typename T, typename DiffType>
class pseudo_sequence_iterator
{
private:
    const T& val;
    DiffType pos;

    typedef pseudo_sequence_iterator<T, DiffType> type;

public:
    pseudo_sequence_iterator(const T& val, DiffType pos) : val(val), pos(pos)
    { }

    //pre-increment operator
    type&
    operator ++ ()
    {
        ++pos;
        return *this;
    }

    //post-increment operator
    const type
    operator ++ (int)
    {
        return type(pos++);
    }

    const T& operator * () const
    {
        return val;
    }

    const T& operator [] (DiffType) const
    {
        return val;
    }

    bool operator == (const type& i2)
    {
        return pos == i2.pos;
    }

    DiffType operator != (const type& i2)
    {
        return pos != i2.pos;
    }

    DiffType operator - (const type& i2)
    {
        return pos - i2.pos;
    }
};

/*!
 * Sequence that conceptually consists of multiple copies of the same element.
 * The copies are not stored explicitly, of course.
 *
 * \param T Sequence value type.
 * \param DiffType Sequence difference type.
 */
template <typename T, typename DiffType>
class pseudo_sequence
{
private:
    const T& val;
    DiffType count;

    typedef pseudo_sequence<T, DiffType> type;

public:
    /** Constructor.
     *  \param val Element of the sequence.
     *  \param count Number of copies. */
    pseudo_sequence(const T& val, DiffType count) : val(val), count(count)
    { }

    /** Begin iterator. */
    pseudo_sequence_iterator<T, DiffType> begin() const
    {
        return pseudo_sequence_iterator<T, DiffType>(val, 0);
    }

    /** End iterator. */
    pseudo_sequence_iterator<T, DiffType> end() const
    {
        return pseudo_sequence_iterator<T, DiffType>(val, count);
    }
};

/** Functor that does nothing */
template <typename ValueType>
class void_functor
{
    inline void operator () (const ValueType& v) const
    { }
};

/*!
 * Compute the median of three referenced elements, according to \c comp.
 *
 * \param a First iterator.
 * \param b Second iterator.
 * \param c Third iterator.
 * \param comp Comparator.
 */
template <typename RandomAccessIterator, typename Comparator>
RandomAccessIterator
median_of_three_iterators(RandomAccessIterator a, RandomAccessIterator b,
                          RandomAccessIterator c, Comparator& comp)
{
    if (comp(*a, *b))
        if (comp(*b, *c))
            return b;
        else if (comp(*a, *c))
            return c;
        else
            return a;
    else        //just swap a and b
    if (comp(*a, *c))
        return a;
    else if (comp(*b, *c))
        return c;
    else
        return b;
}

/** Similar to std::equal_to, but allows two different types. */
template <typename T1, typename T2>
struct equal_to : std::binary_function<T1, T2, bool>
{
    bool operator () (const T1& t1, const T2& t2) const
    {
        return t1 == t2;
    }
};

/** Similar to std::less, but allows two different types. */
template <typename T1, typename T2>
struct less : std::binary_function<T1, T2, bool>
{
    bool operator () (const T1& t1, const T2& t2) const
    {
        return t1 < t2;
    }
};

} // namespace parallel

STXXL_END_NAMESPACE

#endif // !STXXL_PARALLEL_BASE_HEADER
