/***************************************************************************
 *            test.cpp
 *
 *  Sat Aug 24 23:52:33 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include <iostream>
#include "mng.h"

//! \example mng/test.cpp 
//! This is an example of use of completion handlers, \c stxxl::block_manager, and
//! \c stxxl::typed_block

#define BLOCK_SIZE (1024*512)

struct MyType
{
	int integer;
	char chars[5];
};

using namespace stxxl;


struct my_handler
{
	void operator () (request * req)
	{
		STXXL_MSG( req << " done, type="<<req->io_type() )
	}
};

int
main ()
{

	STXXL_MSG(sizeof(MyType)<<" "<<(BLOCK_SIZE % sizeof(MyType)));
	STXXL_MSG(sizeof(typed_block < BLOCK_SIZE, MyType >)<<" "<<BLOCK_SIZE);
	const unsigned nblocks = 64;
	BIDArray < BLOCK_SIZE > bids (nblocks);
	std::vector < int >disks (nblocks, 2);
	stxxl::request_ptr * reqs = new stxxl::request_ptr[nblocks];
	block_manager *bm = block_manager::get_instance ();
	bm->new_blocks (striping (), bids.begin (), bids.end ());

	typed_block < BLOCK_SIZE, MyType > * block = new typed_block < BLOCK_SIZE, MyType >; 
	unsigned i = 0;
	for (i = 0; i < typed_block < BLOCK_SIZE, MyType >::size; i++)
	{
		block->elem[i].integer = 0xdeadbeef;
		memcpy (block->elem[i].chars, "STXXL", 5);
	}
	for (i = 0; i < nblocks; i++)
		reqs[i] = block->write (bids[i], my_handler());

	std::cout << "Waiting " << std::endl;
	stxxl::wait_all (reqs, nblocks);

	bm->delete_blocks (bids.begin(), bids.end ());

	delete [] reqs;
	delete block;

	// variable-size blocks
	BIDArray<0> vbids (nblocks);
  for(i=0;i<nblocks;i++)
    vbids[i].size = 1024 + i;
  
	bm->new_blocks (striping (), vbids.begin (), vbids.end ());
	
  for(i=0;i<nblocks;i++)
    STXXL_MSG("Allocated block: offset="<<vbids[i].offset<<", size="<<vbids[i].size)
  
  bm->delete_blocks(vbids.begin (), vbids.end ());
}
