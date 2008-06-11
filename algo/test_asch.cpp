#include "stxxl/bits/algo/async_schedule.h"
#include "stxxl/random"
#include <cstdlib>

#include "async_schedule.cpp"

// Test async schedule algorithm


int main(int argc, char * argv[])
{
    if (argc < 5)
    {
        STXXL_ERRMSG("Usage: " << argv[0] << " D L m seed");
        return -1;
    }
    int i;
    const int D = atoi(argv[1]);
    const int L = atoi(argv[2]);
    const int m = atoi(argv[3]);
    stxxl::ran32State = atoi(argv[4]);
    int * disks = new int[L];
    int * prefetch_order = new int[L];
    int * count = new int[D];


    for (i = 0; i < D; i++)
        count[i] = 0;


    stxxl::random_number<> rnd;
    for (i = 0; i < L; i++)
    {
        disks[i] = rnd(D);
        count[disks[i]]++;
    }

    for (i = 0; i < D; i++)
        std::cout << "Disk " << i << " has " << count[i] << " blocks" << std::endl;


    stxxl::compute_prefetch_schedule(disks, disks + L, prefetch_order, m, D);

    delete[] count;
    delete[] disks;
    delete[] prefetch_order;
}
