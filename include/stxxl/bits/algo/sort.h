/***************************************************************************
 *  include/stxxl/bits/algo/sort.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2006 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SORT_HEADER
#define STXXL_SORT_HEADER

#include <list>
#include <functional>

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/common/rand.h>
#include <stxxl/bits/mng/adaptor.h>
#include <stxxl/bits/common/simple_vector.h>
#include <stxxl/bits/common/switch.h>
#include <stxxl/bits/common/settings.h>
#include <stxxl/bits/mng/block_alloc_interleaved.h>
#include <stxxl/bits/algo/intksort.h>
#include <stxxl/bits/algo/adaptor.h>
#include <stxxl/bits/algo/async_schedule.h>
#include <stxxl/bits/mng/block_prefetcher.h>
#include <stxxl/bits/mng/buf_writer.h>
#include <stxxl/bits/algo/run_cursor.h>
#include <stxxl/bits/algo/losertree.h>
#include <stxxl/bits/algo/inmemsort.h>
#include <stxxl/bits/parallel.h>


//#define SORT_OPTIMAL_PREFETCHING
//#define INTERLEAVED_ALLOC
//#define STXXL_CHECK_ORDER_IN_SORTS

__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlalgo
//! \{


/*! \internal
 */
namespace sort_local
{
    template <typename BIDTp_, typename ValTp_>
    struct trigger_entry
    {
        typedef BIDTp_ bid_type;
        typedef ValTp_ value_type;

        bid_type bid;
        value_type value;

        operator bid_type ()
        {
            return bid;
        }
    };

    template <typename BIDTp_, typename ValTp_, typename ValueCmp_>
    struct trigger_entry_cmp : public std::binary_function<trigger_entry<BIDTp_, ValTp_>, trigger_entry<BIDTp_, ValTp_>, bool>
    {
        typedef trigger_entry<BIDTp_, ValTp_> trigger_entry_type;
        ValueCmp_ cmp;
        trigger_entry_cmp(ValueCmp_ c) : cmp(c) { }
        trigger_entry_cmp(const trigger_entry_cmp & a) : cmp(a.cmp) { }
        bool operator () (const trigger_entry_type & a, const trigger_entry_type & b) const
        {
            return cmp(a.value, b.value);
        }
    };

    template <typename block_type,
              typename prefetcher_type,
              typename value_cmp>
    struct run_cursor2_cmp
    {
        typedef run_cursor2<block_type, prefetcher_type> cursor_type;
        value_cmp cmp;

        run_cursor2_cmp(value_cmp c) : cmp(c) { }
        run_cursor2_cmp(const run_cursor2_cmp & a) : cmp(a.cmp) { }
        inline bool operator () (const cursor_type & a, const cursor_type & b)
        {
            if (UNLIKELY(b.empty()))
                return true;
            // sentinel emulation
            if (UNLIKELY(a.empty()))
                return false;
            //sentinel emulation

            return (cmp(a.current(), b.current()));
        }
    };

    template <typename block_type, typename bid_type>
    struct write_completion_handler1
    {
        block_type * block;
        bid_type bid;
        request_ptr * req;
        void operator () (request * /*completed_req*/)
        {
            * req = block->read(bid);
        }
    };


    template <
        typename block_type,
        typename run_type,
        typename input_bid_iterator,
        typename value_cmp>
    void
    create_runs(
        input_bid_iterator it,
        run_type ** runs,
        int_type nruns,
        int_type _m,
        value_cmp cmp)
    {
        typedef typename block_type::value_type type;
        typedef typename block_type::bid_type bid_type;
        STXXL_VERBOSE1("stxxl::create_runs nruns=" << nruns << " m=" << _m);

        int_type m2 = _m / 2;
        block_manager * bm = block_manager::get_instance();
        block_type * Blocks1 = new block_type[m2];
        block_type * Blocks2 = new block_type[m2];
        request_ptr * read_reqs1 = new request_ptr[m2];
        request_ptr * read_reqs2 = new request_ptr[m2];
        request_ptr * write_reqs = new request_ptr[m2];
        bid_type * bids = new bid_type[m2];
        write_completion_handler1<block_type, bid_type> * next_run_reads =
            new write_completion_handler1<block_type, bid_type>[m2];
        run_type * run;

        disk_queues::get_instance()->set_priority_op(disk_queue::WRITE);

        int_type i;
        int_type k = 0;
        int_type run_size = 0, next_run_size = 0;

        assert(nruns >= 2);

        run = runs[0];
        run_size = run->size();
        assert(run_size == m2);
        for (i = 0; i < run_size; ++i)
        {
            STXXL_VERBOSE1("stxxl::create_runs posting read " << long(Blocks1[i].elem));
            bids[i] = *(it++);
            read_reqs1[i] = Blocks1[i].read(bids[i]);
        }

        for (i = 0; i < run_size; ++i)
            bm->delete_block(bids[i]);


        run_size = runs[1]->size();

        for (i = 0; i < run_size; ++i)
        {
            STXXL_VERBOSE1("stxxl::create_runs posting read " << long(Blocks2[i].elem));
            bids[i] = *(it++);
            read_reqs2[i] = Blocks2[i].read(bids[i]);
        }

        for (i = 0; i < run_size; ++i)
            bm->delete_block(bids[i]);


        for (k = 0; k < nruns - 1; ++k)
        {
            run = runs[k];
            run_size = run->size();
            assert(run_size == m2);
            next_run_size = runs[k + 1]->size();
            assert((next_run_size == m2) || (next_run_size <= m2 && k == nruns - 2));


            STXXL_VERBOSE1("stxxl::create_runs start waiting read_reqs1");
            wait_all(read_reqs1, run_size);
            STXXL_VERBOSE1("stxxl::create_runs finish waiting read_reqs1");

            if (block_type::has_filler)
                std::sort(
                    ArrayOfSequencesIterator<
                        block_type, typename block_type::value_type, block_type::size
                        >(Blocks1, 0),
                    ArrayOfSequencesIterator<
                        block_type, typename block_type::value_type, block_type::size
                        >(Blocks1, run_size * block_type::size),
                    cmp);
            else
                std::sort(Blocks1[0].elem, Blocks1[run_size].elem, cmp);


            STXXL_VERBOSE1("stxxl::create_runs start waiting write_reqs");
            if (k)
                wait_all(write_reqs, m2);

            STXXL_VERBOSE1("stxxl::create_runs finish waiting write_reqs");

            if (k == nruns - 2)
            {
                // do not need to post read of run k+1
                for (i = 0; i < m2; ++i)
                {
                    STXXL_VERBOSE1("stxxl::create_runs posting write " << long(Blocks1[i].elem));
                    (*run)[i].value = Blocks1[i][0];
                    write_reqs[i] = Blocks1[i].write((*run)[i].bid);
                }
            }
            else
            {
                // do need to post read of run k+1
                int_type runplus2size = runs[k + 2]->size();
                for (i = 0; i < m2; ++i)
                {
                    STXXL_VERBOSE1("stxxl::create_runs posting write " << long(Blocks1[i].elem));
                    (*run)[i].value = Blocks1[i][0];
                    if (i >= runplus2size)
                        write_reqs[i] = Blocks1[i].write((*run)[i].bid);

                    else
                    {
                        next_run_reads[i].block = Blocks1 + i;
                        next_run_reads[i].req = read_reqs1 + i;
                        bm->delete_block(next_run_reads[i].bid = *(it++));
                        write_reqs[i] = Blocks1[i].write((*run)[i].bid, next_run_reads[i]);
                    }
                }
            }
            std::swap(Blocks1, Blocks2);
            std::swap(read_reqs1, read_reqs2);
        }

        run = runs[k];
        run_size = run->size();
        STXXL_VERBOSE1("stxxl::create_runs start waiting read_reqs1");
        wait_all(read_reqs1, run_size);
        STXXL_VERBOSE1("stxxl::create_runs finish waiting read_reqs1");

        if (block_type::has_filler) {
            std::sort(
                ArrayOfSequencesIterator<
                    block_type, typename block_type::value_type, block_type::size
                    >(Blocks1, 0),
                ArrayOfSequencesIterator<
                    block_type, typename block_type::value_type, block_type::size
                    >(Blocks1, run_size * block_type::size),
                cmp);
        } else {
            std::sort(Blocks1[0].elem, Blocks1[run_size].elem, cmp);
        }

        STXXL_VERBOSE1("stxxl::create_runs start waiting write_reqs");
        wait_all(write_reqs, m2);
        STXXL_VERBOSE1("stxxl::create_runs finish waiting write_reqs");

        for (i = 0; i < run_size; ++i)
        {
            STXXL_VERBOSE1("stxxl::create_runs posting write " << long(Blocks1[i].elem));
            (*run)[i].value = Blocks1[i][0];
            write_reqs[i] = Blocks1[i].write((*run)[i].bid);
        }

        STXXL_VERBOSE1("stxxl::create_runs start waiting write_reqs");
        wait_all(write_reqs, run_size);
        STXXL_VERBOSE1("stxxl::create_runs finish waiting write_reqs");

        delete[] Blocks1;
        delete[] Blocks2;
        delete[] read_reqs1;
        delete[] read_reqs2;
        delete[] write_reqs;
        delete[] bids;
        delete[] next_run_reads;
    }


    template <typename block_type, typename run_type, typename value_cmp>
    bool check_sorted_runs(run_type ** runs,
                           unsigned_type nruns,
                           unsigned_type m,
                           value_cmp cmp)
    {
        typedef typename block_type::value_type value_type;

        //STXXL_VERBOSE1("check_sorted_runs  Runs: "<<nruns);
        STXXL_MSG("check_sorted_runs  Runs: " << nruns);
        unsigned_type irun = 0;
        for (irun = 0; irun < nruns; ++irun)
        {
            const unsigned_type nblocks_per_run = runs[irun]->size();
            unsigned_type blocks_left = nblocks_per_run;
            block_type * blocks = new block_type[m];
            request_ptr * reqs = new request_ptr[m];
            value_type last = cmp.min_value();

            for (unsigned_type off = 0; off < nblocks_per_run; off += m)
            {
                const unsigned_type nblocks = STXXL_MIN(blocks_left, m);
                const unsigned_type nelements = nblocks * block_type::size;
                blocks_left -= nblocks;

                for (unsigned_type j = 0; j < nblocks; ++j)
                {
                    reqs[j] = blocks[j].read((*runs[irun])[j + off].bid);
                }
                wait_all(reqs, reqs + nblocks);

                if (off && cmp(blocks[0][0], last))
                {
                    STXXL_MSG("check_sorted_runs  wrong first value in the run " << irun);
                    STXXL_MSG(" first value: " << blocks[0][0]);
                    STXXL_MSG(" last  value: " << last);
                    for (unsigned_type k = 0; k < block_type::size; ++k)
                        STXXL_MSG("Element " << k << " in the block is :" << blocks[0][k]);

                    return false;
                }

                for (unsigned_type j = 0; j < nblocks; ++j)
                {
                    if (!(blocks[j][0] == (*runs[irun])[j + off].value))
                    {
                        STXXL_MSG("check_sorted_runs  wrong trigger in the run " << irun << " block " << (j + off));
                        STXXL_MSG("                   trigger value: " << (*runs[irun])[j + off].value);
                        STXXL_MSG("Data in the block:");
                        for (unsigned_type k = 0; k < block_type::size; ++k)
                            STXXL_MSG("Element " << k << " in the block is :" << blocks[j][k]);

                        STXXL_MSG("BIDS:");
                        for (unsigned_type k = 0; k < nblocks; ++k)
                        {
                            if (k == j)
                                STXXL_MSG("Bad one comes next.");
                            STXXL_MSG("BID " << (k + off) << " is: " << ((*runs[irun])[k + off].bid));
                        }

                        return false;
                    }
                }
                if (!stxxl::is_sorted(
                        ArrayOfSequencesIterator<
                            block_type, typename block_type::value_type, block_type::size
                            >(blocks, 0),
                        ArrayOfSequencesIterator<
                            block_type, typename block_type::value_type, block_type::size
                            >(blocks, nelements),
                        cmp))
                {
                    STXXL_MSG("check_sorted_runs  wrong order in the run " << irun);
                    STXXL_MSG("Data in blocks:");
                    for (unsigned_type j = 0; j < nblocks; ++j)
                    {
                        for (unsigned_type k = 0; k < block_type::size; ++k)
                            STXXL_MSG("     Element " << k << " in block " << (j + off) << " is :" << blocks[j][k]);
                    }
                    STXXL_MSG("BIDS:");
                    for (unsigned_type k = 0; k < nblocks; ++k)
                    {
                        STXXL_MSG("BID " << (k + off) << " is: " << ((*runs[irun])[k + off].bid));
                    }

                    return false;
                }

                last = blocks[nblocks - 1][block_type::size - 1];
            }

            assert(blocks_left == 0);
            delete[] reqs;
            delete[] blocks;
        }

        return true;
    }


    template <typename block_type, typename run_type, typename value_cmp>
    void merge_runs(run_type ** in_runs, int_type nruns, run_type * out_run, unsigned_type _m, value_cmp cmp
                    )
    {
        typedef typename block_type::bid_type bid_type;
        typedef typename block_type::value_type value_type;
        typedef block_prefetcher<block_type, typename run_type::iterator> prefetcher_type;
        typedef run_cursor2<block_type, prefetcher_type> run_cursor_type;
        typedef run_cursor2_cmp<block_type, prefetcher_type, value_cmp> run_cursor2_cmp_type;

        int_type i;
        run_type consume_seq(out_run->size());

        int_type * prefetch_seq = new int_type[out_run->size()];

        typename run_type::iterator copy_start = consume_seq.begin();
        for (i = 0; i < nruns; i++)
        {
            // TODO: try to avoid copy
            copy_start = std::copy(
                in_runs[i]->begin(),
                in_runs[i]->end(),
                copy_start);
        }

        std::stable_sort(consume_seq.begin(), consume_seq.end(),
                         trigger_entry_cmp<bid_type, value_type, value_cmp>(cmp));

        int_type disks_number = config::get_instance()->disks_number();

#ifdef PLAY_WITH_OPT_PREF
        const int_type n_write_buffers = 4 * disks_number;
#else
        const int_type n_prefetch_buffers = STXXL_MAX(2 * disks_number, (3 * (int_type(_m) - nruns) / 4));
        const int_type n_write_buffers = STXXL_MAX(2 * disks_number, int_type(_m) - nruns - n_prefetch_buffers);
 #ifdef SORT_OPTIMAL_PREFETCHING
        // heuristic
        const int_type n_opt_prefetch_buffers = 2 * disks_number + (3 * (n_prefetch_buffers - 2 * disks_number)) / 10;
 #endif
#endif

#ifdef SORT_OPTIMAL_PREFETCHING
        compute_prefetch_schedule(
            consume_seq,
            prefetch_seq,
            n_opt_prefetch_buffers,
            disks_number);
#else
        for (i = 0; i < out_run->size(); i++)
            prefetch_seq[i] = i;

#endif

        prefetcher_type prefetcher(consume_seq.begin(),
                                   consume_seq.end(),
                                   prefetch_seq,
                                   nruns + n_prefetch_buffers);

        buffered_writer<block_type> writer(n_write_buffers, n_write_buffers / 2);

        int_type out_run_size = out_run->size();

        block_type * out_buffer = writer.get_free_block();

//If parallelism is activated, one can still fall back to the
//native merge routine by setting stxxl::SETTINGS::native_merge= true, //otherwise, it is used anyway.

        if (do_parallel_merge())
        {
#if STXXL_PARALLEL_MULTIWAY_MERGE

// begin of STL-style merging

            typedef stxxl::int64 diff_type;
            typedef std::pair<typename block_type::iterator, typename block_type::iterator> sequence;
            typedef typename std::vector<sequence>::size_type seqs_size_type;
            std::vector<sequence> seqs(nruns);
            std::vector<block_type *> buffers(nruns);

            for (int_type i = 0; i < nruns; i++)                //initialize sequences
            {
                buffers[i] = prefetcher.pull_block();           //get first block of each run
                seqs[i] = std::make_pair(buffers[i]->begin(), buffers[i]->end());
                //this memory location stays the same, only the data is exchanged
            }

 #ifdef STXXL_CHECK_ORDER_IN_SORTS
            value_type last_elem = cmp.min_value();
 #endif

            for (int_type j = 0; j < out_run_size; ++j)                 //for the whole output run, out_run_size is in blocks
            {
                diff_type rest = block_type::size;                      //elements still to merge for this output block

                STXXL_VERBOSE1("output block " << j);
                do {
                    value_type * min_last_element = NULL;               //no element found yet
                    diff_type total_size = 0;

                    for (seqs_size_type i = 0; i < seqs.size(); i++)
                    {
                        if (seqs[i].first == seqs[i].second)
                            continue; //run empty

                        if (min_last_element == NULL)
                            min_last_element = &(*(seqs[i].second - 1));
                        else
                            min_last_element = cmp(*min_last_element, *(seqs[i].second - 1)) ? min_last_element : &(*(seqs[i].second - 1));

                        total_size += seqs[i].second - seqs[i].first;
                        STXXL_VERBOSE1("last " << *(seqs[i].second - 1) << " block size " << (seqs[i].second - seqs[i].first));
                    }

                    assert(min_last_element != NULL);           //there must be some element

                    STXXL_VERBOSE1("min_last_element " << min_last_element << " total size " << total_size + (block_type::size - rest));

                    diff_type less_equal_than_min_last = 0;
                    //locate this element in all sequences
                    for (seqs_size_type i = 0; i < seqs.size(); i++)
                    {
                        if (seqs[i].first == seqs[i].second)
                            continue; //empty subsequence

                        typename block_type::iterator position = std::upper_bound(seqs[i].first, seqs[i].second, *min_last_element, cmp);
                        STXXL_VERBOSE1("greater equal than " << position - seqs[i].first);
                        less_equal_than_min_last += position - seqs[i].first;
                    }

                    STXXL_VERBOSE1("finished loop");

                    ptrdiff_t output_size = (std::min)(less_equal_than_min_last, rest);         //at most rest elements

                    STXXL_VERBOSE1("before merge" << output_size);

                    stxxl::parallel::multiway_merge(seqs.begin(), seqs.end(), out_buffer->end() - rest, cmp, output_size);
                    //sequence iterators are progressed appropriately

                    STXXL_VERBOSE1("after merge");

                    (*out_run)[j].value = (*out_buffer)[0];                     //save smallest value

                    rest -= output_size;

                    STXXL_VERBOSE1("so long");

                    for (seqs_size_type i = 0; i < seqs.size(); i++)
                    {
                        if (seqs[i].first == seqs[i].second)                                    //run empty
                        {
                            if (prefetcher.block_consumed(buffers[i]))
                            {
                                seqs[i].first = buffers[i]->begin();                            //reset iterator
                                seqs[i].second = buffers[i]->end();
                                STXXL_VERBOSE1("block ran empty " << i);
                            }
                            else
                            {
                                seqs.erase(seqs.begin() + i);                                   //remove this sequence
                                buffers.erase(buffers.begin() + i);
                                STXXL_VERBOSE1("seq removed " << i);
                            }
                        }
                    }
                } while (rest > 0 && seqs.size() > 0);

 #ifdef STXXL_CHECK_ORDER_IN_SORTS
                if (!stxxl::is_sorted(out_buffer->begin(), out_buffer->end(), cmp))
                {
                    for (value_type * i = out_buffer->begin() + 1; i != out_buffer->end(); i++)
                        if (cmp(*i, *(i - 1)))
                        {
                            STXXL_VERBOSE1("Error at position " << (i - out_buffer->begin()));
                        }
                    assert(false);
                }

                if (j > 0) //do not check in first iteration
                    assert(cmp((*out_buffer)[0], last_elem) == false);

                last_elem = (*out_buffer)[block_type::size - 1];
 #endif


                out_buffer = writer.write(out_buffer, (*out_run)[j].bid);
            }

// end of STL-style merging

#else
            assert(false);
#endif
        }
        else
        {
// begin of native merging procedure

            loser_tree<run_cursor_type, run_cursor2_cmp_type, block_type::size>
            losers(&prefetcher, nruns, run_cursor2_cmp_type(cmp));

#ifdef STXXL_CHECK_ORDER_IN_SORTS
            value_type last_elem = cmp.min_value();
#endif

            for (i = 0; i < out_run_size; ++i)
            {
                losers.multi_merge(out_buffer->elem);
                (*out_run)[i].value = *(out_buffer->elem);

#ifdef STXXL_CHECK_ORDER_IN_SORTS
                assert(stxxl::is_sorted(
                           out_buffer->begin(),
                           out_buffer->end(),
                           cmp));

                if (i)
                    assert(cmp(*(out_buffer->elem), last_elem) == false);

                last_elem = (*out_buffer).elem[block_type::size - 1];
#endif

                out_buffer = writer.write(out_buffer, (*out_run)[i].bid);
            }

// end of native merging procedure
        }

        delete[] prefetch_seq;

        block_manager * bm = block_manager::get_instance();
        for (i = 0; i < nruns; ++i)
        {
            unsigned_type sz = in_runs[i]->size();
            for (unsigned_type j = 0; j < sz; ++j)
                bm->delete_block((*in_runs[i])[j].bid);


            delete in_runs[i];
        }
    }


    template <typename block_type,
              typename alloc_strategy,
              typename input_bid_iterator,
              typename value_cmp>
    simple_vector<trigger_entry<typename block_type::bid_type, typename block_type::value_type> > *
    sort_blocks(input_bid_iterator input_bids,
                unsigned_type _n,
                unsigned_type _m,
                value_cmp cmp
                )
    {
        typedef typename block_type::value_type type;
        typedef typename block_type::bid_type bid_type;
        typedef trigger_entry<bid_type, type> trigger_entry_type;
        typedef simple_vector<trigger_entry_type> run_type;
        typedef typename interleaved_alloc_traits<alloc_strategy>::strategy interleaved_alloc_strategy;

        unsigned_type m2 = _m / 2;
        unsigned_type full_runs = _n / m2;
        unsigned_type partial_runs = ((_n % m2) ? 1 : 0);
        unsigned_type nruns = full_runs + partial_runs;
        unsigned_type i;

        config * cfg = config::get_instance();
        block_manager * mng = block_manager::get_instance();
        const unsigned_type ndisks = cfg->disks_number();

        //STXXL_VERBOSE ("n=" << _n << " nruns=" << nruns << "=" << full_runs << "+" << partial_runs);

#if STXXL_IO_STATS
        stats * iostats = stats::get_instance();
        iostats += 0;
        // iostats->reset();
#endif

        double begin = timestamp(), after_runs_creation, end;

        run_type ** runs = new run_type *[nruns];

        for (i = 0; i < full_runs; i++)
            runs[i] = new run_type(m2);


        if (partial_runs)
            runs[i] = new run_type(_n - full_runs * m2);


        for (i = 0; i < nruns; ++i)
        {
            // FIXME: why has an alloc_strategy to take two arguments disk_index.begin(), disk_index.end() ???
            mng->new_blocks(alloc_strategy(0, ndisks),
                            trigger_entry_iterator<typename run_type::iterator, block_type::raw_size>(runs[i]->begin()),
                            trigger_entry_iterator<typename run_type::iterator, block_type::raw_size>(runs[i]->end()));
        }

        sort_local::create_runs<block_type,
                                run_type,
                                input_bid_iterator,
                                value_cmp>(input_bids, runs, nruns, _m, cmp);

        after_runs_creation = timestamp();

#ifdef COUNT_WAIT_TIME
        double io_wait_after_rf = stxxl::wait_time_counter;
        io_wait_after_rf += 0.0;
#endif

        disk_queues::get_instance()->set_priority_op(disk_queue::WRITE);

        // Optimal merging: merge r = pow(nruns,1/ceil(log(nruns)/log(m))) runs at once

        const int_type merge_factor = static_cast<int_type>(ceil(pow(nruns, 1. / ceil(log(double(nruns)) /
                                                                                      log(double(_m))))));
        run_type ** new_runs;

        while (nruns > 1)
        {
            int_type new_nruns = div_and_round_up(nruns, merge_factor);
            STXXL_VERBOSE("Starting new merge phase: nruns: " << nruns <<
                          " opt_merge_factor: " << merge_factor << " m:" << _m << " new_nruns: " << new_nruns);

            new_runs = new run_type *[new_nruns];

            int_type runs_left = nruns;
            int_type cur_out_run = 0;
            int_type blocks_in_new_run = 0;

            while (runs_left > 0)
            {
                int_type runs2merge = STXXL_MIN(runs_left, merge_factor);
                blocks_in_new_run = 0;
                for (unsigned_type i = nruns - runs_left; i < (nruns - runs_left + runs2merge); i++)
                    blocks_in_new_run += runs[i]->size();

                // allocate run
                new_runs[cur_out_run++] = new run_type(blocks_in_new_run);
                runs_left -= runs2merge;
            }
            // allocate blocks for the new runs
            if (cur_out_run == 1 && blocks_in_new_run == int_type(_n) && (input_bids->storage->get_id() == -1))
            {
                // if we sort a file we can reuse the input bids for the output
                input_bid_iterator cur = input_bids;
                for (int_type i = 0; cur != (input_bids + _n); ++cur)
                {
                    (*new_runs[0])[i++].bid = *cur;
                }

                bid_type & firstBID = (*new_runs[0])[0].bid;
                if (firstBID.storage->get_id() != -1)
                {
                    // the first block does not belong to the file
                    // need to reallocate it
                    mng->new_blocks(FR(), &firstBID, (&firstBID) + 1);
                }
                bid_type & lastBID = (*new_runs[0])[_n - 1].bid;
                if (lastBID.storage->get_id() != -1)
                {
                    // the first block does not belong to the file
                    // need to reallocate it
                    mng->new_blocks(FR(), &lastBID, (&lastBID) + 1);
                }
            }
            else
            {
                mng->new_blocks(interleaved_alloc_strategy(new_nruns, 0, ndisks),
                                RunsToBIDArrayAdaptor2<block_type::raw_size, run_type>(new_runs, 0, new_nruns, blocks_in_new_run),
                                RunsToBIDArrayAdaptor2<block_type::raw_size, run_type>(new_runs, _n, new_nruns, blocks_in_new_run));
            }
            // merge all
            runs_left = nruns;
            cur_out_run = 0;
            while (runs_left > 0)
            {
                int_type runs2merge = STXXL_MIN(runs_left, merge_factor);
#ifdef STXXL_CHECK_ORDER_IN_SORTS
                assert((check_sorted_runs<block_type, run_type, value_cmp>(runs + nruns - runs_left, runs2merge, m2, cmp)));
#endif
                STXXL_VERBOSE("Merging " << runs2merge << " runs");
                merge_runs<block_type, run_type>(runs + nruns - runs_left,
                                                 runs2merge, *(new_runs + (cur_out_run++)), _m, cmp
                                                 );
                runs_left -= runs2merge;
            }

            nruns = new_nruns;
            delete[] runs;
            runs = new_runs;
        }

        run_type * result = *runs;
        delete[] runs;


        end = timestamp();
        (void)(begin);

        STXXL_VERBOSE("Elapsed time        : " << end - begin << " s. Run creation time: " <<
                      after_runs_creation - begin << " s");
#if STXXL_IO_STATS
        STXXL_VERBOSE("reads               : " << iostats->get_reads());
        STXXL_VERBOSE("writes              : " << iostats->get_writes());
        STXXL_VERBOSE("read time           : " << iostats->get_read_time() << " s");
        STXXL_VERBOSE("write time          : " << iostats->get_write_time() << " s");
        STXXL_VERBOSE("parallel read time  : " << iostats->get_pread_time() << " s");
        STXXL_VERBOSE("parallel write time : " << iostats->get_pwrite_time() << " s");
        STXXL_VERBOSE("parallel io time    : " << iostats->get_pio_time() << " s");
#endif
#ifdef COUNT_WAIT_TIME
        STXXL_VERBOSE("Time in I/O wait(rf): " << io_wait_after_rf << " s");
        STXXL_VERBOSE("Time in I/O wait    : " << stxxl::wait_time_counter << " s");
#endif

        return result;
    }
}


//! \brief External sorting routine for records that allow only comparisons
//! \param first object of model of \c ext_random_access_iterator concept
//! \param last object of model of \c ext_random_access_iterator concept
//! \param cmp comparison object
//! \param M amount of memory for internal use (in bytes)
//! \remark Implements external merge sort described in [Dementiev & Sanders'03]
//! \remark non-stable
template <typename ExtIterator_, typename StrictWeakOrdering_>
void sort(ExtIterator_ first, ExtIterator_ last, StrictWeakOrdering_ cmp, unsigned_type M)
{
    typedef simple_vector<sort_local::trigger_entry<typename ExtIterator_::bid_type,
                                                    typename ExtIterator_::vector_type::value_type> > run_type;

    typedef typename ExtIterator_::vector_type::value_type value_type;
    typedef typename ExtIterator_::block_type block_type;

    // verify strict weak ordering of the sentinels
    assert(!cmp(cmp.min_value(), cmp.min_value()));
    assert(cmp(cmp.min_value(), cmp.max_value()));
    assert(!cmp(cmp.max_value(), cmp.max_value()));

    unsigned_type n = 0;

    block_manager * mng = block_manager::get_instance();

    first.flush();

    if ((last - first) * sizeof(value_type) * sort_memory_usage_factor() < M)
    {
        stl_in_memory_sort(first, last, cmp);
    }
    else
    {
        assert(2 * block_type::raw_size * sort_memory_usage_factor() <= M);

        if (first.block_offset())
        {
            if (last.block_offset())              // first and last element are
            // not the first elements of their block
            {
                typename ExtIterator_::block_type * first_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::block_type * last_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::bid_type first_bid, last_bid;
                request_ptr req;

                req = first_block->read(*first.bid());
                mng->new_blocks(FR(), &first_bid, (&first_bid) + 1);                // try to overlap
                mng->new_blocks(FR(), &last_bid, (&last_bid) + 1);
                req->wait();


                req = last_block->read(*last.bid());

                unsigned_type i = 0;
                for ( ; i < first.block_offset(); ++i)
                {
                    first_block->elem[i] = cmp.min_value();
                }

                req->wait();


                req = first_block->write(first_bid);
                for (i = last.block_offset(); i < block_type::size; ++i)
                {
                    last_block->elem[i] = cmp.max_value();
                }

                req->wait();


                req = last_block->write(last_bid);

                n = last.bid() - first.bid() + 1;

                std::swap(first_bid, *first.bid());
                std::swap(last_bid, *last.bid());

                req->wait();


                delete first_block;
                delete last_block;

                run_type * out =
                    sort_local::sort_blocks<
                        typename ExtIterator_::block_type,
                        typename ExtIterator_::vector_type::alloc_strategy,
                        typename ExtIterator_::bids_container_iterator>
                                        (first.bid(), n, M / sort_memory_usage_factor() / block_type::raw_size, cmp);


                first_block = new typename ExtIterator_::block_type;
                last_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::block_type * sorted_first_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::block_type * sorted_last_block = new typename ExtIterator_::block_type;
                request_ptr * reqs = new request_ptr[2];

                reqs[0] = first_block->read(first_bid);
                reqs[1] = sorted_first_block->read((*(out->begin())).bid);

                reqs[0]->wait();
                reqs[1]->wait();

                reqs[0] = last_block->read(last_bid);
                reqs[1] = sorted_last_block->read(((*out)[out->size() - 1]).bid);

                for (i = first.block_offset(); i < block_type::size; i++)
                {
                    first_block->elem[i] = sorted_first_block->elem[i];
                }

                reqs[0]->wait();
                reqs[1]->wait();

                req = first_block->write(first_bid);

                for (i = 0; i < last.block_offset(); ++i)
                {
                    last_block->elem[i] = sorted_last_block->elem[i];
                }

                req->wait();

                req = last_block->write(last_bid);

                mng->delete_block(out->begin()->bid);
                mng->delete_block((*out)[out->size() - 1].bid);

                *first.bid() = first_bid;
                *last.bid() = last_bid;

                typename run_type::iterator it = out->begin();
                ++it;
                typename ExtIterator_::bids_container_iterator cur_bid = first.bid();
                ++cur_bid;

                for ( ; cur_bid != last.bid(); ++cur_bid, ++it)
                {
                    *cur_bid = (*it).bid;
                }

                delete first_block;
                delete sorted_first_block;
                delete sorted_last_block;
                delete[] reqs;
                delete out;

                req->wait();


                delete last_block;
            }
            else
            {
                // first element is
                // not the first element of its block
                typename ExtIterator_::block_type * first_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::bid_type first_bid;
                request_ptr req;

                req = first_block->read(*first.bid());
                mng->new_blocks(FR(), &first_bid, (&first_bid) + 1);                // try to overlap
                req->wait();


                unsigned_type i = 0;
                for ( ; i < first.block_offset(); ++i)
                {
                    first_block->elem[i] = cmp.min_value();
                }

                req = first_block->write(first_bid);

                n = last.bid() - first.bid();

                std::swap(first_bid, *first.bid());

                req->wait();


                delete first_block;

                run_type * out =
                    sort_local::sort_blocks<
                        typename ExtIterator_::block_type,
                        typename ExtIterator_::vector_type::alloc_strategy,
                        typename ExtIterator_::bids_container_iterator>
                                        (first.bid(), n, M / sort_memory_usage_factor() / block_type::raw_size, cmp);


                first_block = new typename ExtIterator_::block_type;

                typename ExtIterator_::block_type * sorted_first_block = new typename ExtIterator_::block_type;

                request_ptr * reqs = new request_ptr[2];

                reqs[0] = first_block->read(first_bid);
                reqs[1] = sorted_first_block->read((*(out->begin())).bid);

                reqs[0]->wait();
                reqs[1]->wait();

                for (i = first.block_offset(); i < block_type::size; ++i)
                {
                    first_block->elem[i] = sorted_first_block->elem[i];
                }

                req = first_block->write(first_bid);

                mng->delete_block(out->begin()->bid);

                *first.bid() = first_bid;

                typename run_type::iterator it = out->begin();
                ++it;
                typename ExtIterator_::bids_container_iterator cur_bid = first.bid();
                ++cur_bid;

                for ( ; cur_bid != last.bid(); ++cur_bid, ++it)
                {
                    *cur_bid = (*it).bid;
                }

                *cur_bid = (*it).bid;

                delete sorted_first_block;
                delete[] reqs;
                delete out;

                req->wait();

                delete first_block;
            }
        }
        else
        {
            if (last.block_offset())            // last is
            // not the first element of its block
            {
                typename ExtIterator_::block_type * last_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::bid_type last_bid;
                request_ptr req;
                unsigned_type i;

                req = last_block->read(*last.bid());
                mng->new_blocks(FR(), &last_bid, (&last_bid) + 1);
                req->wait();


                for (i = last.block_offset(); i < block_type::size; ++i)
                {
                    last_block->elem[i] = cmp.max_value();
                }

                req = last_block->write(last_bid);

                n = last.bid() - first.bid() + 1;

                std::swap(last_bid, *last.bid());

                req->wait();


                delete last_block;

                run_type * out =
                    sort_local::sort_blocks<
                        typename ExtIterator_::block_type,
                        typename ExtIterator_::vector_type::alloc_strategy,
                        typename ExtIterator_::bids_container_iterator>
                                        (first.bid(), n, M / sort_memory_usage_factor() / block_type::raw_size, cmp);


                last_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::block_type * sorted_last_block = new typename ExtIterator_::block_type;
                request_ptr * reqs = new request_ptr[2];

                reqs[0] = last_block->read(last_bid);
                reqs[1] = sorted_last_block->read(((*out)[out->size() - 1]).bid);

                reqs[0]->wait();
                reqs[1]->wait();

                for (i = 0; i < last.block_offset(); ++i)
                {
                    last_block->elem[i] = sorted_last_block->elem[i];
                }

                req = last_block->write(last_bid);

                mng->delete_block((*out)[out->size() - 1].bid);

                *last.bid() = last_bid;

                typename run_type::iterator it = out->begin();
                typename ExtIterator_::bids_container_iterator cur_bid = first.bid();

                for ( ; cur_bid != last.bid(); ++cur_bid, ++it)
                {
                    *cur_bid = (*it).bid;
                }

                delete sorted_last_block;
                delete[] reqs;
                delete out;

                req->wait();

                delete last_block;
            }
            else
            {
                // first and last element are first elements of their of blocks
                n = last.bid() - first.bid();

                run_type * out =
                    sort_local::sort_blocks<typename ExtIterator_::block_type,
                                            typename ExtIterator_::vector_type::alloc_strategy,
                                            typename ExtIterator_::bids_container_iterator>
                                        (first.bid(), n, M / sort_memory_usage_factor() / block_type::raw_size, cmp);

                typename run_type::iterator it = out->begin();
                typename ExtIterator_::bids_container_iterator cur_bid = first.bid();

                for ( ; cur_bid != last.bid(); ++cur_bid, ++it)
                {
                    *cur_bid = (*it).bid;
                }

                delete out;
            }
        }
    }

#ifdef STXXL_CHECK_ORDER_IN_SORTS
    assert(stxxl::is_sorted(first, last, cmp));
#endif
}

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_SORT_HEADER
