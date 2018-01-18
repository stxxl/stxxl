/***************************************************************************
 *  include/stxxl/bits/common/tuple.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_TUPLE_HEADER
#define STXXL_COMMON_TUPLE_HEADER

#include <limits>
#include <ostream>
#include <type_traits>

#include <foxxll/common/tmeta.hpp>

namespace stxxl {

struct Plug { };

template <class T1,
          class T2,
          class T3,
          class T4,
          class T5,
          class T6
          >
struct tuple_base
{
    using first_type = T1;
    using second_type = T2;
    using third_type = T3;
    using fourth_type = T4;
    using fifth_type = T5;
    using sixth_type = T6;

    template <int I>
    struct item_type
    {
/*
        using result =  typename foxxll::SWITCH<I, foxxll::CASE<1,first_type,
                                foxxll::CASE<2,second_type,
                                foxxll::CASE<3,third_type,
                                foxxll::CASE<4,fourth_type,
                                foxxll::CASE<5,fifth_type,
                                foxxll::CASE<6,sixth_type,
                                foxxll::CASE<foxxll::DEFAULT,void
                            > > > > > > > >::type ;
*/
    };
};

//! k-Tuple data type
//!
//! (defined for k < 7)
template <class T1,
          class T2 = Plug,
          class T3 = Plug,
          class T4 = Plug,
          class T5 = Plug,
          class T6 = Plug
          >
struct tuple
{
    //! First tuple component type
    using first_type = T1;
    //! Second tuple component type
    using second_type = T2;
    //! Third tuple component type
    using third_type = T3;
    //! Fourth tuple component type
    using fourth_type = T4;
    //! Fifth tuple component type
    using fifth_type = T5;
    //! Sixth tuple component type
    using sixth_type = T6;

    template <int I>
    struct item_type
    {
        using result = typename foxxll::SWITCH<
                  I, foxxll::CASE<
                      1, first_type, foxxll::CASE<
                          2, second_type, foxxll::CASE<
                              3, third_type, foxxll::CASE<
                                  4, fourth_type, foxxll::CASE<
                                      5, fifth_type, foxxll::CASE<
                                          6, sixth_type,
                                          foxxll::CASE<foxxll::DEFAULT, void
                                                       > > > > > > > >::type;
    };

    //! First tuple component
    first_type first;
    //! Second tuple component
    second_type second;
    //! Third tuple component
    third_type third;
    //! Fourth tuple component
    fourth_type fourth;
    //! Fifth tuple component
    fifth_type fifth;
    //! Sixth tuple component
    sixth_type sixth;

    //! Empty constructor
    tuple() { }

    //! Construct tuple from components
    tuple(first_type _first,
          second_type _second,
          third_type _third,
          fourth_type _fourth,
          fifth_type _fifth,
          sixth_type _sixth
          )
        : first(_first),
          second(_second),
          third(_third),
          fourth(_fourth),
          fifth(_fifth),
          sixth(_sixth)
    { }

    //! Equality comparison
    bool operator == (const tuple& t) const
    {
        return first == t.first && second == t.second && third == t.third
               && fourth == t.fourth && fifth == t.fifth && sixth == t.sixth;
    }

    //! Inequality comparison
    bool operator != (const tuple& t) const
    {
        return !(first == t.first && second == t.second && third == t.third
                 && fourth == t.fourth && fifth == t.fifth && sixth == t.sixth);
    }

    //! Make tuple ostream-able
    friend std::ostream& operator << (std::ostream& os, const tuple& t)
    {
        return os << '(' << t.first << ',' << t.second << ',' << t.third
                  << ',' << t.fourth << ',' << t.fifth << ',' << t.sixth
                  << ')';
    }

    //! Return minimum value of tuple using numeric_limits
    static tuple min_value()
    {
        return tuple(std::numeric_limits<first_type>::min(),
                     std::numeric_limits<second_type>::min(),
                     std::numeric_limits<third_type>::min(),
                     std::numeric_limits<fourth_type>::min(),
                     std::numeric_limits<fifth_type>::min(),
                     std::numeric_limits<sixth_type>::min());
    }

    //! Return maximum value of tuple using numeric_limits
    static tuple max_value()
    {
        return tuple(std::numeric_limits<first_type>::max(),
                     std::numeric_limits<second_type>::max(),
                     std::numeric_limits<third_type>::max(),
                     std::numeric_limits<fourth_type>::max(),
                     std::numeric_limits<fifth_type>::max(),
                     std::numeric_limits<sixth_type>::max());
    }
};

//! Partial specialization for 1- \c tuple
template <class T1>
struct tuple<T1, Plug, Plug, Plug, Plug>
{
    //! First tuple component type
    using first_type = T1;

    //! First tuple component
    first_type first;

    template <int I>
    struct item_type
    {
        using result = typename std::conditional<
                  I == 1, first_type, void>::type;
    };

    //! Empty constructor
    tuple() { }

    //! Initializing constructor
    explicit tuple(first_type first_)
        : first(first_)
    { }

    //! Equality comparison
    bool operator == (const tuple& t) const
    {
        return first == t.first;
    }

    //! Inequality comparison
    bool operator != (const tuple& t) const
    {
        return !(first == t.first);
    }

    //! Make tuple ostream-able
    friend std::ostream& operator << (std::ostream& os, const tuple& t)
    {
        return os << '(' << t.first << ')';
    }

    //! Return minimum value of tuple using numeric_limits
    static tuple min_value()
    {
        return tuple(std::numeric_limits<first_type>::min());
    }

    //! Return maximum value of tuple using numeric_limits
    static tuple max_value()
    {
        return tuple(std::numeric_limits<first_type>::max());
    }
};

//! Partial specialization for 2- \c tuple (equivalent to std::pair)
template <class T1, class T2>
struct tuple<T1, T2, Plug, Plug, Plug, Plug>
{
    //! First tuple component type
    using first_type = T1;
    //! Second tuple component type
    using second_type = T2;

    template <int I>
    struct item_type
    {
        using result = typename foxxll::SWITCH<
                  I, foxxll::CASE<
                      1, first_type, foxxll::CASE<
                          2, second_type, foxxll::CASE<
                              foxxll::DEFAULT, void>
                          > > >::type;
    };

    //! First tuple component
    first_type first;
    //! Second tuple component
    second_type second;

    //! Empty constructor
    tuple() { }

    //! Initializing constructor
    tuple(first_type first_,
          second_type second_)
        : first(first_),
          second(second_)
    { }

    //! Equality comparison
    bool operator == (const tuple& t) const
    {
        return first == t.first && second == t.second;
    }

    //! Inequality comparison
    bool operator != (const tuple& t) const
    {
        return !(first == t.first && second == t.second);
    }

    //! Make tuple ostream-able
    friend std::ostream& operator << (std::ostream& os, const tuple& t)
    {
        return os << '(' << t.first << ',' << t.second << ')';
    }

    //! Return minimum value of tuple using numeric_limits
    static tuple min_value()
    {
        return tuple(std::numeric_limits<first_type>::min(),
                     std::numeric_limits<second_type>::min());
    }

    //! Return maximum value of tuple using numeric_limits
    static tuple max_value()
    {
        return tuple(std::numeric_limits<first_type>::max(),
                     std::numeric_limits<second_type>::max());
    }
};

//! Partial specialization for 3- \c tuple (triple)
template <class T1,
          class T2,
          class T3
          >
struct tuple<T1, T2, T3, Plug, Plug, Plug>
{
    //! First tuple component type
    using first_type = T1;
    //! Second tuple component type
    using second_type = T2;
    //! Third tuple component type
    using third_type = T3;

    template <int I>
    struct item_type
    {
        using result = typename foxxll::SWITCH<I, foxxll::CASE<1, first_type,
                                                               foxxll::CASE<2, second_type,
                                                                            foxxll::CASE<3, second_type,
                                                                                         foxxll::CASE<foxxll::DEFAULT, void>
                                                                                         > > > >::type;
    };

    //! First tuple component
    first_type first;
    //! Second tuple component
    second_type second;
    //! Third tuple component
    third_type third;

    //! Empty constructor
    tuple() { }

    //! Construct tuple from components
    tuple(first_type _first,
          second_type _second,
          third_type _third)
        : first(_first),
          second(_second),
          third(_third)
    { }

    //! Equality comparison
    bool operator == (const tuple& t) const
    {
        return first == t.first && second == t.second && third == t.third;
    }

    //! Inequality comparison
    bool operator != (const tuple& t) const
    {
        return !(first == t.first && second == t.second && third == t.third);
    }

    //! Make tuple ostream-able
    friend std::ostream& operator << (std::ostream& os, const tuple& t)
    {
        return os << '(' << t.first << ',' << t.second << ',' << t.third
                  << ')';
    }

    //! Return minimum value of tuple using numeric_limits
    static tuple min_value()
    {
        return tuple(std::numeric_limits<first_type>::min(),
                     std::numeric_limits<second_type>::min(),
                     std::numeric_limits<third_type>::min());
    }

    //! Return maximum value of tuple using numeric_limits
    static tuple max_value()
    {
        return tuple(std::numeric_limits<first_type>::max(),
                     std::numeric_limits<second_type>::max(),
                     std::numeric_limits<third_type>::max());
    }
};

//! Partial specialization for 4- \c tuple
template <class T1,
          class T2,
          class T3,
          class T4
          >
struct tuple<T1, T2, T3, T4, Plug, Plug>
{
    //! First tuple component type
    using first_type = T1;
    //! Second tuple component type
    using second_type = T2;
    //! Third tuple component type
    using third_type = T3;
    //! Fourth tuple component type
    using fourth_type = T4;

    template <int I>
    struct item_type
    {
        using result = typename foxxll::SWITCH<I, foxxll::CASE<1, first_type,
                                                               foxxll::CASE<2, second_type,
                                                                            foxxll::CASE<3, third_type,
                                                                                         foxxll::CASE<4, fourth_type,
                                                                                                      foxxll::CASE<foxxll::DEFAULT, void
                                                                                                                   > > > > > >::type;
    };

    //! First tuple component
    first_type first;
    //! Second tuple component
    second_type second;
    //! Third tuple component
    third_type third;
    //! Fourth tuple component
    fourth_type fourth;

    //! Empty constructor
    tuple() { }

    //! Construct tuple from components
    tuple(first_type _first,
          second_type _second,
          third_type _third,
          fourth_type _fourth)
        : first(_first),
          second(_second),
          third(_third),
          fourth(_fourth)
    { }

    //! Equality comparison
    bool operator == (const tuple& t) const
    {
        return first == t.first && second == t.second && third == t.third
               && fourth == t.fourth;
    }

    //! Inequality comparison
    bool operator != (const tuple& t) const
    {
        return !(first == t.first && second == t.second && third == t.third
                 && fourth == t.fourth);
    }

    //! Make tuple ostream-able
    friend std::ostream& operator << (std::ostream& os, const tuple& t)
    {
        return os << '(' << t.first << ',' << t.second << ',' << t.third
                  << ',' << t.fourth << ')';
    }

    //! Return minimum value of tuple using numeric_limits
    static tuple min_value()
    {
        return tuple(std::numeric_limits<first_type>::min(),
                     std::numeric_limits<second_type>::min(),
                     std::numeric_limits<third_type>::min(),
                     std::numeric_limits<fourth_type>::min());
    }

    //! Return maximum value of tuple using numeric_limits
    static tuple max_value()
    {
        return tuple(std::numeric_limits<first_type>::max(),
                     std::numeric_limits<second_type>::max(),
                     std::numeric_limits<third_type>::max(),
                     std::numeric_limits<fourth_type>::max());
    }
};

//! Partial specialization for 5- \c tuple
template <class T1,
          class T2,
          class T3,
          class T4,
          class T5
          >
struct tuple<T1, T2, T3, T4, T5, Plug> // NOLINT
{
    //! First tuple component type
    using first_type = T1;
    //! Second tuple component type
    using second_type = T2;
    //! Third tuple component type
    using third_type = T3;
    //! Fourth tuple component type
    using fourth_type = T4;
    //! Fifth tuple component type
    using fifth_type = T5;

    template <int I>
    struct item_type
    {
        using result = typename foxxll::SWITCH<I, foxxll::CASE<1, first_type,
                                                               foxxll::CASE<2, second_type,
                                                                            foxxll::CASE<3, third_type,
                                                                                         foxxll::CASE<4, fourth_type,
                                                                                                      foxxll::CASE<5, fifth_type,
                                                                                                                   foxxll::CASE<foxxll::DEFAULT, void
                                                                                                                                > > > > > > >::type;
    };

    //! First tuple component
    first_type first;
    //! Second tuple component
    second_type second;
    //! Third tuple component
    third_type third;
    //! Fourth tuple component
    fourth_type fourth;
    //! Fifth tuple component
    fifth_type fifth;

    //! Empty constructor
    tuple() { }

    //! Construct tuple from components
    tuple(first_type _first,
          second_type _second,
          third_type _third,
          fourth_type _fourth,
          fifth_type _fifth)
        : first(_first),
          second(_second),
          third(_third),
          fourth(_fourth),
          fifth(_fifth)
    { }

    //! Equality comparison
    bool operator == (const tuple& t) const
    {
        return first == t.first && second == t.second && third == t.third
               && fourth == t.fourth && fifth == t.fifth;
    }

    //! Inequality comparison
    bool operator != (const tuple& t) const
    {
        return !(first == t.first && second == t.second && third == t.third
                 && fourth == t.fourth && fifth == t.fifth);
    }

    //! Make tuple ostream-able
    friend std::ostream& operator << (std::ostream& os, const tuple& t)
    {
        return os << '(' << t.first << ',' << t.second << ',' << t.third
                  << ',' << t.fourth << ',' << t.fifth << ')';
    }

    //! Return minimum value of tuple using numeric_limits
    static tuple min_value()
    {
        return tuple(std::numeric_limits<first_type>::min(),
                     std::numeric_limits<second_type>::min(),
                     std::numeric_limits<third_type>::min(),
                     std::numeric_limits<fourth_type>::min(),
                     std::numeric_limits<fifth_type>::min());
    }

    //! Return maximum value of tuple using numeric_limits
    static tuple max_value()
    {
        return tuple(std::numeric_limits<first_type>::max(),
                     std::numeric_limits<second_type>::max(),
                     std::numeric_limits<third_type>::max(),
                     std::numeric_limits<fourth_type>::max(),
                     std::numeric_limits<fifth_type>::max());
    }
};

/*
   template <class tuple_type,int I>
   typename tuple_type::item_type<I>::result get(const tuple_type & t)
   {
   return nullptr;
   }
*/

template <typename TupleType>
struct tuple_less1st
{
    using value_type = TupleType;

    bool operator () (const value_type& a, const value_type& b) const
    {
        return (a.first < b.first);
    }

    static value_type min_value() { return value_type::min_value(); }
    static value_type max_value() { return value_type::max_value(); }
};

template <typename TupleType>
struct tuple_greater1st
{
    using value_type = TupleType;

    bool operator () (const value_type& a, const value_type& b) const
    {
        return (a.first > b.first);
    }

    static value_type min_value() { return value_type::max_value(); }
    static value_type max_value() { return value_type::min_value(); }
};

template <typename TupleType>
struct tuple_less1st_less2nd
{
    using value_type = TupleType;

    bool operator () (const value_type& a, const value_type& b) const
    {
        if (a.first == b.first)
            return (a.second < b.second);
        return (a.first < b.first);
    }

    static value_type min_value() { return value_type::min_value(); }
    static value_type max_value() { return value_type::max_value(); }
};

template <typename TupleType>
struct tuple_less2nd
{
    using value_type = TupleType;

    bool operator () (const value_type& a, const value_type& b) const
    {
        return (a.second < b.second);
    }

    static value_type min_value() { return value_type::min_value(); }
    static value_type max_value() { return value_type::max_value(); }
};

template <typename TupleType>
struct tuple_less2nd_less1st
{
    using value_type = TupleType;

    bool operator () (const value_type& a, const value_type& b) const
    {
        if (a.second == b.second)
            return (a.first < b.first);
        return (a.second < b.second);
    }

    static value_type min_value() { return value_type::min_value(); }
    static value_type max_value() { return value_type::max_value(); }
};

namespace stream {

/**
 * Counter for creating tuple indexes for example.
 */
template <class ValueType>
struct counter
{
public:
    using value_type = ValueType;

protected:
    value_type m_count;

public:
    explicit counter(const value_type& start = 0)
        : m_count(start)
    { }

    const value_type& operator * () const
    {
        return m_count;
    }

    counter& operator ++ ()
    {
        ++m_count;
        return *this;
    }

    bool empty() const
    {
        return false;
    }
};

/**
 * Concatenates two tuple streams as streamA . streamB
 */
template <class StreamA, class StreamB>
class concatenate
{
public:
    using value_type = typename StreamA::value_type;

private:
    StreamA& A;
    StreamB& B;

public:
    concatenate(StreamA& A_, StreamB& B_) : A(A_), B(B_)
    {
        assert(!A.empty());
        assert(!B.empty());
    }

    const value_type& operator * () const
    {
        assert(!empty());

        if (!A.empty()) {
            return *A;
        }
        else {
            return *B;
        }
    }

    concatenate& operator ++ ()
    {
        assert(!empty());

        if (!A.empty()) {
            ++A;
        }
        else if (!B.empty()) {
            ++B;
        }

        return *this;
    }

    bool empty() const
    {
        return (A.empty() && B.empty());
    }
};

} // namespace stream
} // namespace stxxl

#endif // !STXXL_COMMON_TUPLE_HEADER
