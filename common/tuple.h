#ifndef TUPLE_HEADER
#define TUPLE_HEADER
/***************************************************************************
 *            tuple.h
 *
 *  Thu Oct  2 16:27:06 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../common/namespace.h"
#include "../common/tmeta.h"

__STXXL_BEGIN_NAMESPACE

struct Plug {};

  
template <  class T1,
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
  {/*
    typedef typename SWITCH<I, CASE<1,first_type,
                                CASE<2,second_type,
                                CASE<3,third_type,
                                CASE<4,fourth_type,
                                CASE<5,fifth_type,
                                CASE<6,sixth_type,
                                CASE<DEFAULT,void 
                            > > > > > > > >::result result;*/
  };
};
  

//! \brief k-Tuple data type
//!
//! (defined for k < 7)
template <  class T1,
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
    typedef typename SWITCH<I,  CASE<1,first_type,
                                CASE<2,second_type,
                                CASE<3,third_type,
                                CASE<4,fourth_type,
                                CASE<5,fifth_type,
                                CASE<6,sixth_type,
                                CASE<DEFAULT,void 
                            > > > > > > > >::result result;
  };
  
  //! \brief First tuple component
  first_type first;
  //! \brief Second tuple component
  second_type second;
  //! \brief Third tuple component
  third_type third;
  //! \brief Fourth tuple component
  fourth_type fourth;
  //! \brief Fifth tuple component
  fifth_type fifth;
  //! \brief Sixth tuple component
  sixth_type sixth;
  
  tuple(){}
  tuple(  first_type fir,
          second_type sec,
          third_type thi,
          fourth_type fou,
          fifth_type fif,
          sixth_type six
        ):
    first(fir),
    second(sec),
    third(thi),
    fourth(fou),
    fifth(fif),
    sixth(six)
  {}
};

//! \brief Partial specialization for 1- \c tuple
template <class T1>
struct tuple<T1,Plug,Plug,Plug,Plug>
{
  typedef T1 first_type;
  
  first_type first;
  
  template <int I>
  struct item_type
  {
     typedef typename IF<I==1,first_type,void>::result result;
  };
  
  tuple(){}
  tuple(first_type fi):
    first(fi)
  {}
};

//! \brief Partial specialization for 2- \c tuple (equivalent to std::pair)
template <class T1,class T2>
struct tuple<T1,T2,Plug,Plug,Plug,Plug>
{
  typedef T1 first_type;
  typedef T2 second_type;
  
  template <int I>
  struct item_type
  {
    typedef typename SWITCH<I,  CASE<1,first_type,
                                CASE<2,second_type,
                                CASE<DEFAULT,void >
                             > > >::result result;
  };
  
  first_type first;
  second_type second;
  
  tuple(){}
  tuple(  first_type fi,
          second_type se
        ):
    first(fi),
    second(se)
  {}
    
};


//! \brief Partial specialization for 3- \c tuple (triple)
template <  class T1,
            class T2, 
            class T3
        >
struct tuple<T1,T2,T3,Plug,Plug,Plug>
{
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;
  
  template <int I>
  struct item_type
  {
    typedef typename SWITCH<I,  CASE<1,first_type,
                                CASE<2,second_type,
                                CASE<3,second_type,
                                CASE<DEFAULT,void >
                             > > > >::result result;
  };
  
  
  first_type first;
  second_type second;
  third_type third;
  
  tuple() {}
  tuple(  first_type fir,
          second_type sec,
          third_type thi
        ):
    first(fir),
    second(sec),
    third(thi)
  {}
    
};

//! \brief Partial specialization for 4- \c tuple
template <  class T1,
            class T2, 
            class T3,
            class T4
        >
struct tuple<T1,T2,T3,T4,Plug,Plug>
{
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;
  typedef T4 fourth_type;
  
  template <int I>
  struct item_type
  {
    typedef typename SWITCH<I,  CASE<1,first_type,
                                CASE<2,second_type,
                                CASE<3,third_type,
                                CASE<4,fourth_type,
                                CASE<DEFAULT,void 
                            > > > > > >::result result;
  };
  
  
  first_type first;
  second_type second;
  third_type third;
  fourth_type fourth;
  
  tuple(){}
  tuple(  first_type fir,
          second_type sec,
          third_type thi,
          fourth_type fou
        ):
    first(fir),
    second(sec),
    third(thi),
    fourth(fou)
  {}
};

//! \brief Partial specialization for 5- \c tuple
template <  class T1,
            class T2, 
            class T3,
            class T4,
            class T5
        >
struct tuple<T1,T2,T3,T4,T5,Plug>
{
  typedef T1 first_type;
  typedef T2 second_type;
  typedef T3 third_type;
  typedef T4 fourth_type;
  typedef T5 fifth_type;
  
  
  template <int I>
  struct item_type
  {
    typedef typename SWITCH<I,  CASE<1,first_type,
                                CASE<2,second_type,
                                CASE<3,third_type,
                                CASE<4,fourth_type,
                                CASE<5,fifth_type,
                                CASE<DEFAULT,void 
                            > > > > > > >::result result;
  };
  
  first_type first;
  second_type second;
  third_type third;
  fourth_type fourth;
  fifth_type fifth;
  
  tuple(){}
  tuple(  first_type fir,
          second_type sec,
          third_type thi,
          fourth_type fou,
          fifth_type fif
        ):
    first(fir),
    second(sec),
    third(thi),
    fourth(fou),
    fifth(fif)
  {}
    
};

/*
template <class tuple_type,int I>
typename tuple_type::item_type<I>::result get(const tuple_type & t)
{
  return NULL;
}*/


__STXXL_END_NAMESPACE

#endif
