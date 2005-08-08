/***************************************************************************
 *            test.cpp
 *
 *  Sat Aug 24 23:52:27 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "io.h"

//! \example io/test.cpp
//! This is an example of use of \c \<stxxl\> files, requests, and 
//! completion tracking mechanisms, i.e. \c stxxl::file , \c stxxl::request , and
//! \c stxxl::mc

using namespace stxxl;

struct my_handler
{
  void operator () (request * ptr)
  {
    STXXL_MSG("Request completed: "<< ptr)
  }
};

int main()
{
  std::cout << sizeof(void *) << std::endl;
  const int size = 1024 * 384;
  char *buffer = new char[size];
  #ifdef BOOST_MSVC
  char * paths[2] = {"data1", "data2"};
  #else
  char * paths[2] = {"/var/tmp/data1", "/var/tmp/data2"};
  mmap_file file1(paths[0],file::CREAT | file::RDWR /* | file::DIRECT */,0);
  file1.set_size(size*1024);
  #endif

  syscall_file file2(paths[1],file::CREAT | file::RDWR /* | file::DIRECT */,1);
  
  request_ptr req[16];
  unsigned i=0;
  for(;i<16;i++)
    req[i] = file2.awrite(buffer,i*size,size,my_handler());
  
  wait_all(req,16);
  
  delete [] buffer;

  return 0;
}


