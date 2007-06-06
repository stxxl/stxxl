/***************************************************************************
 *            test_stable_ksort.cpp
 *
 *  Thu Feb  6 11:46:53 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#define COUNT_WAIT_TIME

// #define PLAY_WITH_OPT_PREF


#include "../mng/mng.h"
#include "stable_ksort.h"
#include "../containers/vector"
#include "../common/rand.h"


#ifndef RECORD_SIZE
 #define RECORD_SIZE 128
#endif

struct my_type
{
    typedef stxxl::uint64 key_type;

    key_type _key;
    char _data[RECORD_SIZE - sizeof(key_type)];
    key_type key() const
    {
        return _key;
    };

    my_type() { };
    my_type(key_type __key) : _key(__key) { };

    static my_type min_value()
    {
        return my_type(0);
    };
    static my_type max_value()
    {
        return my_type(0xffffffff);
    };
};

bool operator < (const my_type & a, const my_type & b)
{
    return a.key() < b.key();
}



#define MB (1024 * 1024)


struct zero
{
    unsigned operator ()  ()
    {
        return 0;
    };
};

template <typename alloc_strategy_type, unsigned block_size>
void test(stxxl::int64 records_to_sort, unsigned memory_to_use)
{
    typedef stxxl::vector < my_type, alloc_strategy_type, block_size, stxxl::lru_pager < 8 >, 2 > vector_type;

    memory_to_use = div_and_round_up(memory_to_use, vector_type::block_type::raw_size) * vector_type::block_type::raw_size;

    vector_type v(records_to_sort);
    unsigned ndisks = stxxl::config::get_instance()->disks_number();
    STXXL_MSG("Sorting " << records_to_sort << " records of size " << sizeof(my_type) )
    STXXL_MSG("Total volume " << (records_to_sort * sizeof(my_type)) / MB << " MB")
    STXXL_MSG("Using " << memory_to_use / MB << " MB")
    STXXL_MSG("Using " << ndisks << " disks")
    STXXL_MSG("Using " << alloc_strategy_type::name() << " allocation strategy ")
    STXXL_MSG("Block size " << vector_type::block_type::raw_size / 1024 << " KB")
    STXXL_MSG("Seed " << stxxl::ran32State )
    STXXL_MSG("Filling vector...")

    std::generate(v.begin(), v.end(), stxxl::random_number64());
    //std::generate(v.begin(),v.end(),zero());

    STXXL_MSG("Sorting vector...")

    stxxl::stable_ksort(v.begin(), v.end(), memory_to_use);

    //STXXL_MSG("Checking order...")
    //STXXL_MSG(((stxxl::is_sorted(v.begin(),v.end()))?"OK":"WRONG" ));
}

template <unsigned block_size>
void test_all_strategies(
    stxxl::int64 records_to_sort,
    unsigned memory_to_use,
    int strategy)
{
    switch (strategy)
    {
    case 0:
        test<stxxl::striping, block_size>(records_to_sort, memory_to_use);
        break;
    case 1:
        test<stxxl::SR, block_size>(records_to_sort, memory_to_use);
        break;
    case 2:
        test<stxxl::FR, block_size>(records_to_sort, memory_to_use);
        break;
    case 3:
        test<stxxl::RC, block_size>(records_to_sort, memory_to_use);
        break;
    default:
        STXXL_ERRMSG("Unknown allocation strategy: " << strategy << ", aborting")
        abort();
    };
}

int main(int argc, char * argv[])
{
    if (argc < 6)
    {
        STXXL_ERRMSG("Usage: " << argv[0] <<
                     " <MB to sort> <MB to use> <alloc_strategy> <blk_size> <seed>")
        return -1;
    }

    stxxl::int64 n_records = (atoll(argv[1]) * MB) / sizeof(my_type);
    int sort_mem = atoi(argv[2]) * MB;
    int strategy = atoi(argv[3]);
    int block_size = atoi(argv[4]);
    stxxl::ran32State = strtoul(argv[5], NULL, 10     );

    switch (block_size)
    {
    case 0:
        test_all_strategies < (128 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 1:
        test_all_strategies < (256 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 2:
        test_all_strategies < (512 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 3:
        test_all_strategies < (1024 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 4:
        test_all_strategies < (2 * 1024 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 5:
        test_all_strategies < (4 * 1024 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 6:
        test_all_strategies < (8 * 1024 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 7:
        test_all_strategies < (16 * 1024 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 8:
        test_all_strategies < (640 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 9:
        test_all_strategies < (768 * 1024) > (n_records, sort_mem, strategy);
        break;
    case 10:
        test_all_strategies < (896 * 1024) > (n_records, sort_mem, strategy);
        break;
    default:
        STXXL_ERRMSG("Unknown block size: " << block_size << ", aborting")
        abort();
    };

    return 0;
}
