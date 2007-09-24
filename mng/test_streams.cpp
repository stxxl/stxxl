/***************************************************************************
 *            test_streams.cpp
 *
 *  Sat Aug 24 23:52:33 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include <iostream>
#include "stxxl/bits/mng/mng.h"
#include "stxxl/bits/mng/buf_ostream.h"
#include "stxxl/bits/mng/buf_istream.h"

//! \example mng/test_streams.cpp
//! This is an example of use of \c stxxl::buf_istream and \c stxxl::buf_ostream

#define BLOCK_SIZE (1024 * 512)


using namespace stxxl;


typedef typed_block<BLOCK_SIZE, int> block_type;
typedef buf_ostream<block_type, BIDArray<BLOCK_SIZE>::iterator> buf_ostream_type;
typedef buf_istream<block_type, BIDArray<BLOCK_SIZE>::iterator> buf_istream_type;

int main()
{
    const unsigned nblocks = 64;
    const unsigned nelements = nblocks * block_type::size;
    BIDArray<BLOCK_SIZE> bids(nblocks);

    block_manager * bm = block_manager::get_instance ();
    bm->new_blocks (striping (), bids.begin (), bids.end ());
    {
        buf_ostream_type out(bids.begin(), 2);
        for (unsigned i = 0; i < nelements; i++)
            out << i;
    }
    {
        buf_istream_type in(bids.begin(), bids.end(), 2);
        for (unsigned i = 0; i < nelements; i++)
        {
            int value;
            in >> value;
            if (value != int (i))
            {
                STXXL_ERRMSG("Error at position " << std::hex << i << " (" << value << ") block " << (i / block_type::size) );
            }
        }
    }
    bm->delete_blocks (bids.begin (), bids.end ());
}
