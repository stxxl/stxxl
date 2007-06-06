#include <iostream>
#include <malloc.h>
#include <stdlib.h>
#include "../common/utils.h"

using namespace std;



void print_malloc_stats()
{
    struct mallinfo info = mallinfo();
    STXXL_MSG("MALLOC statistics BEGIN")
    STXXL_MSG("===============================================================")
    STXXL_MSG("non-mmapped space allocated from system (bytes): " << info.arena)
    STXXL_MSG("number of free chunks                          : " << info.ordblks)
    STXXL_MSG("number of fastbin blocks                       : " << info.smblks)
    STXXL_MSG("number of chunks allocated via mmap()          : " << info.hblks)
    STXXL_MSG("total number of bytes allocated via mmap()     : " << info.hblkhd)
    STXXL_MSG("maximum total allocated space (bytes)          : " << info.usmblks)
    STXXL_MSG("space available in freed fastbin blocks (bytes): " << info.fsmblks)
    STXXL_MSG("number of bytes allocated and in use           : " << info.uordblks)
    STXXL_MSG("number of bytes allocated but not in use       : " << info.fordblks)
    STXXL_MSG("top-most, releasable (via malloc_trim) space   : " << info.keepcost)
    STXXL_MSG("================================================================")
};

int main(int argc, char * argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " bytes_to_allocate" << endl;
        return -1;
    }
    sbrk(128 * 1024 * 1024);
    cout << "Nothing allocated" << endl;
    print_malloc_stats();
    char tmp;
    cin >> tmp;
    const unsigned bytes = atoi(argv[1]);
    char * ptr = new char[bytes];
    cout << "Allocated " << bytes << " bytes" << endl;
    print_malloc_stats();
    delete [] ptr;
    cout << "Deallocated " << endl;
    print_malloc_stats();
};
