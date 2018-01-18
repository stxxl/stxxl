/***************************************************************************
 *  tests/stream/test_loop.cpp
 *
 *  example for building a loop of stream operations
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2011 Jaroslaw Fedorowicz <fedorow@cs.uni-frankfurt.de>
 *  Copyright (C) 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example stream/test_loop.cpp
//! This is an example of how to use some basic algorithms from the
//! stream package to form a loop iterating over the data.
//! Some input is generated, sorted, some elements are filtered out.
//! The remaining elements are transformed, sorted and processed in the
//! next pass. The loop will terminate if at most one element remains.
//! A split sorter is used to cut the data flow (and type dependency) cycle.

#include <cassert>
#include <iostream>
#include <limits>

#include <foxxll/mng.hpp>
#include <stxxl/stream>
#include <stxxl/vector>

using std::cout;
using std::endl;

const size_t memory_to_use = 128ul * 1024 * 1024;

bool verbose;

struct random_generator
{
    using value_type = stxxl::random_number32::value_type;
    using size_type = size_t;
    value_type current;
    size_type count;
    stxxl::random_number32 rnd;

    explicit random_generator(size_type _count) : count(_count)
    {
        if (verbose) cout << "Random Stream: ";
        current = rnd();
    }

    value_type operator * () const
    {
        return current;
    }
    random_generator& operator ++ ()
    {
        count--;
        if (verbose) {
            cout << current << ", ";
            if (empty()) cout << endl;
        }
        current = rnd();
        return *this;
    }
    bool empty() const
    {
        return (count == 0);
    }
};

template <typename value_type>
struct Cmp : std::binary_function<value_type, value_type, bool>
{
    bool operator () (const value_type& a, const value_type& b) const
    {
        return a < b;
    }
    static value_type min_value()
    {
        return value_type(std::numeric_limits<value_type>::min());
    }
    static value_type max_value()
    {
        return value_type(std::numeric_limits<value_type>::max());
    }
};

template <typename Input>
struct filter
{
    using value_type = typename Input::value_type;
    using size_type = size_t;
    Input& input;
    value_type filter_value;
    size_type& counter;

    void apply_filter()
    {
        while (!input.empty() && *input == filter_value) {
            ++input;
            counter++;
        }
    }

    filter(Input& _input, value_type _filter_value, size_type& _counter) : input(_input), filter_value(_filter_value), counter(_counter)
    {
        apply_filter();
    }

    const value_type operator * () const
    {
        return *input;
    }

    filter& operator ++ ()
    {
        ++input;
        apply_filter();
        return *this;
    }

    bool empty() const
    {
        return input.empty();
    }
};

template <typename Input>
struct output
{
    using value_type = typename Input::value_type;
    Input& input;

    explicit output(Input& _input) : input(_input) { }

    const value_type operator * () const
    {
        return *input;
    }

    output& operator ++ ()
    {
        if (verbose) cout << *input << ", ";
        ++input;
        if (empty() && verbose)
            cout << endl;
        return *this;
    }

    bool empty() const
    {
        return input.empty();
    }
};

template <typename Input>
struct shuffle
{
    using value_type = typename Input::value_type;
    Input& input;
    value_type current, next;
    bool even, is_empty;

    // from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
    int count_bits(uint64_t v)
    {
        int c;
        for (c = 0; v; c++) {
            v &= v - 1;
        }
        return c;
    }

    void apply_shuffle()
    {
        is_empty = input.empty();
        if (!is_empty) {
            current = *input;
            ++input;
            if (!input.empty()) {
                static_assert(sizeof(value_type) == 4,
                              "sizeof(value_type) == 4");
                uint64_t combined = current;
                combined = combined << 32 | *input;
                combined = (1ul << count_bits(combined)) - 1;
                current = (value_type)(combined >> 32);
                next = (value_type)combined;
            }
        }
    }

    explicit shuffle(Input& _input)
        : input(_input), current(0), next(0), even(true), is_empty(false)
    {
        apply_shuffle();
    }

    value_type operator * () const
    {
        return current;
    }

    shuffle& operator ++ ()
    {
        even = !even;
        is_empty = input.empty();
        if (even) {
            ++input;
            apply_shuffle();
        }
        else {
            current = next;
        }
        return *this;
    }

    bool empty() const
    {
        return is_empty;
    }
};

using input_generator_type = random_generator;
using cmp = Cmp<input_generator_type::value_type>;
using runs_creator_type0 = stxxl::stream::runs_creator<input_generator_type, cmp>;
using sorted_runs_type = runs_creator_type0::sorted_runs_type;
using runs_merger_type = stxxl::stream::runs_merger<sorted_runs_type, cmp>;
using output_type = output<runs_merger_type>;
using filter_type0 = filter<output_type>;
using filter_type1 = filter<filter_type0>;
using shuffle_type = shuffle<filter_type1>;
using runs_creator_type1 = stxxl::stream::runs_creator<shuffle_type, cmp>;

// force instantiation of whole chain
template class stxxl::stream::runs_creator<shuffle_type, cmp>;

int main(int argc, char** argv)
{
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " count [Options]\nOptions: -v \t prints elements of each iteration\n";
        return EXIT_FAILURE;
    }

    foxxll::block_manager::get_instance();

    verbose = (argc == 3) && !strcmp(argv[2], "-v");

    size_t total = foxxll::atouint64(argv[1]);

    input_generator_type input_stream(total);

    runs_creator_type0 runs_creator(input_stream, cmp(), memory_to_use);

    sorted_runs_type sorted_runs = runs_creator.result();

    size_t counter = 0;
    int i;

    for (i = 0; counter < total - 1; ++i) {
        if (verbose) cout << "Iteration " << i << ": ";

        runs_merger_type runs_merger(sorted_runs, cmp(), memory_to_use);

        output_type output_stream(runs_merger);

        filter_type0 filter0(output_stream, 0, counter);

        filter_type1 filter1(filter0, filter_type1::value_type(-1), counter);

        shuffle_type shuffled_stream(filter1);

        runs_creator_type1 runs_creator(shuffled_stream, cmp(), memory_to_use);

        sorted_runs = runs_creator.result();
    }

    runs_merger_type runs_merger(sorted_runs, cmp(), memory_to_use);

    while (!runs_merger.empty()) {
        if (verbose) cout << "Iteration " << i << ": " << *runs_merger << endl;
        ++runs_merger;
    }
    cout << "\nIteration needed: " << i << endl;
}

// vim: et:ts=4:sw=4
