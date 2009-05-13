/***************************************************************************
 *  containers/test_ext_merger2.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright © 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <limits>
#include <iterator>
#include <stxxl/priority_queue>

typedef int my_type;
typedef stxxl::typed_block<4096, my_type> block_type;


struct dummy_merger
{
    int current, delta;
    dummy_merger() : current(0), delta(1) { }
    void operator () (int current_, int delta_)
    {
        current = current_;
        delta = delta_;
    }
    template <class OutputIterator>
    void multi_merge(OutputIterator b, OutputIterator e)
    {
        while (b != e)
        {
            *b = current;
            ++b;
            current += delta;
        }
    }
};

struct my_cmp : public std::greater<my_type>
{
    my_type min_value() const
    {
        return (std::numeric_limits<my_type>::max)();
    }
};

my_type * make_sequence(dummy_merger & dummy, int l)
{
    my_type * seq = new my_type[l + 1]; // + sentinel
    dummy.multi_merge(seq, seq + l);
    seq[l] = my_cmp().min_value();      // sentinel
    return seq;
}

int main()
{
    stxxl::config::get_instance();
    const int B = block_type::size;
    stxxl::prefetch_pool<block_type> p_pool(1);
    stxxl::write_pool<block_type> w_pool(2);
    dummy_merger dummy;

    if (1) {
        const unsigned volume = 3 * 1024 * 1024; // in KiB
        const unsigned mem_for_queue = 256 * 1024 * 1024;
        typedef stxxl::PRIORITY_QUEUE_GENERATOR<my_type, my_cmp, mem_for_queue, volume / sizeof(my_type)>::result pq_type;
        pq_type pq(mem_for_queue, mem_for_queue);
        pq.push(42);
        assert(pq.top() == 42);
        pq.pop();
        pq.push(2);
        pq.push(0);
        pq.push(3);
        pq.push(1);
        my_type x0, x1, x2, x3;
        x0 = pq.top();
        pq.pop();
        x1 = pq.top();
        pq.pop();
        x2 = pq.top();
        pq.pop();
        x3 = pq.top();
        pq.pop();
        STXXL_MSG("Order: " << x0 << " " << x1 << " " << x2 << " " << x3);
        assert(pq.empty());
    }

    if (1) { // ext_merger test
        stxxl::priority_queue_local::ext_merger<block_type, my_cmp, 5> merger(&p_pool, &w_pool);
        dummy(1, 0);
        merger.insert_segment(dummy, B * 2);
        dummy(2, 0);
        merger.insert_segment(dummy, B * 2);
        dummy(B, 1);
        merger.insert_segment(dummy, B * 4);
        dummy(B, 2);
        merger.insert_segment(dummy, B * 4);
        dummy(B, 4);
        merger.insert_segment(dummy, B * 4);

        std::vector<my_type> output(B * 3);

        // zero length merge
        merger.multi_merge(output.begin(), output.begin());
        merger.multi_merge(output.begin(), output.begin());
        merger.multi_merge(output.begin(), output.begin());

        while (merger.size() > 0) {
            int l = (std::min)(merger.size(), stxxl::uint64(output.size()));
            merger.multi_merge(output.begin(), output.begin() + l);
            STXXL_MSG("merged " << l << " elements: (" << *output.begin() << ", ..., " << *(output.begin() + l - 1) << ")");
        }

        // zero length merge on empty data structure
        merger.multi_merge(output.begin(), output.begin());
        merger.multi_merge(output.begin(), output.begin());
        merger.multi_merge(output.begin(), output.begin());
    } // ext_merger test

    if (1) { // loser_tree test
        stxxl::priority_queue_local::loser_tree<my_type, my_cmp, 8> loser;
        dummy(1, 0);
        my_type * seq0 = make_sequence(dummy, 2 * B);
        dummy(2, 0);
        my_type * seq1 = make_sequence(dummy, 2 * B);
        dummy(B, 1);
        my_type * seq2 = make_sequence(dummy, 4 * B);
        dummy(B, 2);
        my_type * seq3 = make_sequence(dummy, 4 * B);
        dummy(2 * B, 1);
        my_type * seq4 = make_sequence(dummy, 4 * B);
        dummy(B, 4);
        my_type * seq5 = make_sequence(dummy, 4 * B);
        dummy(B, 8);
        my_type * seq6 = make_sequence(dummy, 4 * B);
        dummy(2 * B, 2);
        my_type * seq7 = make_sequence(dummy, 4 * B);

        loser.init();
        loser.insert_segment(seq0, 2 * B);
        loser.insert_segment(seq1, 2 * B);
        loser.insert_segment(seq2, 4 * B);
        loser.insert_segment(seq3, 4 * B);
        loser.insert_segment(seq4, 4 * B);
        if (0) {
            loser.insert_segment(seq5, 4 * B);
            loser.insert_segment(seq6, 4 * B);
            loser.insert_segment(seq7, 4 * B);
        }

        my_type * out = new my_type[2 * B];

        // zero length merge
        loser.multi_merge(out, out);
        loser.multi_merge(out, out);
        loser.multi_merge(out, out);

        while (loser.size() > 0) {
            int l = (std::min<stxxl::uint64>)(loser.size(), B + B / 2 + 1);
            loser.multi_merge(out, out + l);
            STXXL_MSG("merged " << l << " elements: (" << out[0] << ", ..., " << out[l - 1] << ")");
        }

        // zero length merge on empty data structure
        loser.multi_merge(out, out);
        loser.multi_merge(out, out);
        loser.multi_merge(out, out);

        delete[] out;
    } // loser_tree test
}

// vim: et:ts=4:sw=4
