/***************************************************************************
 *  include/stxxl/bits/algo/ksort.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008-2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALGO_KSORT_HEADER
#define STXXL_ALGO_KSORT_HEADER

#include <algorithm>
#include <functional>
#include <utility>

#include <tlx/define.hpp>
#include <tlx/logger.hpp>
#include <tlx/simple_vector.hpp>

#include <foxxll/common/onoff_switch.hpp>
#include <foxxll/common/utils.hpp>
#include <foxxll/mng/async_schedule.hpp>
#include <foxxll/mng/block_alloc_strategy_interleaved.hpp>
#include <foxxll/mng/block_manager.hpp>
#include <foxxll/mng/block_prefetcher.hpp>
#include <foxxll/mng/buf_writer.hpp>

#include <stxxl/bits/algo/bid_adapter.h>
#include <stxxl/bits/algo/inmemsort.h>
#include <stxxl/bits/algo/intksort.h>
#include <stxxl/bits/algo/losertree.h>
#include <stxxl/bits/algo/run_cursor.h>
#include <stxxl/bits/algo/sort_base.h>
#include <stxxl/bits/algo/trigger_entry.h>
#include <stxxl/bits/common/is_sorted.h>
#include <stxxl/bits/common/rand.h>

//#define INTERLEAVED_ALLOC

#define OPT_MERGING

namespace stxxl {

//! \addtogroup stllayer

//! \defgroup stlalgo Algorithms
//! \ingroup stllayer
//! Algorithms with STL-compatible interface
//! \{

/*! \internal
 */
namespace ksort_local {

template <typename BIDType, typename KeyType>
struct trigger_entry
{
    using bid_type = BIDType;
    using key_type = KeyType;

    bid_type bid;
    key_type key;

    operator bid_type ()
    {
        return bid;
    }
};

template <typename BIDType, typename KeyType>
inline bool operator < (const trigger_entry<BIDType, KeyType>& a,
                        const trigger_entry<BIDType, KeyType>& b)
{
    return (a.key < b.key);
}

template <typename BIDType, typename KeyType>
inline bool operator > (const trigger_entry<BIDType, KeyType>& a,
                        const trigger_entry<BIDType, KeyType>& b)
{
    return (a.key > b.key);
}

template <typename Type, typename KeyType>
struct type_key
{
    using key_type = KeyType;
    key_type key;
    Type* ptr;

    type_key() { }
    type_key(key_type k, Type* p) : key(k), ptr(p)
    { }
};

template <typename Type, typename KeyType>
bool operator < (const type_key<Type, KeyType>& a, const type_key<Type, KeyType>& b)
{
    return a.key < b.key;
}

template <typename Type, typename KeyType>
bool operator > (const type_key<Type, KeyType>& a, const type_key<Type, KeyType>& b)
{
    return a.key > b.key;
}

template <typename BlockType, typename BidType>
struct write_completion_handler
{
    BlockType* block;
    BidType bid;
    foxxll::request_ptr* req;
    void operator () (foxxll::request* /*completed_req*/, bool /* success */)
    {
        * req = block->read(bid);
    }
};

template <typename TypeKey,
          typename BlockType,
          typename RunType,
          typename InputBidIterator,
          typename KeyExtractor>
inline void write_out(
    TypeKey* begin,
    TypeKey* end,
    BlockType*& cur_blk,
    const BlockType* end_blk,
    size_t& out_block,
    size_t& out_pos,
    RunType& run,
    write_completion_handler<BlockType, typename BlockType::bid_type>*& next_read,
    typename BlockType::bid_type*& bids,
    foxxll::request_ptr* write_reqs,
    foxxll::request_ptr* read_reqs,
    InputBidIterator& it,
    KeyExtractor keyobj)
{
    using type = typename BlockType::type;

    type* elem = cur_blk->elem;
    for (TypeKey* p = begin; p < end; p++)
    {
        elem[out_pos++] = *(p->ptr);

        if (out_pos >= BlockType::size)
        {
            run[out_block].key = keyobj(*(cur_blk->elem));

            if (cur_blk < end_blk)
            {
                next_read->block = cur_blk;
                next_read->req = read_reqs + out_block;
                read_reqs[out_block] = nullptr;
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
    typename BlockType,
    typename RunType,
    typename InputBidIterator,
    typename KeyExtractor>
void
create_runs(
    InputBidIterator it,
    RunType** runs,
    const size_t nruns,
    const size_t m2,
    KeyExtractor keyobj)
{
    using type = typename BlockType::value_type;
    using bid_type = typename BlockType::bid_type;
    using key_type = typename KeyExtractor::key_type;
    using type_key_ = type_key<type, key_type>;
    using request_ptr = foxxll::request_ptr;

    foxxll::block_manager* bm = foxxll::block_manager::get_instance();
    BlockType* Blocks1 = new BlockType[m2];
    BlockType* Blocks2 = new BlockType[m2];
    bid_type* bids = new bid_type[m2];
    type_key_* refs1 = new type_key_[m2 * Blocks1->size];
    type_key_* refs2 = new type_key_[m2 * Blocks1->size];
    request_ptr* read_reqs = new request_ptr[m2];
    request_ptr* write_reqs = new request_ptr[m2];
    write_completion_handler<BlockType, bid_type>* next_run_reads =
        new write_completion_handler<BlockType, bid_type>[m2];

    RunType* run;
    run = *runs;
    size_t run_size = (*runs)->size();
    key_type offset = 0;
    const unsigned int log_k1 = tlx::integer_log2_ceil(
        (m2 * BlockType::size * sizeof(type_key_) / STXXL_L2_SIZE) ?
        (m2 * BlockType::size * sizeof(type_key_) / STXXL_L2_SIZE) : 2);
    const unsigned int log_k2 = tlx::integer_log2_floor(m2 * Blocks1->size) - log_k1 - 1;
    LOG1 << "log_k1: " << log_k1 << " log_k2:" << log_k2;
    const size_t k1 = size_t(1) << log_k1;
    const size_t k2 = size_t(1) << log_k2;
    size_t* bucket1 = new size_t[k1];
    size_t* bucket2 = new size_t[k2];
    size_t i;

    foxxll::disk_queues::get_instance()->set_priority_op(foxxll::request_queue::WRITE);

    for (i = 0; i < run_size; i++)
    {
        bids[i] = *(it++);
        read_reqs[i] = Blocks1[i].read(bids[i]);
    }

    size_t k = 0;
    const int shift1 = (int)(sizeof(key_type) * 8 - log_k1);
    const int shift2 = shift1 - log_k2;
    LOG1 << "shift1: " << shift1 << " shift2:" << shift2;

    for ( ; k < nruns; k++)
    {
        run = runs[k];
        run_size = run->size();

        std::fill(bucket1, bucket1 + k1, 0);

        type_key_* ref_ptr = refs1;
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

        size_t out_block = 0;
        size_t out_pos = 0;
        size_t next_run_size = (k < nruns - 1) ? (runs[k + 1]->size()) : 0;

        // recurse on each bucket
        type_key_* c = refs2;
        type_key_* d = refs1;
        BlockType* cur_blk = Blocks2;
        BlockType* end_blk = Blocks2 + next_run_size;
        write_completion_handler<BlockType, bid_type>* next_read = next_run_reads;

        for (i = 0; i < k1; i++)
        {
            type_key_* cEnd = refs2 + bucket1[i];
            type_key_* dEnd = refs1 + bucket1[i];

            l1sort(c, cEnd, d, bucket2, k2,
                   offset + (key_type(1) << key_type(shift1)) * key_type(i), shift2);             // key_type,key_type,... paranoia

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

template <typename BlockType,
          typename prefetcher_type,
          typename KeyExtractor>
struct run_cursor2_cmp : public std::binary_function<
                             run_cursor2<BlockType, prefetcher_type>,
                             run_cursor2<BlockType, prefetcher_type>,
                             bool
                             >
{
    using cursor_type = run_cursor2<BlockType, prefetcher_type>;
    KeyExtractor keyobj;
    explicit run_cursor2_cmp(KeyExtractor _keyobj)
        : keyobj(_keyobj)
    { }
    inline bool operator () (const cursor_type& a, const cursor_type& b) const
    {
        if (TLX_UNLIKELY(b.empty()))
            return true;
        // sentinel emulation
        if (TLX_UNLIKELY(a.empty()))
            return false;
        //sentinel emulation

        return (keyobj(a.current()) < keyobj(b.current()));
    }

private:
    run_cursor2_cmp() { }
};

template <typename RecordType, typename KeyExtractor>
class key_comparison : public std::binary_function<RecordType, RecordType, bool>
{
    KeyExtractor ke;

public:
    key_comparison() { }
    explicit key_comparison(KeyExtractor ke_) : ke(ke_) { }
    bool operator () (const RecordType& a, const RecordType& b) const
    {
        return ke(a) < ke(b);
    }
};

template <typename BlockType, typename RunType, typename KeyExtractor>
bool check_ksorted_runs(RunType** runs,
                        size_t nruns,
                        size_t m,
                        KeyExtractor keyext)
{
    using block_type = BlockType;
    using value_type = typename BlockType::value_type;
    using request_ptr = foxxll::request_ptr;

    LOG1 << "check_ksorted_runs  Runs: " << nruns;
    size_t irun = 0;
    for (irun = 0; irun < nruns; ++irun)
    {
        const size_t nblocks_per_run = runs[irun]->size();
        size_t blocks_left = nblocks_per_run;
        block_type* blocks = new block_type[m];
        request_ptr* reqs = new request_ptr[m];
        value_type last = keyext.min_value();

        for (size_t off = 0; off < nblocks_per_run; off += m)
        {
            const size_t nblocks = std::min(blocks_left, m);
            const size_t nelements = nblocks * block_type::size;
            blocks_left -= nblocks;

            for (size_t j = 0; j < nblocks; ++j)
            {
                reqs[j] = blocks[j].read((*runs[irun])[off + j].bid);
            }
            wait_all(reqs, reqs + nblocks);

            if (off && (keyext(blocks[0][0]) < keyext(last)))
            {
                LOG1 << "check_sorted_runs  wrong first value in the run " << irun;
                LOG1 << " first value: " << blocks[0][0] << " with key" << keyext(blocks[0][0]);
                LOG1 << " last  value: " << last << " with key" << keyext(last);
                for (size_t k = 0; k < block_type::size; ++k)
                    LOG1 << "Element " << k << " in the block is :" << blocks[0][k] << " key: " << keyext(blocks[0][k]);

                delete[] reqs;
                delete[] blocks;
                return false;
            }

            for (size_t j = 0; j < nblocks; ++j)
            {
                if (keyext(blocks[j][0]) != (*runs[irun])[off + j].key)
                {
                    LOG1 << "check_sorted_runs  wrong trigger in the run " << irun << " block " << (off + j);
                    LOG1 << "                   trigger value: " << (*runs[irun])[off + j].key;
                    LOG1 << "Data in the block:";
                    for (size_t k = 0; k < block_type::size; ++k)
                        LOG1 << "Element " << k << " in the block is :" << blocks[j][k] << " with key: " << keyext(blocks[j][k]);

                    LOG1 << "BIDS:";
                    for (size_t k = 0; k < nblocks; ++k)
                    {
                        if (k == j)
                            LOG1 << "Bad one comes next.";
                        LOG1 << "BID " << (off + k) << " is: " << ((*runs[irun])[off + k].bid);
                    }

                    delete[] reqs;
                    delete[] blocks;
                    return false;
                }
            }
            if (!stxxl::is_sorted(make_element_iterator(blocks, 0),
                                  make_element_iterator(blocks, nelements),
                                  key_comparison<value_type, KeyExtractor>()))
            {
                LOG1 << "check_sorted_runs  wrong order in the run " << irun;
                LOG1 << "Data in blocks:";
                for (size_t j = 0; j < nblocks; ++j)
                {
                    for (size_t k = 0; k < block_type::size; ++k)
                        LOG1 << "     Element " << k << " in block " << (off + j) << " is :" << blocks[j][k] << " with key: " << keyext(blocks[j][k]);
                }
                LOG1 << "BIDS:";
                for (size_t k = 0; k < nblocks; ++k)
                {
                    LOG1 << "BID " << (k + off) << " is: " << ((*runs[irun])[k + off].bid);
                }

                delete[] reqs;
                delete[] blocks;
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

template <typename BlockType, typename RunType, typename KeyExtractor>
void merge_runs(RunType** in_runs, size_t nruns, RunType* out_run, size_t _m, KeyExtractor keyobj)
{
    using block_type = BlockType;
    using prefetcher_type = foxxll::block_prefetcher<BlockType, typename RunType::iterator>;
    using run_cursor_type = run_cursor2<BlockType, prefetcher_type>;

    size_t i;
    RunType consume_seq(out_run->size());

    size_t* prefetch_seq = new size_t[out_run->size()];

    typename RunType::iterator copy_start = consume_seq.begin();
    for (i = 0; i < nruns; i++)
    {
        // TODO: try to avoid copy
        copy_start = std::copy(
            in_runs[i]->begin(),
            in_runs[i]->end(),
            copy_start);
    }
    std::stable_sort(consume_seq.begin(), consume_seq.end() _STXXL_SORT_TRIGGER_FORCE_SEQUENTIAL);

    size_t disks_number = foxxll::config::get_instance()->disks_number();

#ifdef PLAY_WITH_OPT_PREF
    const size_t n_write_buffers = 4 * disks_number;
#else
    const size_t n_prefetch_buffers = std::max(2 * disks_number, (3 * (_m - nruns) / 4));
    LOG1 << "Prefetch buffers " << n_prefetch_buffers;
    const size_t n_write_buffers = std::max(2 * disks_number, _m - nruns - n_prefetch_buffers);
    LOG1 << "Write buffers " << n_write_buffers;
    // heuristic
    const size_t n_opt_prefetch_buffers = 2 * disks_number + (3 * (n_prefetch_buffers - 2 * disks_number)) / 10;
    LOG1 << "Prefetch buffers " << n_opt_prefetch_buffers;
#endif

#if STXXL_SORT_OPTIMAL_PREFETCHING
    compute_prefetch_schedule(
        consume_seq,
        prefetch_seq,
        n_opt_prefetch_buffers,
        foxxll::config::get_instance()->max_device_id());
#else
    for (i = 0; i < out_run->size(); i++)
        prefetch_seq[i] = i;

#endif

    prefetcher_type prefetcher(consume_seq.begin(),
                               consume_seq.end(),
                               prefetch_seq,
                               nruns + n_prefetch_buffers);

    foxxll::buffered_writer<block_type> writer(n_write_buffers, n_write_buffers / 2);

    size_t out_run_size = out_run->size();

    run_cursor2_cmp<block_type, prefetcher_type, KeyExtractor> cmp(keyobj);
    loser_tree<
        run_cursor_type,
        run_cursor2_cmp<block_type, prefetcher_type, KeyExtractor> >
    losers(&prefetcher, nruns, cmp);

    block_type* out_buffer = writer.get_free_block();

    for (i = 0; i < out_run_size; i++)
    {
        losers.multi_merge(out_buffer->elem, out_buffer->elem + block_type::size);
        (*out_run)[i].key = keyobj(out_buffer->elem[0]);
        out_buffer = writer.write(out_buffer, (*out_run)[i].bid);
    }

    delete[] prefetch_seq;

    foxxll::block_manager* bm = foxxll::block_manager::get_instance();
    for (i = 0; i < nruns; i++)
    {
        size_t sz = in_runs[i]->size();
        for (size_t j = 0; j < sz; j++)
            bm->delete_block((*in_runs[i])[j].bid);

        delete in_runs[i];
    }
}

template <typename BlockType,
          typename AllocStrategy,
          typename InputBidIterator,
          typename KeyExtractor>
tlx::simple_vector<
    trigger_entry<typename BlockType::bid_type, typename KeyExtractor::key_type>
    >*
ksort_blocks(InputBidIterator input_bids, size_t _n,
             size_t _m, KeyExtractor keyobj)
{
    using block_type = BlockType;
    using type = typename BlockType::value_type;
    using key_type = typename KeyExtractor::key_type;
    using bid_type = typename BlockType::bid_type;
    using trigger_entry_type = trigger_entry<bid_type, typename KeyExtractor::key_type>;
    using run_type = tlx::simple_vector<trigger_entry_type>;
    using interleaved_alloc_strategy = typename foxxll::interleaved_alloc_traits<AllocStrategy>::strategy;

    size_t m2 = foxxll::div_ceil(_m, 2);
    const size_t m2_rf = m2 * block_type::raw_size /
                         (block_type::raw_size + block_type::size * sizeof(type_key<type, key_type>));
    LOG1 << "Reducing number of blocks in a run from " << m2 << " to " <<
        m2_rf << " due to key size: " << sizeof(typename KeyExtractor::key_type) << " bytes";
    m2 = m2_rf;
    size_t full_runs = _n / m2;
    size_t partial_runs = ((_n % m2) ? 1 : 0);
    size_t nruns = full_runs + partial_runs;
    size_t i;

    foxxll::block_manager* mng = foxxll::block_manager::get_instance();

    LOG1 << "n=" << _n << " nruns=" << nruns << "=" << full_runs << "+" << partial_runs;

    double begin = foxxll::timestamp(), after_runs_creation, end;

    run_type** runs = new run_type*[nruns];

    for (i = 0; i < full_runs; i++)
        runs[i] = new run_type(m2);

#ifdef INTERLEAVED_ALLOC
    if (partial_runs)
    {
        size_t last_run_size = _n - full_runs * m2;
        runs[i] = new run_type(last_run_size);

        mng->new_blocks(interleaved_alloc_strategy(nruns, AllocStrategy()),
                        runs2bid_array_adaptor2<block_type::raw_size, run_type>
                            (runs, 0, nruns, last_run_size),
                        runs2bid_array_adaptor2<block_type::raw_size, run_type>
                            (runs, _n, nruns, last_run_size));
    }
    else
        mng->new_blocks(interleaved_alloc_strategy(nruns, AllocStrategy()),
                        runs2bid_array_adaptor<block_type::raw_size, run_type>
                            (runs, 0, nruns),
                        runs2bid_array_adaptor<block_type::raw_size, run_type>
                            (runs, _n, nruns));

#else
    if (partial_runs)
        runs[i] = new run_type(_n - full_runs * m2);

    for (i = 0; i < nruns; i++)
    {
        mng->new_blocks(AllocStrategy(), make_bid_iterator(runs[i]->begin()), make_bid_iterator(runs[i]->end()));
    }
#endif

    create_runs<block_type, run_type, InputBidIterator, KeyExtractor>(
        input_bids, runs, nruns, m2, keyobj);

    after_runs_creation = foxxll::timestamp();

    double io_wait_after_rf = foxxll::stats::get_instance()->get_io_wait_time();

    foxxll::disk_queues::get_instance()->set_priority_op(foxxll::request_queue::WRITE);

    const size_t merge_factor = optimal_merge_factor(nruns, _m);
    run_type** new_runs;

    while (nruns > 1)
    {
        size_t new_nruns = foxxll::div_ceil(nruns, merge_factor);
        LOG1 << "Starting new merge phase: nruns: " << nruns <<
            " opt_merge_factor: " << merge_factor << " m:" << _m << " new_nruns: " << new_nruns;

        new_runs = new run_type*[new_nruns];

        size_t runs_left = nruns;
        size_t cur_out_run = 0;
        size_t blocks_in_new_run = 0;

        while (runs_left > 0)
        {
            size_t runs2merge = std::min(runs_left, merge_factor);
            blocks_in_new_run = 0;
            for (size_t i = nruns - runs_left; i < (nruns - runs_left + runs2merge); i++)
                blocks_in_new_run += runs[i]->size();

            // allocate run
            new_runs[cur_out_run++] = new run_type(blocks_in_new_run);
            runs_left -= runs2merge;
        }
        // allocate blocks in the new runs
        if (cur_out_run == 1 && blocks_in_new_run == _n && !input_bids->is_managed())
        {
            // if we sort a file we can reuse the input bids for the output
            InputBidIterator cur = input_bids;
            for (size_t i = 0; cur != (input_bids + _n); ++cur)
            {
                (*new_runs[0])[i++].bid = *cur;
            }

            bid_type& firstBID = (*new_runs[0])[0].bid;
            if (firstBID.is_managed())
            {
                // the first block does not belong to the file
                // need to reallocate it
                mng->new_block(foxxll::fully_random(), firstBID);
            }
            bid_type& lastBID = (*new_runs[0])[_n - 1].bid;
            if (lastBID.is_managed())
            {
                // the first block does not belong to the file
                // need to reallocate it
                mng->new_block(foxxll::fully_random(), lastBID);
            }
        }
        else
        {
            mng->new_blocks(interleaved_alloc_strategy(new_nruns, AllocStrategy()),
                            runs2bid_array_adaptor2<block_type::raw_size, run_type>(new_runs, 0, new_nruns, blocks_in_new_run),
                            runs2bid_array_adaptor2<block_type::raw_size, run_type>(new_runs, _n, new_nruns, blocks_in_new_run));
        }

        // merge all
        runs_left = nruns;
        cur_out_run = 0;
        while (runs_left > 0)
        {
            size_t runs2merge = std::min(runs_left, merge_factor);
#if STXXL_CHECK_ORDER_IN_SORTS
            assert((check_ksorted_runs<block_type, run_type, KeyExtractor>(runs + nruns - runs_left, runs2merge, m2, keyobj)));
#endif
            LOG1 << "Merging " << runs2merge << " runs";
            merge_runs<block_type, run_type, KeyExtractor>(runs + nruns - runs_left,
                                                           runs2merge, *(new_runs + (cur_out_run++)), _m, keyobj);
            runs_left -= runs2merge;
        }

        nruns = new_nruns;
        delete[] runs;
        runs = new_runs;
    }

    run_type* result = *runs;
    delete[] runs;

    end = foxxll::timestamp();

    LOG1 << "Elapsed time        : " << end - begin << " s. Run creation time: " <<
        after_runs_creation - begin << " s";
    LOG1 << "Time in I/O wait(rf): " << io_wait_after_rf << " s";
    LOG1 << *foxxll::stats::get_instance();

    return result;
}

} // namespace ksort_local

/*!
 * Sort records with integer keys, see \ref design_algo_ksort.
 *
 * stxxl::ksort sorts the elements in [first, last) into ascending order,
 * meaning that if \c i and \c j are any two valid iterators in [first, last)
 * such that \c i precedes \c j, then \c *j is not less than \c *i. Note: as
 * std::sort and stxxl::sort, stxxl::ksort is not guaranteed to be stable. That
 * is, suppose that \c *i and \c *j are equivalent: neither one is less than
 * the other. It is not guaranteed that the relative order of these two
 * elements will be preserved by stxxl::ksort.
 *
 * The two versions of stxxl::ksort differ in how they define whether one
 * element is less than another. The first version assumes that the elements
 * have \c key() member function that returns an integral key (32 or 64 bit),
 * as well as the minimum and the maximum element values. The second version
 * compares objects extracting the keys using \c keyobj object, that is in turn
 * provides min and max element values.
 *
 * The sorter's internal memory consumption is bounded by \c M bytes.
 *
 * \param first object of model of \c ext_random_access_iterator concept
 * \param last object of model of \c ext_random_access_iterator concept
 * \param keyobj \link design_algo_ksort_key_extractor key extractor \endlink object
 * \param M amount of memory for internal use (in bytes)
 */
template <typename ExtIterator, typename KeyExtractor>
void ksort(ExtIterator first, ExtIterator last, KeyExtractor keyobj, size_t M)
{
    using run_type = tlx::simple_vector<
              ksort_local::trigger_entry<
                  typename ExtIterator::bid_type, typename KeyExtractor::key_type
                  >
              >;
    using value_type = typename ExtIterator::vector_type::value_type;
    using bid_type = typename ExtIterator::bid_type;
    using block_type = typename ExtIterator::block_type;
    using alloc_strategy_type = typename ExtIterator::vector_type::alloc_strategy_type;
    using bids_container_iterator = typename ExtIterator::bids_container_iterator;
    using request_ptr = foxxll::request_ptr;

    size_t n = 0;
    foxxll::block_manager* mng = foxxll::block_manager::get_instance();

    first.flush();

    if ((last - first) * sizeof(value_type) < M)
    {
        stl_in_memory_sort(first, last,
                           ksort_local::key_comparison<value_type, KeyExtractor>(keyobj));
    }
    else
    {
        assert(2 * block_type::raw_size <= M);

        if (first.block_offset())
        {
            if (last.block_offset())            // first and last element reside
            // not in the beginning of the block
            {
                block_type* first_block = new block_type;
                block_type* last_block = new block_type;
                bid_type first_bid, last_bid;
                request_ptr req;

                req = first_block->read(*first.bid());
                mng->new_block(foxxll::fully_random(), first_bid);                // try to overlap
                mng->new_block(foxxll::fully_random(), last_bid);
                req->wait();

                req = last_block->read(*last.bid());

                size_t i = 0;
                for ( ; i < first.block_offset(); i++)
                {
                    first_block->elem[i] = keyobj.min_value();
                }

                req->wait();

                req = first_block->write(first_bid);
                for (i = last.block_offset(); i < block_type::size; i++)
                {
                    last_block->elem[i] = keyobj.max_value();
                }

                req->wait();

                req = last_block->write(last_bid);

                n = last.bid() - first.bid() + 1;

                std::swap(first_bid, *first.bid());
                std::swap(last_bid, *last.bid());

                req->wait();

                delete first_block;
                delete last_block;

                run_type* out =
                    ksort_local::ksort_blocks<
                        block_type, alloc_strategy_type,
                        bids_container_iterator, KeyExtractor
                        >(first.bid(), n, M / block_type::raw_size, keyobj);

                first_block = new block_type;
                last_block = new block_type;
                block_type* sorted_first_block = new block_type;
                block_type* sorted_last_block = new block_type;
                request_ptr* reqs = new request_ptr[2];

                reqs[0] = first_block->read(first_bid);
                reqs[1] = sorted_first_block->read((*(out->begin())).bid);
                wait_all(reqs, 2);

                reqs[0] = last_block->read(last_bid);
                reqs[1] = sorted_last_block->read(((*out)[out->size() - 1]).bid);

                for (i = first.block_offset(); i < block_type::size; i++)
                {
                    first_block->elem[i] = sorted_first_block->elem[i];
                }
                wait_all(reqs, 2);

                req = first_block->write(first_bid);

                for (i = 0; i < last.block_offset(); i++)
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
                it++;
                bids_container_iterator cur_bid = first.bid();
                cur_bid++;

                for ( ; cur_bid != last.bid(); cur_bid++, it++)
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

                block_type* first_block = new block_type;
                bid_type first_bid;
                request_ptr req;

                req = first_block->read(*first.bid());
                mng->new_block(foxxll::fully_random(), first_bid);                // try to overlap
                req->wait();

                size_t i = 0;
                for ( ; i < first.block_offset(); i++)
                {
                    first_block->elem[i] = keyobj.min_value();
                }

                req = first_block->write(first_bid);

                n = last.bid() - first.bid();

                std::swap(first_bid, *first.bid());

                req->wait();

                delete first_block;

                run_type* out =
                    ksort_local::ksort_blocks<
                        block_type, alloc_strategy_type,
                        bids_container_iterator, KeyExtractor
                        >(first.bid(), n, M / block_type::raw_size, keyobj);

                first_block = new block_type;

                block_type* sorted_first_block = new block_type;

                request_ptr* reqs = new request_ptr[2];

                reqs[0] = first_block->read(first_bid);
                reqs[1] = sorted_first_block->read((*(out->begin())).bid);
                wait_all(reqs, 2);

                for (i = first.block_offset(); i < block_type::size; i++)
                {
                    first_block->elem[i] = sorted_first_block->elem[i];
                }

                req = first_block->write(first_bid);

                mng->delete_block(out->begin()->bid);

                *first.bid() = first_bid;

                typename run_type::iterator it = out->begin();
                it++;
                bids_container_iterator cur_bid = first.bid();
                cur_bid++;

                for ( ; cur_bid != last.bid(); cur_bid++, it++)
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
            if (last.block_offset())            // last element resides
            // not in the beginning of the block
            {
                block_type* last_block = new block_type;
                bid_type last_bid;
                request_ptr req;
                size_t i;

                req = last_block->read(*last.bid());
                mng->new_block(foxxll::fully_random(), last_bid);
                req->wait();

                for (i = last.block_offset(); i < block_type::size; i++)
                {
                    last_block->elem[i] = keyobj.max_value();
                }

                req = last_block->write(last_bid);

                n = last.bid() - first.bid() + 1;

                std::swap(last_bid, *last.bid());

                req->wait();

                delete last_block;

                run_type* out =
                    ksort_local::ksort_blocks<
                        block_type, alloc_strategy_type,
                        bids_container_iterator, KeyExtractor
                        >(first.bid(), n, M / block_type::raw_size, keyobj);

                last_block = new block_type;
                block_type* sorted_last_block = new block_type;
                request_ptr* reqs = new request_ptr[2];

                reqs[0] = last_block->read(last_bid);
                reqs[1] = sorted_last_block->read(((*out)[out->size() - 1]).bid);
                wait_all(reqs, 2);

                for (i = 0; i < last.block_offset(); i++)
                {
                    last_block->elem[i] = sorted_last_block->elem[i];
                }

                req = last_block->write(last_bid);

                mng->delete_block((*out)[out->size() - 1].bid);

                *last.bid() = last_bid;

                typename run_type::iterator it = out->begin();
                bids_container_iterator cur_bid = first.bid();

                for ( ; cur_bid != last.bid(); cur_bid++, it++)
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
                n = last.bid() - first.bid();

                run_type* out =
                    ksort_local::ksort_blocks<
                        block_type, alloc_strategy_type,
                        bids_container_iterator, KeyExtractor
                        >(first.bid(), n, M / block_type::raw_size, keyobj);

                typename run_type::iterator it = out->begin();
                bids_container_iterator cur_bid = first.bid();

                for ( ; cur_bid != last.bid(); cur_bid++, it++)
                {
                    *cur_bid = (*it).bid;
                }

                delete out;
            }
        }
    }

#if STXXL_CHECK_ORDER_IN_SORTS
    using const_iterator = typename ExtIterator::const_iterator;
    assert(stxxl::is_sorted(const_iterator(first), const_iterator(last),
                            ksort_local::key_comparison<value_type, KeyExtractor>()));
#endif
}

template <typename RecordType>
struct ksort_defaultkey
{
    using key_type = typename RecordType::key_type;
    key_type operator () (const RecordType& obj) const
    {
        return obj.key();
    }
    RecordType max_value() const
    {
        return RecordType::max_value();
    }
    RecordType min_value() const
    {
        return RecordType::min_value();
    }
};

/*!
 * Sort records with integer keys, see \ref design_algo_ksort.
 *
 * stxxl::ksort sorts the elements in [first, last) into ascending order,
 * meaning that if \c i and \c j are any two valid iterators in [first, last)
 * such that \c i precedes \c j, then \c *j is not less than \c *i. Note: as
 * std::sort and stxxl::sort, stxxl::ksort is not guaranteed to be stable. That
 * is, suppose that \c *i and \c *j are equivalent: neither one is less than
 * the other. It is not guaranteed that the relative order of these two
 * elements will be preserved by stxxl::ksort.
 *
 * \param first object of model of \c ext_random_access_iterator concept
 * \param last object of model of \c ext_random_access_iterator concept
 * \param M amount of buffers for internal use
 * \remark Order in the result is non-stable
 */
template <typename ExtIterator>
void ksort(ExtIterator first, ExtIterator last, size_t M)
{
    ksort(first, last,
          ksort_defaultkey<typename ExtIterator::vector_type::value_type>(), M);
}

//! \}

} // namespace stxxl

#endif // !STXXL_ALGO_KSORT_HEADER
// vim: et:ts=4:sw=4
