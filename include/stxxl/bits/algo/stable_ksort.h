/***************************************************************************
 *  include/stxxl/bits/algo/stable_ksort.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_ALGO_STABLE_KSORT_HEADER
#define STXXL_ALGO_STABLE_KSORT_HEADER

// it is a first try: distribution sort without sampling
// I rework the stable_ksort when I would have a time
#include <algorithm>
#include <utility>

#include <tlx/die.hpp>
#include <tlx/logger.hpp>
#include <tlx/math/integer_log2.hpp>
#include <tlx/simple_vector.hpp>

#include <foxxll/common/utils.hpp>
#include <foxxll/mng/block_manager.hpp>
#include <foxxll/mng/buf_istream.hpp>
#include <foxxll/mng/buf_ostream.hpp>

#include <stxxl/bits/algo/intksort.h>
#include <stxxl/bits/algo/sort_base.h>

namespace stxxl {

constexpr bool debug_stable_ksort = false;

//! \addtogroup stlalgo
//! \{

/*! \internal
 */
namespace stable_ksort_local {

template <typename Type, typename TypeKey, typename KeyExtract>
void classify_block(Type* begin, Type* end, TypeKey*& out,
                    size_t* bucket, typename Type::key_type offset,
                    unsigned shift, KeyExtract key_extract)
{
    for (Type* p = begin; p < end; p++, out++)      // count & create references
    {
        out->ptr = p;
        const auto key = key_extract(*p);
        size_t ibucket = (key - offset) >> shift;
        out->key = key;
        bucket[ibucket]++;
    }
}

template <typename Type>
struct type_key
{
    using key_type = typename Type::key_type;
    key_type key;
    Type* ptr;

    type_key() { }
    type_key(key_type k, Type* p) : key(k), ptr(p)
    { }
};

template <typename Type>
bool operator < (const type_key<Type>& a, const type_key<Type>& b)
{
    return a.key < b.key;
}

template <typename Type>
bool operator > (const type_key<Type>& a, const type_key<Type>& b)
{
    return a.key > b.key;
}

template <typename BIDType, typename AllocStrategy>
class bid_sequence
{
public:
    using bid_type = BIDType;
    using reference = bid_type &;
    using alloc_strategy = AllocStrategy;
    using size_type = typename tlx::simple_vector<bid_type>::size_type;
    using iterator = typename tlx::simple_vector<bid_type>::iterator;

protected:
    tlx::simple_vector<bid_type>* bids;
    alloc_strategy alloc_strategy_;

public:
    bid_sequence() : bids(nullptr) { }
    explicit bid_sequence(size_type size_)
    {
        bids = new tlx::simple_vector<bid_type>(size_);
        foxxll::block_manager* mng = foxxll::block_manager::get_instance();
        mng->new_blocks(alloc_strategy_, bids->begin(), bids->end());
    }
    void init(size_type size_)
    {
        bids = new tlx::simple_vector<bid_type>(size_);
        foxxll::block_manager* mng = foxxll::block_manager::get_instance();
        mng->new_blocks(alloc_strategy_, bids->begin(), bids->end());
    }
    reference operator [] (size_type i)
    {
        size_type size_ = size();                 // cache size in a register
        if (i < size_)
            return *(bids->begin() + i);

        foxxll::block_manager* mng = foxxll::block_manager::get_instance();
        tlx::simple_vector<bid_type>* larger_bids = new tlx::simple_vector<bid_type>((i + 1) * 2);
        std::copy(bids->begin(), bids->end(), larger_bids->begin());
        mng->new_blocks(alloc_strategy_, larger_bids->begin() + size_, larger_bids->end());
        delete bids;
        bids = larger_bids;
        return *(larger_bids->begin() + i);
    }
    size_type size() { return bids->size(); }
    iterator begin() { return bids->begin(); }
    ~bid_sequence()
    {
        foxxll::block_manager::get_instance()->delete_blocks(bids->begin(), bids->end());
        delete bids;
    }
};

template <typename ExtIterator, typename KeyExtract>
void distribute(
    bid_sequence<typename ExtIterator::vector_type::block_type::bid_type,
                 typename ExtIterator::vector_type::alloc_strategy_type>* bucket_bids,
    uint64_t* bucket_sizes,
    const size_t nbuckets,
    const unsigned lognbuckets,
    ExtIterator first,
    ExtIterator last,
    const size_t nread_buffers,
    const size_t nwrite_buffers,
    KeyExtract key_extract)
{
    using key_type = decltype(key_extract(*first));
    using block_type = typename ExtIterator::block_type;
    using bids_container_iterator = typename ExtIterator::bids_container_iterator;

    using buf_istream_type = foxxll::buf_istream<block_type, bids_container_iterator>;

    size_t i = 0;

    buf_istream_type in(first.bid(), last.bid() + ((first.block_offset()) ? 1 : 0),
                        nread_buffers);

    foxxll::buffered_writer<block_type> out(
        nbuckets + nwrite_buffers,
        nwrite_buffers);

    size_t* bucket_block_offsets = new size_t[nbuckets];
    size_t* bucket_iblock = new size_t[nbuckets];
    block_type** bucket_blocks = new block_type*[nbuckets];

    std::fill(bucket_sizes, bucket_sizes + nbuckets, 0);
    std::fill(bucket_iblock, bucket_iblock + nbuckets, 0);
    std::fill(bucket_block_offsets, bucket_block_offsets + nbuckets, 0);

    for (i = 0; i < nbuckets; i++)
        bucket_blocks[i] = out.get_free_block();

    ExtIterator cur = first - first.block_offset();

    // skip part of the block before first untouched
    for ( ; cur != first; cur++)
        ++in;

    const size_t shift = sizeof(key_type) * 8 - lognbuckets;
    // search in the the range [_begin,_end)
    LOGC(debug_stable_ksort) << "Shift by: " << shift << " bits, lognbuckets: " << lognbuckets;
    for ( ; cur != last; cur++)
    {
        key_type cur_key = key_extract(in.current());
        size_t ibucket = cur_key >> shift;

        size_t block_offset = bucket_block_offsets[ibucket];
        in >> (bucket_blocks[ibucket]->elem[block_offset++]);
        if (block_offset == block_type::size)
        {
            block_offset = 0;
            size_t iblock = bucket_iblock[ibucket]++;
            bucket_blocks[ibucket] = out.write(bucket_blocks[ibucket], bucket_bids[ibucket][iblock]);
        }
        bucket_block_offsets[ibucket] = block_offset;
    }
    for (i = 0; i < nbuckets; i++)
    {
        if (bucket_block_offsets[i])
        {
            out.write(bucket_blocks[i], bucket_bids[i][bucket_iblock[i]]);
        }
        bucket_sizes[i] = uint64_t(block_type::size) * bucket_iblock[i] +
                          bucket_block_offsets[i];
        LOGC(debug_stable_ksort) << "Bucket " << i << " has size " << bucket_sizes[i] <<
            ", estimated size: " << ((last - first) / int64_t(nbuckets));
    }

    delete[] bucket_blocks;
    delete[] bucket_block_offsets;
    delete[] bucket_iblock;
}

} // namespace stable_ksort_local

//! Sort records with integer keys
//! \param first object of model of \c ext_random_access_iterator concept
//! \param last object of model of \c ext_random_access_iterator concept
//! \param key_extract must provide a key_type operator(const value_type&) to extract the key from value_type
//! \param M amount of memory for internal use (in bytes)
//! \remark Not yet fully implemented, it assumes that the keys are uniformly
//! distributed between [0,std::numeric_limits<key_type>::max().
template <typename ExtIterator, typename KeyExtract>
void stable_ksort(ExtIterator first, ExtIterator last, KeyExtract key_extract, size_t M)
{
    LOG1 << "Warning: stable_ksort is not yet fully implemented, it assumes that the keys are uniformly distributed between [0,std::numeric_limits<key_type>::max()]";
    using value_type = typename ExtIterator::vector_type::value_type;
    using key_type = typename value_type::key_type;
    using block_type = typename ExtIterator::block_type;
    using bids_container_iterator = typename ExtIterator::bids_container_iterator;
    using bid_type = typename block_type::bid_type;
    using alloc_strategy = typename ExtIterator::vector_type::alloc_strategy_type;
    using bucket_bids_type = stable_ksort_local::bid_sequence<bid_type, alloc_strategy>;
    using type_key_ = stable_ksort_local::type_key<value_type>;
    using request_ptr = foxxll::request_ptr;

    first.flush();     // flush container

    double begin = foxxll::timestamp();

    size_t i = 0;
    foxxll::config* cfg = foxxll::config::get_instance();
    const size_t m = M / block_type::raw_size;
    assert(2 * block_type::raw_size <= M);
    const size_t write_buffers_multiple = 2;
    const size_t read_buffers_multiple = 2;
    const size_t ndisks = cfg->disks_number();
    const size_t min_num_read_write_buffers = (write_buffers_multiple + read_buffers_multiple) * ndisks;
    const size_t nmaxbuckets = m - min_num_read_write_buffers;
    const unsigned int lognbuckets = tlx::integer_log2_floor(nmaxbuckets);
    const size_t nbuckets = size_t(1) << lognbuckets;
    const size_t est_bucket_size = (size_t)foxxll::div_ceil((last - first) / nbuckets, block_type::size);      //in blocks

    if (m < min_num_read_write_buffers + 2 || nbuckets < 2) {
        LOG1 << "stxxl::stable_ksort: Not enough memory. Blocks available: " << m <<
            ", required for r/w buffers: " << min_num_read_write_buffers <<
            ", required for buckets: 2, nbuckets: " << nbuckets;
        throw foxxll::bad_parameter("stxxl::stable_ksort(): INSUFFICIENT MEMORY provided, please increase parameter 'M'");
    }
    LOGC(debug_stable_ksort) << "Elements to sort: " << (last - first);
    LOGC(debug_stable_ksort) << "Number of buckets has to be reduced from " << nmaxbuckets << " to " << nbuckets;
    const size_t nread_buffers = (m - nbuckets) * read_buffers_multiple / (read_buffers_multiple + write_buffers_multiple);
    const size_t nwrite_buffers = (m - nbuckets) * write_buffers_multiple / (read_buffers_multiple + write_buffers_multiple);

    LOGC(debug_stable_ksort) << "Read buffers in distribution phase: " << nread_buffers;
    LOGC(debug_stable_ksort) << "Write buffers in distribution phase: " << nwrite_buffers;

    bucket_bids_type* bucket_bids = new bucket_bids_type[nbuckets];
    for (i = 0; i < nbuckets; ++i)
        bucket_bids[i].init(est_bucket_size);

    uint64_t* bucket_sizes = new uint64_t[nbuckets];

    foxxll::disk_queues::get_instance()->set_priority_op(foxxll::request_queue::WRITE);

    stable_ksort_local::distribute(
        bucket_bids,
        bucket_sizes,
        nbuckets,
        lognbuckets,
        first,
        last,
        nread_buffers,
        nwrite_buffers,
        key_extract);

    double dist_end = foxxll::timestamp(), end;
    double io_wait_after_d = foxxll::stats::get_instance()->get_io_wait_time();

    {
        // sort buckets
        size_t write_buffers_multiple_bs = 2;
        size_t max_bucket_size_bl = (m - write_buffers_multiple_bs * ndisks) / 2;       // in number of blocks
        uint64_t max_bucket_size_rec = uint64_t(max_bucket_size_bl) * block_type::size; // in number of records
        uint64_t max_bucket_size_act = 0;                                               // actual max bucket size
        // establish output stream

        for (i = 0; i < nbuckets; i++)
        {
            max_bucket_size_act = std::max(bucket_sizes[i], max_bucket_size_act);
            if (bucket_sizes[i] > max_bucket_size_rec)
            {
                die("Bucket " << i << " is too large: " << bucket_sizes[i] <<
                    " records, maximum: " << max_bucket_size_rec << "\n"
                    "Recursion on buckets is not yet implemented, aborting.");
            }
        }
        // here we can increase write_buffers_multiple_b knowing max(bucket_sizes[i])
        // ... and decrease max_bucket_size_bl
        const size_t max_bucket_size_act_bl = (size_t)foxxll::div_ceil(max_bucket_size_act, block_type::size);
        LOGC(debug_stable_ksort) << "Reducing required number of required blocks per bucket from " <<
            max_bucket_size_bl << " to " << max_bucket_size_act_bl;
        max_bucket_size_rec = max_bucket_size_act;
        max_bucket_size_bl = max_bucket_size_act_bl;
        const size_t nwrite_buffers_bs = m - 2 * max_bucket_size_bl;
        LOGC(debug_stable_ksort) << "Write buffers in bucket sorting phase: " << nwrite_buffers_bs;

        using buf_ostream_type = foxxll::buf_ostream<block_type, bids_container_iterator>;
        buf_ostream_type out(first.bid(), nwrite_buffers_bs);

        foxxll::disk_queues::get_instance()->set_priority_op(foxxll::request_queue::READ);

        if (first.block_offset())
        {
            // has to skip part of the first block
            block_type* block = new block_type;
            request_ptr req;
            req = block->read(*first.bid());
            req->wait();

            for (i = 0; i < first.block_offset(); i++)
            {
                out << block->elem[i];
            }
            delete block;
        }
        block_type* blocks1 = new block_type[max_bucket_size_bl];
        block_type* blocks2 = new block_type[max_bucket_size_bl];
        request_ptr* reqs1 = new request_ptr[max_bucket_size_bl];
        request_ptr* reqs2 = new request_ptr[max_bucket_size_bl];
        type_key_* refs1 = new type_key_[(size_t)max_bucket_size_rec];
        type_key_* refs2 = new type_key_[(size_t)max_bucket_size_rec];

        // submit reading first 2 buckets (Peter's scheme)
        size_t nbucket_blocks = (size_t)foxxll::div_ceil(bucket_sizes[0], block_type::size);
        for (i = 0; i < nbucket_blocks; i++)
            reqs1[i] = blocks1[i].read(bucket_bids[0][i]);

        nbucket_blocks = (size_t)foxxll::div_ceil(bucket_sizes[1], block_type::size);
        for (i = 0; i < nbucket_blocks; i++)
            reqs2[i] = blocks2[i].read(bucket_bids[1][i]);

        key_type offset = 0;
        const unsigned log_k1 = std::max<unsigned>(
            tlx::integer_log2_ceil(max_bucket_size_rec * sizeof(type_key_) / STXXL_L2_SIZE), 1);
        size_t k1 = size_t(1) << log_k1;
        size_t* bucket1 = new size_t[k1];

        const unsigned int shift = (unsigned int)(sizeof(key_type) * 8 - lognbuckets);
        const unsigned int shift1 = shift - log_k1;

        LOGC(debug_stable_ksort) << "Classifying " << nbuckets << " buckets, max size:" << max_bucket_size_rec <<
            " block size:" << block_type::size << " log_k1:" << log_k1;

        for (size_t k = 0; k < nbuckets; k++)
        {
            nbucket_blocks = (size_t)foxxll::div_ceil(bucket_sizes[k], block_type::size);
            const unsigned log_k1_k = std::max<unsigned>(
                tlx::integer_log2_ceil(bucket_sizes[k] * sizeof(type_key_) / STXXL_L2_SIZE), 1);
            assert(log_k1_k <= log_k1);
            k1 = (size_t)(1) << log_k1_k;
            std::fill(bucket1, bucket1 + k1, 0);

            LOGC(debug_stable_ksort) << "Classifying bucket " << k << " size:" << bucket_sizes[k] <<
                " blocks:" << nbucket_blocks << " log_k1:" << log_k1_k;
            // classify first nbucket_blocks-1 blocks, they are full
            type_key_* ref_ptr = refs1;
            key_type offset1 = offset + (key_type(1) << key_type(shift)) * key_type(k);
            for (i = 0; i < nbucket_blocks - 1; i++)
            {
                reqs1[i]->wait();
                stable_ksort_local::classify_block(
                    blocks1[i].begin(), blocks1[i].end(), ref_ptr,
                    bucket1, offset1, shift1, key_extract);
            }
            // last block might be non-full
            const size_t last_block_size =
                (size_t)(bucket_sizes[k] - (nbucket_blocks - 1) * block_type::size);
            reqs1[i]->wait();

            //STXXL_MSG("block_type::size: "<<block_type::size<<" last_block_size:"<<last_block_size);

            stable_ksort_local::classify_block(
                blocks1[i].begin(), blocks1[i].begin() + last_block_size, ref_ptr,
                bucket1, offset1, shift1, key_extract);

            exclusive_prefix_sum(bucket1, k1);
            classify(refs1, refs1 + bucket_sizes[k], refs2, bucket1, offset1, shift1);

            type_key_* c = refs2;
            type_key_* d = refs1;
            for (i = 0; i < k1; i++)
            {
                type_key_* cEnd = refs2 + bucket1[i];
                type_key_* dEnd = refs1 + bucket1[i];

                const unsigned log_k2 = tlx::integer_log2_floor(bucket1[i]) - 1;        // adaptive bucket size
                const size_t k2 = size_t(1) << log_k2;
                size_t* bucket2 = new size_t[k2];
                const unsigned shift2 = shift1 - log_k2;

                // STXXL_MSG("Sorting bucket "<<k<<":"<<i);
                l1sort(c, cEnd, d, bucket2, k2,
                       offset1 + (key_type(1) << key_type(shift1)) * key_type(i),
                       shift2);

                // write out all
                for (type_key_* p = d; p < dEnd; p++)
                    out << (*(p->ptr));

                delete[] bucket2;
                c = cEnd;
                d = dEnd;
            }
            // submit next read
            const size_t bucket2submit = k + 2;
            if (bucket2submit < nbuckets)
            {
                nbucket_blocks = (size_t)foxxll::div_ceil(bucket_sizes[bucket2submit], block_type::size);
                for (i = 0; i < nbucket_blocks; i++)
                    reqs1[i] = blocks1[i].read(bucket_bids[bucket2submit][i]);
            }

            std::swap(blocks1, blocks2);
            std::swap(reqs1, reqs2);
        }

        delete[] bucket1;
        delete[] refs1;
        delete[] refs2;
        delete[] blocks1;
        delete[] blocks2;
        delete[] reqs1;
        delete[] reqs2;
        delete[] bucket_bids;
        delete[] bucket_sizes;

        if (last.block_offset())
        {
            // has to skip part of the first block
            block_type* block = new block_type;
            request_ptr req = block->read(*last.bid());
            req->wait();

            for (i = last.block_offset(); i < block_type::size; i++)
            {
                out << block->elem[i];
            }
            delete block;
        }

        end = foxxll::timestamp();
    }

    LOG1 << "Elapsed time        : " << end - begin << " s. Distribution time: " << (dist_end - begin) << " s";
    LOG1 << "Time in I/O wait(ds): " << io_wait_after_d << " s";
    LOG1 << *foxxll::stats::get_instance();
}

//! Sort records with integer keys providing a key() method
//! \param first object of model of \c ext_random_access_iterator concept
//! \param last object of model of \c ext_random_access_iterator concept
//! \param M amount of memory for internal use (in bytes)
//! \remark Elements must provide a method key() which returns the integer key.
//! \remark Not yet fully implemented, it assumes that the keys are uniformly
//! distributed between [0,std::numeric_limits<key_type>::max().
template <typename ExtIterator>
void stable_ksort(ExtIterator first, ExtIterator last, size_t M)
{
    using value_type = typename ExtIterator::vector_type::value_type;
    stable_ksort(first, last, [](const value_type& v) { return v.key(); }, M);
}

//! \}

} // namespace stxxl

#endif // !STXXL_ALGO_STABLE_KSORT_HEADER
