/***************************************************************************
 *            test.cpp
 *
 *  Sat Aug 24 23:52:27 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/



#include "io.h"
#include "../common/rand.h"
#include <math.h>

using namespace stxxl;

int main()
{
  stxxl::int64 disk_size = stxxl::int64(1024*1024)*1024*40;	
  std::cout << sizeof(void *) << std::endl;
  const int block_size = 4 * 1024 * 1024;
  char *buffer = new char[block_size];
  char * paths[2] = {"/tmp/data1", "/tmp/data2"};
  sim_disk_file file1(paths[0],file::CREAT | file::RDWR /* | file::DIRECT */,0);
  file1.set_size(disk_size);

  sim_disk_file file2(paths[1],file::CREAT | file::RDWR /* | file::DIRECT */,1);
  file2.set_size(disk_size);
  
  request * reqs[16];
  unsigned i=0;

  /*
  for(;i<16;i++)
    file1.awrite(buffer,pos,,req[i],NULL,NULL);
  
  mc::wait_all(req,16);

  for(i=0;i<16;i++)
    delete reqs[i];
	*/

  stxxl::int64 pos = 0;	

  request * req;
  
  STXXL_MSG("Estimated time:"<<block_size/double(AVERAGE_SPEED))
  STXXL_MSG("Sequential write")
 
  for(i=0;i<40;i++)
  {
 	double begin = stxxl_timestamp();
	file1.awrite(buffer,pos,block_size,req,NULL,NULL);
	req->wait();
	delete req;
	double end = stxxl_timestamp();

	STXXL_MSG("Pos: "<<pos<<" block_size:"<<block_size<<" time:"<<(end - begin));
  	pos += 1024*1024*1024;
  }
 
  double sum =0.;
  double sum2 =0.;
  STXXL_MSG("Random write")
  const int times = 80;
  for(i=0;i<times;i++)
  {
  	random_number rnd;
	pos = rnd(disk_size/block_size)*block_size;
  	double begin = stxxl_timestamp();	
  	file1.awrite(buffer,pos,block_size,req,NULL,NULL);
	req->wait();
	delete req;
	double diff = stxxl_timestamp() - begin;
	
	sum+=diff;
	sum2+=diff*diff;
	
	STXXL_MSG("Pos: "<<pos<<" block_size:"<<block_size<<" time:"<<(diff));
  };

  sum = sum /double(times);
  sum2 = sum2 /double(times);
  assert(sum2 - sum*sum >= 0.0 );
  double err = sqrt(sum2 - sum*sum);
  STXXL_MSG("Error: "<<err<<" s, "<< 100.*(err/sum)<<" %");		
  
  delete [] buffer;

  return 0;
}


