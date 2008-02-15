/***************************************************************************
 *            iterator.h
 *
 *  May 2007, Markus Westphal
 ****************************************************************************/


#ifndef _HASH_MAP_ITERATOR_H_
#define _HASH_MAP_ITERATOR_H_

#include <stxxl>
#include <ext/hash_map>


__STXXL_BEGIN_NAMESPACE

namespace hash_map {


template <class HashMap_> class iterator_map;
template <class HashMap_> class hash_map_iterator;
template <class HashMap_> class hash_map_const_iterator;
template <class HashMap_> class block_cache;


#pragma mark -
#pragma mark Class iterator_base
#pragma mark 
template <class HashMap_>
class hash_map_iterator_base
{
public:
	friend class iterator_map<HashMap_>;
	
	friend void HashMap_::erase(hash_map_iterator<HashMap_> & it);

	typedef HashMap_                                  hash_map_type;
	typedef typename hash_map_type::size_type         size_type;
	typedef typename hash_map_type::value_type        value_type;
	typedef typename hash_map_type::key_type		  key_type;
	typedef typename hash_map_type::reference         reference;
	typedef typename hash_map_type::const_reference   const_reference;
	typedef typename hash_map_type::node_type         node_type;
	typedef typename hash_map_type::bucket_type       bucket_type;
	typedef typename hash_map_type::bid_iterator_type bid_iterator_type;
	typedef typename hash_map_type::source_type       source_type;
	
	typedef buffered_reader<typename hash_map_type::block_cache_type, bid_iterator_type> reader_type;

	typedef std::forward_iterator_tag                 iterator_category;

protected:
	hash_map_type *_map;			/* */
	reader_type   *_reader;			/* */
	bool           _prefetch;		/* true if prefetching enabled; false by default, will be set to true when incrementing (see find_next()) */
	node_type     *_node;			/* current (source=internal) or old (src=external) internal node */
	size_type      _i_bucket;		/* index of current bucket */
	size_type      _i_external;		/* position of current (source=external) or next (source=internal) external value (see _ext_valid) */
	source_type    _source;			/* source of current value: external or internal */
	key_type       _key;			/* key of current value */
	bool           _ext_valid;		/* true if i_external points to the current or next external value
									   example: iterator was created by hash_map::find() and the value was found in internal memory
									   => iterator pointing to internal node is created and location of next external value is unknown (_ext_valid == false)
									   => when incrementing the iterator the external values will be scanned from the beginning of the bucket to find the valid external index */
	bool           _end;			/* true if iterator equals end() */
	

private:
	hash_map_iterator_base()
	{
		STXXL_VERBOSE3("hash_map_iterator_base def contruct addr="<<this);
	}


public:
	//! \brief Construct a new iterator
	hash_map_iterator_base(hash_map_type * map, size_type i_bucket, node_type *node, size_type i_external, source_type source, bool ext_valid, key_type key) :
		_map(map),
		_i_bucket(i_bucket),
		_node(node),
		_i_external(i_external),
		_source(source),
		_ext_valid(ext_valid),
		_reader(NULL),
		_prefetch(false),
		_end(false),
		_key(key)
	{
		STXXL_VERBOSE3("hash_map_iterator_base parameter construct addr="<<this);
		_map->_iterator_map.register_iterator(*this);
	}
	
	
	//! \brief Construct a new iterator pointing to the end of the given hash-map.
	hash_map_iterator_base(hash_map_type *map) :
		_map(map),
		_i_bucket(0),
		_node(NULL),
		_i_external(0),
		_ext_valid(false),
		_reader(NULL),
		_prefetch(false),
		_end(true)
	{ }
	
	
	//! \brief Construct a new iterator from an existing one
	hash_map_iterator_base(const hash_map_iterator_base & obj) :
		_map(obj._map),
		_i_bucket(obj._i_bucket),
		_node(obj._node),
		_i_external(obj._i_external),
		_source(obj._source),
		_ext_valid(obj._ext_valid),
		_reader(NULL),
		_prefetch(false),
		_end(obj._end),
		_key(obj._key)
	{
		STXXL_VERBOSE3("hash_map_iterator_base constr from"<<(&obj)<<" to "<<this);
		if (!_end)
			if(_map) _map->_iterator_map.register_iterator(*this);
	}
	
	
	//! \brief Assignment operator
	hash_map_iterator_base & operator = (const hash_map_iterator_base & obj)
	{
		STXXL_VERBOSE3("hash_map_iterator_base copy from"<<(&obj)<<" to "<<this);
		if(&obj != this)
		{
			if(_map && !_end) _map->_iterator_map.unregister_iterator(*this);
			
			_map        = obj._map;
			_i_bucket   = obj._i_bucket;
			_node       = obj._node;
			_source     = obj._source;
			_i_external = obj._i_external;
			_ext_valid  = obj._ext_valid;
			_reader     = NULL;
			_prefetch	= obj._prefetch;
			_end        = obj._end;
			_key        = obj._key;
			
			if (_map && !_end) _map->_iterator_map.register_iterator(*this);
		}
		return *this;
	}
	
	
	//! \brief Two iterators are equal if the point to the same value in the same map
	bool operator == (const hash_map_iterator_base & obj) const
	{
		if (_end && obj._end)
			return true;
			
		if (_map != obj._map || _i_bucket != obj._i_bucket || _source != obj._source)
			return false;
		
		if (_source == hash_map_type::src_internal)
			return _node == obj._node;
		else
			return _i_external == obj._i_external;	// no need to worry about _ext_valid - it's true if source is external
	}
	
	
	bool operator != (const hash_map_iterator_base & obj) const
	{
		return ! operator==(obj);
	}
	

protected:
	//! \brief Initialize reader object to scan external values
	void init_reader()
	{
		const bucket_type & bucket = _map->_buckets[_i_bucket];
		
		bid_iterator_type first = _map->_bids.begin() + bucket._i_block;
		bid_iterator_type last  = _map->_bids.end();
		
		_reader = new reader_type(first, last, &(_map->_block_cache), bucket._i_first_subblock, _prefetch);
		
		// external value's index already known
		if (_ext_valid)
		{
			for (size_type i = 0; i < _i_external; i++)
				++(*_reader);
		}
		// otw lookup external value.
		// case I: no internal value => first external value is the desired one
		else if (_node == NULL)
		{
			_i_external = 0;
			_ext_valid = true;
		}
		// case II: search for smallest external value > internal value
		else
		{
			_i_external = 0;
			while (_i_external < bucket._num_external)
			{
				if (_map->__gt(_reader->const_value().first, _node->_value.first))
					break;
				
				++(*_reader);
				++_i_external;
			}
			// note: _i_external==_num_external just means that there was no external value > internal value (which is perfectly OK)
			_ext_valid = true;
		}
	}
	
	
	//! \brief Reset reader-object
	void reset_reader() {
		if (_reader != NULL) {
			delete _reader;
			_reader = NULL;
		}
	}


public:
	//! \brief Advance iterator to the next value
	//! The next value is determined in the following way
	//!	- if there are remaining internal or external values in the current bucket, choose the smallest among them, that is not marked as deleted
	//!	- otherwise continue with the next bucket
	//! 
	void find_next() {
		assert(!_end);
	
		size_type i_bucket_old = _i_bucket;

		bucket_type bucket = _map->_buckets[_i_bucket];
		if (_reader == NULL)
			hash_map_iterator_base<hash_map_type>::init_reader();

		// assumption: when incremented once, more increments are likely to follow; therefore start prefetching
		if (!_prefetch)
		{
			_reader->enable_prefetching();
			_prefetch = true;
		}
		
		// assertion: external value is always > internal value
		
		node_type *tmp_node = (_node) ? _node : bucket._internal_list;

		// case 0: current value is external one => next value will be chosen among ...
		if (_source == hash_map_type::src_external)
		{
			// ... smallest internal value > current external value and ...
			while (tmp_node && _map->__leq(tmp_node->_value.first, _reader->const_value().first))
			{
//				_node = tmp_node;
				tmp_node = tmp_node->next();
			}
			
			// ... next external value
			++_i_external;
			++(*_reader);
		}
		// case 1: current value is internal one => choose next value among current external value (which is > current internal value or does not exist) and next internal value
		else if (_source == hash_map_type::src_internal)
		{
			assert(tmp_node && (_i_external >= bucket._num_external || _map->__lt(tmp_node->_value.first, _reader->const_value().first)));
			
			tmp_node = _node->next();
		}
		// case 2: current value is unknown (iterator created by begin()) => choose next value beginning with first internal and first external value 
		else if (_source == hash_map_type::src_unknown)
		{
			assert(tmp_node == bucket._internal_list);
			assert(_i_external == 0);
		}
		
		
		// tmp_node now points to the smallest internal value > old value (tmp_node may be NULL)
		// and _reader now points to the smallest external value > old value (external value may not exists) (old value's key still saved in _key)
//		if (_source != hash_map_type::src_unknown)
//		{
//			assert(tmp_node == NULL                    || _map->__gt(tmp_node->_value.first, _key));
//			assert(_i_external >= bucket._num_external || _map->__gt(_reader->const_value().first, _key));
//		}
		
		// start search for next value beginning with tmp_node->_value and _reaader->const_value()
		while (true)
		{
			// internal and external values available
			while (tmp_node && _i_external < bucket._num_external)
			{
				// internal value less or equal external value => internal wins
				if (_map->__leq(tmp_node->_value.first, _reader->const_value().first))
				{
					_node = tmp_node;

					// make sure external value is always ahead
					if (_map->__eq(_node->_value.first, _reader->const_value().first))
					{
						++_i_external;
						++(*_reader);
					}

					if (!_node->deleted())
					{
						_key = _node->_value.first;
						_source = hash_map_type::src_internal;
						goto end_search;	// just this once - I promise...
					}
					else
						tmp_node = tmp_node->next();	// continue search if internal value flaged as deleted
				}
				// otherwise external wins
				else
				{
					_key = _reader->const_value().first;
					_source = hash_map_type::src_external;
					goto end_search;
				}
			}
			// only external values left
			if (_i_external < bucket._num_external)
			{
				_key = _reader->const_value().first;
				_source = hash_map_type::src_external;
				goto end_search;
			}
			// only internal values left
			while (tmp_node)
			{
				_node = tmp_node;
				
				if (!_node->deleted())
				{
					_key = _node->_value.first;
					_source = hash_map_type::src_internal;
					goto end_search;
				}
				else
					tmp_node = tmp_node->next();	// continue search
			}
			
			// at this point there are obviously no more values in the current bucket
			// let's try the next one (outer while-loop!)
			_i_bucket++;
			if (_i_bucket == _map->_buckets.size())
			{
				_end = true;
				reset_reader();
				goto end_search;
			}
			bucket = _map->_buckets[_i_bucket];
			
			_i_external = 0;
			tmp_node = bucket._internal_list;
			_node = NULL;
			
			_reader->skip_to(_map->_bids.begin()+bucket._i_block, bucket._i_first_subblock);
		}
		
		end_search:

		// we've either reached the end or one of the following conditions hold:
		// - if current source is internal => external value > internal value or no external value
		// - if current source is external => internal value < external value or no internal value
//		assert(
//				_end ||
//				(_source == hash_map_type::src_internal && _node != NULL && (_i_external >= bucket._num_external || _map->__lt(_node->_value.first, _reader->const_value().first))) || 
//				(_source == hash_map_type::src_external && _i_external <  bucket._num_external && (_node == NULL || _map->__lt(_node->_value.first, _reader->const_value().first)))
//			);

		// unregister/re-register if neccessary
		if (_end)
		{
			this->_map->_iterator_map.unregister_iterator(*this, i_bucket_old);
		}
		else if (i_bucket_old != _i_bucket)
		{
			this->_map->_iterator_map.unregister_iterator(*this, i_bucket_old);
			this->_map->_iterator_map.register_iterator(*this, _i_bucket);
		}
	}
		

	virtual ~hash_map_iterator_base()
	{
		STXXL_VERBOSE3("hash_map_iterator_base deconst "<<this);

		if (_map && !_end) _map->_iterator_map.unregister_iterator(*this);
		reset_reader();
	}
};


#pragma mark -
#pragma mark Class iterator
#pragma mark 
template <class HashMap_>
class hash_map_iterator : public hash_map_iterator_base<HashMap_>
{
public:
	typedef HashMap_								  hash_map_type;
	typedef typename hash_map_type::size_type         size_type;
	typedef typename hash_map_type::value_type        value_type;
	typedef typename hash_map_type::key_type          key_type;
	typedef typename hash_map_type::reference         reference;
	typedef typename hash_map_type::const_reference   const_reference;
	typedef typename hash_map_type::node_type         node_type;
	typedef typename hash_map_type::bucket_type       bucket_type;
	typedef typename hash_map_type::bid_iterator_type bid_iterator_type;
	typedef typename hash_map_type::source_type       source_type;
	
	typedef buffered_reader<typename hash_map_type::block_cache_type, bid_iterator_type> reader_type;

	typedef std::forward_iterator_tag                 iterator_category;
	
	typedef hash_map_iterator_base<hash_map_type>	  base_type;
	typedef hash_map_const_iterator<hash_map_type>	  hash_map_const_iterator;


public:
	hash_map_iterator() :
		base_type() {}
	
	hash_map_iterator(hash_map_type *map, size_type i_bucket, node_type *node, size_type i_external, source_type source, bool ext_valid, key_type key) :
		base_type(map, i_bucket, node, i_external, source, ext_valid, key) {}
	
	hash_map_iterator(hash_map_type *map) :
		base_type(map) {}
	
	hash_map_iterator(const hash_map_iterator & obj) :
		base_type(obj) {}
	
	hash_map_iterator & operator = (const hash_map_iterator & obj)
	{
		base_type::operator =(obj);
		return *this;
	}
	
	bool operator == (const hash_map_iterator & obj) const
	{
		return base_type::operator ==(obj);
	}

	bool operator == (const hash_map_const_iterator & obj) const
	{
		return base_type::operator ==(obj);
	}
	
	bool operator != (const hash_map_iterator & obj) const
	{
		return base_type::operator !=(obj);
	}
	
	bool operator != (const hash_map_const_iterator & obj) const
	{
		return base_type::operator !=(obj);
	}
	
	//! \brief Return reference to current value. If source is external, mark the value's block as dirty
	reference operator * ()
	{
		if (this->_source == hash_map_type::src_internal)
			return this->_node->_value;
		
		else
		{
			if (this->_reader == NULL)
				base_type::init_reader();
			
			return this->_reader->value();
		}
	}

	//! \brief Increment iterator
	hash_map_iterator<hash_map_type> & operator ++ ()
	{
 		base_type::find_next();
		return *this;
	}
};

	
	
#pragma mark -
#pragma mark Class const_iterator
#pragma mark 
template <class HashMap_>
class hash_map_const_iterator : public hash_map_iterator_base<HashMap_>
{
public:
	typedef HashMap_								  hash_map_type;
	typedef typename hash_map_type::size_type         size_type;
	typedef typename hash_map_type::value_type        value_type;
	typedef typename hash_map_type::key_type          key_type;
	typedef typename hash_map_type::reference         reference;
	typedef typename hash_map_type::const_reference   const_reference;
	typedef typename hash_map_type::node_type         node_type;
	typedef typename hash_map_type::bucket_type       bucket_type;
	typedef typename hash_map_type::bid_iterator_type bid_iterator_type;
	typedef typename hash_map_type::source_type       source_type;
	
	typedef buffered_reader<typename hash_map_type::block_cache_type, bid_iterator_type> reader_type;

	typedef std::forward_iterator_tag                 iterator_category;
	
	typedef hash_map_iterator_base<hash_map_type>     base_type;
	typedef hash_map_iterator<hash_map_type>		  hash_map_iterator;

public:
	hash_map_const_iterator() :
		base_type() {}

	hash_map_const_iterator(hash_map_type * map, size_type i_bucket, node_type *node, size_type i_external, source_type source, bool ext_valid, key_type key) :
		base_type(map, i_bucket, node, i_external, source, ext_valid, key)
	{ }
	
	hash_map_const_iterator(hash_map_type *map) :
		base_type(map)
	{ }

	hash_map_const_iterator(const hash_map_const_iterator & obj) :
		base_type(obj)
	{ }
	
	hash_map_const_iterator & operator = (const hash_map_const_iterator & obj)
	{
		base_type::operator =(obj);
		return *this;
	}
	
	bool operator == (const hash_map_const_iterator & obj) const
	{
		return base_type::operator ==(obj);
	}

	bool operator == (const hash_map_iterator & obj) const
	{
		return base_type::operator ==(obj);
	}
	
	bool operator != (const hash_map_const_iterator & obj) const
	{
		return base_type::operator !=(obj);
	}
	
	bool operator != (const hash_map_iterator & obj) const
	{
		return base_type::operator !=(obj);
	}
	
	//! \brief Return const-reference to current value
	const_reference operator * ()
	{
		if (this->_source == hash_map_type::src_internal)
			return this->_node->_value;
		
		else
		{
			if (this->_reader == NULL)
				base_type::init_reader();
			
			return this->_reader->const_value();
		}
	}
	
	//! \brief Increment iterator
	hash_map_const_iterator<hash_map_type> & operator ++ ()
	{
		base_type::find_next();
		return *this;
	}

};



#pragma mark -
#pragma mark Iterator Map
#pragma mark 

template <class HashMap_>
class iterator_map
{	
	
public:
	typedef HashMap_ hash_map_type;
	typedef typename hash_map_type::size_type   size_type;
	typedef typename hash_map_type::node_type   node_type;
	typedef typename hash_map_type::source_type source_type;
	typedef typename hash_map_type::key_type	key_type;
	
	typedef hash_map_iterator_base<hash_map_type> iterator_base;
	
	struct hasher
	{
		size_t operator () (const key_type & key) const
		{
			return longhash1(key);
		}
	};

	typedef __gnu_cxx::hash_multimap<size_type, iterator_base *, hasher> multimap_type;	// store iterators by bucket-index

private:
	multimap_type  _it_map;
	hash_map_type *_map;
	
	typedef typename multimap_type::value_type     pair_type;
	typedef typename multimap_type::iterator       mmiterator_type;
	typedef typename multimap_type::const_iterator const_mmiterator_type;
	

public:	
	iterator_map(hash_map_type * map) :
		_map(map),
		_it_map(10)		// we don't expect too many iterators at the same time
	{ }
	
	
	~iterator_map() { _it_map.clear(); }
	

	void register_iterator(iterator_base & it)
	{
		register_iterator(it, it._i_bucket);
	}

	
	void register_iterator(iterator_base & it, size_type i_bucket)
	{
		STXXL_VERBOSE2("hash_map::iterator_map register_iterator addr=" << &it << " bucket=" << i_bucket)
		_it_map.insert(pair_type(i_bucket, &it));
	}
	
	
	void unregister_iterator(iterator_base & it)
	{
		unregister_iterator(it, it._i_bucket);
	}

	
	void unregister_iterator(iterator_base & it, size_type i_bucket)
	{
		STXXL_VERBOSE2("hash_map::iterator_map unregister_iterator addr=" << &it << " bucket=" << i_bucket)

		std::pair<mmiterator_type,mmiterator_type> range = _it_map.equal_range(i_bucket);
		assert(range.first != range.second);
		
		for (mmiterator_type i = range.first; i != range.second; ++i)
		{
			if (i->second == &it)
			{
				_it_map.erase(i);
				return;
			}
		}
  
		STXXL_FORMAT_ERROR_MSG(msg,"unregister_iterator Panic in hash_map::iterator_map, can not find and unregister iterator")
		throw std::runtime_error(msg.str());
	}
	
	
	//! \brief Update iterators with given key and bucket and make them point to the specified location in external memory (will be called when re-hashing)
	void fix_iterators_2ext(size_type i_bucket_old, const key_type & key, size_type i_bucket_new, size_type i_ext)
	{
		STXXL_VERBOSE2("hash_map::iterator_map fix_iterators_2ext i_bucket=" << i_bucket_old << " key=" << key << ", new_i_ext=" << i_ext)
		
		std::vector<iterator_base *> its2fix;
		__find(i_bucket_old, its2fix);
		
		typename std::vector<iterator_base *>::iterator it2fix = its2fix.begin();
		for (; it2fix != its2fix.end(); ++it2fix)
		{
			if (!_map->__eq(key, (**it2fix)._key))
				continue;

			if (i_bucket_old != i_bucket_new)
			{
				unregister_iterator(**it2fix);
				register_iterator(**it2fix, i_bucket_new);
			}
		
			(**it2fix)._i_bucket = i_bucket_new;
			(**it2fix)._node = NULL;
			(**it2fix)._i_external = i_ext;
			(**it2fix)._source = hash_map_type::src_external;
			(**it2fix)._ext_valid = true;	// external position is now known (i_ext) and therefore valid
			(**it2fix).reset_reader();
		}
	}

	
	//! \brief Update iterators with given key and bucket and make them point to the specified node in internal memory (will be called by insert_oblivious)
	void fix_iterators_2int(size_type i_bucket, const key_type & key, node_type * node)
	{
		STXXL_VERBOSE2("hash_map::iterator_map fix_iterators_2int i_bucket=" << i_bucket << " key=" << key << " node=" << node)
		
		std::vector<iterator_base *> its2fix;
		__find(i_bucket, its2fix);
		
		typename std::vector<iterator_base *>::iterator it2fix = its2fix.begin();
		for (; it2fix != its2fix.end(); ++it2fix)
		{
			if (!_map->__eq((**it2fix)._key, key))
				continue;
		
			assert((**it2fix)._source == hash_map_type::src_external);

			(**it2fix)._source = hash_map_type::src_internal;
			(**it2fix)._node = node;
			(**it2fix)._i_external++;
			if ((**it2fix)._reader)
				(**it2fix)._reader->operator++();
		}
	}

	
	//! \brief Update iterators with given key and bucket and make them point to the end of the hash-map (called by erase and erase_oblivious)
	void fix_iterators_2end(size_type i_bucket, const key_type & key)
	{
		STXXL_VERBOSE2("hash_map::iterator_map fix_iterators_2end i_bucket=" << i_bucket << " key=" << key)
		
		std::vector<iterator_base *> its2fix;
		__find(i_bucket, its2fix);
		
		typename std::vector<iterator_base *>::iterator it2fix = its2fix.begin();
		for (; it2fix != its2fix.end(); ++it2fix)
		{
			if (!_map->__eq(key, (**it2fix)._key))
				continue;
			
			(**it2fix)._end = true;
			(**it2fix).reset_reader();
			unregister_iterator(**it2fix);
		}
	}

	
	//! \brief Update all iterators and make them point to the end of the hash-map (used by clear())
	void fix_iterators_all2end()
	{
		for (mmiterator_type i = _it_map.begin(); i != _it_map.end(); ++i)
		{
			(*i).second->_end = true;
			(*i).second->reset_reader();
		}
		_it_map.clear();
	}


private:
	//! \brief Find all iterators registered with given bucket and add them to outc
	template <class OutputContainer>
	void __find(size_type i_bucket, OutputContainer & outc)
	{
		std::pair<mmiterator_type,mmiterator_type> range = _it_map.equal_range(i_bucket);
		
		for (mmiterator_type i = range.first; i != range.second; ++i)
		{
			outc.push_back((*i).second);			
		}
	}
	
	
	// changes hash_map pointer in all contained iterators
	void change_hash_map_pointers(hash_map_type * map) {
		for(const_mmiterator_type it = _it_map.begin(); it != _it_map.end(); ++it) {
			((*it).second)->_map = map;
		}
	}


public:
	void swap(iterator_map<HashMap_> & obj)
	{
		std::swap(_it_map, obj._it_map);
		std::swap(_map, obj._map);
		change_hash_map_pointers(_map);
		obj.change_hash_map_pointers(obj._map);
	}


	void print_registered_iterators()
	{
		for (mmiterator_type i = _it_map.begin(); i != _it_map.end(); ++i)
		{
			std::cout << (*i).second << "  (bucket=" << (*i).second->_i_bucket << ", node=" << (*i).second->_node << ", i_ext=" << (*i).second->_i_external << ")" << std::endl;
		}
	}
};



} // end namespace ns_hash_map


__STXXL_END_NAMESPACE


namespace std
{
	template <class _HashMapType>
	void swap( stxxl::hash_map::iterator_map<_HashMapType> & a,
					stxxl::hash_map::iterator_map<_HashMapType> & b)
	{
		if (&a != & b)
			a.swap(b);
	}
}

#endif /* _HASH_MAP_ITERATOR_H_ */
