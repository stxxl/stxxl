 /***************************************************************************
 *            se_lcp.cpp
 *
 *  Thu Oct  2 15:40:24 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include <vector>
#include "../containers/vector.h"
#include "stream.h"

using namespace stxxl;
using namespace stxxl::stream;


int main()
{
  typedef VECTOR_GENERATOR<int>::result ext_vector_type;
  typedef std::vector<char>             string_vector_type;
  
  string_vector_type T;
  ext_vector_type SA;  // suffix array
  ext_vector_type iSA; // inverse suffix array
  
  //
  // here we generate suffix array to SA and inv. suffix array iSA
  //
  
  // decrare all needed types
  typedef iterator2stream< ext_vector_type::iterator > SA_stream_type;
  typedef SA_stream_type iSA_stream_type;
  typedef transform<Tuple1Gen,SA_stream_type> tuple_gen1_type;
  typedef sort<tuple_gen1_type,Sort1Cmp> Fi_stream_type;
  typedef transform<LCPGen,Fi_stream_type> LCPP_stream_type;
  typedef make_tuple<iSA_stream_type,LCPP_stream_type> tuple_gen2_type;
  typedef sort<tuple_gen2_type,Sort2Cmp> LCP_tuples; // output stream
  
  
}
