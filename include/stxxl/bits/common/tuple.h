/***************************************************************************
 *  include/stxxl/bits/common/tuple.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_TUPLE_HEADER
#define STXXL_TUPLE_HEADER

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/tmeta.h>
#include <limits>

__STXXL_BEGIN_NAMESPACE

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
    typedef T1 first_type;
    typedef T2 second_type;
    typedef T3 third_type;
    typedef T4 fourth_type;
    typedef T5 fifth_type;
    typedef T6 sixth_type;

    template <int I>
    struct item_type
    {
/*
        typedef typename SWITCH<I, CASE<1,first_type,
                                CASE<2,second_type,
                                CASE<3,third_type,
                                CASE<4,fourth_type,
                                CASE<5,fifth_type,
                                CASE<6,sixth_type,
                                CASE<DEFAULT,void
                            > > > > > > > >::result result;
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
    typedef T1 first_type;
    typedef T2 second_type;
    typedef T3 third_type;
    typedef T4 fourth_type;
    typedef T5 fifth_type;
    typedef T6 sixth_type;

    template <int I>
    struct item_type
    {
        typedef typename SWITCH<I, CASE<1, first_type,
                                        CASE<2, second_type,
                                             CASE<3, third_type,
                                                  CASE<4, fourth_type,
                                                       CASE<5, fifth_type,
                                                            CASE<6, sixth_type,
                                                                 CASE<DEFAULT, void
                                                                      > > > > > > > >::result result;
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

    //! Construct tuple from subitems
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

    //! return minimum value of tuple using numeric_limits
    static tuple min_value()
    { return tuple(std::numeric_limits<first_type>::min(),
                   std::numeric_limits<second_type>::min(),
                   std::numeric_limits<third_type>::min(),
                   std::numeric_limits<fourth_type>::min(),
                   std::numeric_limits<fifth_type>::min(),
                   std::numeric_limits<sixth_type>::min()); }

    //! return maximum value of tuple using numeric_limits
    static tuple max_value()
    { return tuple(std::numeric_limits<first_type>::max(),
                   std::numeric_limits<second_type>::max(),
                   std::numeric_limits<third_type>::max(),
                   std::numeric_limits<fourth_type>::max(),
                   std::numeric_limits<fifth_type>::max(),
                   std::numeric_limits<sixth_type>::max()); }
};

//! Partial specialization for 1- \c tuple
template <class T1>
struct tuple<T1, Plug, Plug, Plug, Plug>
{
    typedef T1 first_type;

    first_type first;

    template <int I>
    struct item_type
    {
        typedef typename IF<I == 1, first_type, void>::result result;
    };

    tuple() { }
    tuple(first_type first_)
        : first(first_)
    { }

    //! return minimum value of tuple using numeric_limits
    static tuple min_value()
    { return tuple(std::numeric_limits<first_type>::min()); }

    //! return maximum value of tuple using numeric_limits
    static tuple max_value()
    { return tuple(std::numeric_limits<first_type>::max()); }
};

//! Partial specialization for 2- \c tuple (equivalent to std::pair)
template <class T1, class T2>
struct tuple<T1, T2, Plug, Plug, Plug, Plug>
{
    typedef T1 first_type;
    typedef T2 second_type;

    template <int I>
    struct item_type
    {
        typedef typename SWITCH<I, CASE<1, first_type,
                                        CASE<2, second_type,
                                             CASE<DEFAULT, void>
                                             > > >::result result;
    };

    first_type first;
    second_type second;

    tuple() { }
    tuple(first_type first_,
          second_type second_
        )
        : first(first_),
          second(second_)
    { }

    //! return minimum value of tuple using numeric_limits
    static tuple min_value()
    { return tuple(std::numeric_limits<first_type>::min(),
                   std::numeric_limits<second_type>::min()); }

    //! return maximum value of tuple using numeric_limits
    static tuple max_value()
    { return tuple(std::numeric_limits<first_type>::max(),
                   std::numeric_limits<second_type>::max()); }
};


//! Partial specialization for 3- \c tuple (triple)
template <class T1,
          class T2,
          class T3
          >
struct tuple<T1, T2, T3, Plug, Plug, Plug>
{
    typedef T1 first_type;
    typedef T2 second_type;
    typedef T3 third_type;

    template <int I>
    struct item_type
    {
        typedef typename SWITCH<I, CASE<1, first_type,
                                        CASE<2, second_type,
                                             CASE<3, second_type,
                                                  CASE<DEFAULT, void>
                                                  > > > >::result result;
    };


    first_type first;
    second_type second;
    third_type third;

    tuple() { }
    tuple(first_type _first,
          second_type _second,
          third_type _third
        )
        : first(_first),
          second(_second),
          third(_third)
    { }

    //! return minimum value of tuple using numeric_limits
    static tuple min_value()
    { return tuple(std::numeric_limits<first_type>::min(),
                   std::numeric_limits<second_type>::min(),
                   std::numeric_limits<third_type>::min()); }

    //! return maximum value of tuple using numeric_limits
    static tuple max_value()
    { return tuple(std::numeric_limits<first_type>::max(),
                   std::numeric_limits<second_type>::max(),
                   std::numeric_limits<third_type>::max()); }
};

//! Partial specialization for 4- \c tuple
template <class T1,
          class T2,
          class T3,
          class T4
          >
struct tuple<T1, T2, T3, T4, Plug, Plug>
{
    typedef T1 first_type;
    typedef T2 second_type;
    typedef T3 third_type;
    typedef T4 fourth_type;

    template <int I>
    struct item_type
    {
        typedef typename SWITCH<I, CASE<1, first_type,
                                        CASE<2, second_type,
                                             CASE<3, third_type,
                                                  CASE<4, fourth_type,
                                                       CASE<DEFAULT, void
                                                            > > > > > >::result result;
    };


    first_type first;
    second_type second;
    third_type third;
    fourth_type fourth;

    tuple() { }
    tuple(first_type _first,
          second_type _second,
          third_type _third,
          fourth_type _fourth
        )
        : first(_first),
          second(_second),
          third(_third),
          fourth(_fourth)
    { }

    //! return minimum value of tuple using numeric_limits
    static tuple min_value()
    { return tuple(std::numeric_limits<first_type>::min(),
                   std::numeric_limits<second_type>::min(),
                   std::numeric_limits<third_type>::min(),
                   std::numeric_limits<fourth_type>::min()); }

    //! return maximum value of tuple using numeric_limits
    static tuple max_value()
    { return tuple(std::numeric_limits<first_type>::max(),
                   std::numeric_limits<second_type>::max(),
                   std::numeric_limits<third_type>::max(),
                   std::numeric_limits<fourth_type>::max()); }
};

//! Partial specialization for 5- \c tuple
template <class T1,
          class T2,
          class T3,
          class T4,
          class T5
          >
struct tuple<T1, T2, T3, T4, T5, Plug>
{
    typedef T1 first_type;
    typedef T2 second_type;
    typedef T3 third_type;
    typedef T4 fourth_type;
    typedef T5 fifth_type;


    template <int I>
    struct item_type
    {
        typedef typename SWITCH<I, CASE<1, first_type,
                                        CASE<2, second_type,
                                             CASE<3, third_type,
                                                  CASE<4, fourth_type,
                                                       CASE<5, fifth_type,
                                                            CASE<DEFAULT, void
                                                                 > > > > > > >::result result;
    };

    first_type first;
    second_type second;
    third_type third;
    fourth_type fourth;
    fifth_type fifth;

    tuple() { }
    tuple(first_type _first,
          second_type _second,
          third_type _third,
          fourth_type _fourth,
          fifth_type _fifth
        )
        : first(_first),
          second(_second),
          third(_third),
          fourth(_fourth),
          fifth(_fifth)
    { }

    //! return minimum value of tuple using numeric_limits
    static tuple min_value()
    { return tuple(std::numeric_limits<first_type>::min(),
                   std::numeric_limits<second_type>::min(),
                   std::numeric_limits<third_type>::min(),
                   std::numeric_limits<fourth_type>::min(),
                   std::numeric_limits<fifth_type>::min()); }

    //! return maximum value of tuple using numeric_limits
    static tuple max_value()
    { return tuple(std::numeric_limits<first_type>::max(),
                   std::numeric_limits<second_type>::max(),
                   std::numeric_limits<third_type>::max(),
                   std::numeric_limits<fourth_type>::max(),
                   std::numeric_limits<fifth_type>::max()); }
};

/*
   template <class tuple_type,int I>
   typename tuple_type::item_type<I>::result get(const tuple_type & t)
   {
   return NULL;
   }
*/

__STXXL_END_NAMESPACE

#endif // !STXXL_TUPLE_HEADER
