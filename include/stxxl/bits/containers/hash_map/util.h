/***************************************************************************
 *			  util.h
 *
 *	May 2007, Markus Westphal
 ****************************************************************************/


#ifndef STXXL_CONTAINERS_HASHMAP__UTIL_H
#define STXXL_CONTAINERS_HASHMAP__UTIL_H

#include "stxxl/bits/io/iobase.h"
#include "stxxl/bits/mng/mng.h"
#include "stxxl/bits/mng/buf_writer.h"

#include "stxxl/bits/containers/hash_map/tuning.h"
#include "stxxl/bits/containers/hash_map/block_cache.h"


__STXXL_BEGIN_NAMESPACE

namespace hash_map
{
#pragma mark -
#pragma mark Structs
#pragma mark

// For internal memory chaining
// next-pointer and delete-flag share the same memory: the lowest bit is occupied by the del-flag.
    template <class ValueType>
    struct node
    {
        node<ValueType> * next_and_del_;
        ValueType value_;

        bool deleted() { return ((int_type) next_and_del_ & 0x01) == 1; }
        bool deleted(bool d)
        {
            next_and_del_ = (node<ValueType> *)(((int_type)next_and_del_ & ~0x01) | (int_type)d);
            return d;
        }

        node<ValueType> * next() { return (node<ValueType> *)((int_type)next_and_del_ & ~0x01); }
        node<ValueType> * next(node<ValueType> * n)
        {
            next_and_del_ = (node<ValueType> *)(((int_type)next_and_del_ & 0x01) | (int_type)n);
            return n;
        }
    };


    template <class NodeType>
    struct bucket
    {
        NodeType * list_;                /* entry point to the chain in internal memory */

        size_t n_external_;              /* number of elements in external memory */

        size_t i_block_;                 /* index of first block's bid (to be used as index for hash_map's bids_-array)*/
        size_t i_subblock_;              /* index of first subblock */


        bucket() :
            list_(NULL),
            n_external_(0),
            i_subblock_(0),
            i_block_(0)
        { }

        bucket(NodeType * list, size_t n_external, size_t i_block, size_t i_subblock) :
            list_(list),
            n_external_(n_external),
            i_block_(i_block),
            i_subblock_(i_subblock)
        { }
    };


#pragma mark -
#pragma mark Class buffered_reader
#pragma mark

//! \brief Used to scan external memory with prefetching.
    template <class CacheType, class BidIt>
    class buffered_reader
    {
    public:
        typedef CacheType cache_type;

        typedef typename cache_type::block_type block_type;
        typedef typename block_type::value_type subblock_type;
        typedef typename subblock_type::value_type value_type;

        typedef typename BidIt::value_type bid_type;

        enum { block_size = block_type::size, subblock_size = subblock_type::size };

    private:
        size_t i_value_;                                /* index within current block	*/
        BidIt begin_bid_;                               /* points to the beginning of the block-squence   */
        BidIt curr_bid_;                                /* points to the current block */
        BidIt end_bid_;                                 /* points to the end of the block-sequence	*/
        BidIt pref_bid_;                                /* points to the next block to prefetch */

        cache_type * cache_;                            /* shared block-cache  */

        bool prefetch_;                                 /* true if prefetching enabled	 */
        size_t page_size_;                              /* pages, which are read at once from disk, consist of this many blocks */
        size_t prefetch_pages_;                         /* number of pages to prefetch	 */

        bool dirty_;                                    /* current block dirty ? */
        subblock_type * subblock_;                      /* current subblock		 */

    public:
        //! \brief Create a new buffered reader to read the blocks in [seq_begin, seq_end)
        //! \param seq_begin First block's bid
        //! \param seq_end Last block's bid
        //! \param cache Block-cache used for prefetching
        //! \param i_subblock Start reading from this subblock
        //! \param prefetch Enable/Disable prefetching
        buffered_reader(BidIt seq_begin, BidIt seq_end, cache_type * cache, size_t i_subblock = 0, bool prefetch = true) :
            i_value_(0),
            begin_bid_(seq_begin),
            curr_bid_(seq_begin),
            end_bid_(seq_end),
            cache_(cache),
            prefetch_(false),
#ifdef PLAY_WITH_PREFETCHING
            page_size_(tuning::get_instance()->prefetch_page_size),
            prefetch_pages_(tuning::get_instance()->prefetch_pages),
#else
            page_size_(config::get_instance()->disks_number() * 2),
            prefetch_pages_(2),
#endif
            dirty_(false)
        {
            if (seq_begin == seq_end)
                return;

            if (prefetch)
                enable_prefetching();

            skip_to(seq_begin, i_subblock);     // will (amongst other things) set subblock_ and retain current block
        }

        ~buffered_reader()
        {
            if (curr_bid_ != end_bid_)
                cache_->release_block(*curr_bid_);
        }


        void enable_prefetching()
        {
            if (prefetch_)
                return;

            prefetch_ = true;
            pref_bid_ = curr_bid_;      // start prefetching page_size*prefetch_pages blocks beginning with current one
            for (size_t i = 0; i < page_size_ * prefetch_pages_; i++)
            {
                if (pref_bid_ == end_bid_)
                    break;

                cache_->prefetch_block(*pref_bid_);
                ++pref_bid_;
            }
        }


        //! \brief Get const-reference to current value.
        const value_type & const_value()
        {
            return (*subblock_)[i_value_ % subblock_size];
        }


        //! \brief Get reference to current value. The current value's block's dirty flag will be set.
        value_type & value()
        {
            if (!dirty_) {
                cache_->make_dirty(*curr_bid_);
                dirty_ = true;
            }

            return (*subblock_)[i_value_ % subblock_size];
        }


        //! \brief Advance to the next value
        //! \return false if last value has been reached, otherwise true.
        bool operator ++ ()
        {
            if (curr_bid_ == end_bid_)
                return false;

            // same block
            if (i_value_ + 1 < block_size * subblock_size)
            {
                i_value_++;
            }
            // entered new block
            else
            {
                cache_->release_block(*curr_bid_);

                i_value_ = 0;
                dirty_ = false;
                ++curr_bid_;

                if (curr_bid_ == end_bid_)
                    return false;

                cache_->retain_block(*curr_bid_);

                // if a complete page has been consumed, prefetch the next one
                if (prefetch_ && (curr_bid_ - begin_bid_) % page_size_ == 0)
                {
                    for (unsigned i = 0; i < page_size_; i++)
                    {
                        if (pref_bid_ == end_bid_)
                            break;
                        cache_->prefetch_block(*pref_bid_);
                        ++pref_bid_;
                    }
                }
            }

            // entered new subblock
            if (i_value_ % subblock_size == 0)
            {
                subblock_ = cache_->get_subblock(*curr_bid_, i_value_ / subblock_size);
            }

            return true;
        }


        //! \brief Skip remaining values of the current subblock.
        void next_subblock()
        {
            i_value_ = (i_value_ / subblock_size + 1) * subblock_size - 1;
            operator ++ ();     // takes care of prefetching etc
        }


        //! \brief Continue reading at given block and subblock.
        void skip_to(BidIt bid, size_t i_subblock)
        {
            if (bid != curr_bid_)
                dirty_ = false;

            cache_->release_block(*curr_bid_);

            if (bid == end_bid_)
                return;

            // skip to block
            while (curr_bid_ != bid) {
                ++curr_bid_;

                if (prefetch_ && (curr_bid_ - begin_bid_) % page_size_ == 0)
                {
                    for (unsigned i = 0; i < page_size_; i++)
                    {
                        if (pref_bid_ == end_bid_)
                            break;
                        cache_->prefetch_block(*pref_bid_);
                        ++pref_bid_;
                    }
                }
            }
            // skip to subblock
            i_value_ = i_subblock * subblock_size;
            subblock_ = cache_->get_subblock(*curr_bid_, i_subblock);
            cache_->retain_block(*curr_bid_);
        }
    };


#pragma mark -
#pragma mark Class buffered_writer
#pragma mark

//! \brief Buffered writing of values. New Blocks are allocated as needed.
    template <class BlkType, class BidContainer>
    class buffered_writer
    {
    public:
        typedef BlkType block_type;
        typedef typename block_type::value_type subblock_type;
        typedef typename subblock_type::value_type value_type;

        typedef stxxl::buffered_writer<block_type> writer_type;

        enum { block_size = block_type::size, subblock_size = subblock_type::size };

    private:
        writer_type * writer_;           /* buffered writer */
        block_type * block_;             /* current buffer-block */

        BidContainer * bids_;            /* sequence of allocated blocks (to be expanded as needed) */

        size_t i_block_;                 /* current block's index */
        size_t i_value_;                 /* current value's index in the range of [0..#values per block[ */
        size_t increase_;                /* number of blocks to allocate in a row */

    public:
        //! \brief Create a new buffered writer.
        //! \param c write values to these blocks (c holds the bids)
        //! \param buffer_size Number of write-buffers to use
        //! \param batch_size bulk buffered writing
        buffered_writer(BidContainer * c, int_type buffer_size, int_type batch_size) :
            bids_(c),
            i_block_(0),
            i_value_(0),
            increase_(config::get_instance()->disks_number() * 3)
        {
            writer_ = new writer_type(buffer_size, batch_size);
            block_ = writer_->get_free_block();
        }

        ~buffered_writer()
        {
            flush();
            delete writer_;
        }


        //! \brief Write all values from given stream.
        template <class StreamType>
        void append_from_stream(StreamType & stream)
        {
            while (!stream.empty())
            {
                append(*stream);
                ++stream;
            }
        }


        //! \brief Write given value.
        void append(const value_type & value)
        {
            size_t i_subblock = (i_value_ / subblock_size);
            (*block_)[i_subblock][i_value_ % subblock_size] = value;

            if (i_value_ + 1 < block_size * subblock_size)
                i_value_++;
            // reached end of a block
            else
            {
                i_value_ = 0;

                // allocate new blocks if neccessary ...
                if (i_block_ == bids_->size())
                {
                    bids_->resize(bids_->size() + increase_);
                    block_manager * bm = stxxl::block_manager::get_instance();
                    bm->new_blocks(striping(), bids_->end() - increase_, bids_->end());
                }
                // ... and write current block
                block_ = writer_->write(block_, (*bids_)[i_block_]);

                i_block_++;
            }
        }


        //! \brief Continue writing at the beginning of the next subblock. TODO more efficient
        void finish_subblock()
        {
            i_value_ = (i_value_ / subblock_size + 1) * subblock_size - 1;
            append(value_type());       // writing and allocating blocks etc
        }


        //! \brief Flushes not yet written blocks.
        void flush()
        {
            i_value_ = 0;
            if (i_block_ == bids_->size())
            {
                bids_->resize(bids_->size() + increase_);
                block_manager * bm = stxxl::block_manager::get_instance();
                bm->new_blocks(striping(), bids_->end() - increase_, bids_->end());
            }
            block_ = writer_->write(block_, (*bids_)[i_block_]);
            i_block_++;

            writer_->flush();
        }


        //! \brief Index of current block.
        size_t i_block() { return i_block_; }

        //! \brief Index of current subblock.
        size_t i_subblock() { return i_value_ / subblock_size; }
    };


#pragma mark -
#pragma mark Class	HashedValuesStream
#pragma mark

/** Additionial information about a stored value:
        - the bucket in which it can be found
        - where it is currently stored (intern or extern)
        - the buffer-node
        - the position in external memory
*/
    template <class HashMap>
    struct HashedValue
    {
        typedef HashMap hash_map_type;
        typedef typename hash_map_type::value_type value_type;
        typedef typename hash_map_type::size_type size_type;
        typedef typename hash_map_type::source_type source_type;
        typedef typename hash_map_type::node_type node_type;

        value_type value_;
        size_type i_bucket_;
        size_type i_external_;
        node_type * node_;
        source_type source_;

        HashedValue() { }

        HashedValue(const value_type & value, size_type i_bucket, source_type src, node_type * node, size_type i_external) :
            value_(value),
            i_bucket_(i_bucket),
            source_(src),
            node_(node),
            i_external_(i_external)
        { }
    };


/** Stream interface for all value-pairs currently stored in the map. Returned values are HashedValue-objects
        (actual value enriched with information on where the value can be found (bucket-number, internal, external)).
        Values, marked as deleted in internal-memory, are not returned; for modified values only the one in internal memory is returned.
*/
    template <class HashMap, class Reader>
    struct HashedValuesStream
    {
        typedef HashMap hash_map_type;
        typedef HashedValue<HashMap> value_type;

        typedef typename hash_map_type::size_type size_type;
        typedef typename hash_map_type::node_type node_type;
        typedef typename hash_map_type::bid_container_type::iterator bid_iterator;
        typedef typename hash_map_type::buckets_container_type::iterator bucket_iterator;

        hash_map_type * map_;
        Reader & reader_;
        bid_iterator begin_bid_;
        bucket_iterator curr_bucket_;
        bucket_iterator end_bucket_;
        size_type i_bucket_;
        size_type i_external_;
        node_type * node_;
        value_type value_;


        HashedValuesStream(bucket_iterator begin_bucket, bucket_iterator end_bucket, Reader & reader, bid_iterator begin_bid, hash_map_type * map) :
            map_(map),
            reader_(reader),
            curr_bucket_(begin_bucket),
            end_bucket_(end_bucket),
            begin_bid_(begin_bid),
            i_bucket_(0),
            node_(curr_bucket_->list_),
            i_external_(0)
        {
            value_ = find_next();
        }

        const value_type & operator * () { return value_; }
        bool empty() const { return curr_bucket_ == end_bucket_; }

        void operator ++ ()
        {
            if (value_.source_ == hash_map_type::src_internal)
                node_ = node_->next();
            else
            {
                ++reader_;
                ++i_external_;
            }
            value_ = find_next();
        }

        value_type find_next()
        {
            while (true)
            {
                // internal and external elements available
                while (node_ && i_external_ < curr_bucket_->n_external_)
                {
                    if (map_->__leq(node_->value_.first, reader_.const_value().first))
                    {
                        if (map_->__eq(node_->value_.first, reader_.const_value().first))
                        {
                            ++reader_;
                            ++i_external_;
                        }

                        if (!node_->deleted())
                            return value_type(node_->value_, i_bucket_, hash_map_type::src_internal, node_, i_external_);
                        else
                            node_ = node_->next();
                    }
                    else
                        return value_type(reader_.const_value(), i_bucket_, hash_map_type::src_external, node_, i_external_);
                }
                // only internal elements left
                while (node_)
                {
                    if (!node_->deleted())
                        return value_type(node_->value_, i_bucket_, hash_map_type::src_internal, node_, i_external_);
                    else
                        node_ = node_->next();
                }
                // only external elements left
                while (i_external_ < curr_bucket_->n_external_)
                    return value_type(reader_.const_value(), i_bucket_, hash_map_type::src_external, node_, i_external_);

                // if we made it to this point there are obviously no more values in the current bucket
                // let's try the next one (outer while-loop!)
                ++curr_bucket_;
                ++i_bucket_;
                if (curr_bucket_ == end_bucket_)
                    return value_type();

                node_ = curr_bucket_->list_;
                i_external_ = 0;
                reader_.skip_to(begin_bid_ + curr_bucket_->i_block_, curr_bucket_->i_subblock_);
            }
        }
    };
} // end namespace hash_map

__STXXL_END_NAMESPACE

#endif
