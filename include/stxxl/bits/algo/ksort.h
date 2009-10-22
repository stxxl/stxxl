/***************************************************************************
 *  include/stxxl/bits/algo/ksort.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_KSORT_HEADER
#define STXXL_KSORT_HEADER

#include <cmath>

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/common/rand.h>
#include <stxxl/bits/mng/adaptor.h>
#include <stxxl/bits/common/simple_vector.h>
#include <stxxl/bits/common/switch.h>
#include <stxxl/bits/mng/block_alloc_interleaved.h>
#include <stxxl/bits/algo/intksort.h>
#include <stxxl/bits/algo/adaptor.h>
#include <stxxl/bits/algo/async_schedule.h>
#include <stxxl/bits/mng/block_prefetcher.h>
#include <stxxl/bits/mng/buf_writer.h>
#include <stxxl/bits/algo/run_cursor.h>
#include <stxxl/bits/algo/losertree.h>
#include <stxxl/bits/algo/inmemsort.h>
#include <stxxl/bits/algo/sort_base.h>
#include <stxxl/bits/common/is_sorted.h>


//#define INTERLEAVED_ALLOC

#define OPT_MERGING

__STXXL_BEGIN_NAMESPACE

//! \weakgroup stllayer STL-user layer
//! Layer which groups STL compatible algorithms and containers

//! \weakgroup stlalgo Algorithms
//! \ingroup stllayer
//! Algorithms with STL-compatible interface
//! \{

/*! \internal
 */
namespace ksort_local
{
    template <typename _BIDTp, typename _KeyTp>
    struct trigger_entry
    {
        typedef _BIDTp bid_type;
        typedef _KeyTp key_type;

        bid_type bid;
        key_type key;

        operator bid_type ()
        {
            return bid;
        }
    };


    template <typename _BIDTp, typename _KeyTp>
    inline bool operator < (const trigger_entry<_BIDTp, _KeyTp> & a,
                            const trigger_entry<_BIDTp, _KeyTp> & b)
    {
        return (a.key < b.key);
    }

    template <typename _BIDTp, typename _KeyTp>
    inline bool operator > (const trigger_entry<_BIDTp, _KeyTp> & a,
                            const trigger_entry<_BIDTp, _KeyTp> & b)
    {
        return (a.key > b.key);
    }

    template <typename type, typename key_type1>
    struct type_key
    {
        typedef key_type1 key_type;
        key_type key;
        type * ptr;

        type_key() { }
        type_key(key_type k, type * p) : key(k), ptr(p)
        { }
    };

    template <typename type, typename key1>
    bool operator < (const type_key<type, key1> & a, const type_key<type, key1> & b)
    {
        return a.key < b.key;
    }

    template <typename type, typename key1>
    bool operator > (const type_key<type, key1> & a, const type_key<type, key1> & b)
    {
        return a.key > b.key;
    }


    template <typename block_type, typename bid_type>
    struct write_completion_handler
    {
        block_type * block;
        bid_type bid;
        request_ptr * req;
        void operator () (request * /*completed_req*/)
        {
            * req = block->read(bid);
        }
    };

    template <typename type_key_,
              typename block_type,
              typename run_type,
              typename input_bid_iterator,
              typename key_extractor>
    inline void write_out(
        type_key_ * begin,
        type_key_ * end,
        block_type * & cur_blk,
        const block_type * end_blk,
        int_type & out_block,
        int_type & out_pos,
        run_type & run,
        write_completion_handler<block_type, typename block_type::bid_type> * & next_read,
        typename block_type::bid_type * & bids,
        request_ptr * write_reqs,
        request_ptr * read_reqs,
        input_bid_iterator & it,
        key_extractor keyobj)
    {
        typedef typename block_type::bid_type bid_type;
        typedef typename block_type::type type;

        type * elem = cur_blk->elem;
        for (type_key_ * p = begin; p < end; p++)
        {
            elem[out_pos++] = *(p->ptr);

            if (out_pos >= block_type::size)
            {
                run[out_block].key = keyobj(*(cur_blk->elem));

                if (cur_blk < end_blk)
                {
                    next_read->block = cur_blk;
                    next_read->req = read_reqs + out_block;
                    read_reqs[out_block] = NULL;
                    bids[out_block] = next_read->bid = *(it++);

                    write_reqs[out_block] = cur_blk->write(
                        run[out_block].bid,
                        // postpone read of block from next run
                        // after write of block from this run
                        *(next_read++));
                }
                else
                {
                    write_reqs[out_block] = cur_blk->write(run[out_block].bid);
                }

                cur_blk++;
                elem = cur_blk->elem;
                out_block++;
                out_pos = 0;
            }
        }
    }

    template <
        typename block_type,
        typename run_type,
        typename input_bid_iterator,
        typename key_extractor>
    void
    create_runs(
        input_bid_iterator it,
        run_type ** runs,
        const unsigned_type nruns,
        const unsigned_type m2,
        key_extractor keyobj)
    {
        typedef typename block_type::value_type type;
        typedef typename block_type::bid_type bid_type;
        typedef typename key_extractor::key_type key_type;
        typedef type_key<type, key_type> type_key_;

        block_manager * bm = block_manager::get_instance();
        block_type * Blocks1 = new block_type[m2];
        block_type * Blocks2 = new block_type[m2];
        bid_type * bids = new bid_type[m2];
        type_key_ * refs1 = new type_key_[m2 * Blocks1->size];
        type_key_ * refs2 = new type_key_[m2 * Blocks1->size];
        request_ptr * read_reqs = new request_ptr[m2];
        request_ptr * write_reqs = new request_ptr[m2];
        write_completion_handler<block_type, bid_type> * next_run_reads =
            new write_completion_handler<block_type, bid_type>[m2];

        run_type * run;
        run = *runs;
        int_type run_size = (*runs)->size();
        key_type offset = 0;
        const int log_k1 =
            static_cast<int>(ceil(log2((m2 * block_type::size * sizeof(type_key_) / STXXL_L2_SIZE) ?
                                       (double(m2 * block_type::size * sizeof(type_key_) / STXXL_L2_SIZE)) : 2.)));
        const int log_k2 = int(log2(double(m2 * Blocks1->size))) - log_k1 - 1;
        STXXL_VERBOSE("log_k1: " << log_k1 << " log_k2:" << log_k2);
        const int_type k1 = 1 << log_k1;
        const int_type k2 = 1 << log_k2;
        int_type * bucket1 = new int_type[k1];
        int_type * bucket2 = new int_type[k2];
        int_type i;

        disk_queues::get_instance()->set_priority_op(disk_queue::WRITE);

        for (i = 0; i < run_size; i++)
        {
            bids[i] = *(it++);
            read_reqs[i] = Blocks1[i].read(bids[i]);
        }

        unsigned_type k = 0;
        const int shift1 = sizeof(key_type) * 8 - log_k1;
        const int shift2 = shift1 - log_k2;
        STXXL_VERBOSE("shift1: " << shift1 << " shift2:" << shift2);

        for ( ; k < nruns; k++)
        {
            run = runs[k];
            run_size = run->size();

            std::fill(bucket1, bucket1 + k1, 0);

            type_key_ * ref_ptr = refs1;
            for (i = 0; i < run_size; i++)
            {
                if (k)
                    write_reqs[i]->wait();

                read_reqs[i]->wait();
                bm->delete_block(bids[i]);

                classify_block(Blocks1[i].begin(), Blocks1[i].end(), ref_ptr, bucket1, offset, shift1, keyobj);
            }

            exclusive_prefix_sum(bucket1, k1);
            classify(refs1, refs1 + run_size * Blocks1->size, refs2, bucket1,
                     offset, shift1);

            int_type out_block = 0;
            int_type out_pos = 0;
            unsigned_type next_run_size = (k < nruns - 1) ? (runs[k + 1]->size()) : 0;

            // recurse on each bucket
            type_key_ * c = refs2;
            type_key_ * d = refs1;
            block_type * cur_blk = Blocks2;
            block_type * end_blk = Blocks2 + next_run_size;
            write_completion_handler<block_type, bid_type> * next_read = next_run_reads;

            for (i = 0; i < k1; i++)
            {
                type_key_ * cEnd = refs2 + bucket1[i];
                type_key_ * dEnd = refs1 + bucket1[i];

                l1sort(c, cEnd, d, bucket2, k2,
                       offset + (key_type(1) << key_type(shift1)) * key_type(i), shift2);         // key_type,key_type,... paranoia

                write_out(
                    d, dEnd, cur_blk, end_blk,
                    out_block, out_pos, *run, next_read, bids,
                    write_reqs, read_reqs, it, keyobj);

                c = cEnd;
                d = dEnd;
            }

            std::swap(Blocks1, Blocks2);
        }

        wait_all(write_reqs, m2);

        delete[] bucket1;
        delete[] bucket2;
        delete[] refs1;
        delete[] refs2;
        delete[] Blocks1;
        delete[] Blocks2;
        delete[] bids;
        delete[] next_run_reads;
        delete[] read_reqs;
        delete[] write_reqs;
    }

    template <typename block_type,
              typename prefetcher_type,
              typename key_extractor>
    struct run_cursor2_cmp
    {
        typedef run_cursor2<block_type, prefetcher_type> cursor_type;
        key_extractor keyobj;
        run_cursor2_cmp(key_extractor keyobj_)
        {
            keyobj = keyobj_;
        }
        inline bool operator () (const cursor_type & a, const cursor_type & b)
        {
            if (UNLIKELY(b.empty()))
                return true;
            // sentinel emulation
            if (UNLIKELY(a.empty()))
                return false;
            //sentinel emulation

            return (keyobj(a.current()) < keyobj(b.current()));
        }

    private:
        run_cursor2_cmp() { }
    };


    template <typename record_type, typename key_extractor>
    class key_comparison : public std::binary_function<record_type, record_type, bool>
    {
        key_extractor ke;

    public:
        key_comparison() { }
        key_comparison(key_extractor ke_) : ke(ke_) { }
        bool operator () (const record_type & a, const record_type & b) const
        {
            return ke(a) < ke(b);
        }
    };


    template <typename block_type, typename run_type, typename key_ext_>
    bool check_ksorted_runs(run_type ** runs,
                            unsigned_type nruns,
                            unsigned_type m,
                            key_ext_ keyext)
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
            value_type last = keyext.min_value();

            for (unsigned_type off = 0; off < nblocks_per_run; off += m)
            {
                const unsigned_type nblocks = STXXL_MIN(blocks_left, m);
                const unsigned_type nelements = nblocks * block_type::size;
                blocks_left -= nblocks;

                for (unsigned_type j = 0; j < nblocks; ++j)
                {
                    reqs[j] = blocks[j].read((*runs[irun])[off + j].bid);
                }
                wait_all(reqs, reqs + nblocks);

                if (off && (keyext(blocks[0][0]) < keyext(last)))
                {
                    STXXL_MSG("check_sorted_runs  wrong first value in the run " << irun);
                    STXXL_MSG(" first value: " << blocks[0][0] << " with key" << keyext(blocks[0][0]));
                    STXXL_MSG(" last  value: " << last << " with key" << keyext(last));
                    for (unsigned_type k = 0; k < block_type::size; ++k)
                        STXXL_MSG("Element " << k << " in the block is :" << blocks[0][k] << " key: " << keyext(blocks[0][k]));
                    return false;
                }

                for (unsigned_type j = 0; j < nblocks; ++j)
                {
                    if (keyext(blocks[j][0]) != (*runs[irun])[off + j].key)
                    {
                        STXXL_MSG("check_sorted_runs  wrong trigger in the run " << irun << " block " << (off + j));
                        STXXL_MSG("                   trigger value: " << (*runs[irun])[off + j].key);
                        STXXL_MSG("Data in the block:");
                        for (unsigned_type k = 0; k < block_type::size; ++k)
                            STXXL_MSG("Element " << k << " in the block is :" << blocks[j][k] << " with key: " << keyext(blocks[j][k]));

                        STXXL_MSG("BIDS:");
                        for (unsigned_type k = 0; k < nblocks; ++k)
                        {
                            if (k == j)
                                STXXL_MSG("Bad one comes next.");
                            STXXL_MSG("BID " << (off + k) << " is: " << ((*runs[irun])[off + k].bid));
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
                        key_comparison<value_type, key_ext_>()))
                {
                    STXXL_MSG("check_sorted_runs  wrong order in the run " << irun);
                    STXXL_MSG("Data in blocks:");
                    for (unsigned_type j = 0; j < nblocks; ++j)
                    {
                        for (unsigned_type k = 0; k < block_type::size; ++k)
                            STXXL_MSG("     Element " << k << " in block " << (off + j) << " is :" << blocks[j][k] << " with key: " << keyext(blocks[j][k]));
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


    template <typename block_type, typename run_type, typename key_extractor>
    void merge_runs(run_type ** in_runs, unsigned_type nruns, run_type * out_run, unsigned_type _m, key_extractor keyobj)
    {
        typedef block_prefetcher<block_type, typename run_type::iterator> prefetcher_type;
        typedef run_cursor2<block_type, prefetcher_type> run_cursor_type;

        unsigned_type i;
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
        std::stable_sort(consume_seq.begin(), consume_seq.end());

        unsigned disks_number = config::get_instance()->disks_number();

#ifdef PLAY_WITH_OPT_PREF
        const int_type n_write_buffers = 4 * disks_number;
#else
        const int_type n_prefetch_buffers = STXXL_MAX(int_type(2 * disks_number), (3 * (int_type(_m) - int_type(nruns)) / 4));
        STXXL_VERBOSE("Prefetch buffers " << n_prefetch_buffers);
        const int_type n_write_buffers = STXXL_MAX(int_type(2 * disks_number), int_type(_m) - int_type(nruns) - int_type(n_prefetch_buffers));
        STXXL_VERBOSE("Write buffers " << n_write_buffers);
        // heuristic
        const int_type n_opt_prefetch_buffers = 2 * int_type(disks_number) + (3 * (int_type(n_prefetch_buffers) - int_type(2 * disks_number))) / 10;
        STXXL_VERBOSE("Prefetch buffers " << n_opt_prefetch_buffers);
#endif

#if STXXL_SORT_OPTIMAL_PREFETCHING
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

        unsigned_type out_run_size = out_run->size();

        run_cursor2_cmp<block_type, prefetcher_type, key_extractor> cmp(keyobj);
        loser_tree<
            run_cursor_type,
            run_cursor2_cmp<block_type, prefetcher_type, key_extractor> >
                losers(&prefetcher, nruns, cmp);

        block_type * out_buffer = writer.get_free_block();

        for (i = 0; i < out_run_size; i++)
        {
            losers.multi_merge(out_buffer->elem, out_buffer->elem + block_type::size);
            (*out_run)[i].key = keyobj(out_buffer->elem[0]);
            out_buffer = writer.write(out_buffer, (*out_run)[i].bid);
        }

        delete[] prefetch_seq;

        block_manager * bm = block_manager::get_instance();
        for (i = 0; i < nruns; i++)
        {
            unsigned_type sz = in_runs[i]->size();
            for (unsigned_type j = 0; j < sz; j++)
                bm->delete_block((*in_runs[i])[j].bid);

            delete in_runs[i];
        }
    }


    template <typename block_type,
              typename alloc_strategy,
              typename input_bid_iterator,
              typename key_extractor>

    simple_vector<trigger_entry<typename block_type::bid_type, typename key_extractor::key_type> > *
    ksort_blocks(input_bid_iterator input_bids, unsigned_type _n, unsigned_type _m, key_extractor keyobj)
    {
        typedef typename block_type::value_type type;
        typedef typename key_extractor::key_type key_type;
        typedef typename block_type::bid_type bid_type;
        typedef trigger_entry<bid_type, typename key_extractor::key_type> trigger_entry_type;
        typedef simple_vector<trigger_entry_type> run_type;
        typedef typename interleaved_alloc_traits<alloc_strategy>::strategy interleaved_alloc_strategy;

        unsigned_type m2 = STXXL_DIVRU(_m, 2);
        const unsigned_type m2_rf = m2 * block_type::raw_size /
                                    (block_type::raw_size + block_type::size * sizeof(type_key<type, key_type>));
        STXXL_VERBOSE("Reducing number of blocks in a run from " << m2 << " to " <<
                      m2_rf << " due to key size: " << sizeof(typename key_extractor::key_type) << " bytes");
        m2 = m2_rf;
        unsigned_type full_runs = _n / m2;
        unsigned_type partial_runs = ((_n % m2) ? 1 : 0);
        unsigned_type nruns = full_runs + partial_runs;
        unsigned_type i;

        config * cfg = config::get_instance();
        block_manager * mng = block_manager::get_instance();
        int ndisks = cfg->disks_number();

        STXXL_VERBOSE("n=" << _n << " nruns=" << nruns << "=" << full_runs << "+" << partial_runs);

        double begin = timestamp(), after_runs_creation, end;

        run_type ** runs = new run_type *[nruns];

        for (i = 0; i < full_runs; i++)
            runs[i] = new run_type(m2);


#ifdef INTERLEAVED_ALLOC
        if (partial_runs)
        {
            unsigned_type last_run_size = _n - full_runs * m2;
            runs[i] = new run_type(last_run_size);

            mng->new_blocks(interleaved_alloc_strategy(nruns, 0, ndisks),
                            RunsToBIDArrayAdaptor2<block_type::raw_size, run_type>
                                (runs, 0, nruns, last_run_size),
                            RunsToBIDArrayAdaptor2<block_type::raw_size, run_type>
                                (runs, _n, nruns, last_run_size));
        }
        else
            mng->new_blocks(interleaved_alloc_strategy(nruns, 0, ndisks),
                            RunsToBIDArrayAdaptor<block_type::raw_size, run_type>
                                (runs, 0, nruns),
                            RunsToBIDArrayAdaptor<block_type::raw_size, run_type>
                                (runs, _n, nruns));

#else
        if (partial_runs)
            runs[i] = new run_type(_n - full_runs * m2);


        for (i = 0; i < nruns; i++)
        {
            mng->new_blocks(alloc_strategy(0, ndisks),
                            trigger_entry_iterator<typename run_type::iterator, block_type::raw_size>(runs[i]->begin()),
                            trigger_entry_iterator<typename run_type::iterator, block_type::raw_size>(runs[i]->end()));
        }
#endif

        create_runs<block_type,
                    run_type,
                    input_bid_iterator,
                    key_extractor>(input_bids, runs, nruns, m2, keyobj);

        after_runs_creation = timestamp();

        double io_wait_after_rf = stats::get_instance()->get_io_wait_time();

        disk_queues::get_instance()->set_priority_op(disk_queue::WRITE);

        // Optimal merging: merge r = pow(nruns,1/ceil(log(nruns)/log(m))) at once

        const int_type merge_factor = static_cast<int_type>(ceil(pow(nruns, 1. / ceil(log(double(nruns)) / log(double(_m))))));
        run_type ** new_runs;

        while (nruns > 1)
        {
            int_type new_nruns = STXXL_DIVRU(nruns, merge_factor);
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
            // allocate blocks in the new runs
            if (cur_out_run == 1 && blocks_in_new_run == int_type(_n) && !input_bids->is_managed())
            {
                // if we sort a file we can reuse the input bids for the output
                input_bid_iterator cur = input_bids;
                for (int_type i = 0; cur != (input_bids + _n); ++cur)
                {
                    (*new_runs[0])[i++].bid = *cur;
                }

                bid_type & firstBID = (*new_runs[0])[0].bid;
                if (firstBID.is_managed())
                {
                    // the first block does not belong to the file
                    // need to reallocate it
                    mng->new_block(FR(), firstBID);
                }
                bid_type & lastBID = (*new_runs[0])[_n - 1].bid;
                if (lastBID.is_managed())
                {
                    // the first block does not belong to the file
                    // need to reallocate it
                    mng->new_block(FR(), lastBID);
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
#if STXXL_CHECK_ORDER_IN_SORTS
                assert((check_ksorted_runs<block_type, run_type, key_extractor>(runs + nruns - runs_left, runs2merge, m2, keyobj)));
#endif
                STXXL_VERBOSE("Merging " << runs2merge << " runs");
                merge_runs<block_type, run_type, key_extractor>(runs + nruns - runs_left,
                                                                runs2merge, *(new_runs + (cur_out_run++)), _m, keyobj);
                runs_left -= runs2merge;
            }

            nruns = new_nruns;
            delete[] runs;
            runs = new_runs;
        }

        run_type * result = *runs;
        delete[] runs;

        end = timestamp();

        STXXL_VERBOSE("Elapsed time        : " << end - begin << " s. Run creation time: " <<
                      after_runs_creation - begin << " s");
        STXXL_VERBOSE("Time in I/O wait(rf): " << io_wait_after_rf << " s");
        STXXL_VERBOSE(*stats::get_instance());
        STXXL_UNUSED(begin + io_wait_after_rf);

        return result;
    }
}


/*! \page key_extractor Key extractor concept

   Model of \b Key \b extractor concept must:
   - define type \b key_type ,
   - provide \b operator() that returns key value of an object of user type
   - provide \b max_value method that returns a value that is \b greater than all
   other objects of user type in respect to the key obtained by this key extractor ,
   - provide \b min_value method that returns a value that is \b less than all
   other objects of user type in respect to the key obtained by this key extractor ,
   - operator > , operator <, operator == and operator != on type \b key_type must be defined.

   Example: extractor class \b GetWeight, that extracts weight from an \b Edge
 \verbatim

   struct Edge
   {
     unsigned src,dest,weight;
     Edge(unsigned s,unsigned d,unsigned w):src(s),dest(d),weight(w){}
   };

   struct GetWeight
   {
    typedef unsigned key_type;
    key_type operator() (const Edge & e)
    {
                  return e.weight;
    }
    Edge min_value() const { return Edge(0,0,(std::numeric_limits<key_type>::min)()); };
    Edge max_value() const { return Edge(0,0,(std::numeric_limits<key_type>::max)()); };
   };
 \endverbatim

 */


//! \brief External sorting routine for records with keys
//! \param first_ object of model of \c ext_random_access_iterator concept
//! \param last_ object of model of \c ext_random_access_iterator concept
//! \param keyobj \link key_extractor key extractor \endlink object
//! \param M__ amount of memory for internal use (in bytes)
//! \remark Implements external merge sort described in [Dementiev & Sanders'03]
//! \remark Order in the result is non-stable
template <typename ExtIterator_, typename KeyExtractor_>
void ksort(ExtIterator_ first_, ExtIterator_ last_, KeyExtractor_ keyobj, unsigned_type M__)
{
    typedef simple_vector<ksort_local::trigger_entry<typename ExtIterator_::bid_type,
                                                     typename KeyExtractor_::key_type> > run_type;
    typedef typename ExtIterator_::vector_type::value_type value_type;
    typedef typename ExtIterator_::block_type block_type;


    unsigned_type n = 0;
    block_manager * mng = block_manager::get_instance();

    first_.flush();

    if ((last_ - first_) * sizeof(value_type) < M__)
    {
        stl_in_memory_sort(first_, last_, ksort_local::key_comparison<value_type, KeyExtractor_>(keyobj));
    }
    else
    {
        assert(2 * block_type::raw_size <= M__);

        if (first_.block_offset())
        {
            if (last_.block_offset())            // first and last element reside
            // not in the beginning of the block
            {
                typename ExtIterator_::block_type * first_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::block_type * last_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::bid_type first_bid, last_bid;
                request_ptr req;

                req = first_block->read(*first_.bid());
                mng->new_block(FR(), first_bid);                // try to overlap
                mng->new_block(FR(), last_bid);
                req->wait();


                req = last_block->read(*last_.bid());

                unsigned_type i = 0;
                for ( ; i < first_.block_offset(); i++)
                {
                    first_block->elem[i] = keyobj.min_value();
                }

                req->wait();

                req = first_block->write(first_bid);
                for (i = last_.block_offset(); i < block_type::size; i++)
                {
                    last_block->elem[i] = keyobj.max_value();
                }

                req->wait();

                req = last_block->write(last_bid);

                n = last_.bid() - first_.bid() + 1;

                std::swap(first_bid, *first_.bid());
                std::swap(last_bid, *last_.bid());

                req->wait();

                delete first_block;
                delete last_block;

                run_type * out =
                    ksort_local::ksort_blocks<
                        typename ExtIterator_::block_type,
                        typename ExtIterator_::vector_type::alloc_strategy_type,
                        typename ExtIterator_::bids_container_iterator,
                        KeyExtractor_>
                        (first_.bid(), n, M__ / block_type::raw_size, keyobj);


                first_block = new typename ExtIterator_::block_type;
                last_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::block_type * sorted_first_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::block_type * sorted_last_block = new typename ExtIterator_::block_type;
                request_ptr * reqs = new request_ptr[2];

                reqs[0] = first_block->read(first_bid);
                reqs[1] = sorted_first_block->read((*(out->begin())).bid);
                wait_all(reqs, 2);

                reqs[0] = last_block->read(last_bid);
                reqs[1] = sorted_last_block->read(((*out)[out->size() - 1]).bid);

                for (i = first_.block_offset(); i < block_type::size; i++)
                {
                    first_block->elem[i] = sorted_first_block->elem[i];
                }
                wait_all(reqs, 2);

                req = first_block->write(first_bid);

                for (i = 0; i < last_.block_offset(); i++)
                {
                    last_block->elem[i] = sorted_last_block->elem[i];
                }

                req->wait();


                req = last_block->write(last_bid);

                mng->delete_block(out->begin()->bid);
                mng->delete_block((*out)[out->size() - 1].bid);

                *first_.bid() = first_bid;
                *last_.bid() = last_bid;

                typename run_type::iterator it = out->begin();
                it++;
                typename ExtIterator_::bids_container_iterator cur_bid = first_.bid();
                cur_bid++;

                for ( ; cur_bid != last_.bid(); cur_bid++, it++)
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
                // first element resides
                // not in the beginning of the block

                typename ExtIterator_::block_type * first_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::bid_type first_bid;
                request_ptr req;

                req = first_block->read(*first_.bid());
                mng->new_block(FR(), first_bid);                // try to overlap
                req->wait();


                unsigned_type i = 0;
                for ( ; i < first_.block_offset(); i++)
                {
                    first_block->elem[i] = keyobj.min_value();
                }

                req = first_block->write(first_bid);

                n = last_.bid() - first_.bid();

                std::swap(first_bid, *first_.bid());

                req->wait();

                delete first_block;

                run_type * out =
                    ksort_local::ksort_blocks<
                        typename ExtIterator_::block_type,
                        typename ExtIterator_::vector_type::alloc_strategy_type,
                        typename ExtIterator_::bids_container_iterator,
                        KeyExtractor_>
                        (first_.bid(), n, M__ / block_type::raw_size, keyobj);


                first_block = new typename ExtIterator_::block_type;

                typename ExtIterator_::block_type * sorted_first_block = new typename ExtIterator_::block_type;

                request_ptr * reqs = new request_ptr[2];

                reqs[0] = first_block->read(first_bid);
                reqs[1] = sorted_first_block->read((*(out->begin())).bid);
                wait_all(reqs, 2);

                for (i = first_.block_offset(); i < block_type::size; i++)
                {
                    first_block->elem[i] = sorted_first_block->elem[i];
                }

                req = first_block->write(first_bid);

                mng->delete_block(out->begin()->bid);

                *first_.bid() = first_bid;

                typename run_type::iterator it = out->begin();
                it++;
                typename ExtIterator_::bids_container_iterator cur_bid = first_.bid();
                cur_bid++;

                for ( ; cur_bid != last_.bid(); cur_bid++, it++)
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
            if (last_.block_offset())            // last element resides
            // not in the beginning of the block
            {
                typename ExtIterator_::block_type * last_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::bid_type last_bid;
                request_ptr req;
                unsigned_type i;

                req = last_block->read(*last_.bid());
                mng->new_block(FR(), last_bid);
                req->wait();

                for (i = last_.block_offset(); i < block_type::size; i++)
                {
                    last_block->elem[i] = keyobj.max_value();
                }

                req = last_block->write(last_bid);

                n = last_.bid() - first_.bid() + 1;

                std::swap(last_bid, *last_.bid());

                req->wait();

                delete last_block;

                run_type * out =
                    ksort_local::ksort_blocks<
                        typename ExtIterator_::block_type,
                        typename ExtIterator_::vector_type::alloc_strategy_type,
                        typename ExtIterator_::bids_container_iterator,
                        KeyExtractor_>
                        (first_.bid(), n, M__ / block_type::raw_size, keyobj);


                last_block = new typename ExtIterator_::block_type;
                typename ExtIterator_::block_type * sorted_last_block = new typename ExtIterator_::block_type;
                request_ptr * reqs = new request_ptr[2];

                reqs[0] = last_block->read(last_bid);
                reqs[1] = sorted_last_block->read(((*out)[out->size() - 1]).bid);
                wait_all(reqs, 2);

                for (i = 0; i < last_.block_offset(); i++)
                {
                    last_block->elem[i] = sorted_last_block->elem[i];
                }

                req = last_block->write(last_bid);

                mng->delete_block((*out)[out->size() - 1].bid);

                *last_.bid() = last_bid;

                typename run_type::iterator it = out->begin();
                typename ExtIterator_::bids_container_iterator cur_bid = first_.bid();

                for ( ; cur_bid != last_.bid(); cur_bid++, it++)
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
                // first and last element reside in the beginning of blocks
                n = last_.bid() - first_.bid();

                run_type * out =
                    ksort_local::ksort_blocks<
                        typename ExtIterator_::block_type,
                        typename ExtIterator_::vector_type::alloc_strategy_type,
                        typename ExtIterator_::bids_container_iterator,
                        KeyExtractor_>
                        (first_.bid(), n, M__ / block_type::raw_size, keyobj);

                typename run_type::iterator it = out->begin();
                typename ExtIterator_::bids_container_iterator cur_bid = first_.bid();

                for ( ; cur_bid != last_.bid(); cur_bid++, it++)
                {
                    *cur_bid = (*it).bid;
                }

                delete out;
            }
        }
    }

#if STXXL_CHECK_ORDER_IN_SORTS
    assert(stxxl::is_sorted(first_, last_, ksort_local::key_comparison<value_type, KeyExtractor_>()));
#endif
}

template <typename record_type>
struct ksort_defaultkey
{
    typedef typename record_type::key_type key_type;
    key_type operator () (const record_type & obj) const
    {
        return obj.key();
    }
    record_type max_value() const
    {
        return record_type::max_value();
    }
    record_type min_value() const
    {
        return record_type::min_value();
    }
};


//! \brief External sorting routine for records with keys
//! \param first_ object of model of \c ext_random_access_iterator concept
//! \param last_ object of model of \c ext_random_access_iterator concept
//! \param M__ amount of buffers for internal use
//! \remark Implements external merge sort described in [Dementiev & Sanders'03]
//! \remark Order in the result is non-stable
/*!
   Record's type must:
   - provide \b max_value method that returns an object that is \b greater than all
   other objects of user type ,
   - provide \b min_value method that returns an object that is \b less than all
   other objects of user type ,
   - \b operator \b < that must define strict weak ordering on record's values
    (<A HREF="http://www.sgi.com/tech/stl/StrictWeakOrdering.html">see what it is</A>).
 */
template <typename ExtIterator_>
void ksort(ExtIterator_ first_, ExtIterator_ last_, unsigned_type M__)
{
    ksort(first_, last_,
          ksort_defaultkey<typename ExtIterator_::vector_type::value_type>(), M__);
}


//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_KSORT_HEADER
// vim: et:ts=4:sw=4
