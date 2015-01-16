/***************************************************************************
 *  tools/benchmark_pqs.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
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
#include <stxxl/bits/unused.h>
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

using stxxl::scoped_print_timer;

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

uint64 value_universe_size = 100000000;

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

    value_type(const key_type& _key = 0, const key_type& _data = 0)
        : first(_key), second(_data)
    {
#ifdef STXXL_VALGRIND_AVOID_UNINITIALIZED_WRITE_ERRORS
        memset(data, 0, sizeof(data));
#endif
    }

    friend std::ostream& operator << (std::ostream& o, const value_type& obj)
    {
        o << obj.first;
        return o;
    }
};

struct value_type_cmp_smaller
    : public std::binary_function<value_type, value_type, bool>
{
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
        return std::numeric_limits<value_type::key_type>::max();
    }
};

struct value_type_cmp_greater
    : public std::binary_function<value_type, value_type, bool>
{
    bool operator () (const value_type& a, const value_type& b) const
    {
        return a.first > b.first;
    }
    value_type min_value() const
    {
        return std::numeric_limits<value_type::key_type>::max();
    }
};

template <typename ValueType>
struct less_min_max : public std::binary_function<ValueType, ValueType, bool>
{
    bool operator () (const ValueType& a, const ValueType& b) const
    {
        return a < b;
    }
    ValueType min_value() const
    {
        return std::numeric_limits<ValueType>::min();
    }
    ValueType max_value() const
    {
        return std::numeric_limits<ValueType>::max();
    }
};

/*
 * Progress messages
 */

static const uint64 printmod = 16 * MiB;
static inline void progress(const char* text, uint64 i, uint64 nelements)
{
    if ((i % printmod) == 0) {
        STXXL_MSG(text << " " << i << " (" << std::setprecision(5) << (static_cast<double>(i) * 100. / static_cast<double>(nelements)) << " %)");
    }
}

/*!
 * Abstract Container class with template specialized functions for sorter and
 * more.
 */
template <typename BackendType>
class Container
{
public:
    static std::string name();

    BackendType& backend;

    Container(BackendType& b)
        : backend(b)
    { }

    //! Simple single push() operation
    void push(const value_type& v)
    {
        backend.push(v);
    }

    //! Simple single pop() operation
    void pop()
    {
        backend.pop();
    }

    //! Simple single top() and pop() operation.
    value_type top_pop()
    {
        value_type top = backend.top();
        backend.pop();
        return top;
    }

    //! Test if the container is empty.
    bool empty() const
    {
        return backend.empty();
    }

    //! Return number of items still in container.
    size_t size() const
    {
        return backend.size();
    }

    void bulk_push_begin(size_t /* this_bulk_size */)
    {
        std::cout << name() << ": bulk_push_begin not supported\n";
    }

    void bulk_push_end()
    {
        std::cout << name() << ": bulk_push_end not supported\n";
    }

    void bulk_push(const value_type& v, int /* thread_num */)
    {
        backend.push(v);
    }

    //! Output additional stats
    void print_stats()
    { }
};

// *** Specialization for STXXL PQ

typedef stxxl::PRIORITY_QUEUE_GENERATOR<
        value_type,
        value_type_cmp_greater,
        mem_for_queue,
        _num_elements / 1024> stxxlpq_generator_type;

typedef stxxlpq_generator_type::result stxxlpq_type;

template <>
std::string Container<stxxlpq_type>::name()
{
    return "STXXL PQ";
}

// *** Specialization for STXXL Parallel PQ

#if STXXL_PARALLEL

typedef stxxl::parallel_priority_queue<value_type, value_type_cmp_greater> ppq_type;

template <>
std::string Container<ppq_type>::name()
{
    return "Parallel PQ";
}

template <>
void Container<ppq_type>::bulk_push_begin(size_t this_bulk_size)
{
    backend.bulk_push_begin(this_bulk_size);
}

template <>
void Container<ppq_type>::bulk_push_end()
{
    backend.bulk_push_end();
}

template <>
void Container<ppq_type>::bulk_push(const value_type& v, int thread_num)
{
    backend.bulk_push(v, thread_num);
}

template <>
void Container<ppq_type>::print_stats()
{
    backend.print_stats();
}

#endif

// *** Specialization for STL PQ

typedef std::priority_queue<value_type, std::vector<value_type>, value_type_cmp_greater> stlpq_type;

template <>
std::string Container<stlpq_type>::name()
{
    return "STL PQ";
}

typedef stxxl::sorter<value_type, value_type_cmp_smaller> sorter_type;

// *** Specialization for STL Sorter

template <>
std::string Container<sorter_type>::name()
{
    return "STXXL Sorter";
}

template <>
void Container<sorter_type>::pop()
{
    ++backend;
}

template <>
value_type Container<sorter_type>::top_pop()
{
    value_type top = *backend;
    ++backend;
    return top;
}

/*
 * Benchmark Functions
 */

template <typename ContainerType>
void do_insert(ContainerType& c)
{
    scoped_print_timer timer("Filling " + c.name() + " sequentially",
                             num_elements * value_size);

    for (uint64 i = 0; i < num_elements; i++)
    {
        progress("Inserting element", i, num_elements);
        c.push(value_type(num_elements - i));
    }
}

template <typename ContainerType>
void do_rand_insert(ContainerType& c, unsigned int seed)
{
    scoped_print_timer timer("Filling " + c.name() + " randomly",
                             num_elements * value_size);

    for (uint64 i = 0; i < num_elements; i++)
    {
        uint64 k = rand_r(&seed) % value_universe_size;
        progress("Inserting element", i, num_elements);
        c.push(value_type(k));
    }
}

template <typename ContainerType>
void do_read(ContainerType& c)
{
    scoped_print_timer timer("Reading " + c.name(),
                             num_elements * value_size);

    STXXL_CHECK_EQUAL(c.size(), num_elements);

    for (uint64 i = 0; i < num_elements; ++i)
    {
        STXXL_CHECK(!c.empty());
        STXXL_CHECK_EQUAL(c.size(), num_elements - i);
        c.pop();
        progress("Popped element", i, num_elements);
    }
}

template <typename ContainerType>
void do_read_check(ContainerType& c)
{
    scoped_print_timer timer("Reading " + c.name() + " and checking order",
                             num_elements * value_size);

    STXXL_CHECK_EQUAL(c.size(), num_elements);

    for (uint64 i = 0; i < num_elements; ++i)
    {
        STXXL_CHECK(!c.empty());
        STXXL_CHECK_EQUAL(c.size(), num_elements - i);

        value_type top = c.top_pop();

        STXXL_CHECK_EQUAL(top.first, i + 1);
        progress("Popped element", i, num_elements);
    }
}

template <typename ContainerType>
void do_rand_read_check(ContainerType& c, unsigned int seed)
{
    stxxl::sorter<uint64, less_min_max<uint64> >
    sorted_vals(less_min_max<uint64>(), RAM / 2);

    {
        scoped_print_timer timer("Filling sorter for comparison");

        for (uint64 i = 0; i < num_elements; ++i)
        {
            uint64 k = rand_r(&seed) % value_universe_size;
            sorted_vals.push(k);
        }
    }
    sorted_vals.sort();

    {
        scoped_print_timer timer("Reading " + c.name() + " and check order",
                                 num_elements * value_size);

        for (uint64 i = 0; i < num_elements; ++i)
        {
            STXXL_CHECK(!c.empty());
            STXXL_CHECK_EQUAL(c.size(), num_elements - i);
            STXXL_CHECK_EQUAL(c.size(), sorted_vals.size());

            value_type top = c.top_pop();

            STXXL_CHECK_EQUAL(top.first, *sorted_vals);
            progress("Popped element", i, num_elements);
            ++sorted_vals;
        }
    }
}

template <typename ContainerType>
void do_bulk_rand_read_check(ContainerType& c, unsigned int _seed,
                             bool parallel)
{
    stxxl::sorter<uint64, less_min_max<uint64> >
    sorted_vals(less_min_max<uint64>(), RAM / 2);

    {
        uint64 bulk_step = bulk_size / omp_get_max_threads();

        scoped_print_timer timer("Filling sorter for comparison",
                                 num_elements * value_size);

        for (uint64_t i = 0; i < num_elements / bulk_size; ++i)
        {
#if STXXL_PARALLEL
            for (int thr = 0; thr < omp_get_max_threads(); ++thr)
#endif
            {
#if !STXXL_PARALLEL
                const unsigned thread_id = 0;
                stxxl::STXXL_UNUSED(_seed);
#else
                const unsigned thread_id = parallel
                                           ? thr
                                           : rand() % num_insertion_heaps;

                unsigned int seed = static_cast<unsigned>(i) * _seed * thread_id;
#endif
                for (uint64 j = thread_id * bulk_step;
                     j < std::min((thread_id + 1) * bulk_step, bulk_size); ++j)
                {
                    uint64 k = rand_r(&seed) % value_universe_size;
                    sorted_vals.push(k);
                }
            }

            progress("Inserting element", i * bulk_size, num_elements);
        }

        STXXL_CHECK_EQUAL(sorted_vals.size(), num_elements - (num_elements % bulk_size));

        uint64 bulk_remain = num_elements % bulk_size;
        bulk_step = (bulk_remain + omp_get_max_threads() - 1) / omp_get_max_threads();

#if STXXL_PARALLEL
        for (int thr = 0; thr < omp_get_max_threads(); ++thr)
#endif
        {
#if !STXXL_PARALLEL
            const unsigned thread_id = 0;
#else
            const unsigned thread_id = parallel
                                       ? thr
                                       : rand() % num_insertion_heaps;

            unsigned int seed = static_cast<unsigned>(num_elements / bulk_size) * _seed * thread_id;
#endif
            for (uint64 j = thread_id * bulk_step;
                 j < std::min((thread_id + 1) * bulk_step, bulk_remain); ++j)
            {
                uint64 k = rand_r(&seed) % value_universe_size;
                sorted_vals.push(k);
            }
        }

        progress("Inserting element", num_elements, num_elements);
    }

    sorted_vals.sort();
    STXXL_CHECK_EQUAL(c.size(), sorted_vals.size());

    {
        scoped_print_timer timer("Reading " + c.name() + " and check order",
                                 num_elements * value_size);

        for (uint64 i = 0; i < num_elements; ++i)
        {
            STXXL_CHECK(!c.empty());
            STXXL_CHECK_EQUAL(c.size(), num_elements - i);
            STXXL_CHECK_EQUAL(c.size(), sorted_vals.size());

            value_type top = c.top_pop();

            //STXXL_MSG("val[" << i << "] = " << top.first << " - " << *sorted_vals);

            STXXL_CHECK_EQUAL(top.first, *sorted_vals);
            progress("Popped element", i, num_elements);
            ++sorted_vals;
        }
    }
}

template <typename ContainerType>
void do_rand_intermixed(ContainerType& c, unsigned int _seed, bool filled)
{
    uint64 num_inserts = filled ? num_elements : 0;
    uint64 num_deletes = 0;

    scoped_print_timer timer(c.name() + ": Intermixed rand insert and delete",
                             2 * num_elements * value_size);

    for (uint64_t i = 0; i < 2 * num_elements; ++i)
    {
        unsigned r = rand_r(&_seed) % 2;

        if (num_deletes < num_inserts &&
            (r > 0 || num_inserts >= (filled ? 2 * num_elements : num_elements)))
        {
            STXXL_CHECK(!c.empty());

            c.pop();

            progress("Deleting / inserting element", i, 2 * num_elements);
            ++num_deletes;
            STXXL_CHECK_EQUAL(c.size(), num_inserts - num_deletes);
        }
        else
        {
            progress("Inserting/deleting element", i, 2 * num_elements);

            uint64 k = rand_r(&_seed) % value_universe_size;
            c.push(value_type(k));

            num_inserts++;
            STXXL_CHECK_EQUAL(c.size(), num_inserts - num_deletes);
        }
    }
}

template <typename ContainerType>
void do_bulk_insert(ContainerType& c, bool parallel)
{
    std::string parallel_str = parallel ? " in parallel" : "";

    scoped_print_timer timer("Filling " + c.name() + " with bulks" + parallel_str,
                             num_elements * value_size);

    for (uint64_t i = 0; i < num_elements / bulk_size; ++i)
    {
        c.bulk_push_begin(bulk_size);

#if STXXL_PARALLEL
#pragma omp parallel if(parallel)
#endif
        {
#if !STXXL_PARALLEL
            const unsigned thread_id = 0;
#else
            const unsigned thread_id = parallel
                                       ? omp_get_thread_num()
                                       : rand() % num_insertion_heaps;

            #pragma omp for
#endif
            for (uint64 j = 0; j < bulk_size; ++j)
            {
                c.bulk_push(value_type(num_elements - (i * bulk_size + j)), thread_id);
            }
        }

        c.bulk_push_end();

        progress("Inserting element", i * bulk_size, num_elements);
    }

    c.bulk_push_begin(num_elements % bulk_size);

#if STXXL_PARALLEL
#pragma omp parallel if(parallel)
#endif
    {
#if !STXXL_PARALLEL
        const unsigned thread_id = 0;
#else
        const unsigned thread_id = parallel
                                   ? omp_get_thread_num()
                                   : rand() % num_insertion_heaps;

        #pragma omp for
#endif
        for (uint64 j = 0; j < num_elements % bulk_size; j++)
        {
            c.bulk_push(value_type(num_elements % bulk_size - j), thread_id);
        }
    }

    c.bulk_push_end();

    progress("Inserting element", num_elements, num_elements);
}

template <typename ContainerType>
void do_bulk_rand_insert(ContainerType& c,
                         unsigned int _seed, bool parallel)
{
    std::string parallel_str = parallel ? " in parallel" : "";

    scoped_print_timer timer("Filling " + c.name() + " with bulks" + parallel_str,
                             num_elements * value_size);

    uint64 bulk_step = bulk_size / omp_get_max_threads();

    for (uint64_t i = 0; i < num_elements / bulk_size; ++i)
    {
        c.bulk_push_begin(bulk_size);

#if STXXL_PARALLEL
#pragma omp parallel if(parallel)
#endif
        {
#if !STXXL_PARALLEL
            const unsigned thread_id = 0;
            stxxl::STXXL_UNUSED(_seed);
#else
            const unsigned thread_id = parallel
                                       ? omp_get_thread_num()
                                       : rand() % num_insertion_heaps;

            unsigned int seed = static_cast<unsigned>(i) * _seed * thread_id;
#endif
            for (uint64 j = thread_id * bulk_step;
                 j < std::min((thread_id + 1) * bulk_step, bulk_size); ++j)
            {
                uint64 k = rand_r(&seed) % value_universe_size;
                c.bulk_push(value_type(k), thread_id);
            }
        }

        c.bulk_push_end();

        progress("Inserting element", i * bulk_size, num_elements);
    }

    STXXL_CHECK_EQUAL(c.size(), num_elements - (num_elements % bulk_size));

    uint64 bulk_remain = num_elements % bulk_size;
    bulk_step = (bulk_remain + omp_get_max_threads() - 1) / omp_get_max_threads();

    c.bulk_push_begin(bulk_remain);

#if STXXL_PARALLEL
#pragma omp parallel if(parallel)
#endif
    {
#if !STXXL_PARALLEL
        const unsigned thread_id = 0;
#else
        const unsigned thread_id = parallel
                                   ? omp_get_thread_num()
                                   : rand() % num_insertion_heaps;

        unsigned int seed = static_cast<unsigned>(num_elements / bulk_size) * _seed * thread_id;
#endif
        for (uint64 j = thread_id * bulk_step;
             j < std::min((thread_id + 1) * bulk_step, bulk_remain); ++j)
        {
            uint64 k = rand_r(&seed) % value_universe_size;
            c.bulk_push(value_type(k), thread_id);
        }
    }

    c.bulk_push_end();

    progress("Inserting element", num_elements, num_elements);

    STXXL_CHECK_EQUAL(c.size(), num_elements);
}

template <typename ContainerType>
void do_bulk_rand_intermixed(ContainerType& c,
                             unsigned int _seed, bool filled,
                             bool parallel)
{
    std::string parallel_str = parallel ? " in parallel" : "";

    uint64 num_inserts = filled ? num_elements : 0;
    uint64 num_deletes = 0;

    scoped_print_timer timer(c.name() + ": Intermixed parallel rand bulk insert" + parallel_str + " and delete",
                             (filled ? 3 : 2) * num_elements * value_size);

    for (uint64_t i = 0; i < (filled ? 3 : 2) * num_elements; ++i)
    {
        uint64 r = rand() % ((filled ? 2 : 1) * bulk_size);

        // probability for an extract is bulk_size times larger than for a
        // bulk_insert.  when already filled with num_elements elements:
        // probability for an extract is 2*bulk_size times larger than for
        // a bulk_insert.
        if (num_deletes < num_inserts &&
            num_deletes < (filled ? 2 : 1) * num_elements &&
            (r > 0 || num_inserts >= (filled ? 2 : 1) * num_elements))
        {
            STXXL_CHECK(!c.empty());

            c.top_pop();

            progress("Deleting / inserting element", i, (filled ? 3 : 2) * num_elements);
            ++num_deletes;
            STXXL_CHECK_EQUAL(c.size(), num_inserts - num_deletes);
        }
        else {
            uint64 this_bulk_size =
                (num_inserts + bulk_size > (filled ? 2 : 1) * num_elements)
                ? num_elements % bulk_size
                : bulk_size;

            c.bulk_push_begin(this_bulk_size);

#if STXXL_PARALLEL
#pragma omp parallel if(parallel)
#endif
            {
#if !STXXL_PARALLEL
                const unsigned thread_num = 0;
                stxxl::STXXL_UNUSED(_seed);
#else
                const unsigned int thread_num = parallel
                                                ? omp_get_thread_num()
                                                : rand() % num_insertion_heaps;

                unsigned int seed = thread_num * static_cast<unsigned>(i) * _seed;
                #pragma omp for
#endif
                for (uint64 j = 0; j < this_bulk_size; j++)
                {
                    progress("Inserting / deleting element",
                             i + j, (filled ? 3 : 2) * num_elements);

                    uint64 k = rand_r(&seed) % value_universe_size;
                    c.bulk_push(value_type(k), thread_num);
                }
            }

            c.bulk_push_end();

            num_inserts += this_bulk_size;
            i += this_bulk_size - 1;
            STXXL_CHECK_EQUAL(c.size(), num_inserts - num_deletes);
        }
    }
}

template <typename ContainerType>
void do_bulk_intermixed_check(ContainerType& c, bool parallel)
{
    std::string parallel_str = parallel ? " in parallel" : "";

    uint64 num_inserts = 0, num_deletes = 0, num_failures = 0;
    std::vector<bool> extracted_values(num_elements, false);

    {
        scoped_print_timer timer(c.name() + ": Intermixed parallel bulk insert" + parallel_str + " and delete",
                                 2 * num_elements * value_size);

        for (uint64_t i = 0; i < 2 * num_elements; ++i)
        {
            uint64 r = rand() % bulk_size;

            if (num_deletes < num_inserts && num_deletes < num_elements &&
                (r > 0 || num_inserts >= num_elements))
            {
                STXXL_CHECK(!c.empty());

                value_type top = c.top_pop();

                if (top.first < 1 || top.first > num_elements) {
                    std::cout << top.first << " has never been inserted." << std::endl;
                    ++num_failures;
                }
                else if (extracted_values[top.first - 1]) {
                    std::cout << top.first << " has already been extracted." << std::endl;
                    ++num_failures;
                }
                else {
                    extracted_values[top.first - 1] = true;
                }

                progress("Deleting / inserting element", i, 2 * num_elements);
                ++num_deletes;
                STXXL_CHECK_EQUAL(c.size(), num_inserts - num_deletes);
            }
            else
            {
                uint64 this_bulk_size = (num_inserts + bulk_size > num_elements)
                                        ? num_elements % bulk_size
                                        : bulk_size;

                c.bulk_push_begin(this_bulk_size);
#if STXXL_PARALLEL
#pragma omp parallel if(parallel)
#endif
                {
#if !STXXL_PARALLEL
                    const unsigned int thread_num = 0;
#else
                    const unsigned int thread_num = parallel
                                                    ? omp_get_thread_num()
                                                    : rand() % num_insertion_heaps;
                    #pragma omp for
#endif

                    for (uint64 j = 0; j < this_bulk_size; j++)
                    {
                        uint64 k = num_elements - num_inserts - j;
                        progress("Inserting/deleting element", i + j, 2 * num_elements);

                        c.bulk_push(value_type(k), thread_num);
                    }
                }

                c.bulk_push_end();

                num_inserts += this_bulk_size;
                i += this_bulk_size - 1;
                STXXL_CHECK_EQUAL(c.size(), num_inserts - num_deletes);
            }
        }
    }
    {
        scoped_print_timer timer("Check if all values have been extracted");
        for (uint64 i = 0; i < num_elements; ++i) {
            if (!extracted_values[i]) {
                std::cout << i + 1 << " has never been extracted." << std::endl;
                ++num_failures;
            }
        }
    }
    std::cout << num_failures << " failures." << std::endl;
}

/*
 * Dijkstra Graph SSP Algorithm with different PQs on a random graph.
 */

template <typename ContainerType>
class dijkstra_graph
{
public:
    typedef uint64 size_type;

protected:
    size_type n, m;
    std::vector<size_type> nodes;
    std::vector<bool> node_visited;
    std::vector<size_type> edge_targets;
    std::vector<size_type> edge_lengths;
    std::vector<std::vector<std::pair<size_type, size_type> > > temp_edges;

public:
    //! construct a random graph for applying Dijkstra's SSP algorithm
    dijkstra_graph(size_type n, size_type m)
        : n(n),
          m(m),
          nodes(n + 1),
          node_visited(n, false),
          edge_targets(m),
          edge_lengths(m),
          temp_edges(n)
    {
        unsigned int seed = 12345;

        for (size_type i = 0; i < m; ++i) {
            size_type source = rand_r(&seed) % n;
            size_type target = rand_r(&seed) % n;
            size_type length = rand_r(&seed);
            temp_edges[source].push_back(std::make_pair(target, length));
        }

        size_type offset = 0;

        for (size_type i = 0; i < n; ++i) {
            nodes[i] = offset;

            for (size_type j = 0; j < temp_edges[i].size(); ++j) {
                edge_targets[offset + j] = temp_edges[i][j].first;
                edge_lengths[offset + j] = temp_edges[i][j].second;
            }

            offset += temp_edges[i].size();
        }

        nodes[n] = offset;
        temp_edges.clear();
    }

    //! run using container c
    void run_random_dijkstra(ContainerType& c)
    {
        unsigned int seed = 678910;
        size_type source = rand_r(&seed) % n;
        c.push(value_type(source, 0));

        while (!c.empty())
        {
            value_type top = c.top_pop();

            size_type target = top.first;
            size_type distance = top.second;

            if (node_visited[target]) {
                continue;
            }
            else {
                node_visited[target] = true;
            }

            size_type edges_begin = nodes[target];
            size_type edges_end = nodes[target + 1];

            size_type num_edges = edges_end - edges_begin;
            c.bulk_push_begin(num_edges);

#if STXXL_PARALLEL
#pragma omp parallel
#endif
            {
#if !STXXL_PARALLEL
                const unsigned thread_num = 0;
#else
                const unsigned thread_num = omp_get_thread_num();
                #pragma omp for
#endif
                for (size_type i = edges_begin; i < edges_end; ++i)
                {
                    c.bulk_push(value_type(edge_targets[i], distance + edge_lengths[i]),
                                thread_num);
                }
            }

            c.bulk_push_end();
        }
    }
};

template <typename ContainerType>
void do_dijkstra(ContainerType& c, uint64 num_nodes, uint64 num_edges)
{
    std::cout << "Generating Graph" << std::endl;

    dijkstra_graph<ContainerType> dg(num_nodes, num_edges);

    scoped_print_timer timer("Running random dijkstra",
                             num_elements * value_size);
    dg.run_random_dijkstra(c);
}

/*
 * Defining priority queues.
 */

stxxlpq_type* stxxlpq;

#if STXXL_PARALLEL
ppq_type* ppq;
#endif

sorter_type* stxxlsorter;

stlpq_type* stlpq;

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

/*
 * Run the selected benchmark procedure.
 */

bool do_random = false;
bool do_check = false;
bool do_bulk = false;
bool do_intermixed = false;
bool do_parallel = false;
bool do_prefill = false;
bool do_no_read = false;

template <typename ContainerType>
void run_benchmark(ContainerType& c)
{
    scoped_print_timer timer("Running selected benchmarks on " + c.name(),
                             (do_prefill ? 4 : 2) * num_elements * value_size);

    if (do_bulk) {
        if (do_intermixed) {
            if (do_check) {
                do_bulk_intermixed_check(c, do_parallel);
            }
            else {
                if (do_prefill) {
                    do_bulk_rand_insert(c, seed, do_parallel);
                }
                do_bulk_rand_intermixed(c, seed, do_prefill, do_parallel);
            }
        }
        else {
            if (do_random) {
                do_bulk_rand_insert(c, seed, do_parallel);
            }
            else {
                do_bulk_insert(c, do_parallel);
            }
            if (do_random && do_check) {
                do_bulk_rand_read_check(c, seed, do_parallel);
            }
            else if (do_check) {
                do_read_check(c);
            }
            else if (!do_no_read) {
                do_read(c);
            }
        }
    }
    else {
        if (do_intermixed) {
            if (do_prefill) {
                do_rand_insert(c, seed);
            }
            do_rand_intermixed(c, seed, do_prefill);
        }
        else {
            if (do_random) {
                do_rand_insert(c, seed);
            }
            else {
                do_insert(c);
            }
            if (do_random && do_check) {
                do_rand_read_check(c, seed);
            }
            else if (do_check) {
                do_read_check(c);
            }
            else if (!do_no_read) {
                do_read(c);
            }
        }
    }
    c.print_stats();
}

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

    bool do_dijkstra = false;

    stxxl::cmdline_parser cp;
    cp.set_description(description);

    cp.add_flag('n', "ppq", "Benchmark parallel priority queue", do_ppq);
    cp.add_flag('o', "stxxl", "Benchmark stxxl priority queue", do_stxxlpq);
    cp.add_flag('s', "sorter", "Benchmark stxxl::sorter", do_sorter);
    cp.add_flag('l', "stl", "Benchmark std::priority_queue", do_stlpq);

    cp.add_flag('d', "dijkstra", "Run dijkstra benchmark", do_dijkstra);
    cp.add_flag('r', "random", "Use random insert", do_random);
    cp.add_flag('p', "parallel", "Insert bulks in parallel", do_parallel);
    cp.add_flag('c', "check", "Check results", do_check);
    cp.add_flag('b', "bulk", "Use bulk insert", do_bulk);
    cp.add_flag('i', "intermixed", "Intermixed insert/delete", do_intermixed);

    cp.add_flag(0, "prefill", "Prefill queue before starting intermixed insert/delete (only with -i and without -r and -c together)", do_prefill);
    cp.add_flag(0, "no-read", "Do not read items from queue after insert", do_no_read);

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
        ppq = new ppq_type(value_type_cmp_greater(),
                           RAM, num_prefetchers, num_write_buffers,
                           num_insertion_heaps, single_heap_ram, extract_buffer_ram);
#endif
    }
    else if (do_stxxlpq) {
        stxxlpq = new stxxlpq_type(mem_for_prefetch_pool, mem_for_write_pool);
    }
    else if (do_sorter) {
        stxxlsorter = new sorter_type(value_type_cmp_smaller(), RAM);
    }
    else if (do_stlpq) {
        stlpq = new stlpq_type;
    }

    /*
     * Run benchmarks
     */

    if (do_ppq + do_stxxlpq + do_sorter + do_stlpq == 0) {
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
            Container<ppq_type> cppq(*ppq);
            ::do_dijkstra(cppq, n, m);
            cppq.print_stats();
#endif
        }
        else if (do_stxxlpq) {
            Container<stxxlpq_type> cstxxlpq(*stxxlpq);
            ::do_dijkstra(cstxxlpq, n, m);
        }
        else if (do_sorter) {
            STXXL_MSG("Sorter not supported.");
        }
        else if (do_stlpq) {
            Container<stlpq_type> cstlpq(*stlpq);
            ::do_dijkstra(cstlpq, n, m);
        }
    }
    else if (do_ppq)
    {
#if STXXL_PARALLEL
        Container<ppq_type> cppq(*ppq);
        run_benchmark(cppq);
#endif  // STXXL_PARALLEL
    }
    else if (do_stxxlpq)
    {
        Container<stxxlpq_type> cstxxlpq(*stxxlpq);
        run_benchmark(cstxxlpq);
    }
    else if (do_sorter)
    {
        Container<sorter_type> csorter(*stxxlsorter);
        run_benchmark(csorter);
    }
    else if (do_stlpq)
    {
        Container<stlpq_type> cstlpq(*stlpq);
        run_benchmark(cstlpq);
    }

    if (do_ppq) {
#if STXXL_PARALLEL
        delete ppq;
#endif
    }
    else if (do_stxxlpq) {
        delete stxxlpq;
    }
    else if (do_sorter) {
        delete stxxlsorter;
    }
    else if (do_stlpq) {
        delete stlpq;
    }

    return EXIT_SUCCESS;
}
