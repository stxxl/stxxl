 #ifndef STXXL_META_TEMPLATE_PROGRAMMING
 #define STXXL_META_TEMPLATE_PROGRAMMING
 /***************************************************************************
 *            tmeta.h
 *  Template Metaprogramming Tools 
 *  (from the Generative Programming book Krysztof Czarnecki, Ulrich Eisenecker)
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

const int DEFAULT = ~(~0u >> 1); // initialize with the smallest int

struct NilCase {};
  
template <int tag_,class Type_,class Next_ = NilCase>
struct CASE
{
  enum { tag = tag_ };
  typedef Type_ Type;
  typedef Next_ Next;
};

template <int tag,class Case>
class SWITCH
{
  typedef typename Case::Next NextCase;
  enum
  {
    caseTag = Case::tag,
    found   = (caseTag == tag || caseTag == DEFAULT)
  };
public:
  typedef typename IF<found,
          typename Case::Type,
          typename SWITCH<tag,NextCase>::result
          >::result result;
  
};

template <int tag>
class SWITCH<tag,NilCase>
{
public:
  typedef NilCase result;
};

template <unsigned Input>
class LOG
{
public:
	enum 
	{
		value = LOG<Input/2>::value +1
	};
};

template <>
class LOG<0>
{
public:
	enum 
	{
		value = 0
	};
};

__STXXL_END_NAMESPACE
 
#endif
