/***************************************************************************
 *  tools/benchmark_pqs_helper.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

/*
 * Macros for each supported priority CONTAINER
 */

#if defined(PPQ)
        #undef PREFIX
        #define PREFIX(name) ppq_ ## name
        #undef NAME
        #define NAME "Parallel PQ"
        #undef CONTAINER
        #define CONTAINER ppq
#elif defined(STXXLPQ)
        #undef PREFIX
        #define PREFIX(name) stxxlpq_ ## name
        #undef NAME
        #define NAME "STXXL PQ"
        #undef CONTAINER
        #define CONTAINER stxxlpq
#elif defined(STLPQ)
        #undef PREFIX
        #define PREFIX(name) stlpq_ ## name
        #undef NAME
        #define NAME "STL PQ"
        #undef CONTAINER
        #define CONTAINER stlpq
#elif defined(TBBPQ)
        #undef PREFIX
        #define PREFIX(name) tbbpq_ ## name
        #undef NAME
        #define NAME "TBB PQ"
        #undef CONTAINER
        #define CONTAINER tbbpq
#elif defined(STXXLSORTER)
        #undef PREFIX
        #define PREFIX(name) stxxlsorter_ ## name
        #undef NAME
        #define NAME "STXXL Sorter"
        #undef CONTAINER
        #define CONTAINER stxxlsorter
#endif

#include <vector>
#include <utility>

class PREFIX(dijkstra_graph) {
public:
    typedef uint64_t size_type;

    PREFIX(dijkstra_graph) (size_type n, size_type m) :
        n(n),
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

    void run_random_dijkstra()
    {
        unsigned int seed = 678910;
        size_type source = rand_r(&seed) % n;
        CONTAINER->push(value_type(source, 0));

        while (!CONTAINER->empty()) {
            #if defined(STXXLSORTER)
            value_type top = *(*CONTAINER);
            ++(*CONTAINER);
            #else
            value_type top = CONTAINER->top();
            CONTAINER->pop();
            #endif

            size_type target = top.first;
            size_type distance = top.second;

            if (node_visited[target]) {
                continue;
            } else {
                node_visited[target] = true;
            }

            size_type edges_begin = nodes[target];
            size_type edges_end = nodes[target + 1];

#if defined(PPQ)
            size_type num_edges = edges_end - edges_begin;
            CONTAINER->bulk_push_begin(num_edges);
            #pragma omp parallel
            {
                //unsigned thread_id = omp_get_thread_num();
            #pragma omp for
#endif
            for (size_type i = edges_begin; i < edges_end; ++i) {
                #if defined(PPQ)
                CONTAINER->bulk_push_step(value_type(edge_targets[i], distance + edge_lengths[i]));                                 //, thread_id);
                #else
                CONTAINER->push(value_type(edge_targets[i], distance + edge_lengths[i]));
                #endif
            }
#if defined(PPQ)
        }
        CONTAINER->bulk_push_end();
#endif
        }
    }

private:
    size_type n, m;
    std::vector<size_type> nodes;
    std::vector<bool> node_visited;
    std::vector<size_type> edge_targets;
    std::vector<size_type> edge_lengths;
    std::vector<std::vector<std::pair<size_type, size_type> > > temp_edges;
};

/*
 * The benchmarks.
 */

void PREFIX(dijkstra) (stxxl::uint64 num_nodes, stxxl::uint64 num_edges)
{
    std::cout << "Generating Graph" << std::endl;
    PREFIX(dijkstra_graph) dg(num_nodes, num_edges);
    {
        stxxl::scoped_print_timer timer("Running random dijkstra", num_elements * value_size);
        dg.run_random_dijkstra();
    }
}

void PREFIX(insert) ()
{
    stxxl::scoped_print_timer timer("Filling " NAME " sequentially", num_elements * value_size);
    for (stxxl::uint64 i = 0; i < num_elements; i++) {
        progress("Inserting element", i, num_elements);
        CONTAINER->push(value_type(num_elements - i));
    }
}

void PREFIX(rand_insert) (unsigned int seed)
{
    stxxl::scoped_print_timer timer("Filling " NAME " randomly", num_elements * value_size);
    for (stxxl::uint64 i = 0; i < num_elements; i++) {
        uint32 k = rand_r(&seed) % value_universe_size;
        progress("Inserting element", i, num_elements);
        CONTAINER->push(value_type(k));
    }
}

void PREFIX(read) ()
{
    stxxl::scoped_print_timer timer("Reading " NAME, num_elements * value_size);
    for (stxxl::uint64 i = 0; i < num_elements; ++i) {
        STXXL_CHECK(!CONTAINER->empty());

#if defined(STXXLSORTER)
        value_type top = *(*CONTAINER);
        (void)top;
        ++(*CONTAINER);
#else
        CONTAINER->top();
        CONTAINER->pop();
#endif

        progress("Popped element", i, num_elements);
    }
}

void PREFIX(read_check) ()
{
    stxxl::scoped_print_timer timer("Reading " NAME " and check order", num_elements * value_size);
    for (stxxl::uint64 i = 0; i < num_elements; ++i) {
        STXXL_CHECK(!CONTAINER->empty());

#if defined(STXXLSORTER)
        value_type top = *(*CONTAINER);
        ++(*CONTAINER);
#else
        value_type top = CONTAINER->top();
        CONTAINER->pop();
#endif

        STXXL_CHECK_EQUAL(top.first, i + 1);
        progress("Popped element", i, num_elements);
    }
}

void PREFIX(rand_read_check) (unsigned int seed)
{
    std::vector<uint32> vals;
    vals.reserve(num_elements);
    {
        stxxl::scoped_print_timer timer("Filling value vector for comarison");
        for (stxxl::uint64 i = 0; i < num_elements; ++i) {
            uint32 k = rand_r(&seed) % value_universe_size;
            vals.push_back(k);
        }
    }
    {
        stxxl::scoped_print_timer timer("Sorting value vector for comarison");
        __gnu_parallel::sort(vals.begin(), vals.end());
    }
    {
        stxxl::scoped_print_timer timer("Reading " NAME " and check order", num_elements * value_size);
        for (stxxl::uint64 i = 0; i < num_elements; ++i) {
            STXXL_CHECK(!CONTAINER->empty());

#if defined(STXXLSORTER)
            value_type top = *(*CONTAINER);
            ++(*CONTAINER);
#else
            value_type top = CONTAINER->top();
            CONTAINER->pop();
#endif

            STXXL_CHECK_EQUAL(top.first, vals[i]);
            progress("Popped element", i, num_elements);
        }
    }
}

void PREFIX(rand_intermixed) (unsigned int _seed, bool filled)
{
    uint64 num_inserts = filled ? num_elements : 0;
    uint64 num_deletes = 0;
    {
        stxxl::scoped_print_timer timer(NAME ": Intermixed rand insert and delete", 2 * num_elements * value_size);
        for (uint64_t i = 0; i < 2 * num_elements; ++i) {
            unsigned r = rand_r(&_seed) % 2;
            if (num_deletes < num_inserts && (r > 0 || num_inserts >= (filled ? 2 * num_elements : num_elements))) {
                STXXL_CHECK(!CONTAINER->empty());

                #if defined(STXXLSORTER)
                value_type top = *(*CONTAINER);
                (void)top;
                ++(*CONTAINER);
                #else
                CONTAINER->top();
                CONTAINER->pop();
                #endif

                progress("Deleting / inserting element", i, 2 * num_elements);
                ++num_deletes;
                STXXL_CHECK_EQUAL(CONTAINER->size(), num_inserts - num_deletes);
            } else {
                progress("Inserting/deleting element", i, 2 * num_elements);
                uint32 k = rand_r(&_seed) % value_universe_size;
                CONTAINER->push(value_type(k));
                num_inserts++;
                STXXL_CHECK_EQUAL(CONTAINER->size(), num_inserts - num_deletes);
            }
        }
    }
}

void PREFIX(bulk_insert) (bool parallel = true)
{
    std::string parallel_str = parallel ? " in parallel" : "";
#if !defined(PPQ)
    (void)parallel;
#endif
    {
        stxxl::scoped_print_timer timer("Filling " NAME " with bulks" + parallel_str, num_elements * value_size);
        for (uint64_t i = 0; i < num_elements / bulk_size; ++i) {
#if defined(PPQ)
            CONTAINER->bulk_push_begin(bulk_size);
            #pragma omp parallel if(parallel)
            {
                const unsigned thread_id = parallel ? omp_get_thread_num() : rand() % num_insertion_heaps;
                #pragma omp for
#endif
            for (stxxl::uint64 j = 0; j < bulk_size; ++j) {
                progress("Inserting element", i * bulk_size + j, num_elements);
#if defined(PPQ)
                CONTAINER->bulk_push_step(value_type(num_elements - (i * bulk_size + j)), thread_id);
#else
                CONTAINER->push(value_type(num_elements - (i * bulk_size + j)));
#endif
            }
#if defined(PPQ)
        }
        CONTAINER->bulk_push_end();
#endif
        }
#if defined(PPQ)
        CONTAINER->bulk_push_begin(num_elements % bulk_size);
        #pragma omp parallel if(parallel)
        {
            const unsigned thread_id = parallel ? omp_get_thread_num() : rand() % num_insertion_heaps;
            #pragma omp for
#endif
        for (stxxl::uint64 j = 0; j < num_elements % bulk_size; j++) {
            progress("Inserting element", num_elements - (num_elements % bulk_size) + j, num_elements);
#if defined(PPQ)
            CONTAINER->bulk_push_step(value_type(num_elements % bulk_size - j), thread_id);
#else
            CONTAINER->push(value_type(num_elements % bulk_size - j));
#endif
        }
#if defined(PPQ)
    }
    CONTAINER->bulk_push_end();
#endif
    }
}

void PREFIX(bulk_rand_insert) (unsigned int _seed, bool parallel = true)
{
    std::string parallel_str = parallel ? " in parallel" : "";
#if !defined(PPQ)
    (void)parallel;
#endif
    {
        stxxl::scoped_print_timer timer("Filling " NAME " with bulks" + parallel_str, num_elements * value_size);
        for (uint64_t i = 0; i < num_elements / bulk_size; ++i) {
#if defined(PPQ)
            CONTAINER->bulk_push_begin(bulk_size);
            #pragma omp parallel if(parallel)
            {
                const unsigned int thread_id = parallel ? omp_get_thread_num() : rand() % num_insertion_heaps;
                unsigned int seed = i * _seed * thread_id;
                #pragma omp for
#endif
            for (stxxl::uint64 j = 0; j < bulk_size; ++j) {
                progress("Inserting element", i * bulk_size + j, num_elements);
                                            #if defined(PPQ)
                uint32 k = rand_r(&seed) % value_universe_size;
                CONTAINER->bulk_push_step(value_type(k), thread_id);
                                            #else
                uint32 k = rand_r(&_seed) % value_universe_size;
                CONTAINER->push(value_type(k));
                                            #endif
            }
#if defined(PPQ)
        }
        CONTAINER->bulk_push_end();
#endif
        }

        STXXL_CHECK_EQUAL(CONTAINER->size(), num_elements - (num_elements % bulk_size));

#if defined(PPQ)
        CONTAINER->bulk_push_begin(num_elements % bulk_size);
        #pragma omp parallel if(parallel)
        {
            const unsigned thread_id = parallel ? omp_get_thread_num() : rand() % num_insertion_heaps;
            unsigned int seed = num_elements / bulk_size * _seed * thread_id;
            #pragma omp for
#endif
        for (stxxl::uint64 j = 0; j < num_elements % bulk_size; j++) {
            progress("Inserting element", num_elements - (num_elements % bulk_size) + j, num_elements);
#if defined(PPQ)
            uint32 k = rand_r(&seed) % value_universe_size;
            CONTAINER->bulk_push_step(value_type(k), thread_id);
#else
            uint32 k = rand_r(&_seed) % value_universe_size;
            CONTAINER->push(value_type(k));
#endif
        }
#if defined(PPQ)
    }
    CONTAINER->bulk_push_end();
#endif
    }
    STXXL_CHECK_EQUAL(CONTAINER->size(), num_elements);
}

void PREFIX(bulk_rand_intermixed) (unsigned int _seed, bool filled, bool parallel = true)
{
    std::string parallel_str = parallel ? " in parallel" : "";
#if !defined(PPQ)
    (void)parallel;
#endif
    uint64 num_inserts = filled ? num_elements : 0;
    uint64 num_deletes = 0;
    {
        stxxl::scoped_print_timer timer(NAME ": Intermixed parallel rand bulk insert" + parallel_str + " and delete", (filled ? 3 : 2) * num_elements * value_size);
        for (uint64_t i = 0; i < (filled ? 3 : 2) * num_elements; ++i) {
            unsigned r = rand() % ((filled ? 2 : 1) * bulk_size);
            // probability for an extract is bulk_size times larger than for a bulk_insert.
            // when already filled with num_elements elements: probability for an extract is 2*bulk_size times larger than for a bulk_insert.
            if (num_deletes < num_inserts && num_deletes < (filled ? 2 : 1) * num_elements && (r > 0 || num_inserts >= (filled ? 2 : 1) * num_elements)) {
                STXXL_CHECK(!CONTAINER->empty());

#if defined(STXXLSORTER)
                value_type top = *(*CONTAINER);
                (void)top;
                ++(*CONTAINER);
#else
                CONTAINER->top();
                CONTAINER->pop();
#endif

                progress("Deleting / inserting element", i, (filled ? 3 : 2) * num_elements);
                ++num_deletes;
                STXXL_CHECK_EQUAL(CONTAINER->size(), num_inserts - num_deletes);
            } else {
                uint64 this_bulk_size = (num_inserts + bulk_size > (filled ? 2 : 1) * num_elements) ? num_elements % bulk_size : bulk_size;
#if defined(PPQ)
                CONTAINER->bulk_push_begin(this_bulk_size);
                #pragma omp parallel if(parallel)
                {
                    const unsigned int thread_num = parallel ? omp_get_thread_num() : rand() % num_insertion_heaps;
                    unsigned int seed = thread_num * i * _seed;
                    #pragma omp for
#endif
                for (stxxl::uint64 j = 0; j < this_bulk_size; j++) {
                    progress("Inserting / deleting element", i + j, (filled ? 3 : 2) * num_elements);
                                                    #if defined(PPQ)
                    uint32 k = rand_r(&seed) % value_universe_size;
                    CONTAINER->bulk_push_step(value_type(k), thread_num);
                                                    #else
                    uint32 k = rand_r(&_seed) % value_universe_size;
                    CONTAINER->push(value_type(k));
                                                    #endif
                }
#if defined(PPQ)
            }
            CONTAINER->bulk_push_end();
#endif
                num_inserts += this_bulk_size;
                i += this_bulk_size - 1;
                STXXL_CHECK_EQUAL(CONTAINER->size(), num_inserts - num_deletes);
            }
        }
    }
}

void PREFIX(bulk_intermixed_check) (bool parallel = true)
{
    std::string parallel_str = parallel ? " in parallel" : "";
#if !defined(PPQ)
    (void)parallel;
#endif
    uint64 num_inserts = 0;
    uint64 num_deletes = 0;
    std::vector<bool> extracted_values(num_elements, false);
    uint64 num_failures = 0;
    {
        stxxl::scoped_print_timer timer(NAME ": Intermixed parallel bulk insert" + parallel_str + " and delete", 2 * num_elements * value_size);
        for (uint64_t i = 0; i < 2 * num_elements; ++i) {
            unsigned r = rand() % bulk_size;
            if (num_deletes < num_inserts && num_deletes < num_elements && (r > 0 || num_inserts >= num_elements)) {
                STXXL_CHECK(!CONTAINER->empty());

#if defined(STXXLSORTER)
                value_type top = *(*CONTAINER);
                ++(*CONTAINER);
#else
                value_type top = CONTAINER->top();
                CONTAINER->pop();
#endif

                if (top.first < 1 || top.first > num_elements) {
                    std::cout << top.first << " has never been inserted." << std::endl;
                    ++num_failures;
                } else if (extracted_values[top.first - 1]) {
                    std::cout << top.first << " has already been extracted." << std::endl;
                    ++num_failures;
                } else {
                    extracted_values[top.first - 1] = true;
                }
                progress("Deleting / inserting element", i, 2 * num_elements);
                ++num_deletes;
                STXXL_CHECK_EQUAL(CONTAINER->size(), num_inserts - num_deletes);
            } else {
                uint64 this_bulk_size = (num_inserts + bulk_size > num_elements) ? num_elements % bulk_size : bulk_size;
#if defined(PPQ)
                CONTAINER->bulk_push_begin(this_bulk_size);
                #pragma omp parallel if(parallel)
                {
                    const unsigned int thread_num = parallel ? omp_get_thread_num() : rand() % num_insertion_heaps;
                    #pragma omp for
#endif
                for (stxxl::uint64 j = 0; j < this_bulk_size; j++) {
                    uint32 k = num_elements - num_inserts - j;
                    progress("Inserting/deleting element", i + j, 2 * num_elements);

#if defined(PPQ)
                    CONTAINER->bulk_push_step(value_type(k), thread_num);
#else
                    CONTAINER->push(value_type(k));
#endif
                }
#if defined(PPQ)
            }
            CONTAINER->bulk_push_end();
#endif
                num_inserts += this_bulk_size;
                i += this_bulk_size - 1;
                STXXL_CHECK_EQUAL(CONTAINER->size(), num_inserts - num_deletes);
            }
        }
    }
    {
        stxxl::scoped_print_timer timer("Check if all values have been extracted");
        for (uint64 i = 0; i < num_elements; ++i) {
            if (!extracted_values[i]) {
                std::cout << i + 1 << " has never been extracted." << std::endl;
                ++num_failures;
            }
        }
    }
    std::cout << num_failures << " failures." << std::endl;
}

#ifdef PPQ
void PREFIX(merge_external_arrays) ()
{
    stxxl::scoped_print_timer timer("Merging external arrays", num_elements * value_size);
    CONTAINER->merge_external_arrays();
}
#endif
