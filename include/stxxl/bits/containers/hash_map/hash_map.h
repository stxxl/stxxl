/***************************************************************************
 *            hash_map.h
 *
 *  April 2007, Markus Westphal
 ****************************************************************************/


#ifndef _STXXL_HASH_MAP_H_
#define _STXXL_HASH_MAP_H_


#include <stxxl>

#include "util.h"
#include "block_cache.h"
#include "iterator.h"


__STXXL_BEGIN_NAMESPACE

namespace hash_map {


#pragma mark -
#pragma mark Class HashMap
#pragma mark

//! \brief External memory hash-map
template <	class Key_,
			class T_,
			class Hash_,
			class KeyCmp_,
			unsigned SubBlkSize_ = 4*1024,
			unsigned BlkSize_ = 256,
			class Alloc_ = std::allocator< std::pair<const Key_, T_> >
		>
class hash_map {

private:
	typedef hash_map<Key_, T_, Hash_, KeyCmp_, SubBlkSize_, BlkSize_> self_type;

public:
#pragma mark type declarations
	typedef Key_                key_type;				/* type of the keys being used        */
	typedef T_                  mapped_type;			/* type of the data to be stored      */
	typedef std::pair<Key_, T_> value_type;				/* actually store (key-data)-pairs    */
	typedef value_type &        reference;				/* type for value-references          */
	typedef const value_type &  const_reference;		/* type for constant value-references */
	typedef value_type *        pointer;				/* */
	typedef value_type const *  const_pointer;			/* */

	typedef stxxl::uint64       size_type;				/* */
	typedef stxxl::int64        difference_type;		/* */

	typedef Hash_               hasher;					/* type of (mother) hash-function */
	typedef KeyCmp_             keycmp;					/* functor that imposes a ordering on keys (but see __lt()) */
	
	typedef hash_map_iterator<self_type>       iterator;
	typedef hash_map_const_iterator<self_type> const_iterator;

	enum { block_raw_size = BlkSize_*SubBlkSize_, subblock_raw_size = SubBlkSize_ };	/* subblock- and block-size in bytes */
	enum { block_size = BlkSize_, subblock_size = SubBlkSize_/sizeof(value_type) };		/* Subblock-size as number of elements, block-size as number of subblocks */
	
	typedef typed_block<subblock_raw_size, value_type> subblock_type;	/* a subblock consists of subblock_size values */
	typedef typed_block<block_raw_size, subblock_type> block_type;		/* a block consists of block_size subblocks    */
	
	typedef typename subblock_type::bid_type subblock_bid_type;			/* block-identifier for subblocks */
	typedef typename block_type::bid_type    bid_type;					/* block-identifier for blocks    */
	typedef std::vector<bid_type>            bid_container_type;		/* container for block-bids  */
	typedef typename bid_container_type::iterator bid_iterator_type;	/* iterator for block-bids */

	enum source_type { src_internal, src_external, src_unknown };

	typedef node<value_type> node_type;							/* nodes for internal-memory buffer */
	typedef bucket<node_type> bucket_type;						/* buckets */
	typedef std::vector<bucket_type> buckets_container_type;	/* */

	typedef iterator_map<self_type> iterator_map_type;			/* for tracking active iterators */
	
	typedef block_cache<block_type> block_cache_type;
	
	typedef buffered_reader<block_cache_type, bid_iterator_type> reader_type;
	
	typedef typename Alloc_::template rebind<node_type>::other node_allocator_type;
	
//private:
#pragma mark members
	size_type _num_total;					/* (estimated) number of values */
	bool _oblivious;						/* false if the total-number of values is correct (false) or true if estimated (true); see *_oblivious-methods  */

	float _opt_load_factor;					/* desired load factor after rehashing */

	node_allocator_type _node_allocator;	/* used to allocate new nodes for internal buffer */

	hasher _hash;						/* user supplied mother hash-function */
	keycmp _cmp;						/* user supplied strict-weak-ordering for keys */
  
	buckets_container_type _buckets;	/* array of bucket */
	bid_container_type     _bids;		/* blocks-ids of allocated blocks  */
  
	size_type _buffer_size;				/* size of internal-memory buffer in number of entries */
	size_type _max_buffer_size;			/* maximum size for internal-memory buffer */
	
	iterator_map_type _iterator_map;	/* keeps track of all active iterators */
	
	block_cache_type _block_cache;		/* */

  
public:
#pragma mark construct/destroy/copy
	//! \brief Construct a new hash-map
	//! \param n initial number of buckets
	//! \param hf hash-function
	//! \param cmp comparator-object
	//! \param buffer_size size for internal-memory buffer in bytes
	hash_map(size_type n = 10000, const hasher & hf = hasher(), const keycmp& cmp = keycmp(), size_type buffer_size = 100*1024*1024, const Alloc_& a = Alloc_()) :
		_hash(hf),
		_cmp(cmp),
		_buckets(n),
		_bids(0),
		_buffer_size(0),
		_iterator_map(this),
#ifdef PLAY_WITH_PREFETCHING
		_block_cache(tuning::get_instance()->blockcache_size),
#else
		_block_cache(config::get_instance()->disks_number()*12),
#endif

		_node_allocator(a),
		_oblivious(false),
		_num_total(0),
		_opt_load_factor(0.875)
	{
		_max_buffer_size = buffer_size / sizeof(node_type);
	}
	
	
	//! \brief Construct a new hash-map and insert all values in the range [f,l)
	//! \param f beginning of the range
	//! \param l end of the range
	//! \param mem_to_sort additional memory (in bytes) for bulk-insert
	template <class InputIterator>
	hash_map(InputIterator f, InputIterator l, size_type mem_to_sort = 256*1024*1024, size_type n = 10000, const hasher& hf = hasher(), const keycmp& cmp = keycmp(), size_type buffer_size = 100*1024*1024, const Alloc_& a = Alloc_()) :
		_hash(hf),
		_cmp(cmp),
		_buckets(n),
		_bids(0),
		_buffer_size(0),
		_iterator_map(this),
#ifdef PLAY_WITH_PREFETCHING
		_block_cache(tuning::get_instance()->blockcache_size),
#else
		_block_cache(config::get_instance()->disks_number()*12),
#endif
		_node_allocator(a),
		_oblivious(false),
		_num_total(0),
		_opt_load_factor(0.875)
	{
		 _max_buffer_size = buffer_size / sizeof(node_type);
		 insert(f, l, mem_to_sort);
	}
	
	
	~hash_map()
	{
		clear();
	}
	
	
private:	
	// Copying external hash_maps is discouraged (and not implemented)
	hash_map(const self_type & map)
	{
		STXXL_FORMAT_ERROR_MSG(msg,"stxxl::hash_map copy constructor is not implemented (for good reason)");
		throw std::runtime_error(msg.str());
	}
	
	
public:
#pragma mark observers
	//! \brief Hash-function used by this hash-map
	hasher hash_function() const { return _hash; }
	
	//! \brief Strict-weak-ordering used by this hash-map
	keycmp key_cmp()       const { return _cmp; }


#pragma mark size & capacity
	bool empty() const { return size() != 0; }

private:
	/* After using *_oblivious-methods only an estimate for the total number of elements can be given.
	   This method accesses external memory to calculate the exact number.
	*/
	void __make_conscious()
	{
		if (!_oblivious)
			return;

		typedef HashedValuesStream<self_type, reader_type> values_stream_type;

		reader_type reader(_bids.begin(), _bids.end(), &_block_cache);	//will start prefetching automatically
		values_stream_type values(_buckets.begin(), _buckets.end(), _bids.begin(), reader, this);
		
		_num_total = 0;
		while (!values.empty())
		{
			++_num_total;
			++values;
		}
		_oblivious = false;
	}
	

public:
	//! \brief Number of values currenlty stored. Note: If the correct number is currently unknown (because *_oblivous-methods were used), external memory will be scanned.
	size_type size() const
	{
		if (_oblivious)
		{
			// this method is "conceptually" const in that it doesn't modify the stored values
			// but __make_conscious will chance some members (_oblivious and _num_total), so const-ness must be "casted away"
			self_type * non_const_this = (self_type *)this;
			non_const_this->__make_conscious();
		}
		return _num_total;
	}
	
	//! \brief The hash-map may store up to this number of values
	size_type max_size() const { return std::numeric_limits<size_type>::max(); }
	

#pragma mark modifiers
	//! \brief Insert a new value; also check if already stored in external memory.
	//! \param value what to insert 
	//! \return a tuple: the second value is true iff the value was actually added (no value with the same key present); the first value is an iterator pointing to the newly inserted or already stored value
	std::pair<iterator, bool> insert(const value_type& value)
	{
		size_type i_bucket = __bkt_num(value.first);
		bucket_type &bucket = _buckets[i_bucket];
		node_type* node = __find_key_internal(bucket, value.first);

		// found value in internal memory
		if (node && __eq(node->_value.first, value.first))
		{
			if (node->deleted())
			{
				node->deleted(false);
				node->_value = value;
				_num_total++;
				return std::pair<iterator, bool>(iterator(this, i_bucket, node, 0, src_internal, false, value.first), true);
			}
			else
			{
				return std::pair<iterator, bool>(iterator(this, i_bucket, node, 0, src_internal, false, value.first), false);
			}
		}

		// search external memory ...
		else
		{
			tuple<size_type, value_type> result = __find_key_external(bucket, value.first);
			size_type  i_external = result.first;
			value_type ext_value  = result.second;
			
			// ... if found, return iterator pointing to external position ...
			if (i_external < bucket._num_external && __eq(ext_value.first, value.first) )
			{
				return std::pair<iterator, bool>(iterator(this, i_bucket, node, i_external, src_external, true, value.first), false);
			}
			// ... otherwise add value to internal list
			else
			{
				_num_total++;
				node_type *new_node = (node) ? 
										(node->next(__new_node(value, node->next(), false))) :						// remember: node->_value is the biggest value < new value
										(bucket._internal_list = __new_node(value, bucket._internal_list, false));	// there aro no internal values < new value
			
				iterator it(this, i_bucket, new_node, 0, src_internal, false, value.first);

				_buffer_size++;
				if (_buffer_size >= _max_buffer_size)
					__rebuild_buckets();	// will fix it as well
				
				return std::pair<iterator, bool>(it, true);
			}
		}
	}
	
	
	//! \brief Insert a value; external memory is not accessed so that another value with the same key may be overwritten
	//! \param value what to insert
	//! \return iterator pointing to the inserted value
	iterator insert_oblivious(const value_type& value)
	{
		size_type i_bucket = __bkt_num(value.first);
		bucket_type& bucket = _buckets[i_bucket];
		node_type* node = __find_key_internal(bucket, value.first);
		
		// found value in internal memory
		if (node && __eq(node->_value.first, value.first))
		{
			if (node->deleted())
				_num_total++;

			node->deleted(false);
			node->_value = value;
			return iterator(this, i_bucket, node, 0, src_internal, false, value.first);
		}
		// not found; ignore external memory and add a new node to the internal-memory buffer
		else
		{
			_oblivious = true;
			_num_total++;
			node_type *new_node = (node) ? 
									(node->next(__new_node(value, node->next(), false))) :
									(bucket._internal_list = __new_node(value, bucket._internal_list, false));

			// there may be some iterators that reference the newly inserted value in external memory
			// these need to fixed (make them point to new_node)
			_iterator_map.fix_iterators_2int(i_bucket, value.first, new_node);
			
			iterator it(this, i_bucket, new_node, 0, src_internal, false, value.first);
			
			_buffer_size++;
			if (_buffer_size >= _max_buffer_size)
				__rebuild_buckets();
			
			return it;
		}
	}
	
	
	//! \brief Erase value by iterator
	//! \param it iterator pointing to the value to erase
	void erase(iterator & it)
	{
		_num_total--;
		bucket_type & bucket = _buckets[it._i_bucket];

		if (it._source == src_internal)
		{
			it._node->deleted(true);
			_iterator_map.fix_iterators_2end(it._i_bucket, it._key);	// will fix it as well
		}
		else {
			node_type *node = __find_key_internal(bucket, it._key);		// find biggest value < iterator's value
			assert(!node || !__eq(node->_value.first, it._key));
			
			// add delete-node to buffer
			if (node) node->next(__new_node(value_type(it._key, mapped_type()), node->next(), true));
			else      bucket._internal_list = __new_node(value_type(it._key, mapped_type()), bucket._internal_list, true);

			_iterator_map.fix_iterators_2end(it._i_bucket, it._key);
			
			_buffer_size++;
			if (_buffer_size >= _max_buffer_size)
				__rebuild_buckets();
		}
	}
	
	
	//! \brief Erase value by key; check external memory
	//! \param key key of value to erase
	//! \return number of values actually erased (0 or 1)
	size_type erase(const key_type& key)
	{
		size_type   i_bucket = __bkt_num(key);
		bucket_type & bucket = _buckets[i_bucket];
		node_type   * node   = __find_key_internal(bucket, key);
	
		// found in internal memory
		if (node && __eq(node->_value.first, key))
		{
			if (!node->deleted())
			{
				node->deleted(true);
				_num_total--;
				_iterator_map.fix_iterators_2end(i_bucket, key);
				return 1;
			}
			else return 0;	// already deleted
		}
		// check external memory
		else
		{
			tuple<size_type, value_type> result = __find_key_external(bucket, key);
			size_type  i_external = result.first;
			value_type value      = result.second;
			
			// found in external memory; add delete-node
			if (i_external < bucket._num_external && __eq(value.first, key))
			{
				_num_total--;

				if (node) node->next(__new_node(value_type(key, mapped_type()), node->next(), true));
				else      bucket._internal_list = __new_node(value_type(key, mapped_type()), bucket._internal_list, true);
				
				_iterator_map.fix_iterators_2end(i_bucket, key);
				
				_buffer_size++;
				if (_buffer_size >= _max_buffer_size)
					__rebuild_buckets();
				
				return 1;
			}
			// no value for given key
			else return 0;
		}
	}
	
	
	//! \brief Erase value by key but without looking at external memrory
	//! \param key key for value to release
	void erase_oblivious(const key_type& key)
	{
		size_type i_bucket = __bkt_num(key);
		bucket_type& bucket = _buckets[i_bucket];
		node_type* node = __find_key_internal(bucket, key);
		
		// found value in internal-memory
		if (node && __eq(node->_value.first , key))
		{
			if (!node->deleted())
			{
				_num_total--;
				node->deleted(true);
				_iterator_map.fix_iterators_2end(i_bucket, key);
			}
		}
		// not found; ignore external memory and add delete-node
		else
		{
			_oblivious = true;
			_num_total--;
		
			if (node) node->next(__new_node(value_type(key, mapped_type()), node->next(), true));
			else      bucket._internal_list = __new_node(value_type(key, mapped_type()), bucket._internal_list, true);
			
			_iterator_map.fix_iterators_2end(i_bucket, key);
		
			_buffer_size++;
			if (_buffer_size >= _max_buffer_size)
				__rebuild_buckets();
		}
	}
	
	
	//! \brief Reset hash-map: erase all values, invalidate all iterators
	void clear()
	{
		_iterator_map.fix_iterators_all2end();
		_block_cache.flush();
		_block_cache.clear();
		
		// reset buckets and clear internal memory
		for (size_type i_bucket = 0; i_bucket < _buckets.size(); i_bucket++) {
			__erase_nodes(_buckets[i_bucket]._internal_list, NULL);
			_buckets[i_bucket] = bucket_type(NULL, 0, 0, false, 0, 0);
		}
		_oblivious = false;
		_num_total = 0;
		
		// free external memory
		block_manager *bm = block_manager::get_instance ();
		bm->delete_blocks(_bids.begin(), _bids.end());
		_bids.clear();
	}
	
	
	//! \brief Exchange stored values with another hash-map
	//! \param obj hash-map to swap values with
	void swap(self_type& obj)
	{
		std::swap(_buckets, obj._buckets);
		std::swap(_bids, obj._bids);
		
		std::swap(_oblivious, obj._oblivious);
		std::swap(_num_total, obj._num_total);
		
		std::swap(_node_allocator, obj._node_allocator);
		
		std::swap(_hash, obj._hash);
		std::swap(_cmp, obj._cmp);
		
		std::swap(_buffer_size, obj._buffer_size);
		std::swap(_max_buffer_size, obj._max_buffer_size);
		
		std::swap(_opt_load_factor, obj._opt_load_factor);
		
		std::swap(_iterator_map, obj._iterator_map);
		
		std::swap(_block_cache, obj._block_cache);
	}
	
	
#pragma mark lookup
private:
	// find statistics
	size_type _stats_subblocks_loaded;
	size_type _stats_found_internal;
	size_type _stats_found_external;
	size_type _stats_not_found;
	

public:
	void reset_find_statistics()
	{
		_stats_subblocks_loaded = _stats_found_external = _stats_found_internal = _stats_not_found = 0;
	}
	

	void print_find_statistics(std::ostream & o = std::cout) const
	{
		o << "Found internal    : " << _stats_found_internal << std::endl;
		o << "Found external    : " << _stats_found_external << std::endl;
		o << "Not found         : " << _stats_not_found << std::endl;
		o << "Subblocks accessed: " << _stats_subblocks_loaded << std::endl;
	}
	

	//! \brief Look up value by key. Non-const access.
	//! \param key key for value to look up
	iterator find(const key_type& key) {
		size_type i_bucket = __bkt_num(key);
		bucket_type& bucket = _buckets[i_bucket];
		node_type* node = __find_key_internal(bucket, key);
	
		// found in internal list
		if (node && __eq(node->_value.first, key)) {
			_stats_found_internal++;
			if (node->deleted())
				return this->__end<iterator>();
			else
				return iterator(this, i_bucket, node, 0, src_internal, false, key);
		}
		// search external elements
		else {
			tuple<size_type, value_type> result = __find_key_external(bucket, key);
			size_type  i_external = result.first;
			value_type value = result.second;
		
			// found in external memory
			if (i_external < bucket._num_external && __eq(value.first, key)) {
				_stats_found_external++;
				
				// create new buffer-node (we expect the value to be changed and
				// therefore return a reference to an in-memory node rather than to external-memory)
				node_type *new_node = (node) ? 
										(node->next(__new_node(value, node->next(), false))) :
										(bucket._internal_list = __new_node(value, bucket._internal_list, false));
										
				_iterator_map.fix_iterators_2int(i_bucket, value.first, new_node);
				
				return iterator(this, i_bucket, new_node, i_external+1, src_internal, true, key);
			}
			// not found in external memory
			else {
				_stats_not_found++;
				return this->__end<iterator>();
			}
		}
	}
	
	
	//! \brief Look up value by key. Const access.
	//! \param key key for value to look up	
	const_iterator find(const key_type& key) const {
		size_type i_bucket = __bkt_num(key);
		const bucket_type& bucket = _buckets[i_bucket];
		node_type* node = __find_key_internal(bucket, key);
	
		// found in internal list
		if (node && __eq(node->_value.first, key)) {
			((self_type *)this)->_stats_found_internal++;
			if (node->deleted())
				return this->__end<const_iterator>();
			else
				return const_iterator((self_type *)this, i_bucket, node, 0, src_internal, false, key);
		}
		// search external elements
		else {
			tuple<size_type, value_type> result = __find_key_external(bucket, key);
			size_type  i_external = result.first;
			value_type value = result.second;
		
			// found in external memory
			if (i_external < bucket._num_external && __eq(value.first, key)) {
				((self_type *)this)->_stats_found_external++;
				return const_iterator((self_type *)this, i_bucket, node, i_external, src_external, true, key);
			}
			// not found in external memory
			else {
				((self_type *)this)->_stats_not_found++;
				return this->__end<const_iterator>();
			}
		}
	}

	
	//! \brief Number of values with given key
	//! \param k key for value to look up
	//! \return 0 or 1 depending on the presence of a value with the given key
	size_type count(const key_type& k) const
	{
		const_iterator cit = find(k);
		return (cit == end()) ? 0 : 1;
	}
	
	
	//! \brief Finds a range containing all values with given key. Non-const access
	//! \param key key to look for#
	//! \return range may be empty or contains exactly one value
	std::pair<iterator, iterator> equal_range(const key_type& key)
	{
		iterator it = find(key);
		return std::pair<iterator, iterator>(it, it);
	}
	

	//! \brief Finds a range containing all values with given key. Const access
	//! \param key key to look for#
	//! \return range may be empty or contains exactly one value
	std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const
	{
		const_iterator cit = find(key);
		return std::pair<const_iterator, const_iterator>(cit, cit);
	}
	
	
	//! \brief Convenience operator to quickly insert or find values. Use with caution since using this operator will check external-memory.
	mapped_type & operator[] (const key_type& k) {
		std::pair<iterator, bool> res = insert(value_type(k, mapped_type()));
		return (*(res.first)).second;
	}
	

#pragma mark bucket-interface
	//! \brief Number of buckets
	size_type  bucket_count() const { return _buckets.size(); }
	
	//! \brief Maximum number of buckets
	size_type  max_bucket_count() const { return max_size() / subblock_size; }
	
	//! \brief Bucket-index for values with given key.
	size_type  bucket(const key_type &k) const { return __bkt_num(k); }
	
	
	//! \brief Minimum number of buckets
	size_type  min_bucket_count() const { return 10000; }

	
#pragma mark hash policy
public:
	//! \brief Average number of (sub)blocks occupied by a bucket.
	float load_factor() const { return (float)_num_total / ((float)subblock_size * (float)_buckets.size()); }
	
	
	//! \brief Set desired load-factor
	float opt_load_factor() const { return _opt_load_factor; }
	
	//! \brief Set desired load-factor 
	void  opt_load_factor(float z)
	{ 
		_opt_load_factor = z;
		if (load_factor() > _opt_load_factor)
			__rebuild_buckets();
	}
	
	//! \brief Rehash with (at least) n buckets */
	void  rehash(size_type n = 0)
	{
		__rebuild_buckets(n);
	}
	

#pragma mark buffer policy	
	//! \brief Number of bytes occupied by buffer
	size_type buffer_size()
	{
		return _buffer_size*sizeof(node_type);	// buffer-size internally stored as number of nodes
	}
	
	
	//! \brief Maximum buffer size in byte
	size_type max_buffer_size() { return _max_buffer_size*sizeof(node_type); }
	
	
	//! \brief Set maximum buffer size
	//! \param buffer_size new size in byte
	void max_buffer_size(size_type buffer_size) 
	{
		_max_buffer_size = buffer_size / sizeof(node_type);
		if (_buffer_size >= _max_buffer_size)
			__rebuild_buckets();
	}
	
	
#pragma mark Sequentially access to values
private:
	/* iterator pointing to the beginnning of the hash-map */
	template <class _ItType>
	_ItType __begin() const {
		self_type *non_const_this = (self_type *)this;
		
		_ItType it(non_const_this, 0, _buckets[0]._internal_list, 0, src_unknown, true, key_type());	// correct key will be set by find_next()
		it.find_next();
		
		return it;
	}
	

	/* iterator pointing to the end of the hash-map (iterator-type as template-parameter) */
	template <class _ItType>
	_ItType __end() const {
		self_type *non_const_this = (self_type *)this;
		return _ItType(non_const_this);
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


#pragma mark Node handling
private:
	/* Allocate a new buffer-node */
	node_type* __get_node()
	{
		return _node_allocator.allocate(1);
	}
	

	/* Free given node */
	void __put_node(node_type* node)
	{
		_node_allocator.deallocate(node, 1);
	}
	

	/* Allocate a new buffer-node and initialize with given value, node and deleted-flag */
	node_type* __new_node(const value_type& value, node_type* nxt, bool del)
	{
		node_type* node = __get_node();
		node->_value = value;
		node->next(nxt);
		node->deleted(del);
		return node;
	}
	
	
	/* Free nodes in range [first, last). If last is NULL all nodes will be freed. */
	void __erase_nodes(node_type *first, node_type *last)
	{
		node_type *curr = first;
		while (curr != last)
		{
			node_type *next = curr->next();
			__put_node(curr);
			curr = next;
		}
	}
	

#pragma mark Uniform hashing
	/* Bucket-index for values with given key */
	size_type __bkt_num(const key_type& key) const
	{
		return __bkt_num(key, _buckets.size());
	}
	
	
	/* Bucket-index for values with given key. The total number of buckets has to be specified as well.
	   TODO: find more clever way to calculate, let user supply max-hash-value?
	*/
	size_type __bkt_num(const key_type &key, size_type n) const
	{
		return (size_type)((double)n * ((double)_hash(key)/(double)std::numeric_limits<size_type>::max()));
	}


#pragma mark Searching keys
	/* Locate the given key in the internal chained list.
	   If the key is not present, the node with the biggest key smaller than the given key is returned.
	   Note that the returned value may be zero: either because the chained list is empty or because
	   the given key is smaller than all other keys in the chained list.
	*/
	node_type* __find_key_internal(const bucket_type& bucket, const key_type& key) const
	{
		node_type *curr = bucket._internal_list;
		node_type *old = 0;
		for (; curr && __leq(curr->_value.first, key); curr = curr->next()) {
			old = curr;
		}
		
		return old;
	}


	/* Search for key in external part of bucket. Return value is (i_external, value),
	   where i_ext = bucket._num_external if key could not be found.
	*/
	tuple<size_type, value_type> __find_key_external(const bucket_type& bucket, const key_type& key) const
	{
		subblock_type *subblock;
	
		// number of subblocks occupied by bucket
		size_type num_subblocks = bucket._num_external / subblock_size;
		if (bucket._num_external % subblock_size != 0)
			num_subblocks++;
		
		for (size_type i_subblock = 0; i_subblock < num_subblocks; i_subblock++)
		{
			subblock = __load_subblock(bucket, i_subblock);
			// number of values in i-th subblock
			size_type num_values = (i_subblock+1 < num_subblocks) ? subblock_size : (bucket._num_external-i_subblock*subblock_size);

			// biggest key in current subblock still too small => next subblock
			if (__lt((*subblock)[num_values-1].first, key))
				continue;
			
			// binary search in current subblock
			size_type i_lower = 0, i_upper = num_values;
			while (i_lower + 1 != i_upper)
			{
				size_type i_middle = (i_lower + i_upper) / 2;
				if (__leq((*subblock)[i_middle].first, key))
					i_lower = i_middle;
				else
					i_upper = i_middle;
			}

			value_type value= (*subblock)[i_lower];
	
			if (__eq(value.first, key))
				return tuple<size_type, value_type>(i_subblock*subblock_size+i_lower, value);
			else
				return tuple<size_type, value_type>(bucket._num_external, value_type());
		}
		
		return tuple<size_type, value_type>(bucket._num_external, value_type());
	}


	/* Load the given bucket's i-th subblock.
	   Since a bucket may be spread over several blocks, we must
	   1. determine in which block the requested subblock is located
	   2. at which position within the obove-mentioned block the questioned subblock is located 
	*/
	subblock_type * __load_subblock(const bucket_type& bucket, size_type i_subblock) const
	{
		((self_type *)this)->_stats_subblocks_loaded++;
	
		// index of the requested subblock counted from the very beginning of the bucket's first block
		size_type i_abs_subblock = bucket._i_first_subblock + i_subblock;
		
		size_type i_block = i_abs_subblock / block_size + bucket._i_block;	/* 1. */
		size_type i_subblock_within = i_abs_subblock % block_size;			/* 2. */
	
		bid_type bid = _bids[i_block];
		return ((self_type *)this)->_block_cache.get_subblock(bid, i_subblock_within);
	}


#pragma mark Rebuilding/Rehashing
	typedef HashedValue<self_type> hashed_value_type;

	/* Functor to extracts the actual value from a HashedValue-struct */
	struct HashedValueExtractor
	{
		value_type & operator() (hashed_value_type & hvalue) { return hvalue._value; }
	};


	/* Will return from its input-stream all values that are to be stored in the given bucket.
	   Those values must appear in consecutive order beginning with the input-stream's current value.
	*/
	template <class _InputStream, class _ValueExtractor>
	struct HashingStream
	{
		typedef typename _InputStream::value_type value_type;
		
		self_type *_map;
		_InputStream & _input;
		size_type _i_bucket;
		size_type _bucket_size;
		value_type _value;
		bool _empty;
		_ValueExtractor _vextract;
		
		HashingStream (_InputStream & input, size_type i_bucket, _ValueExtractor vextract, self_type *map) :
			_map(map),
			_vextract(vextract),
			_input(input),
			_i_bucket(i_bucket),
			_bucket_size(0),
			_empty(find_next())
		{ }
		
		const value_type & operator *() { return _value; }

		bool empty() const { return _empty; }
			
		void operator ++ ()
		{
			++_input;
			_empty = find_next();
		}

		bool find_next()
		{
			if (_input.empty())
				return true;
			_value = *_input;
			if (_map->__bkt_num(_vextract(_value).first) != _i_bucket)
				return true;

			_bucket_size++;
			return false;
		}
	};
	
	
	/*	Rebuild hash-map. The desired number of buckets may be supplied. */
	void __rebuild_buckets(size_type n_desired = 0)
	{
		typedef my_buffered_writer<block_type, bid_container_type> writer_type;
		typedef HashedValuesStream<self_type, reader_type> values_stream_type;
		typedef HashingStream<values_stream_type, HashedValueExtractor> hashing_stream_type;

		const int_type write_buffer_size = config::get_instance()->disks_number()*4;

		// determine new number of buckets from desired load_factor ...
		size_type n_new;
		n_new = (size_type)ceil((double)_num_total / ((double)subblock_size * (double)opt_load_factor()));
		
		// ... but give the user the chance to request even more buckets
		if (n_desired > n_new)
			n_new = std::min<size_type>(n_desired, max_bucket_count());

		// allocate new buckets and bids
		buckets_container_type *old_buckets = new buckets_container_type(n_new);
		bid_container_type *old_bids = new bid_container_type();	// writer will allocate new blocks as necessary

		std::swap(_buckets, *old_buckets);
		std::swap(_bids, *old_bids);

		// read stored values in consecutive order
		reader_type * reader = new reader_type(old_bids->begin(), old_bids->end(), &_block_cache);
		values_stream_type values_stream(old_buckets->begin(), old_buckets->end(), old_bids->begin(), *reader, this);
		
		writer_type writer(&_bids, write_buffer_size, write_buffer_size/2);
		
		// re-distribute values among new buckets.
		// this makes use of the fact that if value1 preceeds value2 before resizing, value1 will preceed value2 after resizing as well (uniform rehashing)
		_num_total = 0;
		for (size_type i_bucket = 0; i_bucket < _buckets.size(); i_bucket++)
		{
			_buckets[i_bucket] = bucket_type(NULL, 0, 0, false, writer.curr_i_block(), writer.curr_i_subblock());
			
			hashing_stream_type hasher(values_stream, i_bucket, HashedValueExtractor(), this);	// gives all values for current bucket
			size_type i_ext = 0;
			while (!hasher.empty())
			{
				const hashed_value_type & hvalue = *hasher;
				_iterator_map.fix_iterators_2ext(hvalue._i_bucket, hvalue._value.first, i_bucket, i_ext);
				
				writer.append(hvalue._value);
				++hasher;
				++i_ext;
			}
			
			writer.finish_subblock();
			_buckets[i_bucket]._num_external = hasher._bucket_size;
			_num_total += hasher._bucket_size;
		}
		writer.flush();
		_block_cache.clear();
		
		delete reader;	// reader must be deleted before deleting old_bids (its destructor will dereference the bid-iterator)
		
		// get rid of old blocks and buckets
		block_manager *bm = stxxl::block_manager::get_instance();
		bm->delete_blocks(old_bids->begin(), old_bids->end());
		delete old_bids;
		delete old_buckets;

		_buffer_size = 0;
		_oblivious = false;
	}


#pragma mark bulk insert
	/* Stream for filtering duplicates. Used to eliminate duplicated values when bulk-inserting
	   Note: input has to be sorted, so that duplicates will occure in row
	*/
	template <class  InputStream>
	struct UniqueValueStream
	{
		typedef typename InputStream::value_type value_type;
		self_type *_map;
		InputStream  & _in;
		
		UniqueValueStream(InputStream & input, self_type *map) : _map(map), _in(input) { }
		
		bool empty() const { return _in.empty(); }
	
		value_type operator * () { return *_in; }
		
		void operator++ ()
		{
			value_type v_old = *_in;
			++_in;
			while (!_in.empty() && _map->__eq(v_old.first, (*_in).first))
				++_in;
		}
	};
	
	template <class InputStream>
	struct AddHashStream
	{
		typedef std::pair<size_type, typename InputStream::value_type> value_type;
		self_type *_map;
		InputStream &_in;
		
		AddHashStream(InputStream &input, self_type *map) : _map(map), _in(input) { }
		
		bool empty() const { return _in.empty(); }
		value_type operator * () { return value_type(_map->_hash((*_in).first), *_in); }
		void operator++() { ++_in; }
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
		self_type * _map;
		Cmp(self_type *map) : _map(map) { }
		
		bool operator () (const std::pair<size_type, value_type> & a, const std::pair<size_type, value_type> & b) const
		{
			return (a.first < b.first) ||
			       ((a.first == b.first) && _map->_cmp(a.second.first, b.second.first));
		}
		std::pair<size_type, value_type> min_value() const { return std::pair<size_type, value_type>(std::numeric_limits<size_type>::min(), value_type(_map->_cmp.min_value(), mapped_type())); }
		std::pair<size_type, value_type> max_value() const { return std::pair<size_type, value_type>(std::numeric_limits<size_type>::max(), value_type(_map->_cmp.max_value(), mapped_type())); }
	};

public:
	//! \brief Bulk-insert of values in the range [f, l)
	//! \param f beginning of the range
	//! \param l end of the range
	//! \param mem internal memory that may be used (note: this memory will be used additionally to the buffer). The more the better
	template <class InputIterator>
	void insert(InputIterator f, InputIterator l, size_type mem)
	{
		typedef HashedValuesStream<self_type, reader_type>             old_values_stream;		// values already stored in the hashtable ("old values")
		typedef HashingStream<old_values_stream, HashedValueExtractor> old_hashing_stream;		// old values, that are to be stored in a certain (new) bucket
		
		typedef typeof(stxxl::stream::streamify(f,l))              input_stream;				// values to insert ("new values")
		typedef AddHashStream<input_stream>                        new_values_stream;			// new values with added hash: (hash, (key, mapped))
		typedef stxxl::stream::sort<new_values_stream, Cmp>        new_sorted_values_stream;	// new values sorted by <hash-value, key>
		typedef UniqueValueStream<new_sorted_values_stream>        new_unique_values_stream;	// new values sorted by <hash-value, key> with duplicates eliminated
		typedef HashingStream<new_unique_values_stream, StripHashFunctor> new_hashing_stream;		// new values, that are to be stored in a certain bucket

		typedef my_buffered_writer<block_type, bid_container_type> writer_type;

		int_type write_buffer_size = config::get_instance()->disks_number()*2;
		
		
		// calculate new number of buckets
		size_type num_total_new = _num_total + (l-f);	// estimated number of elements
		size_type n_new = (size_type)ceil((double)num_total_new / ((double)subblock_size * (double)opt_load_factor()));
		if (n_new > max_bucket_count())
			n_new = max_bucket_count();
		else if (n_new < min_bucket_count())
			n_new = min_bucket_count();
		
		// prepare new buckets and bids
		buckets_container_type *old_buckets = new buckets_container_type(n_new);
		bid_container_type *old_bids = new bid_container_type();	// writer will allocate new blocks as necessary

		std::swap(_buckets, *old_buckets);
		std::swap(_bids, *old_bids);

		// already stored values ("old values")
		reader_type       reader(old_bids->begin(), old_bids->end(), &_block_cache);
		old_values_stream old_values(old_buckets->begin(), old_buckets->end(), old_bids->begin(), reader, this);
		
		// values to insert ("new values")
		input_stream              input = stxxl::stream::streamify(f, l);
		new_values_stream         new_values(input, this);
		new_sorted_values_stream  new_sorted_values(new_values, Cmp(this), mem);
		new_unique_values_stream  new_unique_values(new_sorted_values, this);
		
		writer_type writer(&_bids, write_buffer_size, write_buffer_size/2);
		
		_num_total = 0;
		for (size_type i_bucket = 0; i_bucket < _buckets.size(); i_bucket++)
		{
			_buckets[i_bucket] = bucket_type(NULL, 0, 0, false, writer.curr_i_block(), writer.curr_i_subblock());
			
			old_hashing_stream old_hasher(old_values, i_bucket, HashedValueExtractor(), this);
			new_hashing_stream new_hasher(new_unique_values, i_bucket, StripHashFunctor(), this);
			size_type bucket_size = 0;
			
			// more old and new values for the current bucket => choose smallest
			while (!old_hasher.empty() && !new_hasher.empty())
			{
				size_type old_hash = _hash((*old_hasher)._value.first);
				size_type new_hash = (*new_hasher).first;
				key_type old_key = (*old_hasher)._value.first;
				key_type new_key = (*new_hasher).second.first;
				
				// old value wins
				if ((old_hash < new_hash) ||(old_hash == new_hash && _cmp(old_key, new_key))) // (__lt((*old_hasher)._value.first, (*new_hasher).second.first))
				{
					const hashed_value_type & hvalue = *old_hasher;
					_iterator_map.fix_iterators_2ext(hvalue._i_bucket, hvalue._value.first, i_bucket, bucket_size);
					writer.append(hvalue._value);
					++old_hasher;
				}
				// new value smaller or equal => new value wins
				else
				{
					if (__eq(old_key, new_key))
					{
						const hashed_value_type & hvalue = *old_hasher;
						_iterator_map.fix_iterators_2ext(hvalue._i_bucket, hvalue._value.first, i_bucket, bucket_size);
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
				_iterator_map.fix_iterators_2ext(hvalue._i_bucket, hvalue._value.first, i_bucket, bucket_size);
				writer.append(hvalue._value);
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
			_buckets[i_bucket]._num_external = bucket_size;
			_num_total += bucket_size;
		}
		writer.flush();
		
		block_manager *bm = stxxl::block_manager::get_instance();
		bm->delete_blocks(old_bids->begin(), old_bids->end());
		delete old_bids;
		delete old_buckets;
		
		_buffer_size = 0;
		_oblivious = false;
	}


#pragma mark key ordering
//private:
	/* 1 iff a <  b
	   The comparison is done lexicographically by (hash-value, key)
	*/
	bool __lt (const key_type & a, const key_type & b) const
	{
		size_type hash_a = _hash(a);
		size_type hash_b = _hash(b);

		return  (hash_a < hash_b) ||
		       ((hash_a == hash_b) && _cmp(a,b));
	}
	
	bool __gt (const key_type & a, const key_type & b) const { return __lt(b,a);  }	/* 1 iff a >  b */		
	bool __leq(const key_type & a, const key_type & b) const { return !__gt(a,b); } /* 1 iff a <= b */
	bool __geq(const key_type & a, const key_type & b) const { return !__lt(a,b); }	/* 1 iff a >= b */

	/* 1 iff a == b. note: it is mandatory that equal keys yield equal hash-values => hashing not neccessary for equality-testing */
	bool __eq (const key_type & a, const key_type & b) const { return !_cmp(a,b) && !_cmp(b,a); }

	
#pragma mark Friends & testing
	friend class hash_map_iterator_base<self_type>;	
	friend class hash_map_iterator<self_type>;
	friend class hash_map_const_iterator<self_type>;
	friend class iterator_map<self_type>;
	friend class block_cache<block_type>;
	friend struct HashedValuesStream<self_type, reader_type>;


#pragma statistics
	void __dump_external() {
		reader_type reader(_bids.begin(), _bids.end(), &_block_cache);
	
		for (size_type i_block = 0; i_block < _bids.size(); i_block++) {
			std::cout << "block " << i_block << ":\n";
			
			for (size_type i_subblock = 0; i_subblock < block_size; i_subblock++) {
				std::cout << "  subblock " << i_subblock <<":\n    ";
				
				for (size_type i_element = 0; i_element < subblock_size; i_element++) {
					std::cout << reader.const_value().first << ", ";
					++reader;
				}
				std::cout << std::endl;
			}
		}
	}
	
	void __dump_buckets() {
		reader_type reader(_bids.begin(), _bids.end(), &_block_cache);

		std::cout << "number of buckets: " << _buckets.size() << std::endl;
		for (size_type i_bucket = 0; i_bucket < _buckets.size(); i_bucket++) {
			const bucket_type & bucket = _buckets[i_bucket];
			reader.skip_to(_bids.begin()+bucket._i_block, bucket._i_first_subblock);
			
			std::cout << "  bucket " << i_bucket << ": block=" << bucket._i_block << ", subblock=" << bucket._i_first_subblock << ", external=" << bucket._num_external << std::endl;
					  
			node_type *node = bucket._internal_list;
			std::cout << "     internal_list=";
			while (node) {
				std::cout << node->_value.first << " (del=" << node->deleted() <<"), ";
				node = node->next();
			}
			std::cout << std::endl;
			
			std::cout << "     external=";
			for (size_type i_element = 0; i_element < bucket._num_external; i_element++) {
				std::cout << reader.const_value().first << ", ";
				++reader;
			}
			std::cout << std::endl;
		}
	}
	
	void __dump_bucket_statistics() {
		std::cout << "number of buckets: " << _buckets.size() << std::endl;
		for (size_type i_bucket = 0; i_bucket < _buckets.size(); i_bucket++) {
			const bucket_type & bucket = _buckets[i_bucket];
			std::cout << "  bucket " << i_bucket << ": block=" << bucket._i_block << ", subblock=" << bucket._i_first_subblock << ", external=" << bucket._num_external << std::endl;
		}
	}

public:
	//! \brief Print some statistics: Number of buckets, number of values, buffer-size, values per bucket
	void print_load_statistics() {
		size_type sum_external = 0;
		size_type square_sum_external = 0;
		size_type max_external = 0;

		
		for (size_type i_bucket = 0; i_bucket < _buckets.size(); i_bucket++) {
			const bucket_type & b = _buckets[i_bucket];
			
			sum_external += b._num_external;
			square_sum_external += b._num_external * b._num_external;
			if (b._num_external > max_external)
				max_external = b._num_external;
		}
	
		double avg_external = (double)sum_external / (double)_buckets.size();
		double std_external = sqrt(((double)square_sum_external / (double)_buckets.size()) - (avg_external*avg_external));

		STXXL_MSG("Buckets count        : " << _buckets.size())
		STXXL_MSG("Values total         : " << _num_total)
		STXXL_MSG("Values buffered      : " << _buffer_size)
		STXXL_MSG("Max Buffer-Size      : " << _max_buffer_size)
		STXXL_MSG("Max external/bucket  : " << max_external)
		STXXL_MSG("Avg external/bucket  : " << avg_external)
		STXXL_MSG("Std external/bucket  : " << std_external)
		STXXL_MSG("Load-factor          : " << load_factor())
		STXXL_MSG("Blocks allocated     : " << _bids.size() << " => " << (_bids.size()*block_type::raw_size) << " bytes")
		STXXL_MSG("Bytes per value      : " << ((double)(_bids.size()*block_type::raw_size) / (double)_num_total))
	}

}; /* end of class hash_map */


} /* end of namespace hash_map */

__STXXL_END_NAMESPACE


namespace std
{
	template <	class _Key, class _T, class _Hash, class _KeyCmp, unsigned _SubBlkSize, unsigned _BlkSize,  class _Alloc >
	void swap(stxxl::hash_map::hash_map<_Key, _T, _Hash, _KeyCmp, _SubBlkSize, _BlkSize,  _Alloc> & a, 
				   stxxl::hash_map::hash_map<_Key, _T, _Hash, _KeyCmp, _SubBlkSize, _BlkSize,  _Alloc> & b)
	{
		if(&a != &b) a.swap(b);
	}
}


#endif /* _STXXL_HASH_MAP_H_ */
