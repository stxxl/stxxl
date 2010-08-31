/***************************************************************************
 *  include/stxxl/bits/containers/hash_map/block_cache.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Markus Westphal <marwes@users.sourceforge.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_CONTAINERS_HASHMAP__BLOCK_CACHE_H
#define STXXL_CONTAINERS_HASHMAP__BLOCK_CACHE_H

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/compat_hash_map.h>
#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/containers/btree/btree_pager.h>

#include <vector>
#include <list>

__STXXL_BEGIN_NAMESPACE


namespace hash_map
{
    namespace cache_block_helper
    {
        template <class BlockType>
        class write_buffer
        {
        public:
            typedef BlockType block_type;
            typedef typename block_type::bid_type bid_type;

        private:
            std::vector<block_type *> blocks_;
            std::vector<request_ptr> reqs_;
            std::vector<unsigned> free_blocks_;
            std::list<unsigned> busy_blocks_;                   // TODO make that a circular-buffer

        public:
            write_buffer(unsigned size)
            {
                blocks_.reserve(size);
                free_blocks_.reserve(size);
                reqs_.resize(size);

                for (unsigned_type i = 0; i < size; i++) {
                    blocks_.push_back(new block_type());
                    free_blocks_.push_back(i);
                }
            }

            // Writes the given block back to disk;
            // callers have to exchange the passed block with the returned one!
            block_type * write(block_type * write_block, const bid_type & bid)
            {
                if (free_blocks_.empty()) {
                    unsigned i_buffer = busy_blocks_.front();
                    busy_blocks_.pop_front();

                    if (reqs_[i_buffer].valid())
                        reqs_[i_buffer]->wait();

                    free_blocks_.push_back(i_buffer);
                }

                unsigned i_buffer = free_blocks_.back();
                free_blocks_.pop_back();
                block_type * buffer = blocks_[i_buffer];

                blocks_[i_buffer] = write_block;
                reqs_[i_buffer] = blocks_[i_buffer]->write(bid);
                busy_blocks_.push_back(i_buffer);

                return buffer;
            }

            void flush()
            {
                while (!busy_blocks_.empty()) {
                    unsigned i_buffer = busy_blocks_.front();
                    busy_blocks_.pop_front();
                    if (reqs_[i_buffer].valid())
                        reqs_[i_buffer]->wait();
                }
                busy_blocks_.clear();
                free_blocks_.clear();
                for (unsigned i = 0; i < blocks_.size(); i++)
                    free_blocks_.push_back(i);
            }

            ~write_buffer()
            {
                flush();
                for (unsigned i = 0; i < blocks_.size(); i++)
                    delete blocks_[i];
            }
        };
    }     // end namespace cache_block_helper


    template <class BlockType>
    class block_cache : private noncopyable
    {
    public:
        typedef BlockType block_type;
        typedef typename block_type::bid_type bid_type;
        typedef typename block_type::value_type subblock_type;
        typedef typename subblock_type::bid_type subblock_bid_type;

    private:
/*		struct bid_eq
                {
            bool operator () (const bid_type & a, const bid_type & b) const
            {
                return (a.storage == b.storage && a.offset == b.offset);
            }
        };
*/

        struct bid_hash
        {
            size_t operator () (const bid_type & bid) const
            {
                return longhash1(bid.offset + uint64(bid.storage));
            }
#ifdef BOOST_MSVC
            bool operator () (const bid_type & a, const bid_type & b) const
            {
                return (a.storage < b.storage) || (a.storage == b.storage && a.offset < b.offset);
            }
            enum
            {                                   // parameters for hash table
                bucket_size = 4,                // 0 < bucket_size
                min_buckets = 8                 // min_buckets = 2 ^^ N, 0 < N
            };
#endif
        };

        typedef stxxl::btree::lru_pager pager_type;
        typedef cache_block_helper::write_buffer<block_type> write_buffer_type;

        typedef typename compat_hash_map<bid_type, unsigned_type, bid_hash>::result bid_map_type;
//        typedef __gnu_cxx::hash_map<bid_type, unsigned_type, bid_hash, bid_eq> bid_map_type;


        enum { valid_all = block_type::size };


        write_buffer_type write_buffer_;

        std::vector<block_type *> blocks_;                                              /* cached blocks         */
        std::vector<bid_type> bids_;                                                    /* bids of cached blocks */
        std::vector<unsigned_type> retain_count_;                                       /* */
        std::vector<bool> dirty_;                                                       /* true iff block has been altered while in cache */
        std::vector<unsigned_type> valid_subblock_;                                     /* valid_all or the actually loaded subblock's index */
        std::vector<unsigned_type> free_blocks_;                                        /* free blocks as indices to blocks_-vector */
        std::vector<request_ptr> reqs_;

        bid_map_type bid_map_;
        pager_type pager_;

        /* statistics */
        int64 n_found;
        int64 n_not_found;
        int64 n_read;
        int64 n_written;
        int64 n_clean_forced;
        int64 n_wrong_subblock;

    public:
        //! \brief Construct a new block-cache.
        //! \param cache_size cache-size in number of blocks
        block_cache(unsigned_type cache_size) :
            write_buffer_(config::get_instance()->disks_number() * 2),
            pager_(cache_size),
            n_found(0),
            n_not_found(0),
            n_read(0),
            n_written(0),
            n_clean_forced(0),
            n_wrong_subblock(0)
        {
            blocks_.reserve(cache_size);
            free_blocks_.reserve(cache_size);
            bids_.resize(cache_size);
            reqs_.resize(cache_size);
            dirty_.resize(cache_size, false);
            valid_subblock_.resize(cache_size);
            retain_count_.resize(cache_size);

            for (unsigned_type i = 0; i < cache_size; i++)
            {
                blocks_.push_back(new block_type());
                free_blocks_.push_back(i);
            }

//				pager_type tmp_pager(cache_size);
//				std::swap(pager_, tmp_pager);
        }

        //! \brief Cache-size
        unsigned_type size() const
        {
            return blocks_.size();
        }

        ~block_cache()
        {
            STXXL_VERBOSE1("hash_map::block_cache destructor addr=" << this);

            typename bid_map_type::const_iterator i = bid_map_.begin();
            for ( ; i != bid_map_.end(); ++i)
            {
                const unsigned_type i_block = (*i).second;
                if (reqs_[i_block].valid())
                    reqs_[i_block]->wait();

                if (dirty_[i_block])
                    blocks_[i_block] = write_buffer_.write(blocks_[i_block], bids_[i_block]);
            }
            write_buffer_.flush();

            for (unsigned_type i = 0; i < size(); ++i)
                delete blocks_[i];
        }

    private:
        /* Force a block from the cache; write back to disk if dirty */
        void __kick_block()
        {
            unsigned_type i_block2kick;

            unsigned_type max_tries = size() + 1;
            unsigned_type i = 0;
            do
            {
                ++i;
                i_block2kick = pager_.kick();
                if (i == max_tries)
                {
                    throw std::runtime_error("The block cache is too small, no block can be kicked out (all blocks are retained)!");
                }
                pager_.hit(i_block2kick);
            } while (retain_count_[i_block2kick] > 0);

            if (valid_subblock_[i_block2kick] == valid_all && reqs_[i_block2kick].valid())
                reqs_[i_block2kick]->wait();

            if (dirty_[i_block2kick])
            {
                blocks_[i_block2kick] = write_buffer_.write(blocks_[i_block2kick], bids_[i_block2kick]);
                ++n_written;
            }
            else
                ++n_clean_forced;

            bid_map_.erase(bids_[i_block2kick]);
            free_blocks_.push_back(i_block2kick);
        }

    public:
        //! \brief Retain a block in cache. Blocks, that are retained by at least one client, won't get kicked. Make sure to release all retained blocks again.
        //! \param bid block, whose retain-count is to be increased
        //! \return true if block was cached, false otherwise
        bool retain_block(const bid_type & bid)
        {
            typename bid_map_type::const_iterator it = bid_map_.find(bid);
            if (it == bid_map_.end())
                return false;

            unsigned_type i_block = (*it).second;
            retain_count_[i_block]++;
            return true;
        }

        //! \brief Release a block (decrement retain-count). If the retain-count reaches 0, a block may be kicked again.
        //! \param bid block, whose retain-count is to be decremented
        //! \return true if operation was successfull (block cached and retain-count > 0), false otherwise
        bool release_block(const bid_type & bid)
        {
            typename bid_map_type::const_iterator it = bid_map_.find(bid);
            if (it == bid_map_.end())
                return false;

            unsigned_type i_block = (*it).second;
            if (retain_count_[i_block] == 0)
                return false;

            retain_count_[i_block]--;
            return true;
        }

        //! \brief Set given block's dirty-flag. Note: If the given block was only partially loaded, it will be completely reloaded.
        //! \return true if block cached, false otherwise
        bool make_dirty(const bid_type & bid)
        {
            typename bid_map_type::const_iterator it = bid_map_.find(bid);
            if (it == bid_map_.end())
                return false;

            unsigned_type i_block = (*it).second;

            // only complete blocks can be marked as dirty
            if (valid_subblock_[i_block] != valid_all)
            {
                reqs_[i_block] = blocks_[i_block]->read(bid);
                valid_subblock_[i_block] = valid_all;
            }

            if (reqs_[i_block].valid()) {
                if (reqs_[i_block]->poll() == false)
                    reqs_[i_block]->wait();
            }

            dirty_[i_block] = true;
            return true;
        }


        //! \brief Retrieve a subblock from the cache. If not yet cached, only the subblock will be loaded.
        //! \param bid block, to which the requested subblock belongs
        //! \param i_subblock index of requested subblock
        //! \return pointer to subblock
        subblock_type * get_subblock(const bid_type & bid, unsigned_type i_subblock)
        {
            block_type * block;
            unsigned_type i_block;
            n_read++;

            // block (partly) cached?
            typename bid_map_type::const_iterator it = bid_map_.find(bid);
            if (it != bid_map_.end())
            {
                i_block = (*it).second;
                block = blocks_[i_block];

                // complete block or wanted subblock is in the cache
                if (valid_subblock_[i_block] == valid_all || valid_subblock_[i_block] == i_subblock)
                {
                    ++n_found;
                    if (valid_subblock_[i_block] == valid_all && reqs_[i_block].valid())
                    {
                        if (reqs_[i_block]->poll() == false)                                    // request not yet completed
                            reqs_[i_block]->wait();
                    }

                    return &((*block)[i_subblock]);
                }

                // wrong subblock in cache
                else
                {
                    ++n_not_found;
                    ++n_wrong_subblock;
                    // actually loading the subblock will be done below
                    // note: if a client still holds a reference to the "old" subblock, it will find its data to be still valid.
                }
            }
            // block not cached
            else
            {
                n_not_found++;

                if (free_blocks_.empty())
                    __kick_block();

                i_block = free_blocks_.back(), free_blocks_.pop_back();
                block = blocks_[i_block];

                bid_map_[bid] = i_block;
                bids_[i_block] = bid;
                dirty_[i_block] = false;
                retain_count_[i_block] = 0;
            }

            // now actually load the wanted subblock and store it within *block
            subblock_bid_type subblock_bid(bid.storage, bid.offset + i_subblock * subblock_type::raw_size);
            request_ptr req = ((*block)[i_subblock]).read(subblock_bid);
            req->wait();

            valid_subblock_[i_block] = i_subblock;
            pager_.hit(i_block);

            return &((*block)[i_subblock]);
        }


        //! \brief Load a block in advance.
        //! \param bid Identifier of the block to load
        void prefetch_block(const bid_type & bid)
        {
            unsigned_type i_block;

            // cached
            typename bid_map_type::const_iterator it = bid_map_.find(bid);
            if (it != bid_map_.end())
            {
                i_block = (*it).second;

                // complete block cached; we can finish here
                if (valid_subblock_[i_block] == valid_all) {
                    pager_.hit(i_block);
                    return;
                }

                // only a single subblock is cached; we have to load the
                // complete block (see below)
            }
            // not even a subblock cached
            else {
                if (free_blocks_.empty())
                    __kick_block();

                i_block = free_blocks_.back(), free_blocks_.pop_back();

                bid_map_[bid] = i_block;
                bids_[i_block] = bid;
                retain_count_[i_block] = 0;
                dirty_[i_block] = false;
            }

            // now actually load the block
            reqs_[i_block] = blocks_[i_block]->read(bid);
            valid_subblock_[i_block] = valid_all;
            pager_.hit(i_block);
        }

        //! \brief Write all dirty blocks back to disk
        void flush()
        {
            typename bid_map_type::const_iterator i = bid_map_.begin();
            for ( ; i != bid_map_.end(); ++i)
            {
                const unsigned_type i_block = (*i).second;
                if (dirty_[i_block])
                {
                    blocks_[i_block] = write_buffer_.write(blocks_[i_block], bids_[i_block]);
                    dirty_[i_block] = false;
                }
            }
            write_buffer_.flush();
        }

        //! \brief Empty cache; don't write back dirty blocks
        void clear()
        {
            free_blocks_.clear();
            for (unsigned_type i = 0; i < size(); i++)
            {
                free_blocks_.push_back(i);
            }
            bid_map_.clear();
        }


        //! \brief Print statistics: Number of hits/misses, blocks forced from cache or written back.
        void print_statistics(std::ostream & o = std::cout) const
        {
            o << "Blocks found                      : " << n_found << " (" << 100. * double(n_found) / double(n_read) << "%)" << std::endl;
            o << "Blocks not found                  : " << n_not_found << std::endl;
            o << "Blocks read                       : " << n_read << std::endl;
            o << "Blocks written                    : " << n_written << std::endl;
            o << "Clean blocks forced from the cache: " << n_clean_forced << std::endl;
            o << "Wrong subblock cached             : " << n_wrong_subblock << std::endl;
        }


        //! \brief Reset all counters to zero
        void reset_statistics()
        {
            n_found = 0;
            n_not_found = 0;
            n_read = 0;
            n_written = 0;
            n_clean_forced = 0;
            n_wrong_subblock = 0;
        }


        //! \brief Exchange contents of two caches
        //! \param obj cache to swap contents with
        void swap(block_cache & obj)
        {
            std::swap(blocks_, obj.blocks_);
            std::swap(bids_, obj.bids_);
            std::swap(reqs_, obj.reqs_);
            std::swap(free_blocks_, obj.free_blocks_);
            std::swap(valid_subblock_, obj.valid_subblock_);
            std::swap(bid_map_, obj.bid_map_);
            std::swap(pager_, obj.pager_);

            std::swap(n_found, obj.n_found);
            std::swap(n_not_found, obj.n_found);
            std::swap(n_read, obj.n_read);
            std::swap(n_written, obj.n_written);
            std::swap(n_clean_forced, obj.n_clean_forced);
        }


//		private:
//			/* show currently cached blocks */
//			void __dump_cache() const {
//				for (unsigned i = 0; i < blocks_.size(); i++) {
//					bid_type bid = bids_[i];
//					if (bid_map_.count(bid) == 0) {
//						std::cout << "Block " << i << ": empty\n";
//						continue;
//					}
//
//					std::cout << "Block " << i << ": bid=" << bids_[i] << " dirty=" << dirty_[i] << " retain_count=" << retain_count_[i] << " valid_subblock=" << valid_subblock_[i] << "\n";
//					for (unsigned k = 0; k < block_type::size; k++) {
//						std::cout << "  Subbblock " << k << ": ";
//						if (valid_subblock_[i] != valid_all && valid_subblock_[i] != k) {
//							std::cout << "not valid\n";
//							continue;
//						}
//						for (unsigned l = 0; l < block_type::value_type::size; l++) {
//							std::cout << "(" << (*blocks_[i])[k][l].first << ", " << (*blocks_[i])[k][l].second << ") ";
//						}
//						std::cout << std::endl;
//					}
//				}
//			}
    };
} /* namespace hash_map */

__STXXL_END_NAMESPACE

namespace std
{
    template <class HashMap>
    void swap(stxxl::hash_map::block_cache<HashMap> & a,
              stxxl::hash_map::block_cache<HashMap> & b)
    {
        a.swap(b);
    }
}

#endif /* STXXL_CONTAINERS_HASHMAP__BLOCK_CACHE_H */
