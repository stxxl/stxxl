/***************************************************************************
 *            simdisk.cpp
 *
 *  Sat Aug 24 23:51:53 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


//#include "../MNG/mng.h"
#include "../IO/io.h"

#define BLKSIZE (3*128*1024)

#define INPUT_SIZE 100


using namespace stxxl;

//void gen_input(BIDArray<BLKSIZE> * input, int ndisks)
//{
//  
//}


int main()
{
  //  Config * cfg = Config::get_instance();
  //  BIDArray<BLKSIZE> *input = BIDArray<BLKSIZE> [cfg->disks_number()/2]; // input will reside on first half on disks
  //  gen_input(input,cfg->disks_number()/2);
  const unsigned size=2048;
  SimDiskFile file1(0),file2(1);
 

  Request * reqs[2];
  file1.set_size(size*sizeof(int));
  file2.set_size(size*sizeof(int));
  int array1[size];
  int array2[size];
  unsigned i=0;
  for(i=0;i<size;i++)
    {
      array1[i] = i;
      array2[i] = size - i -1;
    }

  file1.awrite(array1,0,size*sizeof(int),reqs[0]);
  file2.awrite(array2,0,size*sizeof(int),reqs[1]);

  MC::wait_all(reqs,2);
  
  delete reqs[0];
  delete reqs[1];

  memset(array1,1,size*sizeof(int));
  memset(array2,2,size*sizeof(int));
  
  file1.aread(array1,0,size*sizeof(int),reqs[0]);
  file2.aread(array2,0,size*sizeof(int),reqs[1]);

  MC::wait_all(reqs,2);
  
  for(i=0;i<size;i++)
    {
      cout << array1[i]<< " "<< array2[i]<< endl;
    }
  

  delete reqs[0];
  delete reqs[1];

  return 0;
}

