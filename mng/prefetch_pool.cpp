/***************************************************************************
 *            write_pool.cpp
 *
 *  Wed Jul  2 15:32:34 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include <iostream>
#include "../mng/mng.h"
#include "prefetch_pool.h"

//! \example mng/prefetch_pool.cpp 

#define BLOCK_SIZE (1024*512)

struct MyType
{
	int integer;
	char chars[5];
};

using namespace stxxl;


typedef typed_block<BLOCK_SIZE,MyType> block_type;

int main ()
{
  prefetch_pool<block_type> pool(2); 
  pool.resize(10);
  pool.resize(5); 
  block_type * blk = new block_type; 
  block_type::bid_type bid;
  block_manager::get_instance()->new_blocks(single_disk(),&bid,(&bid) +1);
  pool.hint(bid);
  pool.read(blk,bid)->wait();
  delete blk;
}
