/***************************************************************************
 *  tests/containers/ppq/test_ppq_arrays_and_iterator.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
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
#include <iostream>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>
#include <tlx/string.hpp>

#include <foxxll/common/utils.hpp>
#include <foxxll/mng/block_manager.hpp>

#include <stxxl/bits/common/cmdline.h>
#include <stxxl/bits/containers/parallel_priority_queue.h>
#include <stxxl/timer>

using value_type = std::pair<uint64_t, uint64_t>;
using ea_type = stxxl::ppq_local::external_array<value_type>;
using ia_type = stxxl::ppq_local::internal_array<value_type>;

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
        LOG1 << text << " " << i << " (" << std::setprecision(5) << (static_cast<double>(i) * 100. / static_cast<double>(nelements)) << " %)";
    }
}

//! Fills an external array with increasing numbers
void fill(ea_type& ea, size_t index, size_t n)
{
    die_unless(n > 0);

    ea_type::writer_type ea_writer(ea);

    size_t i = index;

    die_unequal(ea_writer.end() - ea_writer.begin(), (ssize_t)n);

    for (ea_type::writer_type::iterator it = ea_writer.begin();
         it != ea_writer.end(); ++it)
    {
        die_unless(i < index + n);
        progress("Inserting element", i, n);
        *it = std::make_pair(i + 1, i + 1);
        ++i;
    }

    die_unless(i == index + n);
}

//! Run external array test.
//! Testing...
//!     - iterators
//!     - request_write_buffer()
//!     - random access
//!     - remove()
//!     - get_current_max()
//!     - request_further_block()
//!     - wait()
//!     - sizes
//!
//! \param volume Volume
//! \param numwbs Number of write buffer blocks for each external array.
int run_external_array_test(size_t volume)
{
    const size_t block_size = ea_type::block_size;
    const size_t N = volume / sizeof(value_type);
    const size_t num_blocks = foxxll::div_ceil(N, block_size);

    LOG1 << "block_size = " << block_size;
    LOG1 << "N = " << N;
    LOG1 << "num_blocks = " << num_blocks;

    ea_type::pool_type rw_pool(6 * num_blocks, 4);
    ea_type ea(N, &rw_pool);

    {
        foxxll::scoped_print_timer timer("filling", volume);

        die_unless(N >= 10 + 355 + 2 * block_size);

        fill(ea, 0, N);
    }

    {
        foxxll::scoped_print_timer timer("reading and checking", volume);

        die_unless(N > 5 * block_size + 876);
        die_unless(block_size > 34);

        die_unequal(ea.size(), N);

        // fetch first block
        ea.hint_next_block();
        ea.wait_next_blocks();

        die_unless(ea.buffer_size() > 7);

        // Testing iterator...

        for (unsigned i = 0; i < 7; ++i) {
            die_unequal((ea.begin() + i)->first, i + 1);
        }

        die_unless(ea.buffer_size() > 12);

        // Testing random access...

        for (unsigned i = 7; i < 12; ++i) {
            die_unequal(ea[i].first, i + 1);
        }

        // Testing remove...

        ea.remove_items(33);

        die_unequal(ea[33].first, 34u);
        die_unequal(ea.begin().get_index(), 33u);
        die_unequal(ea.begin()->first, 34u);

        // Testing get_next_block_min()...

        unsigned maxround = 0;

        while (ea.get_next_block_min().first < 5 * block_size + 876) {
            switch (maxround) {
            case 0: die_unequal(ea.get_next_block_min().first, 131073u);
                break;
            case 1: die_unequal(ea.get_next_block_min().first, 262145u);
                break;
            case 2: die_unequal(ea.get_next_block_min().first, 393217u);
                break;
            case 3: die_unequal(ea.get_next_block_min().first, 524289u);
                break;
            case 4: die_unequal(ea.get_next_block_min().first, 655361u);
                break;
            }

            ea.hint_next_block();
            ea.wait_next_blocks();
            ++maxround;
        }

        // Testing request_further_block() (called above)...

        die_unless((ea.end() - 1)->first >= 5 * block_size + 876);

        size_t index = 33;

        for (ea_type::iterator it = ea.begin(); it != ea.end(); ++it) {
            progress("Extracting element", index, N);
            die_unequal(it.get_index(), index);
            die_unequal(it->first, index + 1);
            ++index;
        }

        ea.remove_items(ea.buffer_size());
        die_unless(ea.buffer_size() == 0);

        // Extracting the rest...

        size_t round = 0;

        while (!ea.empty()) {
            if (ea.has_em_data() && round % 2 == 0) {
                ea.hint_next_block();
                ea.wait_next_blocks();
            }

            die_unless((size_t)(ea.end() - ea.begin()) == ea.buffer_size());

            for (ea_type::iterator it = ea.begin(); it != ea.end(); ++it) {
                progress("Extracting element", index, N);
                die_unequal(it.get_index(), index);
                die_unequal(it->first, index + 1);
                ++index;
            }

            //std::cout << ea.buffer_size() << " - " << round << "\n";

            if (round % 3 == 0 && ea.buffer_size() > 0) {
                // remove all items but one
                ea.remove_items(ea.buffer_size() - 1);
                index--;
            }
            else {
                ea.remove_items(ea.buffer_size());
            }

            ++round;
        }
    }

    return EXIT_SUCCESS;
}

//! Run test for multiway_merge compatibility.
//! Fills NumEAs external arrays and one internal array.
//! Merges them into one external array and checks the content.
//!
//! \param volume Volume
//! \param numwbs Number of write buffer blocks for each external array.
template <int NumEAs>
int run_multiway_merge(size_t volume)
{
    const size_t block_size = ea_type::block_size;
    const size_t size = volume / sizeof(value_type);
    const size_t num_blocks = foxxll::div_ceil(size, block_size);

    LOG1 << "--- Running run_multiway_merge() test with " << NumEAs << " external arrays";
    LOG1 << "block_size = " << block_size;
    LOG1 << "size = " << size;
    LOG1 << "num_blocks = " << num_blocks;

    std::vector<value_type> v(size);

    {
        foxxll::scoped_print_timer timer("filling vector / internal array for multiway_merge test", volume);
        for (size_t i = 0; i < size; ++i) {
            v[i] = std::make_pair(i + 1, i + 1);
        }
    }

    ia_type ia(v);

    using ea_vector_type = std::vector<ea_type*>;
    using ea_iterator = ea_vector_type::iterator;

    ea_type::pool_type rw_pool1;
    ea_vector_type ealist;
    for (int i = 0; i < NumEAs; ++i)
        ealist.push_back(new ea_type(size, &rw_pool1));

    ea_type::pool_type rw_pool2;
    ea_type out(size * (NumEAs + 1), &rw_pool2);

    {
        foxxll::scoped_print_timer timer("filling external arrays for multiway_merge test", volume* NumEAs);

        for (ea_iterator ea = ealist.begin(); ea != ealist.end(); ++ea)
            fill(*(*ea), 0, size);
    }

    {
        foxxll::scoped_print_timer timer("loading input arrays into RAM and requesting output buffer", volume* NumEAs);

        if (ealist.size())
            rw_pool1.resize_prefetch(ealist.size() * ealist[0]->num_blocks());

        for (ea_iterator ea = ealist.begin(); ea != ealist.end(); ++ea)
        {
            while ((*ea)->has_unhinted_em_data())
                (*ea)->hint_next_block();

            while ((*ea)->num_hinted_blocks())
                (*ea)->wait_next_blocks();
        }

        for (ea_iterator ea = ealist.begin(); ea != ealist.end(); ++ea)
            die_unequal((*ea)->buffer_size(), size);
    }

    if (NumEAs > 0) // test ea ppq_iterators
    {
        foxxll::scoped_print_timer timer("test ea ppq_iterators", volume);

        ea_type::iterator begin = ealist[0]->begin(), end = ealist[0]->end();
        size_t index = 1;

        // prefix operator ++
        for (ea_type::iterator it = begin; it != end; ++it, ++index)
            die_unequal(it->first, index);

        // prefix operator --
        for (ea_type::iterator it = end; it != begin; )
        {
            --it, --index;
            die_unequal(it->first, index);
        }

        die_unequal(index, 1u);

        // addition operator +
        for (ea_type::iterator it = begin; it != end; it = it + 1, ++index)
            die_unequal(it->first, index);

        // subtraction operator -
        for (ea_type::iterator it = end; it != begin; )
        {
            it = it - 1, --index;
            die_unequal(it->first, index);
        }
    }

    {
        foxxll::scoped_print_timer timer("running multiway merge", volume * (NumEAs + 1));

        std::vector<std::pair<ea_type::iterator, ea_type::iterator> > seqs;
        for (ea_iterator ea = ealist.begin(); ea != ealist.end(); ++ea)
            seqs.push_back(std::make_pair((*ea)->begin(), (*ea)->end()));

        seqs.push_back(std::make_pair(ia.begin(), ia.end()));

        ea_type::writer_type ea_writer(out);

        stxxl::potentially_parallel::multiway_merge(
            seqs.begin(), seqs.end(),
            ea_writer.begin(), (NumEAs + 1) * size, value_comp());

        // sequential:
        //__gnu_parallel::multiway_merge(seqs.begin(), seqs.end(), c.begin(),
        //    2*size, value_comp()); // , __gnu_parallel::sequential_tag()
    }

    {
        foxxll::scoped_print_timer timer("checking the order", volume* NumEAs);

        // each index is contained (NumEAs + 1) times
        size_t index = 1 * (NumEAs + 1);

        while (out.has_em_data())
        {
            out.hint_next_block();
            out.wait_next_blocks();

            for (ea_type::iterator it = out.begin(); it != out.end(); ++it)
            {
                progress("Extracting element", index, (NumEAs + 1) * size);
                die_unequal(it->first, index / (NumEAs + 1));

                ++index;
            }

            out.remove_items(out.buffer_size());
        }
    }

    return EXIT_SUCCESS;
}

//! Fill internal array, remove 3 elements, read and check result.
//! \param volume Volume
int run_internal_array_test(size_t volume)
{
    const size_t size = volume / sizeof(value_type);
    LOG1 << "size = " << size;
    die_unless(size > 3);

    std::vector<value_type> v(size);

    {
        foxxll::scoped_print_timer timer("filling vector / internal array", volume);
        for (size_t i = 0; i < size; ++i) {
            v[i] = std::make_pair(i + 1, i + 1);
        }
    }

    ia_type ia(v);

    {
        foxxll::scoped_print_timer timer("reading and checking the order", volume);

        ia.inc_min();
        ia.inc_min(2);

        using iterator = ia_type::iterator;
        size_t index = 4;

        // test iterator
        for (iterator it = ia.begin(); it != ia.end(); ++it) {
            die_unequal(it->first, index++);
        }

        for (size_t i = 3; i < size; ++i) {
            die_unequal(ia.begin()[i - 3].first, i + 1);
            die_unequal(ia[i].first, i + 1);
        }

        die_unequal(index, size + 1);
        die_unequal(ia.size(), size - 3);
    }

    return EXIT_SUCCESS;
}

//! Testing upper_bound compatibility
//! \param volume Volume
int run_upper_bound_test(size_t volume)
{
    const size_t size = volume / sizeof(value_type);
    die_unless(volume > ea_type::block_size);
    LOG1 << "size = " << size;
    die_unless(size > 2000);

    ea_type::pool_type rw_pool;

    std::vector<value_type> v(size);

    {
        foxxll::scoped_print_timer timer("filling vector / internal array", volume);
        for (size_t i = 0; i < size; ++i) {
            v[i] = std::make_pair(i + 1, i + 1);
        }
    }

    ia_type a(v);
    ea_type b(size - 10, &rw_pool);

    {
        foxxll::scoped_print_timer timer("filling external array", volume);

        ea_type::writer_type ea_writer(b, 1);

        ea_type::writer_type::iterator it = ea_writer.begin();

        for (size_t i = 0; i < size - 10; ++i, ++it) {
            *it = std::make_pair(2 * i, 2 * i);
        }

        it = ea_writer.begin();
        for (size_t i = 0; i < size - 10; ++i, ++it) {
            die_unequal(it->first, 2 * i);
        }
    }

    {
        foxxll::scoped_print_timer timer("running upper_bound", 2 * volume);

        b.hint_next_block();
        b.wait_next_blocks();

        value_type minmax = std::make_pair(2000, 2000);

        ea_type::iterator uba = std::upper_bound(a.begin(), a.end(), minmax, value_comp());
        ea_type::iterator ubb = std::upper_bound(b.begin(), b.end(), minmax, value_comp());

        die_unequal((uba - 1)->first, 2000u);
        die_unequal((ubb - 1)->first, 2000u);
        die_unequal(uba->first, 2001u);
        die_unequal(ubb->first, 2002u);
        die_unequal(uba.get_index(), 2000u);
        die_unequal(ubb.get_index(), 1001u);
    }

    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    size_t volume = 512 * 1024 * 1024;
    size_t mwmvolume = 50 * 1024 * 1024;
    size_t iavolume = 10 * 1024 * 1024;

    unsigned numpbs = 1;
    unsigned numwbs = 14;

    stxxl::cmdline_parser cp;
    cp.set_description("STXXL external array test");
    cp.set_author("Thomas Keh <thomas.keh@student.kit.edu>");
    cp.add_bytes('v', "volume", volume,
                 "Volume to fill into the external array, "
                 "default: 512 MiB, 0 = disable test");
    cp.add_bytes('m', "mwmvolume", mwmvolume,
                 "Testing multiway merge of two external arrays - "
                 "the volume of each input array, "
                 "default: 100 MiB, 0 = disable test");
    cp.add_bytes('i', "iavolume", iavolume,
                 "Volume to fill into the internal array, "
                 "default: 10 MiB, 0 = disable test");
    cp.add_uint('n', "num_prefetch_buffers", numpbs,
                "Number of prefetch buffer blocks, default: 1");
    cp.add_uint('w', "num_write_buffers", numwbs,
                "Number of write buffer blocks, default: 14");

    if (!cp.process(argc, argv))
        return EXIT_FAILURE;

    const size_t block_size = 2 * 1024 * 1024 / sizeof(value_type);
    if (volume > 0 && volume / sizeof(value_type) < 5 * block_size + 876) {
        LOG1 << "The volume is too small for this test. It must be >= " <<
        (5 * block_size + 876) * sizeof(value_type);
        return EXIT_FAILURE;
    }

    if (iavolume > 0 && iavolume / sizeof(value_type) < 3) {
        LOG1 << "The internal array volume is too small for this test. "
            "It must be > " << 3;
        return EXIT_FAILURE;
    }

    LOG1 << "volume = " << tlx::format_iec_units(volume);
    LOG1 << "mwmvolume = " << tlx::format_iec_units(mwmvolume);
    LOG1 << "iavolume = " << tlx::format_iec_units(iavolume);
    LOG1 << "sizeof(value_type) = " << tlx::format_iec_units(sizeof(value_type));
    LOG1 << "numpbs = " << numpbs;
    LOG1 << "numwbs = " << numwbs;

    bool succ = EXIT_SUCCESS;

    if (volume > 0) {
        succ = run_external_array_test(volume) && succ;
    }

    if (iavolume > 0) {
        succ = run_internal_array_test(iavolume) && succ;
    }

    if (mwmvolume > 0) {
        succ = run_multiway_merge<0>(mwmvolume) && succ;
        succ = run_multiway_merge<1>(mwmvolume) && succ;
        succ = run_multiway_merge<2>(mwmvolume) && succ;
        succ = run_multiway_merge<3>(mwmvolume) && succ;
        succ = run_multiway_merge<4>(mwmvolume) && succ;
        succ = run_multiway_merge<5>(mwmvolume) && succ;
    }

    succ = run_upper_bound_test(3 * ea_type::block_size * sizeof(value_type)) && succ;

    LOG1 << "success = " << succ;

    return succ;
}
