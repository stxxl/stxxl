/***************************************************************************
 *			  util.h
 *
 *	May 2007, Markus Westphal
 ****************************************************************************/


#ifndef _STXXL_HASH_MAP_UTIL_H_
#define _STXXL_HASH_MAP_UTIL_H_

#include <stxxl>
#include "tuning.h"


__STXXL_BEGIN_NAMESPACE

namespace hash_map
{


#pragma mark -
#pragma mark Structs
#pragma mark

// For internal memory chaining
// next-pointer and delete-flag share the same memory: the lowest bit is occupied by the del-flag.
template <class Value_>
struct node
{
	node<Value_>* _next_and_del;
	Value_ _value;
  
	bool deleted() { return ((int_type)_next_and_del & 0x01) == 1; }

	bool deleted(bool d)
	{
		_next_and_del = (node<Value_> *)(((int_type)_next_and_del & ~0x01) | (int_type)d);
		return d;
	}

	node<Value_> * next() { return (node<Value_> *)((int_type)_next_and_del & ~0x01); }
	
	node<Value_> * next(node<Value_> * n)
	{
		_next_and_del = (node<Value_> *)(((int_type)_next_and_del & 0x01) | (int_type)n);
		return n;
	}
};


template <class Node_>
struct bucket
{
	Node_* _internal_list;		/* entry point to the chain in internal memory */
	
	size_t _num_external;		/* number of elements in external memory */
	
	size_t _i_block;			/* index for map's bid-array; points to the first block's bid */
	size_t _i_first_subblock;	/* index of first subblock */

	
	bucket() :
		_internal_list(NULL),
		_num_external(0),
		_i_block(0),
		_i_first_subblock(0)
	{ }
	
	bucket(Node_ * internal_list, size_t num_ext, size_t num_total, bool obl, size_t i_block, size_t i_first_subblock) :
		_internal_list(internal_list),
		_num_external(num_ext),
		_i_block(i_block),
		_i_first_subblock(i_first_subblock)
	{ }
};



#pragma mark -
#pragma mark Class buffered_reader
#pragma mark

//! \brief Used to scan external memory with prefetching.
template <class Cache_, class BidIt_>
class buffered_reader
{
public:
#pragma mark Type Definitions
	typedef Cache_ cache_type;
	
	typedef typename cache_type::block_type		block_type;
	typedef typename block_type::value_type		subblock_type;
	typedef typename subblock_type::value_type	value_type;
	
	typedef BidIt_ bid_iterator_type;
	typedef typename bid_iterator_type::value_type bid_type;

	enum { block_size = block_type::size, subblock_size = subblock_type::size };		

private:
#pragma mark Members
	size_t			  _i_value;		/* index within current block	*/
	bid_iterator_type _curr_bid;	/* bid of current block			*/
	bid_iterator_type _last_bid;	/* read blocks up to _last_bid	*/
	bid_iterator_type _pref_bid;	/* bid of next block to prefetch */

	cache_type * _cache;			/* shared block-cache  */
	
	bool   _prefetch;				/* true if prefetching enabled	 */
	size_t _n_blocks_consumed;		/* number of blocks already read */
	size_t _page_size;				/* a page consists of this many blocks and is read at once from disk */
	size_t _prefetch_pages;			/* number of pages to prefetch	 */
	
	bool			_dirty;			/* current block set dirty ? */
	subblock_type * _subblock;		/* current subblock		 */

#pragma mark Methods
public:
	//! \brief Create a new buffered reader to read the blocks in [seq_begin, seq_end)
	//! \param seq_begin First block's bid
	//! \param seq_end Last block's bid
	//! \param cache Block-cache used for prefetching
	//! \param prefetch Enable/Disable prefetching
	buffered_reader(bid_iterator_type seq_begin, bid_iterator_type seq_end, cache_type *cache, size_t i_subblock = 0, bool prefetch = true) :
		_i_value (0),
		_curr_bid (seq_begin),
		_last_bid (seq_end),
		_cache(cache),
#ifdef PLAY_WITH_PREFETCHING
		_page_size(tuning::get_instance()->prefetch_page_size),
		_prefetch_pages(tuning::get_instance()->prefetch_pages),
#else
		_page_size(config::get_instance()->disks_number()*2),
		_prefetch_pages(2),
#endif	
		_n_blocks_consumed(0),
		_prefetch(false),
		_dirty(false)
	{
		if (seq_begin == seq_end)
			return;

		if (prefetch)
			enable_prefetching();
			
		skip_to(seq_begin, i_subblock);
	}
	
	~buffered_reader() {
		if (_curr_bid != _last_bid)
			_cache->release_block(*_curr_bid);
	}
	
	
	//! \brief If not yet enabled, prefetching is started when calling this method.
	void enable_prefetching()
	{
		if (_prefetch) return;
		
		_prefetch = true;
		_pref_bid = _curr_bid;
		for (size_t i = 0; i < _page_size*_prefetch_pages; i++)
		{
			if (_pref_bid == _last_bid)
				break;
			
			_cache->prefetch_block(*_pref_bid);
			++_pref_bid;
		}
	}
	
	
	//! \brief Get const-reference to current value.
	const value_type & const_value() { return (*_subblock)[_i_value % subblock_size]; }

	
	//! \brief Get reference to current value. The current value's block's dirty flag will be set.
	value_type & value()
	{
		if (!_dirty) {
			_cache->make_dirty(*_curr_bid);
			_dirty = true;
		}
		
		return (*_subblock)[_i_value % subblock_size];
	}
	
	
	//! \brief Advance to the next value
	//! \return false if last value has been reached, otherwise true.
	bool operator++()
	{
		if (_curr_bid == _last_bid)
			return false;

		// same block
		if (_i_value+1 < block_size*subblock_size) {
			_i_value++;
		}
		// entered new block
		else
		{
			_cache->release_block(*_curr_bid);
		
			_i_value = 0;
			_dirty = false;
			++_curr_bid;
			
			if (_curr_bid == _last_bid)
				return false;
			
			_cache->retain_block(*_curr_bid);
				
			_n_blocks_consumed++;
			if (_prefetch && _n_blocks_consumed % _page_size == 0)
			{
				for (unsigned i = 0; i < _page_size; i++)
				{
					if (_pref_bid == _last_bid)
						break;
					_cache->prefetch_block(*_pref_bid);
					++_pref_bid;
				}
			}

		}
		
		// entered new subblock	(note: this also applies if we entered a new block)
		if (_i_value % subblock_size == 0) {
			_subblock = _cache->get_subblock(*_curr_bid, _i_value / subblock_size);
		}
		
		return true;
	}

	
	//! \brief Skip remaining values of the current subblock. Postcondition: curr_i_subblock = curr_i_subblock@pre +1
	void next_subblock()
	{
		_i_value = (curr_i_subblock()+1)*subblock_size - 1;
		operator++();
	}
	
	
	//! \brief Continue reading at given block and subblock.
	void skip_to(bid_iterator_type bid, size_t i_subblock)
	{
		if (bid != _curr_bid)
			_dirty = false;
		
		// skip to block
		_cache->release_block(*_curr_bid);
		while (_curr_bid != bid) {
			++_curr_bid;
			++_n_blocks_consumed;
		
			if (_prefetch && _n_blocks_consumed % _page_size == 0) {
				for (unsigned i = 0; i < _page_size; i++)
				{
					if (_pref_bid == _last_bid)
						break;
					_cache->prefetch_block(*_pref_bid);
					++_pref_bid;
				}
			} 
		}
		// skip to subblock
		_i_value = i_subblock*subblock_size;
		_subblock = _cache->get_subblock(*_curr_bid, i_subblock);
		_cache->retain_block(*_curr_bid);
	}

	
	//! \brief Index of current subblock.
	size_t curr_i_subblock() { return _i_value / subblock_size; }
	
	//! \brief Index of current value within current subblock.
	size_t curr_i_value() { return _i_value % subblock_size; }
};




#pragma mark -
#pragma mark Class element_writer
#pragma mark

//! \brief Buffered writing of values. New Blocks are allocated as needed.
template <class _BlkType, class _BidContainer>
class my_buffered_writer
{
#pragma mark Type Definitions
public:
	typedef _BlkType block_type;
	typedef typename block_type::value_type subblock_type;
	typedef typename subblock_type::value_type value_type;

	typedef _BidContainer bid_container_type;
	
	typedef buffered_writer<block_type> writer_type;

	enum { block_size = block_type::size, subblock_size = subblock_type::size };		

#pragma mark Members
private:
	writer_type *_writer;		/* buffered writer */
	block_type *_block;			/* current buffer-block */
	
	bid_container_type *_bids_container;	/* append bids of allocated blocks to this one */
	
	size_t _i_block;			/* index of current block */
	size_t _i_value;			/* index of current value within the current block */
	size_t _increase;			/* number of blocks newly allocated */
	

#pragma mark Methods
public:
	//! \brief Create a new buffered writer.
	//! \param c Bids of newly allocated blocks are appened to this container
	//! \param buffer_size Number of write buffers to use
	//! \param batch_size Number of blocks to accumulate in order to flush write requests (bulk buffered writing)
	my_buffered_writer(bid_container_type *c, int_type buffer_size, int_type batch_size) :
		_bids_container(c),
		_increase(config::get_instance()->disks_number()*3),
		_i_value(0),
		_i_block(0)
	{
		_writer = new writer_type(buffer_size, batch_size);
		_block = _writer->get_free_block();
	}
	
	~my_buffered_writer()
	{
		flush();
		delete _writer;
	}
	
	
	//! \brief Write all values from given stream.
	template <class Stream_>
	void append_from_stream(Stream_ & stream)
	{
		while (!stream.empty())
		{
			append(*stream);
			++stream;
		}
	}
	
	
	//! \brief Write given value.
	void append(const value_type &value)
	{
		size_t i_subblock = (_i_value / subblock_size);
		(*_block)[i_subblock][_i_value % subblock_size] = value;
		
		if (_i_value+1 < block_size*subblock_size)
		{
			_i_value++;
		}
		// reached end of a block
		else
		{
			_i_value = 0;
			
			// allocate new blocks if neccessary ...
			if (_i_block == _bids_container->size())
			{
				_bids_container->resize(_bids_container->size() + _increase);
				block_manager *bm = stxxl::block_manager::get_instance ();
				bm->new_blocks (striping (), _bids_container->end()-_increase, _bids_container->end ());
			}
			// ... and write current block
			_block = _writer->write(_block, (*_bids_container)[_i_block]);
			_i_block++;
		}
	}
	
	
	//! \brief Continue writing at the beginning of the next subblock.
	void finish_subblock()
	{
		while (_i_value % subblock_size != 0)
			append(value_type());
	}
	
	
	//! \brief Flushes not yet written blocks.
	void flush()
	{
		_i_value = 0;
		if (_i_block == _bids_container->size())
		{
			_bids_container->resize(_bids_container->size() + _increase);
			block_manager *bm = stxxl::block_manager::get_instance ();
			bm->new_blocks (striping (), _bids_container->end()-_increase, _bids_container->end ());
		}
		_block = _writer->write(_block, (*_bids_container)[_i_block]);
		_i_block++;
		
		_writer->flush();
	}
	
	
	//! \brief Index of current block.
	size_t curr_i_block() { return _i_block; }
	
	//! \brief Index of current subblock.
	size_t curr_i_subblock() { return _i_value / subblock_size; }
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
template <class HashMap_>
struct HashedValue
{
	typedef HashMap_ hash_map_type;
	typedef typename hash_map_type::value_type	value_type;
	typedef typename hash_map_type::size_type	size_type;
	typedef typename hash_map_type::source_type source_type;
	typedef typename hash_map_type::node_type	node_type;

	value_type _value;
	size_type _i_bucket;
	size_type _i_external;
	node_type *_node;
	source_type _source;
	
	HashedValue() { }
	
	HashedValue(const value_type & value, size_type i_bucket, source_type src, node_type *node, size_type i_external) :
		_value(value),
		_i_bucket(i_bucket),
		_source(src),
		_node(node),
		_i_external(i_external)
	{ }
};


/** Stream interface for all value-pairs currently stored in the map. Returned values are HashedValue-objects
	(actual value enriched with information on where the value can be found (bucket-number, internal, external)).
	Values, marked as deleted in internal-memory, are not returned; for modified values only the one in internal memory is returned.
*/
template <class HashMap_, class Reader_>
struct HashedValuesStream
{
	typedef HashMap_ hash_map_type;
	typedef HashedValue<HashMap_> value_type;

	typedef typename hash_map_type::size_type size_type;
	typedef typename hash_map_type::node_type node_type;
	typedef typename hash_map_type::bid_container_type::iterator bid_iterator;
	typedef typename hash_map_type::buckets_container_type::iterator bucket_iterator;
	
	hash_map_type * _map;
	Reader_ &		_reader;
	bucket_iterator _curr_bucket;
	bucket_iterator _last_bucket;
	bid_iterator	_first_bid;
	size_type		_i_bucket;
	size_type		_i_external;
	node_type *		_node;
	value_type		_value;


	HashedValuesStream(bucket_iterator first_bucket, bucket_iterator last_bucket, bid_iterator first_bid, Reader_ & reader, hash_map_type *map) :
		_map(map),
		_reader(reader),
		_curr_bucket(first_bucket),
		_last_bucket(last_bucket),
		_first_bid(first_bid),
		_i_bucket(0),
		_node((*_curr_bucket)._internal_list),
		_i_external(0)
	{
		find_next();
	}

	const value_type & operator *() { return _value; }
	
	bool empty() const { return _curr_bucket == _last_bucket; }
	
	
	void operator++()
	{
		if (_value._source == hash_map_type::src_internal)
			_node = _node->next();
		else
		{
			++_reader;
			++_i_external;
		}
		find_next();
	}


	bool __curr_bucket_finished() const { return (_node == NULL) && (_i_external >= (*_curr_bucket)._num_external); }
	
	
	void find_next()
	{
		while (true)
		{
			// internal and external elements available
			while (_node && _i_external < (*_curr_bucket)._num_external)	
			{
				if (_map->__eq(_node->_value.first, _reader.const_value().first))	// internal == external => internal wins
				{
					++_reader;
					++_i_external;
					if (!_node->deleted())
					{
						_value = value_type(_node->_value, _i_bucket, hash_map_type::src_internal, _node, _i_external);
						return;
					}
					else
						_node = _node->next();	// value deleted => continue search
				}
				else if (_map->__lt(_node->_value.first, _reader.const_value().first))	// internal < external => internal wins
				{
					if (!_node->deleted())
					{
						_value = value_type(_node->_value, _i_bucket, hash_map_type::src_internal, _node, _i_external);
						return;
					}
					else
						_node = _node->next();	// value deleted => continue search
				}
				else	// internal > external => external wins
				{
					_value = value_type(_reader.const_value(), _i_bucket, hash_map_type::src_external, _node, _i_external);
					return;
				}
			}
			// only internal elements left
			while (_node)
			{
				if (!_node->deleted())
				{
					_value = value_type(_node->_value, _i_bucket, hash_map_type::src_internal, _node, _i_external);
					return;
				}
				else
					_node = _node->next();	// deleted => continue search
			}
			if (_i_external < (*_curr_bucket)._num_external)	// only external elements left
			{
				_value = value_type(_reader.const_value(), _i_bucket, hash_map_type::src_external, _node, _i_external);
				return;
			}
			
			// if we made it to this point there are obviously no more values in the current bucket
			// let's try the next one (outer while-loop!)
			++_curr_bucket;
			++_i_bucket;
			if (_curr_bucket == _last_bucket)
				return;
			
			_node = (*_curr_bucket)._internal_list;
			_i_external = 0;
			_reader.skip_to(_first_bid+(*_curr_bucket)._i_block, (*_curr_bucket)._i_first_subblock);
		}
	}
};



} // end namespace hash_map

__STXXL_END_NAMESPACE

#endif
