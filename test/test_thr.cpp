#include "../mng/mng.h"


#define MB (1024*1024)
#define BLOCK_SIZE (4*1024*1024)


using namespace stxxl;

struct my_type
{
	int key;
	char filler[128 - sizeof(int)];
};

typedef typed_block<BLOCK_SIZE,my_type> BLOCK;

int main(int argc, char * argv[])
{
	if(argc < 3)
	{
		STXXL_MSG("Too few parameters")
		return -1;
	}
	int m = atoi(argv[1])*MB/BLOCK_SIZE;
	int times = atoi(argv[2]);
	int n = m*times;
	int i = 0, j = 0;

	BIDArray<BLOCK_SIZE> bids(2*n);
	block_manager * bm = block_manager::get_instance();
	bm->new_blocks(striping(),bids.begin(),bids.end());
	BLOCK * blocks = new BLOCK[m];
	request ** reqs = new request * [m];
	
	double begin = stxxl_timestamp(),end;
	
	for( i = 0; i < times; i++)
	{
		for(j=0; j<m; j++)
			blocks[j].read(bids[m*i + j],reqs[j]);
		
		mc::waitdel_all(reqs,m);
		
		for(j=0; j<m; j++)
			blocks[j].write(bids[n + m*i + j],reqs[j]);
		
		mc::waitdel_all(reqs,m);
	}
	
	end = stxxl_timestamp();
	
	delete [] blocks;
	delete [] reqs;
	
	double thr = (2*times*m*BLOCK_SIZE/MB)/(end-begin);
	
	STXXL_MSG("Elapsed time: "<<(end-begin)<<" s")
	STXXL_MSG("Throughput  : "<<  thr <<" MB/s")
	STXXL_MSG("Per one disk: "<< thr/config::get_instance()->disks_number() << " MB/s")
	
}

