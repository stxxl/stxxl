#include "../mng/mng.h"


struct my_type
{
	int key;
	char recnum[96];
};

using namespace stxxl;

#define BLOCK_SIZE (512*1024)

typedef typed_block<BLOCK_SIZE,my_type> BLOCK;

template <typename T,unsigned a, bool b=a%sizeof(T)>
class some_class
{
};

int main(void)
{
	STXXL_MSG("Block size " << BLOCK_SIZE)
	STXXL_MSG("Block size " << BLOCK::raw_size)
	STXXL_MSG("Block size " << sizeof(BLOCK))
	STXXL_MSG("Type size  " << sizeof(my_type))
	STXXL_MSG("Elements   " << BLOCK::size)
	STXXL_MSG(sizeof(__filler_struct<0>))
	STXXL_MSG(sizeof(some_class<double,0>))
}
