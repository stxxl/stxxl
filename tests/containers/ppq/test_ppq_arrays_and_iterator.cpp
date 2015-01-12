/***************************************************************************
 *  tests/containers/ppq/test_ppq_arrays_and_iterator.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2014 Thomas Keh <thomas.keh@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! \example containers/external_array.cpp
//! This is an example of how to use \c stxxl::external_array

#include <cstddef>
#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/containers/parallel_priority_queue.h>
#include <stxxl/bits/mng/block_manager.h>
#include <stxxl/bits/common/utils.h>
#include <stxxl/bits/verbose.h>
#include <stxxl/timer>

typedef std::pair<size_t, size_t> value_type;
typedef stxxl::ppq_local::external_array<value_type> ea;
typedef stxxl::ppq_local::internal_array<value_type> ia;

//! Comparator for value_type / size_t-pairs
struct value_comp {
    bool operator () (value_type const& a, value_type const& b)
    {
        return a.first < b.first;
    }
};

//! Frequency of printing the progress
static const size_t printmod = 16 * 1024 * 1024;

//! Prints progress
static inline void progress(const char* text, size_t i, size_t nelements)
{
    if ((i % printmod) == 0) {
        STXXL_MSG(text << " " << i << " (" << std::setprecision(5) << (static_cast<double>(i) * 100. / static_cast<double>(nelements)) << " %)");
    }
}

//! Fills an external array with increasing numbers
void fill(ea& a, size_t N, size_t index, size_t n)
{
    STXXL_CHECK(n > 0);

    a.request_write_buffer(n);
    size_t i = index;

    STXXL_CHECK(a.end() - a.begin() == (int)n);

    for (ea::iterator it = a.begin(); it < a.end(); ++it) {
        STXXL_CHECK(i < index + n);
        progress("Inserting element", i, N);
        *it = std::make_pair(i + 1, i + 1);
        ++i;
    }

    STXXL_CHECK(i == index + n);
    a.flush_write_buffer();
}

//! Run external array test.
//! Testing...
//!     ⁻ iterators
//!     - request_write_buffer()
//!     ⁻ random access
//!     ⁻ remove()
//!     ⁻ get_current_max()
//!     ⁻ request_further_block()
//!     ⁻ wait()
//!     - sizes
//!
//! \param volume Volume
//! \param numpbs Number of prefetch buffer blocks for each external array.
//! \param numwbs Number of write buffer blocks for each external array.
int run_external_array_test(size_t volume, size_t numpbs, size_t numwbs)
{
    const size_t block_size = ea::block_size;
    const size_t N = volume / sizeof(value_type);
    const size_t num_blocks = stxxl::div_ceil(N, block_size);

    STXXL_VARDUMP(block_size);
    STXXL_VARDUMP(N);
    STXXL_VARDUMP(num_blocks);

    ea a(N, numpbs, numwbs);

    {
        stxxl::scoped_print_timer timer("filling", volume);

        STXXL_CHECK(N >= 10 + 355 + 2 * block_size);

        size_t index = 0;

        // Testing different block sizes...

        fill(a, N, index, 10);
        index += 10;

        fill(a, N, index, 355);
        index += 355;

        fill(a, N, index, 2 * block_size);
        index += 2 * block_size;

        // Filling the rest...

        while (index < N) {
            size_t diff = std::min(N - index, 2 * block_size - 167);
            fill(a, N, index, diff);
            index += diff;
        }
    }

    {
        stxxl::scoped_print_timer timer("finishing write phase");
        a.finish_write_phase();
    }

    {
        stxxl::scoped_print_timer timer("reading and checking", volume);

        STXXL_CHECK(N > 5 * block_size + 876);
        STXXL_CHECK(block_size > 34);

        STXXL_CHECK_EQUAL(a.size(), N);
        STXXL_CHECK(a.buffer_size() > 7);

        // Testing iterator...

        for (unsigned i = 0; i < 7; ++i) {
            STXXL_CHECK_EQUAL((a.begin() + i)->first, i + 1);
        }

        STXXL_CHECK(a.buffer_size() > 12);

        // Testing random access...

        for (unsigned i = 7; i < 12; ++i) {
            STXXL_CHECK_EQUAL(a[i].first, i + 1);
        }

        // Testing remove...

        a.remove(33);
        a.wait();

        STXXL_CHECK_EQUAL(a[33].first, 34);
        STXXL_CHECK_EQUAL(a.begin().get_index(), 33);
        STXXL_CHECK_EQUAL(a.begin()->first, 34);

        // Testing get_current_max()...

        unsigned maxround = 0;

        while (a.get_current_max().first < 5 * block_size + 876) {
            switch (maxround) {
            case 0: STXXL_CHECK_EQUAL(a.get_current_max().first, 131072);
                break;
            case 1: STXXL_CHECK_EQUAL(a.get_current_max().first, 262144);
                break;
            case 2: STXXL_CHECK_EQUAL(a.get_current_max().first, 393216);
                break;
            case 3: STXXL_CHECK_EQUAL(a.get_current_max().first, 524288);
                break;
            case 4: STXXL_CHECK_EQUAL(a.get_current_max().first, 655360);
                break;
            }

            a.request_further_block();
            ++maxround;
        }

        // Testing request_further_block() (called above)...

        a.wait();

        STXXL_CHECK((a.end() - 1)->first >= 5 * block_size + 876);

        size_t index = 33;

        for (ea::iterator it = a.begin(); it != a.end(); ++it) {
            progress("Extracting element", index, N);
            STXXL_CHECK_EQUAL(it.get_index(), index);
            STXXL_CHECK_EQUAL(it->first, index + 1);
            ++index;
        }

        a.remove(a.buffer_size());
        STXXL_CHECK(a.buffer_size() > 0);

        // Extracting the rest...

        size_t round = 0;

        while (!a.empty()) {
            if (a.has_em_data() && round % 2 == 0) {
                a.request_further_block();
            }

            a.wait();

            for (ea::iterator it = a.begin(); it != a.end(); ++it) {
                progress("Extracting element", index, N);
                STXXL_CHECK_EQUAL(it.get_index(), index);
                STXXL_CHECK_EQUAL(it->first, index + 1);
                ++index;
            }

            if (round % 3 == 0) {
                // Test the case that not the whole buffer is removed...
                STXXL_CHECK(a.buffer_size() > 0);
                a.remove(a.buffer_size() - 1);
                index--;
            }
            else {
                a.remove(a.buffer_size());
            }

            STXXL_CHECK(a.buffer_size() > 0 || a.empty());

            ++round;
        }
    }

    return EXIT_SUCCESS;
}

//! Run test for multiway_merge compatibility.
//! Fills two external arrays and one internal array.
//! Merges them into one external array and checks the content.
//!
//! \param volume Volume
//! \param numpbs Number of prefetch buffer blocks for each external array.
//! \param numwbs Number of write buffer blocks for each external array.
int run_multiway_merge(size_t volume, size_t numpbs, size_t numwbs)
{
    const size_t block_size = ea::block_size;
    const size_t size = volume / sizeof(value_type);
    const size_t num_blocks = stxxl::div_ceil(size, block_size);

    STXXL_VARDUMP(block_size);
    STXXL_VARDUMP(size);
    STXXL_VARDUMP(num_blocks);

    std::vector<value_type> v(size);

    {
        stxxl::scoped_print_timer timer("filling vector / internal array for multiway_merge test", volume);
        for (size_t i = 0; i < size; ++i) {
            v[i] = std::make_pair(i + 1, i + 1);
        }
    }

    ia d(v);

    ea a(size, numpbs, numwbs);
    ea b(size, numpbs, numwbs);
    ea c(size * 3, numpbs, numwbs);

    {
        stxxl::scoped_print_timer timer("filling external arrays for multiway_merge test", volume * 2);

        fill(a, size * 2, 0, size);
        fill(b, size * 2, 0, size);

        a.finish_write_phase();
        b.finish_write_phase();
    }

    {
        stxxl::scoped_print_timer timer("loading input arrays into RAM and requesting output buffer", volume * 2);

        while (a.has_em_data()) {
            a.request_further_block();
        }

        while (b.has_em_data()) {
            b.request_further_block();
        }

        a.wait();
        b.wait();
        c.request_write_buffer(size * 3);

        STXXL_CHECK_EQUAL(a.buffer_size(), size);
        STXXL_CHECK_EQUAL(b.buffer_size(), size);
        STXXL_CHECK_EQUAL(c.buffer_size(), 3 * size);
    }

    {
        stxxl::scoped_print_timer timer("running multiway merge", volume * 4);

        std::vector<std::pair<ea::iterator, ea::iterator> > seqs;
        seqs.push_back(std::make_pair(a.begin(), a.end()));
        seqs.push_back(std::make_pair(b.begin(), b.end()));
        seqs.push_back(std::make_pair(d.begin(), d.end()));

        stxxl::parallel::sw_multiway_merge(seqs.begin(), seqs.end(),
                                           c.begin(), value_comp(), 3 * size);

        // sequential:
        //__gnu_parallel::multiway_merge(seqs.begin(), seqs.end(), c.begin(),
        //    2*size, value_comp()); // , __gnu_parallel::sequential_tag()
    }

    {
        stxxl::scoped_print_timer timer("flushing the result to EM", volume * 3);
        c.flush_write_buffer();
        c.finish_write_phase();
    }

    {
        stxxl::scoped_print_timer timer("loading the result back into RAM", volume * 3);

        while (c.has_em_data()) {
            c.request_further_block();
        }

        c.wait();
    }

    {
        stxxl::scoped_print_timer timer("checking the order", volume * 3);

        size_t index = 1;

        for (ea::iterator it = c.begin(); it != c.end(); ++it) {
            progress("Extracting element", index, 3 * size);
            STXXL_CHECK_EQUAL(it->first, index);

            ++it;
            STXXL_CHECK(it != c.end());

            progress("Extracting element", index, 3 * size);
            STXXL_CHECK_EQUAL(it->first, index);

            ++it;
            STXXL_CHECK(it != c.end());

            progress("Extracting element", index, 3 * size);
            STXXL_CHECK_EQUAL(it->first, index);

            ++index;
        }
    }

    return EXIT_SUCCESS;
}

//! Fill internal array, remove 3 elements, read and check result.
//! \param volume Volume
int run_internal_array_test(size_t volume)
{
    const size_t size = volume / sizeof(value_type);
    STXXL_VARDUMP(size);
    STXXL_CHECK(size > 3);

    std::vector<value_type> v(size);

    {
        stxxl::scoped_print_timer timer("filling vector / internal array", volume);
        for (size_t i = 0; i < size; ++i) {
            v[i] = std::make_pair(i + 1, i + 1);
        }
    }

    ia a(v);

    {
        stxxl::scoped_print_timer timer("reading and checking the order", volume);

        a.inc_min();
        a.inc_min(2);

        typedef ia::iterator iterator;
        size_t index = 4;

        // test iterator
        for (iterator it = a.begin(); it != a.end(); ++it) {
            STXXL_CHECK_EQUAL(it->first, index++);
        }

        for (size_t i = 3; i < size; ++i) {
            STXXL_CHECK_EQUAL(a.begin()[i].first, i + 1);
            STXXL_CHECK_EQUAL(a[i].first, i + 1);
        }

        STXXL_CHECK_EQUAL(index, size + 1);
        STXXL_CHECK_EQUAL(a.size(), size - 3);
    }

    return EXIT_SUCCESS;
}

//! Testing upper_bound compatibility
//! \param volume Volume
int run_upper_bound_test(size_t volume)
{
    const size_t size = volume / sizeof(value_type);
    STXXL_CHECK(volume > ea::block_size);
    STXXL_VARDUMP(size);
    STXXL_CHECK(size > 2000);

    std::vector<value_type> v(size);

    {
        stxxl::scoped_print_timer timer("filling vector / internal array", volume);
        for (size_t i = 0; i < size; ++i) {
            v[i] = std::make_pair(i + 1, i + 1);
        }
    }

    ia a(v);
    ea b(size - 10, 2, 2);

    {
        stxxl::scoped_print_timer timer("filling external array", volume);
        b.request_write_buffer(size - 10);
        for (size_t i = 0; i < size - 10; ++i) {
            *(b.begin() + i) = std::make_pair(2 * i, 2 * i);
            STXXL_CHECK_EQUAL(b[i].first, 2 * i);
            STXXL_CHECK_EQUAL(b.begin()[i].first, 2 * i);
            STXXL_CHECK_EQUAL((b.begin() + i)->first, 2 * i);
        }
        b.flush_write_buffer();
        b.finish_write_phase();
    }

    {
        stxxl::scoped_print_timer timer("running upper_bound", 2 * volume);

        b.request_further_block();
        b.wait();

        value_type minmax = std::make_pair(2000, 2000);

        ea::iterator uba = std::upper_bound(a.begin(), a.end(), minmax, value_comp());
        ea::iterator ubb = std::upper_bound(b.begin(), b.end(), minmax, value_comp());

        STXXL_CHECK_EQUAL((uba - 1)->first, 2000);
        STXXL_CHECK_EQUAL((ubb - 1)->first, 2000);
        STXXL_CHECK_EQUAL(uba->first, 2001);
        STXXL_CHECK_EQUAL(ubb->first, 2002);
        STXXL_CHECK_EQUAL(uba.get_index(), 2000);
        STXXL_CHECK_EQUAL(ubb.get_index(), 1001);
    }

    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    stxxl::uint64 volume = 512 * 1024 * 1024;
    stxxl::uint64 mwmvolume = 100 * 1024 * 1024;
    stxxl::uint64 iavolume = 10 * 1024 * 1024;

    unsigned numpbs = 1;
    unsigned numwbs = 14;

    stxxl::cmdline_parser cp;
    cp.set_description("STXXL external array test");
    cp.set_author("Thomas Keh <thomas.keh@student.kit.edu>");
    cp.add_bytes('v', "volume", "Volume to fill into the external array, default: 512 MiB, 0 = disable test", volume);
    cp.add_bytes('m', "mwmvolume", "Testing multiway merge of two external arrays - the volume of each input array, default: 100 MiB, 0 = disable test", mwmvolume);
    cp.add_bytes('i', "iavolume", "Volume to fill into the internal array, default: 10 MiB, 0 = disable test", iavolume);
    cp.add_uint('n', "numprefetchbuffers", "Number of prefetch buffer blocks, default: 1", numpbs);
    cp.add_uint('w', "numwritebuffers", "Number of write buffer blocks, default: 14", numwbs);

    if (!cp.process(argc, argv))
        return EXIT_FAILURE;

    const size_t block_size = 2 * 1024 * 1024 / sizeof(value_type);
    if (volume > 0 && volume / sizeof(value_type) < 5 * block_size + 876) {
        STXXL_ERRMSG("The volume is too small for this test. It must be >= " << (5 * block_size + 876) * sizeof(value_type));
        return EXIT_FAILURE;
    }

    if (iavolume > 0 && iavolume / sizeof(value_type) < 3) {
        STXXL_ERRMSG("The internal array volume is too small for this test. It must be > " << 3);
        return EXIT_FAILURE;
    }

    STXXL_MEMDUMP(volume);
    STXXL_MEMDUMP(mwmvolume);
    STXXL_MEMDUMP(iavolume);
    STXXL_MEMDUMP(sizeof(value_type));
    STXXL_VARDUMP(numpbs);
    STXXL_VARDUMP(numwbs);

    bool succ = EXIT_SUCCESS;

    if (volume > 0) {
        succ = run_external_array_test(volume, numpbs, numwbs) && succ;
    }

    if (iavolume > 0) {
        succ = run_internal_array_test(iavolume) && succ;
    }

    if (mwmvolume > 0) {
        succ = run_multiway_merge(mwmvolume, numpbs, numwbs) && succ;
    }

    succ = run_upper_bound_test(3 * ea::block_size * sizeof(value_type)) && succ;

    return succ;
}
