/***************************************************************************
 *  tools/benchmark_sort.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

/*
 * This program will benchmark the different sorting methods provided by STXXL
 * using three different data types: first a pair of 32-bit uints, "then a pair
 * 64-bit uint and then a larger structure of 64 bytes.
 */

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>

#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/common/comparator.h>
#include <stxxl/bits/common/tuple.h>
#include <stxxl/ksort>
#include <stxxl/sort>
#include <stxxl/stream>
#include <stxxl/vector>

using foxxll::timestamp;
using foxxll::external_size_type;

#define MB (1024 * 1024)

// pair of uint32_t = 8 bytes
using pair32_type = std::pair<uint32_t, uint32_t>;

// pair of uint64_t = 16 bytes
using pair64_type = std::pair<uint64_t, uint64_t>;

// larger struct of 64 bytes
using pair64_with_junk_type = std::tuple<
          uint64_t, uint64_t,                                        // keys
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t // junk
          >;

// construct a simple sorting benchmark for the value type
template <typename ValueType>
class BenchmarkSort
{
    using value_type = ValueType;

    struct value_key_second
    {
        using key_type = typename std::tuple_element<1, value_type>::type;

        key_type operator () (const value_type& p) const
        {
            return std::get<1>(p);
        }

        static value_type min_value()
        {
            value_type res;
            std::get<0>(res) = std::numeric_limits<key_type>::min();
            std::get<1>(res) = std::numeric_limits<key_type>::min();
            return res;
        }

        static value_type max_value()
        {
            value_type res;
            std::get<0>(res) = std::numeric_limits<key_type>::max();
            std::get<1>(res) = std::numeric_limits<key_type>::max();
            return res;
        }
    };

    struct random_stream
    {
        using value_type = ValueType;

        value_type m_value;

        external_size_type m_counter;

        explicit random_stream(external_size_type size)
            : m_counter(size)
        {
            std::get<0>(m_value) = m_rng();
            std::get<1>(m_value) = m_rng();
        }

        const value_type& operator * () const
        {
            return m_value;
        }

        random_stream& operator ++ ()
        {
            assert(m_counter > 0);
            --m_counter;

            std::get<0>(m_value) = m_rng();
            std::get<1>(m_value) = m_rng();
            return *this;
        }

        bool empty() const
        {
            return (m_counter == 0);
        }

    private:
        using key_type = typename std::tuple_element<0, value_type>::type;
        std::default_random_engine m_rng;
    };

    static void output_result(double elapsed, external_size_type vec_size)
    {
        std::cout << "finished in " << elapsed << " seconds @ "
                  << ((double)vec_size * sizeof(value_type) / MB / elapsed)
                  << " MiB/s" << std::endl;
    }

public:
    BenchmarkSort(const char* desc, external_size_type length, size_t memsize)
    {
        // construct vector
        external_size_type vec_size = foxxll::div_ceil(length, sizeof(ValueType));

        stxxl::vector<ValueType> vec(vec_size);

        auto comp_fst = stxxl::make_struct_comparator<value_type>([](auto& o) { return std::tie(std::get<0>(o)); });

        // construct random stream
        std::cout << "#!!! running sorting test with " << desc << " = " << sizeof(ValueType) << " bytes."
                  << std::endl;
        {
            std::cout << "# materialize random_stream into vector of size " << vec.size() << std::endl;
            double ts1 = timestamp();

            random_stream rs(vec_size);
            stxxl::stream::materialize(rs, vec.begin(), vec.end());

            double elapsed = timestamp() - ts1;
            output_result(elapsed, vec_size);
        }
        {
            std::cout << "# stxxl::sort vector of size " << vec.size() << std::endl;
            double ts1 = timestamp();

            stxxl::sort(vec.begin(), vec.end(), comp_fst, memsize);

            double elapsed = timestamp() - ts1;
            output_result(elapsed, vec_size);
        }
        {
            std::cout << "# stxxl::ksort vector of size " << vec.size() << std::endl;
            double ts1 = timestamp();

            stxxl::ksort(vec.begin(), vec.end(), value_key_second(), memsize);

            double elapsed = timestamp() - ts1;
            output_result(elapsed, vec_size);
        }
        vec.clear();

        {
            std::cout << "# stxxl::stream::sort of size " << vec_size << std::endl;
            double ts1 = timestamp();
            using random_stream_sort_type = stxxl::stream::sort<random_stream, decltype(comp_fst)>;

            random_stream stream(vec_size);
            random_stream_sort_type stream_sort(stream, comp_fst, memsize);

            stxxl::stream::discard(stream_sort);

            double elapsed = timestamp() - ts1;
            output_result(elapsed, vec_size);
        }

        std::cout << std::endl;
    }
};

// run sorting benchmark for the three types defined above.
int benchmark_sort(int argc, char* argv[])
{
    // parse command line
    stxxl::cmdline_parser cp;

    cp.set_description(
        "This program will benchmark the different sorting methods provided by "
        "STXXL using three different data types: first a pair of 32-bit uints, "
        "then a pair 64-bit uint and then a larger structure of 64 bytes.");
    cp.set_author("Timo Bingmann <tb@panthema.net>");

    external_size_type length = 0;
    cp.add_param_bytes("size", length,
                       "Amount of data to sort (e.g. 1GiB)");

    size_t memsize = 256 * MB;
    cp.add_bytes('M', "ram", memsize,
                 "Amount of RAM to use when sorting, default: 256 MiB");

    if (!cp.process(argc, argv))
        return -1;

    BenchmarkSort<pair32_type>(
        "pair of uint32_t", length, memsize);

    BenchmarkSort<pair64_type>(
        "pair of uint64_t", length, memsize);

    static_assert(sizeof(pair64_with_junk_type) == 64, "Junk type too small");
    BenchmarkSort<pair64_with_junk_type>(
        "struct of 64 bytes", length, memsize);

    return 0;
}
