/***************************************************************************
 *  tools/benchmark_pqs.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

static const char* description =
    "Benchmark different priority queue implementations using a sequence of "
    "operations. The PQ contains of 24 byte values. The operation sequence is either a simple fill/delete "
    "cycle or fill/intermixed inserts/deletes.";

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <limits>
#include <queue>

#include <stxxl/bits/common/tuple.h>
#include <stxxl/bits/containers/priority_queue.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/cmdline>
#include <stxxl/random>
#include <stxxl/sorter>
#include <stxxl/timer>

#if STXXL_PARALLEL
    #include <omp.h>
    #include <stxxl/bits/containers/parallel_priority_queue.h>
#endif

using stxxl::uint32;
using stxxl::uint64;

static const uint64 KiB = 1024L;
static const uint64 MiB = 1024L * KiB;
static const uint64 GiB = 1024L * MiB;

// value size must be > 16
static const unsigned value_size = 24;

// constant values for STXXL PQ
const uint64 _RAM = 3 * GiB;
const uint64 _volume = 6 * GiB;
const uint64 _num_elements = _volume / value_size;
const uint64 mem_for_queue = _RAM / 2;
const uint64 mem_for_prefetch_pool = _RAM / 4;
const uint64 mem_for_write_pool = _RAM / 4;

uint64 RAM = _RAM;
uint64 volume = _volume;
uint64 num_elements = _num_elements;

uint64 bulk_ram = 1024 * value_size;
uint64 single_heap_ram = 1 * MiB;
uint64 extract_buffer_ram = 0;
unsigned num_write_buffers = 14;
unsigned num_prefetchers = 1;
unsigned num_read_blocks = 1;
uint64 block_size = STXXL_DEFAULT_BLOCK_SIZE(value_type);

#if STXXL_PARALLEL
unsigned num_insertion_heaps = omp_get_max_threads();
#endif

uint64 ram_write_buffers;
uint64 bulk_size;

uint64 num_arrays = 0;
uint64 max_merge_buffer_ram_stat = 0;

uint64 value_universe_size = 1000;

//const unsigned int seed = 12345;
unsigned int seed = 12345;

void calculate_depending_parameters()
{
    num_elements = volume / value_size;
    ram_write_buffers = num_write_buffers * block_size;
    bulk_size = bulk_ram / value_size;
}

/*
 * The value type and corresponding functions-
 */

struct value_type {
    typedef uint64 key_type;
    key_type first;
    key_type second;
    char data[value_size - 2 * sizeof(key_type)];
    value_type() :
        first(0), second(0)
    {
        #ifdef STXXL_VALGRIND_AVOID_UNINITIALIZED_WRITE_ERRORS
        memset(data, 0, sizeof(data));
        #endif
    }
    value_type(const key_type& _key) :
        first(_key)
    {
        #ifdef STXXL_VALGRIND_AVOID_UNINITIALIZED_WRITE_ERRORS
        memset(data, 0, sizeof(data));
        #endif
    }
    value_type(const key_type& _key, const key_type& _data) :
        first(_key), second(_data)
    {
        #ifdef STXXL_VALGRIND_AVOID_UNINITIALIZED_WRITE_ERRORS
        memset(data, 0, sizeof(data));
        #endif
    }
};

struct value_type_cmp_smaller : public std::binary_function<value_type, value_type, bool>{
    bool operator () (const value_type& a, const value_type& b) const
    {
        return a.first < b.first;
    }
    value_type min_value() const
    {
        return value_type(0);
    }
    value_type max_value() const
    {
        return value_type(std::numeric_limits<value_type::key_type>::max());
    }
};

struct value_type_cmp_greater : public std::binary_function<value_type, value_type, bool>{
    bool operator () (const value_type& a, const value_type& b) const
    {
        return a.first > b.first;
    }
    value_type min_value() const
    {
        return value_type(std::numeric_limits<value_type::key_type>::max());
    }
};


std::ostream& operator << (std::ostream& o, const value_type& obj)
{
    o << obj.first;
    return o;
}


/*
 * Progress messages
 */

static const uint64 printmod = 16 * MiB;
static inline void progress(const char* text, uint64 i, uint64 nelements)
{
    if ((i % printmod) == 0) {
        STXXL_MSG(text << " " << i << " (" << std::setprecision(5) << (static_cast<double>(i)* 100. / static_cast<double>(nelements)) << " %)");
    }
}


/*
 * Defining priority queues.
 */

typedef stxxl::PRIORITY_QUEUE_GENERATOR<
    value_type,
    value_type_cmp_greater,
    mem_for_queue,
    _num_elements / 1024> gen;
typedef gen::result stxxlpq_type;
stxxlpq_type* stxxlpq;

#if STXXL_PARALLEL
    typedef stxxl::parallel_priority_queue<value_type, value_type_cmp_greater> ppq_type;
    ppq_type* ppq;
#endif

typedef stxxl::sorter<value_type, value_type_cmp_smaller> sorter_type;
sorter_type* stxxlsorter;

typedef std::priority_queue<value_type, std::vector<value_type>, value_type_cmp_greater> stlpq_type;
stlpq_type* stlpq;


/*
 * Including the tests
 */


#if STXXL_PARALLEL
    #define PPQ
        #include "benchmark_pqs_helper.h"
    #undef PPQ
#endif
#define STXXLPQ
        #include "benchmark_pqs_helper.h"
#undef STXXLPQ
#define STXXLSORTER
        #include "benchmark_pqs_helper.h"
#undef STXXLSORTER
#define STLPQ
        #include "benchmark_pqs_helper.h"
#undef STLPQ


/*
 * Print statistics and parameters.
 */

void print_params()
{
    STXXL_MEMDUMP(RAM);
    STXXL_MEMDUMP(volume);
    STXXL_MEMDUMP(block_size);
    STXXL_MEMDUMP(single_heap_ram);
    STXXL_MEMDUMP(bulk_ram);
    STXXL_VARDUMP(bulk_size);
    STXXL_MEMDUMP(value_size);
    STXXL_MEMDUMP(extract_buffer_ram);
    STXXL_VARDUMP(num_elements);
    #if STXXL_PARALLEL
        STXXL_VARDUMP(num_insertion_heaps);
    #endif
    STXXL_VARDUMP(num_write_buffers);
    STXXL_VARDUMP(num_prefetchers);
    STXXL_VARDUMP(num_read_blocks);
}

#if STXXL_PARALLEL
inline void ppq_stats()
{
    ppq->print_stats();
}
#endif


/*
 * The main procedure.
 */


int benchmark_pqs(int argc, char* argv[])
{
    seed = static_cast<unsigned>(time(NULL));

    /*
     * Parse flags
     */

    bool do_ppq = false;
    bool do_stxxlpq = false;
    bool do_sorter = false;
    bool do_stlpq = false;
    bool do_tbbpq = false;

    bool do_random = false;
    bool do_check = false;
    bool do_bulk = false;
    bool do_intermixed = false;
    bool do_dijkstra = false;
    bool do_fill = false;
    bool do_parallel = false;
    bool do_flush_directly = false;

    stxxl::cmdline_parser cp;
    cp.set_description(description);

    cp.add_flag('n', "ppq", "Benchmark parallel priority queue", do_ppq);
    cp.add_flag('o', "stxxl", "Benchmark stxxl priority queue", do_stxxlpq);
    cp.add_flag('s', "sorter", "Benchmark stxxl::sorter", do_sorter);
    cp.add_flag('l', "stl", "Benchmark std::priority_queue", do_stlpq);
    cp.add_flag('t', "tbb", "Benchmark tbb::parallel_priority_queue", do_tbbpq);

    cp.add_flag('d', "dijkstra", "Run dijkstra benchmark", do_dijkstra);
    cp.add_flag('r', "random", "Use random insert", do_random);
    cp.add_flag('p', "parallel", "Insert bulks in parallel", do_parallel);
    cp.add_flag('c', "check", "Check results", do_check);
    cp.add_flag('b', "bulk", "Use bulk insert", do_bulk);
    cp.add_flag('i', "intermixed", "Intermixed insert/delete", do_intermixed);
    cp.add_flag('f', "fill", "Fill queue before starting intermixed insert/delete (only with -i and without -r and -c together)", do_fill);
    cp.add_flag('a', "flushtohd", "Flush insertion heaps directly into external memory", do_flush_directly);

    uint64 temp_volume = 0;
    cp.add_bytes('v', "volume", "Amount of data to insert in Bytes (not recommended for STXXL PQ)", temp_volume);

    uint64 temp_ram = 0;
    cp.add_bytes('m', "ram", "Amount of main memory in Bytes (not possible for STXXL PQ)", temp_ram);

    unsigned int temp_bulk_size = 0;
    cp.add_uint('k', "bulksize", "Number of elements per bulk", temp_bulk_size);

    unsigned int temp_universe_size = 0;
    cp.add_uint('u', "universesize", "Range for random values", temp_universe_size);

    unsigned int temp_num_write_buffers = std::numeric_limits<unsigned int>::max();
    cp.add_uint('w', "writebuffers", "Number of buffered blocks when writing to external memory", num_write_buffers);

    uint64 temp_extract_buffer_ram = 0;
    cp.add_bytes('x', "extractbufferram", "Maximum memory consumption of the extract buffer in Bytes", temp_extract_buffer_ram);

    uint64 temp_single_heap_ram = 0;
    cp.add_bytes('y', "heapram", "Size of a single insertion heap in Bytes", temp_single_heap_ram);

    unsigned int temp_num_prefetchers = std::numeric_limits<unsigned int>::max();
    cp.add_uint('z', "prefetchers", "Number of prefetched blocks for each external array", temp_num_prefetchers);


    if (!cp.process(argc, argv))
        return -1;

    if (temp_volume > 0)
        volume = temp_volume;
    if (temp_ram > 0)
        RAM = temp_ram;
    if (temp_bulk_size > 0)
        bulk_ram = temp_bulk_size * value_size;
    if (temp_universe_size > 0)
        value_universe_size = temp_universe_size;
    if (temp_single_heap_ram > 0)
        single_heap_ram = temp_single_heap_ram;
    if (temp_extract_buffer_ram > 0)
        extract_buffer_ram = temp_extract_buffer_ram;
    if (temp_num_write_buffers < std::numeric_limits<unsigned int>::max())
        num_write_buffers = temp_num_write_buffers;
    if (temp_num_prefetchers < std::numeric_limits<unsigned int>::max())
        num_prefetchers = temp_num_prefetchers;


    calculate_depending_parameters();
    print_params();

    if (do_ppq) {
        #if STXXL_PARALLEL
            ppq = new ppq_type(num_prefetchers, num_write_buffers, RAM,
                num_insertion_heaps, single_heap_ram, extract_buffer_ram,
                do_flush_directly);
        #endif
    } else if (do_stxxlpq) {
        stxxlpq = new stxxlpq_type(mem_for_prefetch_pool, mem_for_write_pool);
    } else if (do_sorter) {
        stxxlsorter = new sorter_type(value_type_cmp_smaller(), RAM);
    } else if (do_stlpq) {
        stlpq = new stlpq_type;
    }

    /*
     * Run benchmarks
     */

    if (do_ppq + do_stxxlpq + do_sorter + do_stlpq + do_tbbpq == 0) {
        STXXL_MSG("Please choose a conatiner type. Use -h for help.");
        return EXIT_FAILURE;
    }
    
    #if !STXXL_PARALLEL
    
        if (do_ppq) {
            STXXL_MSG("STXXL is compiled without STXXL_PARALLEL flag. Parallel priority queue cannot be used here.");
            return EXIT_FAILURE;
        }
    
    #endif

    if (do_dijkstra) {
        uint64 n = num_elements / value_size;
        uint64 m = 100 * n;

        if (do_ppq) {
            #if STXXL_PARALLEL
                ppq_dijkstra(n, m);
                ppq_stats();
            #endif
        } else if (do_stxxlpq) {
            stxxlpq_dijkstra(n, m);
        } else if (do_sorter) {
            STXXL_MSG("Sorter not supported.");
        } else if (do_stlpq) {
            stlpq_dijkstra(n, m);
        } else if (do_tbbpq) {
            STXXL_MSG("TBB not supported yet");
        }
    } else if (do_ppq) {
        #if STXXL_PARALLEL
        stxxl::scoped_print_timer timer("Filling and reading Parallel PQ", (do_fill ? 4 : 2) * num_elements * value_size);
        if (do_bulk) {
            if (do_intermixed) {
                if (do_check) {
                    ppq_bulk_intermixed_check(do_parallel);
                } else {
                    if (do_fill) {
                        ppq_bulk_rand_insert(seed, do_parallel);
                    }
                    ppq_bulk_rand_intermixed(seed, do_fill, do_parallel);
                }
            } else {
                if (do_random) {
                    ppq_bulk_rand_insert(seed, do_parallel);
                } else {
                    ppq_bulk_insert(do_parallel);
                }
                if (do_check) {
                    STXXL_CHECK(!do_random);
                    ppq_read_check();
                } else {
                    ppq_read();
                }
            }
        } else {
            if (do_intermixed) {
                if (do_fill) {
                    ppq_rand_insert(seed);
                }
                ppq_rand_intermixed(seed, do_fill);
                ppq_stats();
                return EXIT_SUCCESS;
            }
            if (do_random) {
                ppq_rand_insert(seed);
            } else {
                ppq_insert();
            }
            if (do_random && do_check) {
                ppq_rand_read_check(seed);
            } else if (do_check) {
                ppq_read_check();
            } else {
                ppq_read();
            }
        }
        ppq_stats();
        #endif // STXXL_PARALLEL
    } else if (do_stxxlpq) {
        stxxl::scoped_print_timer timer("Filling and reading STXXL PQ", (do_fill ? 4 : 2) * num_elements * value_size);
        if (do_bulk) {
            if (do_intermixed) {
                if (do_check) {
                    stxxlpq_bulk_intermixed_check();
                } else {
                    if (do_fill) {
                        stxxlpq_bulk_rand_insert(seed);
                    }
                    stxxlpq_bulk_rand_intermixed(seed, do_fill);
                }
            } else {
                if (do_random) {
                    stxxlpq_bulk_rand_insert(seed);
                } else {
                    stxxlpq_bulk_insert();
                }
                if (do_check) {
                    STXXL_CHECK(!do_random);
                    stxxlpq_read_check();
                } else {
                    stxxlpq_read();
                }
            }
        } else {
            if (do_intermixed) {
                if (do_fill) {
                    stxxlpq_rand_insert(seed);
                }
                stxxlpq_rand_intermixed(seed, do_fill);
                return EXIT_SUCCESS;
            }
            if (do_random) {
                stxxlpq_rand_insert(seed);
            } else {
                stxxlpq_insert();
            }
            if (do_random && do_check) {
                stxxlpq_rand_read_check(seed);
            } else if (do_check) {
                stxxlpq_read_check();
            } else {
                stxxlpq_read();
            }
        }
    } else if (do_sorter) {
        stxxl::scoped_print_timer timer("Filling and reading STXXL Sorter", (do_fill ? 4 : 2) * num_elements * value_size);
        if (do_bulk) {
            if (do_intermixed) {
                if (do_check) {
                    stxxlsorter_bulk_intermixed_check();
                } else {
                    if (do_fill) {
                        stxxlsorter_bulk_rand_insert(seed);
                    }
                    stxxlsorter_bulk_rand_intermixed(seed, do_fill);
                }
            } else {
                if (do_random) {
                    stxxlsorter_bulk_rand_insert(seed);
                } else {
                    stxxlsorter_bulk_insert();
                }
                if (do_check) {
                    STXXL_CHECK(!do_random);
                    stxxlsorter_read_check();
                } else {
                    stxxlsorter_read();
                }
            }
        } else {
            if (do_intermixed) {
                if (do_fill) {
                    stxxlsorter_rand_insert(seed);
                }
                stxxlsorter_rand_intermixed(seed, do_fill);
                return EXIT_SUCCESS;
            }
            if (do_random) {
                stxxlsorter_rand_insert(seed);
            } else {
                stxxlsorter_insert();
            }
            if (do_random && do_check) {
                stxxlsorter_rand_read_check(seed);
            } else if (do_check) {
                stxxlsorter_read_check();
            } else {
                stxxlsorter_read();
            }
        }
    } else if (do_stlpq) {
        stxxl::scoped_print_timer timer("Filling and reading STL PQ", (do_fill ? 4 : 2) * num_elements * value_size);
        if (do_bulk) {
            if (do_intermixed) {
                if (do_check) {
                    stlpq_bulk_intermixed_check();
                } else {
                    if (do_fill) {
                        stlpq_bulk_rand_insert(seed);
                    }
                    stlpq_bulk_rand_intermixed(seed, do_fill);
                }
            } else {
                if (do_random) {
                    stlpq_bulk_rand_insert(seed);
                } else {
                    stlpq_bulk_insert();
                }
                if (do_check) {
                    STXXL_CHECK(!do_random);
                    stlpq_read_check();
                } else {
                    stlpq_read();
                }
            }
        } else {
            if (do_intermixed) {
                if (do_fill) {
                    stlpq_rand_insert(seed);
                }
                stlpq_rand_intermixed(seed, do_fill);
                return EXIT_SUCCESS;
            }
            if (do_random) {
                stlpq_rand_insert(seed);
            } else {
                stlpq_insert();
            }
            if (do_random && do_check) {
                stlpq_rand_read_check(seed);
            } else if (do_check) {
                stlpq_read_check();
            } else {
                stlpq_read();
            }
        }
    } else if (do_tbbpq) {
        STXXL_MSG("TBB not supported yet");
    }

    return EXIT_SUCCESS;
}
