#include "../containers/stack.h"
#include "../containers/vector.h"
#include "../algo/scan.h"
#include "../common/timer.h"

#define STRUCT_SIZE (1) 

struct my_type
{
		int array[STRUCT_SIZE];
};


struct counter 
{
	static int cnt;
	void operator()(my_type & arg)
	{
			arg.array[0] = cnt++;
	}
};
					
int counter::cnt = 0;

int main(int argc, char * argv[])
{
		stxxl::int64 i,size = atol(argv[1])*(1024*1024*1024)/sizeof(my_type);
		stxxl::stack<my_type> Stack;
//		stxxl::vector<my_type,stxxl::striping,(2*1024*1024),stxxl::lru_pager<4>,2> Vector(size);
		stxxl::timer Timer;
		
		Timer.start();
		for(i=0;i<size;i++)
		{
				my_type cur;
				cur.array[0]=i;
				Stack.push(cur);
				//Vector.push_back(cur);
		}
//		int j;
//		stxxl::for_each(Vector.begin(),Vector.end(),counter(),16);
		Timer.stop();
		
		STXXL_MSG("Bandwidth = "<< int(size*sizeof(my_type)/Timer.seconds())/(1024*1024)<<" MB/s" )
}
