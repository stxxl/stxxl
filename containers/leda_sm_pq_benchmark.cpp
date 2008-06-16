/***************************************************************************
 *  containers/leda_sm_pq_benchmark.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/leda_sm_pq_benchmark.cpp
//! This is a benchmark mentioned in the paper
//! R. Dementiev, L. Kettner, P. Sanders "STXXL: standard template library for XXL data sets"
//! Software: Practice and Experience
//! Volume 38, Issue 6, Pages 589-637, May 2008
//! DOI: 10.1002/spe.844

#include <LEDA-SM/ext_memory_manager.h>
#include <LEDA-SM/ext_memory_manager.h>
#include <LEDA-SM/block.h>
#include <LEDA-SM/ext_array.h>
#include <LEDA-SM/debug.h>
#include <LEDA-SM/array_heap.h>
#include <LEDA-SM/ext_r_heap.h>
#include <LEDA-SM/buffer_tree.h>
#include <LEDA/random.h>
#include <algorithm>

#include "stxxl/bits/common/utils_ledasm.h"
#include "stxxl/timer"


#define PQ_MEM_SIZE     (512 * 1024 * 1024)


#define MAX_ELEMENTS (2000 * 1024 * 1024)


struct my_record
{
    int key;
    int data;
    my_record() { }
    my_record(int k, int d) : key(k), data(d) { }
};

std::ostream & operator << (std::ostream & o, const my_record & obj)
{
    o << obj.key << " " << obj.data;
    return o;
}

bool operator == (const my_record & a, const my_record & b)
{
    return a.key == b.key;
}

bool operator != (const my_record & a, const my_record & b)
{
    return a.key != b.key;
}

bool operator < (const my_record & a, const my_record & b)
{
    return a.key < b.key;
}

bool operator > (const my_record & a, const my_record & b)
{
    return a.key > b.key;
}

int compare(const my_record & a, const my_record & b)
{
    if (a.key != b.key)
        return (a.key - b.key);

    return (a.key - b.key);
}


#define    BLOCK_SIZE1 (EXT_BLK_SZ * 4)
#define    BLOCK_SIZE2 (DISK_BLOCK_SIZE * 4)


#if 1
unsigned ran32State = 0xdeadbeef;
inline int myrand()
{
    return ((int)((ran32State = 1664525 * ran32State + 1013904223) >> 1)) - 1;
}
#else // a longer pseudo random sequence
long long unsigned ran32State = 0xdeadbeef;
inline long long unsigned myrand()
{
    return (ran32State = (ran32State * 0x5DEECE66DULL + 0xBULL) & 0xFFFFFFFFFFFFULL);
}
#endif

int pq_blocks;

void run_ledasm_insert_all_delete_all(stxxl::int64 ops)
{
    array_heap<int, int> PQ(pq_blocks);

    stxxl::int64 i;

    my_record cur;


    stxxl::timer Timer;
    Timer.start();

    for (i = 0; i < ops; ++i)
    {
        PQ.insert(myrand(), 0);
    }

    Timer.stop();

    STXXL_MSG("Records in PQ: " << PQ.size());
    if (i != PQ.size())
    {
        STXXL_MSG("Size does not match");
        abort();
    }

    STXXL_MSG("Insertions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(ops) / (Timer.mseconds() / 1000.)) << " key/data pairs per sec");

    ext_mem_mgr.print_statistics();
    ext_mem_mgr.reset_statistics();
    ////////////////////////////////////////////////
    Timer.reset();

    for (i = 0; i < ops; ++i)
    {
        PQ.del_min();
    }

    Timer.stop();

    STXXL_MSG("Records in PQ: " << PQ.size());
    if (!PQ.empty())
    {
        STXXL_MSG("PQ must be empty");
        abort();
    }

    STXXL_MSG("Deletions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(ops) / (Timer.mseconds() / 1000.)) << " key/data pairs per sec");


    ext_mem_mgr.print_statistics();
}


void run_ledasm_intermixed(stxxl::int64 ops)
{
    array_heap<int, int> PQ(pq_blocks);

    stxxl::int64 i;

    my_record cur;


    stxxl::timer Timer;
    Timer.start();

    for (i = 0; i < ops; ++i)
    {
        PQ.insert(myrand(), 0);
    }

    Timer.stop();

    STXXL_MSG("Records in PQ: " << PQ.size());
    if (i != PQ.size())
    {
        STXXL_MSG("Size does not match");
        abort();
    }

    STXXL_MSG("Insertions elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(ops) / (Timer.mseconds() / 1000.)) << " key/data pairs per sec");

    ext_mem_mgr.print_statistics();
    ext_mem_mgr.reset_statistics();
    ////////////////////////////////////////////////
    Timer.reset();

    for (i = 0; i < ops; ++i)
    {
        int o = myrand() % 3;
        if (o == 0)
        {
            PQ.insert(myrand(), 0);
        }
        else
        {
            assert(!PQ.empty());
            PQ.del_min();
        }
    }

    Timer.stop();

    STXXL_MSG("Records in PQ: " << PQ.size());

    STXXL_MSG("Deletions/Insertion elapsed time: " << (Timer.mseconds() / 1000.) <<
              " seconds : " << (double(ops) / (Timer.mseconds() / 1000.)) << " key/data pairs per sec");


    ext_mem_mgr.print_statistics();
}

int main(int argc, char * argv[])
{
    STXXL_MSG("block size 1: " << BLOCK_SIZE1 << " bytes");
    STXXL_MSG("block size 2: " << BLOCK_SIZE2 << " bytes");


    if (argc < 3)
    {
        STXXL_MSG("Usage: " << argv[0] << " version #ops");
        STXXL_MSG("\t version = 1: insert-all-delete-all leda-sm pqueue");
        STXXL_MSG("\t version = 2: insert-all-delete-all leda-sm pqueue");
        return 0;
    }

    int version = atoi(argv[1]);
    stxxl::int64 ops = atoll(argv[2]);

    if (ops > MAX_ELEMENTS)
    {
        STXXL_MSG("#ops can not be larger than " << MAX_ELEMENTS);
        return 0;
    }


    pq_blocks = PQ_MEM_SIZE / (EXT_BLK_SZ * sizeof(GenPtr));

    if (pq_blocks <= 500)
    {
        std::cout << "Array heap must have > 500 blocks, current number of blocks " <<
        pq_blocks << std::endl;
        return -1;
    }


    STXXL_MSG("Running version      : " << version);
    STXXL_MSG("Operations to perform: " << ops);

    switch (version)
    {
    case 1:
        run_ledasm_insert_all_delete_all(ops);
        break;
    case 2:
        run_ledasm_intermixed(ops);
        break;
    default:
        STXXL_MSG("Unsupported version " << version);
    }
}
