/***************************************************************************
 *  include/stxxl/bits/containers/hash_map/hash_map.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2007 Markus Westphal <marwes@users.sourceforge.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef _STXXL_HASH_MAP_H_
#define _STXXL_HASH_MAP_H_

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/namespace.h>
#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/common/tuple.h>
#include <stxxl/bits/stream/stream.h>
#include <stxxl/bits/stream/sort_stream.h>

#include <stxxl/bits/containers/hash_map/iterator.h>
#include <stxxl/bits/containers/hash_map/iterator_map.h>
#include <stxxl/bits/containers/hash_map/block_cache.h>
#include <stxxl/bits/containers/hash_map/util.h>


__STXXL_BEGIN_NAMESPACE

namespace hash_map
{
	//! \brief External memory hash-map
    template <class Key_,
              class T_,
              class Hash_,
              class KeyCmp_,
              unsigned SubBlkSize_ = 4 *1024,
              unsigned BlkSize_ = 256,
              class Alloc_ = std::allocator<std::pair<const Key_, T_> >
              >
    class hash_map : private noncopyable
	{
    private:
        typedef hash_map<Key_, T_, Hash_, KeyCmp_, SubBlkSize_, BlkSize_> self_type;

    public:
        typedef Key_ key_type;                                                  /* type of the keys being used        */
        typedef T_ mapped_type;                                                 /* type of the data to be stored      */
        typedef std::pair<Key_, T_> value_type;                                 /* actually store (key-data)-pairs    */
        typedef value_type & reference;                                         /* type for value-references          */
        typedef const value_type & const_reference;                             /* type for constant value-references */
        typedef value_type * pointer;                                           /* */
        typedef value_type const * const_pointer;                               /* */

        typedef stxxl::uint64 size_type;                                        /* */
        typedef stxxl::int64 difference_type;                                   /* */

        typedef Hash_ hasher;                                                   /* type of (mother) hash-function */
        typedef KeyCmp_ keycmp;                                                 /* functor that imposes a ordering on keys (but see __lt()) */

        typedef hash_map_iterator<self_type> iterator;
        typedef hash_map_const_iterator<self_type> const_iterator;

        enum { block_raw_size = BlkSize_ * SubBlkSize_, subblock_raw_size = SubBlkSize_ };        /* subblock- and block-size in bytes */
        enum { block_size = BlkSize_, subblock_size = SubBlkSize_ / sizeof(value_type) };         /* Subblock-size as number of elements, block-size as number of subblocks */

        //TODO: typed_block<subblock_raw_size, value_type, 0, void> (the default) would be better, but fails due to wrong structure sizing
        typedef typed_block<subblock_raw_size, value_type, 0, int> subblock_type;                         /* a subblock consists of subblock_size values */
        typedef typed_block<block_raw_size, subblock_type> block_type;                            /* a block consists of block_size subblocks    */

        typedef typename subblock_type::bid_type subblock_bid_type;                               /* block-identifier for subblocks */
        typedef typename block_type::bid_type bid_type;                                           /* block-identifier for blocks    */
        typedef std::vector<bid_type> bid_container_type;                                         /* container for block-bids  */
        typedef typename bid_container_type::iterator bid_iterator_type;                          /* iterator for block-bids */

        enum source_type { src_internal, src_external, src_unknown };

        typedef node<value_type> node_type;                                                       /* nodes for internal-memory buffer */
        typedef bucket<node_type> bucket_type;                                                    /* buckets */
        typedef std::vector<bucket_type> buckets_container_type;                                  /* */

        typedef iterator_map<self_type> iterator_map_type;                                        /* for tracking active iterators */

        typedef block_cache<block_type> block_cache_type;

        typedef buffered_reader<block_cache_type, bid_iterator_type> reader_type;

        typedef typename Alloc_::template rebind<node_type>::other node_allocator_type;

//private:
        hasher hash_;                                                           /* user supplied mother hash-function */
        keycmp cmp_;                                                            /* user supplied strict-weak-ordering for keys */
        buckets_container_type buckets_;                                        /* array of bucket */
        bid_container_type bids_;                                               /* blocks-ids of allocated blocks  */
        size_type buffer_size_;                                                 /* size of internal-memory buffer in number of entries */
        size_type max_buffer_size_;                                             /* maximum size for internal-memory buffer */
        iterator_map_type iterator_map_;                                        /* keeps track of all active iterators */
        mutable block_cache_type block_cache_;                                  /* */
        node_allocator_type node_allocator_;                                    /* used to allocate new nodes for internal buffer */
        mutable bool oblivious_;                                                /* false if the total-number of values is correct (false) or true if estimated (true); see *oblivious_-methods  */
        mutable size_type num_total_;                                           /* (estimated) number of values */
        float opt_load_factor_;                                                 /* desired load factor after rehashing */

    public:
        //! \brief Construct a new hash-map
        //! \param n initial number of buckets
        //! \param hf hash-function
        //! \param cmp comparator-object
        //! \param buffer_size size for internal-memory buffer in bytes
        hash_map(size_type n = 10000, const hasher & hf = hasher(), const keycmp & cmp = keycmp(), size_type buffer_size = 100 *1024 *1024, const Alloc_ & a = Alloc_()) :
            hash_(hf),
            cmp_(cmp),
            buckets_(n),
            bids_(0),
            buffer_size_(0),
            iterator_map_(this),
            block_cache_(tuning::get_instance()->blockcache_size),
            node_allocator_(a),
            oblivious_(false),
            num_total_(0),
            opt_load_factor_(0.875)
        {
            max_buffer_size_ = buffer_size / sizeof(node_type);
        }


        //! \brief Construct a new hash-map and insert all values in the range [f,l)
        //! \param f beginning of the range
        //! \param l end of the range
        //! \param mem_to_sort additional memory (in bytes) for bulk-insert
        template <class InputIterator>
        hash_map(InputIterator f, InputIterator l, size_type mem_to_sort = 256 *1024 *1024, size_type n = 10000, const hasher & hf = hasher(), const keycmp & cmp = keycmp(), size_type buffer_size = 100 *1024 *1024, const Alloc_ & a = Alloc_()) :
            hash_(hf),
            cmp_(cmp),
          //  buckets_(n),
			buckets_(0), // insert will determine a good size
            bids_(0),
            buffer_size_(0),
            iterator_map_(this),
            block_cache_(tuning::get_instance()->blockcache_size),
            node_allocator_(a),
            oblivious_(false),
            num_total_(0),
            opt_load_factor_(0.875)
        {
            max_buffer_size_ = buffer_size / sizeof(node_type);
            insert(f, l, mem_to_sort);
        }


        ~hash_map()
        {
            clear();
        }

    public:
        //! \brief Hash-function used by this hash-map
        hasher hash_function() const { return hash_; }

        //! \brief Strict-weak-ordering used by this hash-map
        keycmp key_cmp()       const { return cmp_; }


        bool empty() const { return size() != 0; }

    private:
        /* After using *oblivious_-methods only an estimate for the total number of elements can be given.
           This method accesses external memory to calculate the exact number.
        */
        void __make_conscious() /* const */ // todo: make const again
        {
            if (!oblivious_)
                return;

            typedef HashedValuesStream<self_type, reader_type> values_stream_type;

            reader_type reader(bids_.begin(), bids_.end(), &block_cache_);      //will start prefetching automatically
            values_stream_type values(buckets_.begin(), buckets_.end(), reader, bids_.begin(), (self_type *)this);

            num_total_ = 0;
            while (!values.empty())
            {
                ++num_total_;
                ++values;
            }
            oblivious_ = false;
        }

    public:
        //! \brief Number of values currenlty stored. Note: If the correct number is currently unknown (because *_oblivous-methods were used), external memory will be scanned.
        size_type size() const
        {
            if (oblivious_)
                ((self_type *)this)->__make_conscious();
            return num_total_;
        }

        //! \brief The hash-map may store up to this number of values
        size_type max_size() const { return (std::numeric_limits<size_type>::max)(); }


        //! \brief Insert a new value if no value with the same key is already present; external memory must therefore be accessed
        //! \param value what to insert
        //! \return a tuple whose second part is true iff the value was actually added (no value with the same key present); the first part is an iterator pointing to the newly inserted or already stored value
        std::pair<iterator, bool> insert(const value_type & value)
        {
            size_type i_bucket = __bkt_num(value.first);
            bucket_type & bucket = buckets_[i_bucket];
            node_type * node = __find_key_internal(bucket, value.first);

            // found value in internal memory
            if (node && __eq(node->value_.first, value.first))
            {
                bool old_deleted = node->deleted();
                if (old_deleted)
                {
                    node->deleted(false);
                    node->value_ = value;
                    ++num_total_;
                }
                return std::pair<iterator, bool>(iterator(this, i_bucket, node, 0, src_internal, false, value.first), old_deleted);
            }

            // search external memory ...
            else
            {
                tuple<size_type, value_type> result = __find_key_external(bucket, value.first);
                size_type i_external = result.first;
                value_type ext_value = result.second;

                // ... if found, return iterator pointing to external position ...
                if (i_external < bucket.n_external_ && __eq(ext_value.first, value.first))
                {
                    return std::pair<iterator, bool>(iterator(this, i_bucket, node, i_external, src_external, true, value.first), false);
                }
                // ... otherwise create a new buffer-node to add the value
                else
                {
                    ++num_total_;
                    node_type * new_node = (node) ?
                                           (node->next(__new_node(value, node->next(), false))) :
                                           (bucket.list_ = __new_node(value, bucket.list_, false));

                    iterator it(this, i_bucket, new_node, 0, src_internal, false, value.first);

                    ++buffer_size_;
                    if (buffer_size_ >= max_buffer_size_)
                        __rebuild_buckets();                    // will fix it as well

                    return std::pair<iterator, bool>(it, true);
                }
            }
        }


        //! \brief Insert a value; external memory is not accessed so that another value with the same key may be overwritten
        //! \param value what to insert
        //! \return iterator pointing to the inserted value
        iterator insert_oblivious(const value_type & value)
        {
            size_type i_bucket = __bkt_num(value.first);
            bucket_type & bucket = buckets_[i_bucket];
            node_type * node = __find_key_internal(bucket, value.first);

            // found value in internal memory
            if (node && __eq(node->value_.first, value.first))
            {
                if (node->deleted())
                    ++num_total_;

                node->deleted(false);
                node->value_ = value;
                return iterator(this, i_bucket, node, 0, src_internal, false, value.first);
            }
            // not found; ignore external memory and add a new node to the internal-memory buffer
            else
            {
                oblivious_ = true;
                ++num_total_;
                node_type * new_node = (node) ?
                                       (node->next(__new_node(value, node->next(), false))) :
                                       (bucket.list_ = __new_node(value, bucket.list_, false));

                // there may be some iterators that reference the newly inserted value in external memory
                // these need to be fixed (make them point to new_node)
                iterator_map_.fix_iterators_2int(i_bucket, value.first, new_node);

                iterator it(this, i_bucket, new_node, 0, src_internal, false, value.first);

                ++buffer_size_;
                if (buffer_size_ >= max_buffer_size_)
                    __rebuild_buckets();

                return it;
            }
        }


        //! \brief Erase value by iterator
        //! \param it iterator pointing to the value to erase
        void erase(const_iterator it)
        {
            --num_total_;
            bucket_type & bucket = buckets_[it.i_bucket_];

            if (it.source_ == src_internal)
            {
                it.node_->deleted(true);
                iterator_map_.fix_iterators_2end(it.i_bucket_, it.key_);
            }
            else {
                node_type * node = __find_key_internal(bucket, it.key_);                // find biggest value < iterator's value
                assert(!node || !__eq(node->value_.first, it.key_));

                // add delete-node to buffer
                if (node)
                    node->next(__new_node(value_type(it.key_, mapped_type()), node->next(), true));
                else
                    bucket.list_ = __new_node(value_type(it.key_, mapped_type()), bucket.list_, true);

                iterator_map_.fix_iterators_2end(it.i_bucket_, it.key_);

                ++buffer_size_;
                if (buffer_size_ >= max_buffer_size_)
                    __rebuild_buckets();
            }
        }


        //! \brief Erase value by key; check external memory
        //! \param key key of value to erase
        //! \return number of values actually erased (0 or 1)
        size_type erase(const key_type & key)
        {
            size_type i_bucket = __bkt_num(key);
            bucket_type & bucket = buckets_[i_bucket];
            node_type * node = __find_key_internal(bucket, key);

            // found in internal memory
            if (node && __eq(node->value_.first, key))
            {
                if (!node->deleted())
                {
                    node->deleted(true);
                    --num_total_;
                    iterator_map_.fix_iterators_2end(i_bucket, key);
                    return 1;
                }
                else
                    return 0;           // already deleted
            }
            // check external memory
            else
            {
                tuple<size_type, value_type> result = __find_key_external(bucket, key);
                size_type i_external = result.first;
                value_type ext_value = result.second;

                // found in external memory; add delete-node
                if (i_external < bucket.n_external_ && __eq(ext_value.first, key))
                {
                    --num_total_;

                    if (node)
                        node->next(__new_node(value_type(key, mapped_type()), node->next(), true));
                    else
                        bucket.list_ = __new_node(value_type(key, mapped_type()), bucket.list_, true);

                    iterator_map_.fix_iterators_2end(i_bucket, key);

                    ++buffer_size_;
                    if (buffer_size_ >= max_buffer_size_)
                        __rebuild_buckets();

                    return 1;
                }
                // no value with given key
                else
                    return 0;
            }
        }


        //! \brief Erase value by key but without looking at external memrory
        //! \param key key for value to release
        void erase_oblivious(const key_type & key)
        {
            size_type i_bucket = __bkt_num(key);
            bucket_type & bucket = buckets_[i_bucket];
            node_type * node = __find_key_internal(bucket, key);

            // found value in internal-memory
            if (node && __eq(node->value_.first, key))
            {
                if (!node->deleted())
                {
                    --num_total_;
                    node->deleted(true);
                    iterator_map_.fix_iterators_2end(i_bucket, key);
                }
            }
            // not found; ignore external memory and add delete-node
            else
            {
                oblivious_ = true;
                --num_total_;

                if (node)
                    node->next(__new_node(value_type(key, mapped_type()), node->next(), true));
                else
                    bucket.list_ = __new_node(value_type(key, mapped_type()), bucket.list_, true);

                iterator_map_.fix_iterators_2end(i_bucket, key);

                ++buffer_size_;
                if (buffer_size_ >= max_buffer_size_)
                    __rebuild_buckets();
            }
        }


        //! \brief Reset hash-map: erase all values, invalidate all iterators
        void clear()
        {
            iterator_map_.fix_iterators_all2end();
            block_cache_.flush();
            block_cache_.clear();

            // reset buckets and release buffer-memory
            for (size_type i_bucket = 0; i_bucket < buckets_.size(); i_bucket++) {
                __erase_nodes(buckets_[i_bucket].list_, NULL);
                buckets_[i_bucket] = bucket_type();
            }
            oblivious_ = false;
            num_total_ = 0;
            buffer_size_ = 0;

            // free external memory
            block_manager * bm = block_manager::get_instance();
            bm->delete_blocks(bids_.begin(), bids_.end());
            bids_.clear();
        }


        //! \brief Exchange stored values with another hash-map
        //! \param obj hash-map to swap values with
        void swap(self_type & obj)
        {
            std::swap(buckets_, obj.buckets_);
            std::swap(bids_, obj.bids_);

            std::swap(oblivious_, obj.oblivious_);
            std::swap(num_total_, obj.num_total_);

            std::swap(node_allocator_, obj.node_allocator_);

            std::swap(hash_, obj.hash_);
            std::swap(cmp_, obj.cmp_);

            std::swap(buffer_size_, obj.buffer_size_);
            std::swap(max_buffer_size_, obj.max_buffer_size_);

            std::swap(opt_load_factor_, obj.opt_load_factor_);

            std::swap(iterator_map_, obj.iterator_map_);

            std::swap(block_cache_, obj.block_cache_);
        }


    private:
        // find statistics
        mutable size_type n_subblocks_loaded;
        mutable size_type n_found_internal;
        mutable size_type n_found_external;
        mutable size_type n_not_found;

    public:
        void reset_statistics()
        {
            iterator_map_.reset_statistics();
            block_cache_.reset_statistics();
            n_subblocks_loaded = n_found_external = n_found_internal = n_not_found = 0;
        }


        void print_statistics(std::ostream & o = std::cout) const
        {
            o << "Find-statistics:" << std::endl;
            o << "  Found internal     : " << n_found_internal << std::endl;
            o << "  Found external     : " << n_found_external << std::endl;
            o << "  Not found          : " << n_not_found << std::endl;
            o << "  Subblocks searched : " << n_subblocks_loaded << std::endl;

            iterator_map_.print_statistics(o);
            block_cache_.print_statistics(o);
        }


        //! \brief Look up value by key. Non-const access.
        //! \param key key for value to look up
        iterator find(const key_type & key)
        {
            if (buffer_size_ + 1 >= max_buffer_size_)   // (*)
                __rebuild_buckets();

            size_type i_bucket = __bkt_num(key);
            bucket_type & bucket = buckets_[i_bucket];
            node_type * node = __find_key_internal(bucket, key);

            // found in internal-memory buffer
            if (node && __eq(node->value_.first, key)) {
                n_found_internal++;
                if (node->deleted())
                    return this->__end<iterator>();
                else
                    return iterator(this, i_bucket, node, 0, src_internal, false, key);
            }
            // search external elements
            else {
                tuple<size_type, value_type> result = __find_key_external(bucket, key);
                size_type i_external = result.first;
                value_type value = result.second;

                // found in external memory
                if (i_external < bucket.n_external_ && __eq(value.first, key)) {
                    n_found_external++;

                    // we ultimately expect the user to de-reference the returned iterator to change its value (non-const!).
                    // to prevent an additional disk-access, we create a new node in the internal-memory buffer overwriting the external value.
                    // note: by checking and rebuilding (if neccessary) in (*) we made sure that the new node will fit into the buffer and
                    // no rebuild is neccessary here.
                    node_type * new_node = (node) ?
                                           (node->next(__new_node(value, node->next(), false))) :
                                           (bucket.list_ = __new_node(value, bucket.list_, false));
                    ++buffer_size_;

                    iterator_map_.fix_iterators_2int(i_bucket, value.first, new_node);

                    return iterator(this, i_bucket, new_node, i_external + 1, src_internal, true, key);
                }
                // not found in external memory
                else {
                    n_not_found++;
                    return this->__end<iterator>();
                }
            }
        }


        //! \brief Look up value by key. Const access.
        //! \param key key for value to look up
        const_iterator find(const key_type & key) const
        {
            size_type i_bucket = __bkt_num(key);
            const bucket_type & bucket = buckets_[i_bucket];
            node_type * node = __find_key_internal(bucket, key);

            // found in internal-memory buffer
            if (node && __eq(node->value_.first, key)) {
                n_found_internal++;
                if (node->deleted())
                    return this->__end<const_iterator>();
                else
                    return const_iterator((self_type *)this, i_bucket, node, 0, src_internal, false, key);
            }
            // search external elements
            else {
                tuple<size_type, value_type> result = __find_key_external(bucket, key);
                size_type i_external = result.first;
                value_type value = result.second;

                // found in external memory
                if (i_external < bucket.n_external_ && __eq(value.first, key)) {
                    n_found_external++;
                    return const_iterator((self_type *)this, i_bucket, node, i_external, src_external, true, key);
                }
                // not found in external memory
                else {
                    n_not_found++;
                    return this->__end<const_iterator>();
                }
            }
        }


        //! \brief Number of values with given key
        //! \param k key for value to look up
        //! \return 0 or 1 depending on the presence of a value with the given key
        size_type count(const key_type & k) const
        {
            const_iterator cit = find(k);
            return (cit == end()) ? 0 : 1;
        }


        //! \brief Finds a range containing all values with given key. Non-const access
        //! \param key key to look for#
        //! \return range may be empty or contains exactly one value
        std::pair<iterator, iterator> equal_range(const key_type & key)
        {
            iterator it = find(key);
            return std::pair<iterator, iterator>(it, it);
        }


        //! \brief Finds a range containing all values with given key. Const access
        //! \param key key to look for#
        //! \return range may be empty or contains exactly one value
        std::pair<const_iterator, const_iterator> equal_range(const key_type & key) const
        {
            const_iterator cit = find(key);
            return std::pair<const_iterator, const_iterator>(cit, cit);
        }


        //! \brief Convenience operator to quickly insert or find values. Use with caution since using this operator will check external-memory.
        mapped_type & operator [] (const key_type & key)
        {
            if (buffer_size_ + 1 >= max_buffer_size_)   // (*)
                __rebuild_buckets();

            size_type i_bucket = __bkt_num(key);
            bucket_type & bucket = buckets_[i_bucket];
            node_type * node = __find_key_internal(bucket, key);

            // found in internal-memory buffer
            if (node && __eq(node->value_.first, key)) {
                if (node->deleted()) {
                    node->deleted(false);
                    node->value_.second = mapped_type();
                    ++num_total_;
                }
                return node->value_.second;
            }
            // search external elements
            else {
                tuple<size_type, value_type> result = __find_key_external(bucket, key);
                size_type i_external = result.first;
                value_type found_value = result.second;

                value_type buffer_value = (i_external < bucket.n_external_ && __eq(found_value.first, key)) ?
                                          found_value :
                                          value_type(key, mapped_type());

                // add a new node to the buffer. this new node's value overwrites the external value if it was found and otherwise is set to
                // (key, mapped_type())
                node_type * new_node = (node) ?
                                       (node->next(__new_node(buffer_value, node->next(), false))) :
                                       (bucket.list_ = __new_node(buffer_value, bucket.list_, false));
                ++buffer_size_;
                // note that we already checked the buffer-size in (*)

                return new_node->value_.second;
            }
        }


        //! \brief Number of buckets
        size_type bucket_count() const { return buckets_.size(); }

        //! \brief Maximum number of buckets
        size_type max_bucket_count() const { return max_size() / subblock_size; }

        //! \brief Bucket-index for values with given key.
//        size_type bucket(const key_type & k) const { return __bkt_num(k); }


    public:
        //! \brief Average number of (sub)blocks occupied by a bucket.
        float load_factor() const { return (float)num_total_ / ((float)subblock_size * (float)buckets_.size()); }


        //! \brief Set desired load-factor
        float opt_load_factor() const { return opt_load_factor_; }

        //! \brief Set desired load-factor
        void opt_load_factor(float z)
        {
            opt_load_factor_ = z;
            if (load_factor() > opt_load_factor_)
                __rebuild_buckets();
        }

        //! \brief Rehash with (at least) n buckets */
        void rehash(size_type n = 0)
        {
            __rebuild_buckets(n);
        }


        //! \brief Number of bytes occupied by buffer
        size_type buffer_size() const
        {
            return buffer_size_ * sizeof(node_type);    // buffer-size internally stored as number of nodes
        }


        //! \brief Maximum buffer size in byte
        size_type max_buffer_size() const { return max_buffer_size_ * sizeof(node_type); }


        //! \brief Set maximum buffer size
        //! \param buffer_size new size in byte
        void max_buffer_size(size_type buffer_size)
        {
            max_buffer_size_ = buffer_size / sizeof(node_type);
            if (buffer_size_ >= max_buffer_size_)
                __rebuild_buckets();
        }


    private:
        /* iterator pointing to the beginnning of the hash-map */
        template <class ItType>
        ItType __begin() const
        {
            ItType it((self_type *)this, 0, buckets_[0].list_, 0, src_unknown, true, key_type());       // correct key will be set by find_next()
            it.find_next();

            return it;
        }


        /* iterator pointing to the end of the hash-map (iterator-type as template-parameter) */
        template <class ItType>
        ItType __end() const
        {
            self_type * non_const_this = (self_type *)this;
            return ItType(non_const_this);
        }

    public:
        //! \brief Returns an iterator pointing to the beginning of the hash-map
        iterator begin() { return __begin<iterator>(); }

        //! \brief Returns a const_interator pointing to the beginning of the hash-map
        const_iterator begin() const { return __begin<const_iterator>(); }

        //! \brief Returns an iterator pointing to the end of the hash-map
        iterator end() { return __end<iterator>(); }

        //! \brief Returns a const_iterator pointing to the end of the hash-map
        const_iterator end() const { return __end<const_iterator>(); }


    private:
        /* Allocate a new buffer-node */
        node_type * __get_node()
        {
            return node_allocator_.allocate(1);
        }


        /* Free given node */
        void __put_node(node_type * node)
        {
            node_allocator_.deallocate(node, 1);
        }


        /* Allocate a new buffer-node and initialize with given value, node and deleted-flag */
        node_type * __new_node(const value_type & value, node_type * nxt, bool del)
        {
            node_type * node = __get_node();
            node->value_ = value;
            node->next(nxt);
            node->deleted(del);
            return node;
        }


        /* Free nodes in range [first, last). If last is NULL all nodes will be freed. */
        void __erase_nodes(node_type * first, node_type * last)
        {
            node_type * curr = first;
            while (curr != last)
            {
                node_type * next = curr->next();
                __put_node(curr);
                curr = next;
            }
        }


        /* Bucket-index for values with given key */
        size_type __bkt_num(const key_type & key) const
        {
            return __bkt_num(key, buckets_.size());
        }


        /* Bucket-index for values with given key. The total number of buckets has to be specified as well.
           The bucket number is determined by
              bucket_num = (hash/max_hash)*n_buckets
           max_hash is in fact 2^63-1 (size_type=uint64) but we rather divide by 2^64, so we can use plain integer
           arithmetic easily (there should be only a small difference): this way we must only calculate the upper
           64 bits of the product hash*n_buckets and we're done. See http://www.cs.uaf.edu/~cs301/notes/Chapter5/node5.html
        */
        size_type __bkt_num(const key_type & key, size_type n) const
        {
            size_type a, b, c, d, x, y;
            size_type low, high;

            const size_type hash = hash_(key);
            a = (hash >> 32) & 0xffffffff;
            b = hash & 0xffffffff;
            c = (n >> 32) & 0xffffffff;
            d = n & 0xffffffff;

            low = b * d;
            x = a * d + c * b;
            y = ((low >> 32) & 0xffffffff) + x;

//		low = (low & 0xffffffff) | ((y & 0xffffffff) << 32);	// we actually do not need the lower part
            high = (y >> 32) & 0xffffffff;
            high += a * c;

            return high;

//		return (size_type)((double)n * ((double)hash_(key)/(double)std::numeric_limits<size_type>::max()));
        }


        /* Locate the given key in the internal-memory chained list.
           If the key is not present, the node with the biggest key smaller than the given key is returned.
           Note that the returned value may be zero: either because the chained list is empty or because
           the given key is smaller than all other keys in the chained list.
        */
        node_type * __find_key_internal(const bucket_type & bucket, const key_type & key) const
        {
            node_type * curr = bucket.list_;
            node_type * old = 0;
            for ( ; curr && __leq(curr->value_.first, key); curr = curr->next()) {
                old = curr;
            }

            return old;
        }


        /* Search for key in external part of bucket. Return value is (i_external, value),
           where i_ext = bucket._num_external if key could not be found.
        */
        tuple<size_type, value_type> __find_key_external(const bucket_type & bucket, const key_type & key) const
        {
            subblock_type * subblock;

            // number of subblocks occupied by bucket
            size_type n_subblocks = bucket.n_external_ / subblock_size;
            if (bucket.n_external_ % subblock_size != 0)
                n_subblocks++;

            for (size_type i_subblock = 0; i_subblock < n_subblocks; i_subblock++)
            {
                subblock = __load_subblock(bucket, i_subblock);
                // number of values in i-th subblock
                size_type n_values = (i_subblock + 1 < n_subblocks) ? static_cast<size_type>(subblock_size) : (bucket.n_external_ - i_subblock * subblock_size);
                // TODO: replace with bucket.n_external_ % subblock_size

                // biggest key in current subblock still too small => next subblock
                if (__lt((*subblock)[n_values - 1].first, key))
                    continue;

                // binary search in current subblock
                size_type i_lower = 0, i_upper = n_values;
                while (i_lower + 1 != i_upper)
                {
                    size_type i_middle = (i_lower + i_upper) / 2;
                    if (__leq((*subblock)[i_middle].first, key))
                        i_lower = i_middle;
                    else
                        i_upper = i_middle;
                }

                value_type value = (*subblock)[i_lower];

                if (__eq(value.first, key))
                    return tuple<size_type, value_type>(i_subblock * subblock_size + i_lower, value);
                else
                    return tuple<size_type, value_type>(bucket.n_external_, value_type());
            }

            return tuple<size_type, value_type>(bucket.n_external_, value_type());
        }


        /* Load the given bucket's i-th subblock.
           Since a bucket may be spread over several blocks, we must
           1. determine in which block the requested subblock is located
           2. at which position within the obove-mentioned block the questioned subblock is located
        */
        subblock_type * __load_subblock(const bucket_type & bucket, size_type which_subblock) const
        {
            n_subblocks_loaded++;

            // index of the requested subblock counted from the very beginning of the bucket's first block
            size_type i_abs_subblock = bucket.i_subblock_ + which_subblock;

            bid_type bid = bids_[bucket.i_block_ + (i_abs_subblock / block_size)];   /* 1. */
            size_type i_subblock_within = i_abs_subblock % block_size;               /* 2. */

            return block_cache_.get_subblock(bid, i_subblock_within);
        }


        typedef HashedValue<self_type> hashed_value_type;

        /* Functor to extracts the actual value from a HashedValue-struct */
        struct HashedValueExtractor
        {
            value_type & operator () (hashed_value_type & hvalue) { return hvalue.value_; }
        };


        /* Will return from its input-stream all values that are to be stored in the given bucket.
           Those values must appear in consecutive order beginning with the input-stream's current value.
        */
        template <class InputStream, class ValueExtractor>
        struct HashingStream
        {
            typedef typename InputStream::value_type value_type;

            self_type * map_;
            InputStream & input_;
            size_type i_bucket_;
            size_type bucket_size_;
            value_type value_;
            bool empty_;
            ValueExtractor vextract_;

            HashingStream(InputStream & input, size_type i_bucket, ValueExtractor vextract, self_type * map) :
                map_(map),
                input_(input),
                i_bucket_(i_bucket),
                bucket_size_(0),
                vextract_(vextract)
            {
                empty_ = find_next();
            }

            const value_type & operator * () { return value_; }

            bool empty() const { return empty_; }

            void operator ++ ()
            {
                ++input_;
                empty_ = find_next();
            }

            bool find_next()
            {
                if (input_.empty())
                    return true;
                value_ = *input_;
                if (map_->__bkt_num(vextract_(value_).first) != i_bucket_)
                    return true;

                ++bucket_size_;
                return false;
            }
        };


        /*	Rebuild hash-map. The desired number of buckets may be supplied. */
        void __rebuild_buckets(size_type n_desired = 0)
        {
            typedef buffered_writer<block_type, bid_container_type> writer_type;
            typedef HashedValuesStream<self_type, reader_type> values_stream_type;
            typedef HashingStream<values_stream_type, HashedValueExtractor> hashing_stream_type;

            const int_type write_buffer_size = config::get_instance()->disks_number() * 4;

            // determine new number of buckets from desired load_factor ...
            size_type n_new;
            n_new = (size_type)ceil((double)num_total_ / ((double)subblock_size * (double)opt_load_factor()));

            // ... but give the user the chance to request even more buckets
            if (n_desired > n_new)
                n_new = std::min<size_type>(n_desired, max_bucket_count());

            // allocate new buckets and bids
            buckets_container_type * old_buckets = new buckets_container_type(n_new);
            std::swap(buckets_, *old_buckets);
            bid_container_type * old_bids = new bid_container_type();
            std::swap(bids_, *old_bids);

            // read stored values in consecutive order
            reader_type * reader = new reader_type(old_bids->begin(), old_bids->end(), &block_cache_);          // use new to controll point of destruction (see below)
            values_stream_type values_stream(old_buckets->begin(), old_buckets->end(), *reader, old_bids->begin(), this);

            writer_type writer(&bids_, write_buffer_size, write_buffer_size / 2);

            // re-distribute values among new buckets.
            // this makes use of the fact that if value1 preceeds value2 before resizing, value1 will preceed value2 after resizing as well (uniform rehashing)
            num_total_ = 0;
            for (size_type i_bucket = 0; i_bucket < buckets_.size(); i_bucket++)
            {
                buckets_[i_bucket] = bucket_type();
                buckets_[i_bucket].i_block_ = writer.i_block();
                buckets_[i_bucket].i_subblock_ = writer.i_subblock();

                hashing_stream_type hasher(values_stream, i_bucket, HashedValueExtractor(), this);
                size_type i_ext = 0;
                while (!hasher.empty())
                {
                    const hashed_value_type & hvalue = *hasher;
                    iterator_map_.fix_iterators_2ext(hvalue.i_bucket_, hvalue.value_.first, i_bucket, i_ext);

                    writer.append(hvalue.value_);
                    ++hasher;
                    ++i_ext;
                }

                writer.finish_subblock();
                buckets_[i_bucket].n_external_ = hasher.bucket_size_;
                num_total_ += hasher.bucket_size_;
            }
            writer.flush();
            delete reader;      // reader must be deleted before deleting old_bids because its destructor will dereference the bid-iterator
            block_cache_.clear();

            // get rid of old blocks and buckets
            block_manager * bm = stxxl::block_manager::get_instance();
            bm->delete_blocks(old_bids->begin(), old_bids->end());
            delete old_bids;
            delete old_buckets;

            buffer_size_ = 0;
            oblivious_ = false;
        }


        /* Stream for filtering duplicates. Used to eliminate duplicated values when bulk-inserting
           Note: input has to be sorted, so that duplicates will occure in row
        */
        template <class InputStream>
        struct UniqueValueStream
        {
            typedef typename InputStream::value_type value_type;
            self_type * map_;
            InputStream & in_;

            UniqueValueStream(InputStream & input, self_type * map) : map_(map), in_(input) { }

            bool empty() const { return in_.empty(); }

            value_type operator * () { return *in_; }

            void operator ++ ()
            {
                value_type v_old = *in_;
                ++in_;
                while (!in_.empty() && map_->__eq(v_old.first, (*in_).first))
                    ++in_;
            }
        };

        template <class InputStream>
        struct AddHashStream
        {
            typedef std::pair<size_type, typename InputStream::value_type> value_type;
            self_type * map_;
            InputStream & in_;

            AddHashStream(InputStream & input, self_type * map) : map_(map), in_(input) { }

            bool empty() const { return in_.empty(); }
            value_type operator * () { return value_type(map_->hash_((*in_).first), *in_); }
            void operator ++ () { ++in_; }
        };


        /* Extracts the value-part (ignoring the hashvalue); required by HashingStream (see above) */
        struct StripHashFunctor
        {
            value_type & operator () (std::pair<size_type, value_type> & v) { return v.second; }
        };


        /* Comparator object for values as required by stxxl::sort. Sorting is done lexicographically by <hash-value, key>
           Note: the hash-value has already been computed.
        */
        struct Cmp
        {
            self_type * map_;
            Cmp(self_type * map) : map_(map) { }

            bool operator () (const std::pair<size_type, value_type> & a, const std::pair<size_type, value_type> & b) const
            {
                return (a.first < b.first) ||
                       ((a.first == b.first) && map_->cmp_(a.second.first, b.second.first));
            }
            std::pair<size_type, value_type> min_value() const { return std::pair<size_type, value_type>((std::numeric_limits<size_type>::min)(), value_type(map_->cmp_.min_value(), mapped_type())); }
            std::pair<size_type, value_type> max_value() const { return std::pair<size_type, value_type>((std::numeric_limits<size_type>::max)(), value_type(map_->cmp_.max_value(), mapped_type())); }
        };

    public:
        //! \brief Bulk-insert of values in the range [f, l)
        //! \param f beginning of the range
        //! \param l end of the range
        //! \param mem internal memory that may be used (note: this memory will be used additionally to the buffer). The more the better
        template <class InputIterator>
        void insert(InputIterator f, InputIterator l, size_type mem)
        {
            typedef HashedValuesStream<self_type, reader_type> old_values_stream;                               // values already stored in the hashtable ("old values")
            typedef HashingStream<old_values_stream, HashedValueExtractor> old_hashing_stream;                  // old values, that are to be stored in a certain (new) bucket

            #ifdef BOOST_MSVC
             typedef typename stxxl::stream::streamify_traits<InputIterator>::stream_type   input_stream;                      // values to insert ("new values")
            #else
             typedef __typeof__(stxxl::stream::streamify(f, l))              input_stream;                      // values to insert ("new values")
            #endif
            typedef AddHashStream<input_stream> new_values_stream;                                              // new values with added hash: (hash, (key, mapped))
            typedef stxxl::stream::sort<new_values_stream, Cmp> new_sorted_values_stream;                       // new values sorted by <hash-value, key>
            typedef UniqueValueStream<new_sorted_values_stream> new_unique_values_stream;                       // new values sorted by <hash-value, key> with duplicates eliminated
            typedef HashingStream<new_unique_values_stream, StripHashFunctor> new_hashing_stream;               // new values, that are to be stored in a certain bucket

            typedef buffered_writer<block_type, bid_container_type> writer_type;

            int_type write_buffer_size = config::get_instance()->disks_number() * 2;


            // calculate new number of buckets
            size_type num_total_new = num_total_ + (l - f);     // estimated number of elements
            size_type n_new = (size_type)ceil((double)num_total_new / ((double)subblock_size * (double)opt_load_factor()));
            if (n_new > max_bucket_count())
                n_new = max_bucket_count();

            // prepare new buckets and bids
            buckets_container_type * old_buckets = new buckets_container_type(n_new);
            std::swap(buckets_, *old_buckets);
            bid_container_type * old_bids = new bid_container_type();           // writer will allocate new blocks as necessary
            std::swap(bids_, *old_bids);

            // already stored values ("old values")
            reader_type * reader = new reader_type(old_bids->begin(), old_bids->end(), &block_cache_);
            old_values_stream old_values(old_buckets->begin(), old_buckets->end(), *reader, old_bids->begin(), this);

            // values to insert ("new values")
            input_stream input = stxxl::stream::streamify(f, l);
            new_values_stream new_values(input, this);
            new_sorted_values_stream new_sorted_values(new_values, Cmp(this), mem);
            new_unique_values_stream new_unique_values(new_sorted_values, this);

            writer_type writer(&bids_, write_buffer_size, write_buffer_size / 2);

            num_total_ = 0;
            for (size_type i_bucket = 0; i_bucket < buckets_.size(); i_bucket++)
            {
                buckets_[i_bucket] = bucket_type();
                buckets_[i_bucket].i_block_ = writer.i_block();
                buckets_[i_bucket].i_subblock_ = writer.i_subblock();

                old_hashing_stream old_hasher(old_values, i_bucket, HashedValueExtractor(), this);
                new_hashing_stream new_hasher(new_unique_values, i_bucket, StripHashFunctor(), this);
                size_type bucket_size = 0;

                // more old and new values for the current bucket => choose smallest
                while (!old_hasher.empty() && !new_hasher.empty())
                {
                    size_type old_hash = hash_((*old_hasher).value_.first);
                    size_type new_hash = (*new_hasher).first;
                    key_type old_key = (*old_hasher).value_.first;
                    key_type new_key = (*new_hasher).second.first;

                    // old value wins
                    if ((old_hash < new_hash) || (old_hash == new_hash && cmp_(old_key, new_key)))            // (__lt((*old_hasher)._value.first, (*new_hasher).second.first))
                    {
                        const hashed_value_type & hvalue = *old_hasher;
                        iterator_map_.fix_iterators_2ext(hvalue.i_bucket_, hvalue.value_.first, i_bucket, bucket_size);
                        writer.append(hvalue.value_);
                        ++old_hasher;
                    }
                    // new value smaller or equal => new value wins
                    else
                    {
                        if (__eq(old_key, new_key))
                        {
                            const hashed_value_type & hvalue = *old_hasher;
                            iterator_map_.fix_iterators_2ext(hvalue.i_bucket_, hvalue.value_.first, i_bucket, bucket_size);
                            ++old_hasher;
                        }
                        writer.append((*new_hasher).second);
                        ++new_hasher;
                    }
                    ++bucket_size;
                }
                // no more new values for the current bucket
                while (!old_hasher.empty())
                {
                    const hashed_value_type & hvalue = *old_hasher;
                    iterator_map_.fix_iterators_2ext(hvalue.i_bucket_, hvalue.value_.first, i_bucket, bucket_size);
                    writer.append(hvalue.value_);
                    ++old_hasher;
                    ++bucket_size;
                }
                // no more old values for the current bucket
                while (!new_hasher.empty())
                {
                    writer.append((*new_hasher).second);
                    ++new_hasher;
                    ++bucket_size;
                }

                writer.finish_subblock();
                buckets_[i_bucket].n_external_ = bucket_size;
                num_total_ += bucket_size;
            }
            writer.flush();
            delete reader;
            block_cache_.clear();

            block_manager * bm = stxxl::block_manager::get_instance();
            bm->delete_blocks(old_bids->begin(), old_bids->end());
            delete old_bids;
            delete old_buckets;

            buffer_size_ = 0;
            oblivious_ = false;
        }


//private:
        /* 1 iff a <  b
           The comparison is done lexicographically by (hash-value, key)
        */
        bool __lt(const key_type & a, const key_type & b) const
        {
            size_type hash_a = hash_(a);
            size_type hash_b = hash_(b);

            return (hash_a < hash_b) ||
                   ((hash_a == hash_b) && cmp_(a, b));
        }

        bool __gt(const key_type & a, const key_type & b) const { return __lt(b, a); }   /* 1 iff a >  b */
        bool __leq(const key_type & a, const key_type & b) const { return !__gt(a, b); } /* 1 iff a <= b */
        bool __geq(const key_type & a, const key_type & b) const { return !__lt(a, b); } /* 1 iff a >= b */

        /* 1 iff a == b. note: it is mandatory that equal keys yield equal hash-values => hashing not neccessary for equality-testing */
        bool __eq(const key_type & a, const key_type & b) const { return !cmp_(a, b) && !cmp_(b, a); }


        friend class hash_map_iterator_base<self_type>;
        friend class hash_map_iterator<self_type>;
        friend class hash_map_const_iterator<self_type>;
        friend class iterator_map<self_type>;
        friend class block_cache<block_type>;
        friend struct HashedValuesStream<self_type, reader_type>;


//	void __dump_external() {
//		reader_type reader(bids_.begin(), bids_.end(), &block_cache_);
//
//		for (size_type i_block = 0; i_block < bids_.size(); i_block++) {
//			std::cout << "block " << i_block << ":\n";
//
//			for (size_type i_subblock = 0; i_subblock < block_size; i_subblock++) {
//				std::cout << "  subblock " << i_subblock <<":\n    ";
//
//				for (size_type i_element = 0; i_element < subblock_size; i_element++) {
//					std::cout << reader.const_value().first << ", ";
//					++reader;
//				}
//				std::cout << std::endl;
//			}
//		}
//	}
//
//	void __dump_buckets() {
//		reader_type reader(bids_.begin(), bids_.end(), &block_cache_);
//
//		std::cout << "number of buckets: " << buckets_.size() << std::endl;
//		for (size_type i_bucket = 0; i_bucket < buckets_.size(); i_bucket++) {
//			const bucket_type & bucket = buckets_[i_bucket];
//			reader.skip_to(bids_.begin()+bucket.i_block_, bucket.i_subblock_);
//
//			std::cout << "  bucket " << i_bucket << ": block=" << bucket.i_block_ << ", subblock=" << bucket.i_subblock_ << ", external=" << bucket.n_external_ << std::endl;
//
//			node_type *node = bucket.list_;
//			std::cout << "     internal_list=";
//			while (node) {
//				std::cout << node->value_.first << " (del=" << node->deleted() <<"), ";
//				node = node->next();
//			}
//			std::cout << std::endl;
//
//			std::cout << "     external=";
//			for (size_type i_element = 0; i_element < bucket.n_external_; i_element++) {
//				std::cout << reader.const_value().first << ", ";
//				++reader;
//			}
//			std::cout << std::endl;
//		}
//	}
//
//	void __dump_bucket_statistics() {
//		std::cout << "number of buckets: " << buckets_.size() << std::endl;
//		for (size_type i_bucket = 0; i_bucket < buckets_.size(); i_bucket++) {
//			const bucket_type & bucket = buckets_[i_bucket];
//			std::cout << "  bucket " << i_bucket << ": block=" << bucket.i_block_ << ", subblock=" << bucket.i_subblock_ << ", external=" << bucket.n_external_ << std::endl;
//		}
//	}

    public:
        //! \brief Even more statistics: Number of buckets, number of values, buffer-size, values per bucket
        void print_load_statistics(std::ostream & o = std::cout)
        {
            size_type sum_external = 0;
            size_type square_sum_external = 0;
            size_type max_external = 0;

            for (size_type i_bucket = 0; i_bucket < buckets_.size(); i_bucket++) {
                const bucket_type & b = buckets_[i_bucket];

                sum_external += b.n_external_;
                square_sum_external += b.n_external_ * b.n_external_;
                if (b.n_external_ > max_external)
                    max_external = b.n_external_;
            }

            double avg_external = (double)sum_external / (double)buckets_.size();
            double std_external = sqrt(((double)square_sum_external / (double)buckets_.size()) - (avg_external * avg_external));

            o << "Bucket count         : " << buckets_.size() << std::endl;
            o << "Values total         : " << num_total_ << std::endl;
            o << "Values buffered      : " << buffer_size_ << std::endl;
            o << "Max Buffer-Size      : " << max_buffer_size_ << std::endl;
            o << "Max external/bucket  : " << max_external << std::endl;
            o << "Avg external/bucket  : " << avg_external << std::endl;
            o << "Std external/bucket  : " << std_external << std::endl;
            o << "Load-factor          : " << load_factor() << std::endl;
            o << "Blocks allocated     : " << bids_.size() << " => " << (bids_.size() * block_type::raw_size) << " bytes" << std::endl;
            o << "Bytes per value      : " << ((double)(bids_.size() * block_type::raw_size) / (double)num_total_) << std::endl;
        }
    }; /* end of class hash_map */
} /* end of namespace hash_map */

__STXXL_END_NAMESPACE


namespace std
{
    template <class _Key, class _T, class _Hash, class _KeyCmp, unsigned _SubBlkSize, unsigned _BlkSize, class _Alloc>
    void swap(stxxl::hash_map::hash_map<_Key, _T, _Hash, _KeyCmp, _SubBlkSize, _BlkSize, _Alloc> & a,
              stxxl::hash_map::hash_map<_Key, _T, _Hash, _KeyCmp, _SubBlkSize, _BlkSize, _Alloc> & b)
    {
        if (&a != &b)
            a.swap(b);
    }
}


#endif /* _STXXL_HASH_MAP_H_ */
