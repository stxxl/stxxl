 #ifndef STXXL_META_TEMPLATE_PROGRAMMING
 #define STXXL_META_TEMPLATE_PROGRAMMING
 /***************************************************************************
 *            tmeta.h
 *  Template Metaprogramming Tools
 *  Thu May 29 11:43:44 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../common/utils.h"
 
__STXXL_BEGIN_NAMESPACE

//! \brief IF template metaprogramming statement

//! If \c Flag is \c true then \c IF<>::result is of type Type1
//! otherwise of \c IF<>::result is of type Type2
template <bool Flag,class Type1, class Type2>
struct IF
{
  typedef Type1 result;
};

template <class Type1, class Type2>
struct IF<false,Type1,Type2>
{
  typedef Type2 result;
};


__STXXL_END_NAMESPACE
 
#endif
