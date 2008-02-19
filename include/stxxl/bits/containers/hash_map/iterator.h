/***************************************************************************
 *            iterator.h
 *
 *  May 2007, Markus Westphal
 ****************************************************************************/


#ifndef STXXL_CONTAINERS_HASH_MAP__ITERATOR_H
#define STXXL_CONTAINERS_HASH_MAP__ITERATOR_H


__STXXL_BEGIN_NAMESPACE

namespace hash_map {


template <class HashMap> class iterator_map;
template <class HashMap> class hash_map_iterator;
template <class HashMap> class hash_map_const_iterator;
template <class HashMap> class block_cache;


#pragma mark -
#pragma mark Class iterator_base
#pragma mark 
template <class HashMap>
class hash_map_iterator_base
{
public:

	friend class iterator_map<HashMap>;
	friend void HashMap::erase(hash_map_iterator<HashMap> & it);

	typedef HashMap                                   hash_map_type;
	typedef typename hash_map_type::size_type         size_type;
	typedef typename hash_map_type::value_type        value_type;
	typedef typename hash_map_type::key_type		  key_type;
	typedef typename hash_map_type::reference         reference;
	typedef typename hash_map_type::const_reference   const_reference;
	typedef typename hash_map_type::node_type         node_type;
	typedef typename hash_map_type::bucket_type       bucket_type;
	typedef typename hash_map_type::bid_iterator_type bid_iterator_type;
	typedef typename hash_map_type::source_type       source_type;
	
	typedef bufferedreader_<typename hash_map_type::block_cache_type, bid_iterator_type> reader_type;

	typedef std::forward_iterator_tag                 iterator_category;


protected:
	HashMap     * map_
	reader_type * reader_;
	bool          prefetch_;	/* true if prefetching enabled; false by default, will be set to true when incrementing (see find_next()) */
	node_type   * node_;		/* current (source=internal) or old (src=external) internal node */
	size_type     i_bucket_;	/* index of current bucket */
	size_type     i_external_;	/* position of current (source=external) or next (source=internal) external value (see _ext_valid) */
	source_type   source_;		/* source of current value: external or internal */
	key_type      key_;			/* key of current value */
	bool          ext_valid_;	/* true if i_external points to the current or next external value
								   example: iterator was created by hash_map::find() and the value was found in internal memory
								   => iterator pointing to internal node is created and location of next external value is unknown (_ext_valid == false)
								   => when incrementing the iterator the external values will be scanned from the beginning of the bucket to find the valid external index */
	bool          end_;			/* true if iterator equals end() */
	

private:
	hash_map_iterator_base()
	{
		STXXL_VERBOSE3("hash_map_iterator_base def contruct addr="<<this);
	}


public:
	//! \brief Construct a new iterator
	hash_map_iterator_base(hash_map_type * map, size_type i_bucket, node_type *node, size_type i_external, source_type source, bool ext_valid, key_type key) :
		map_(map),
		i_bucket_(i_bucket),
		node_(node),
		i_external_(i_external),
		source_(source),
		ext_valid_(ext_valid),
		reader_(NULL),
		prefetch_(false),
		end_(false),
		key_(key)
	{
		STXXL_VERBOSE3("hash_map_iterator_base parameter construct addr="<<this);
		map_->iterator_map_.register_iterator(*this);
	}
	
	
	//! \brief Construct a new iterator pointing to the end of the given hash-map.
	hash_map_iterator_base(hash_map_type *map) :
		map_(map),
		i_bucket_(0),
		node_(NULL),
		i_external_(0),
		ext_valid_(false),
		reader_(NULL),
		prefetch_(false),
		end_(true)
	{ }
	
	
	//! \brief Construct a new iterator from an existing one
	hash_map_iterator_base(const hash_map_iterator_base & obj) :
		map_(obj.map_),
		i_bucket_(obj.i_bucket_),
		node_(obj.node_),
		i_external_(obj.i_external_),
		source_(obj.source_),
		ext_valid_(obj.ext_valid_),
		reader_(NULL),
		prefetch_(obj.prefetch_),
		end_(obj.end_),
		key_(obj.key_)
	{
		STXXL_VERBOSE3("hash_map_iterator_base constr from"<<(&obj)<<" to "<<this);
		if (!end_ && map_)
			map_->iterator_map_.register_iterator(*this);
	}
	
	
	//! \brief Assignment operator
	hash_map_iterator_base & operator = (const hash_map_iterator_base & obj)
	{
		STXXL_VERBOSE3("hash_map_iterator_base copy from"<<(&obj)<<" to "<<this);
		if(&obj != this)
		{
			if(map_ && !end_) map_->iterator_map_.unregister_iterator(*this);
			
			map_        = obj.map_;
			i_bucket_   = obj.i_bucket_;
			node_       = obj.node_;
			source_     = obj.source_;
			i_external_ = obj.i_external_;
			ext_valid_  = obj.ext_valid_;
			reader_     = NULL;
			prefetch_	= obj.prefetch_;
			end_        = obj.end_;
			key_        = obj.key_;
			
			if (map_ && !end_) map_->iterator_map_.register_iterator(*this);
		}
		return *this;
	}
	
	
	//! \brief Two iterators are equal if the point to the same value in the same map
	bool operator == (const hash_map_iterator_base & obj) const
	{
		if (end_ && obj.end_)
			return true;
			
		if (map_ != obj.map_ || i_bucket_ != obj.i_bucket_ || source_ != obj.source_)
			return false;
		
		if (source_ == hash_map_type::src_internal)
			return node_ == obj.node_;
		else
			return i_external_ == obj.i_external_;
	}
	
	
	bool operator != (const hash_map_iterator_base & obj) const
	{
		return ! operator==(obj);
	}
	

protected:
	//! \brief Initialize reader object to scan external values
	void init_reader()
	{
		const bucket_type & bucket = map_->buckets_[i_bucket_];
		
		bid_iterator_type first = map_->bids_.begin() + bucket.i_block_;
		bid_iterator_type last  = map_->bids_.end();
		
		reader_ = new reader_type(first, last, &(map_->block_cache_), bucket.i_first_subblock_, prefetch_);
		
		// external value's index already known
		if (ext_valid_)
		{
			for (size_type i = 0; i < i_external_; i++)
				++(*reader_);
		}
		// otw lookup external value.
		// case I: no internal value => first external value is the desired one
		else if (node_ == NULL)
		{
			i_external_ = 0;
			ext_valid_ = true;
		}
		// case II: search for smallest external value > internal value
		else
		{
			i_external_ = 0;
			while (i_external_ < bucket.num_external_)
			{
				if (map_->__gt(reader_->const_value().first, node_->value_.first))
					break;
				
				++(*reader_);
				++i_external_;
			}
			// note: i_external==num_external just means that there was no external value > internal value (which is perfectly OK)
			ext_valid_ = true;
		}
	}
	
	
	//! \brief Reset reader-object
	void reset_reader() {
		if (reader_ != NULL) {
			delete reader_;
			reader_ = NULL;
		}
	}


public:
	//! \brief Advance iterator to the next value
	//! The next value is determined in the following way
	//!	- if there are remaining internal or external values in the current bucket, choose the smallest among them, that is not marked as deleted
	//!	- otherwise continue with the next bucket
	//! 
	void find_next() {
		// invariant: current external value is always > current internal value
		assert(!end_);
	
		size_type i_bucket_old = i_bucket_;
		bucket_type bucket = map_->buckets_[i_bucket_];

		if (reader_ == NULL)
			init_reader();

		// when incremented once, more increments are likely to follow; therefore start prefetching
		if (!prefetch_)
		{
			reader_->enable_prefetching();
			prefetch_ = true;
		}
		
		// determine starting-points for comparision, which are given by:
		// - tmp_node: smallest internal value > old value (tmp_node may be NULL)
		// - reader_: smallest external value > old value (external value may not exists)
		node_type *tmp_node = (node_) ? node_ : bucket.internal_list_;
		if (source_ == hash_map_type::src_external)
		{
			while (tmp_node && map_->__leq(tmp_node->value_.first, key_)
				tmp_node = tmp_node->next();
			
			++i_external_;
			++(*reader_);
		}
		else if (source_ == hash_map_type::src_internal)
			tmp_node = node_->next();
		// else (source unknown): tmp_node and reader_ already point to the correct values
				
		
		while (true) {
			// internal and external values available
			while (tmp_node && i_external_ < bucket.num_external_)
			{
				// internal value less or equal external value => internal wins
				if (map_->__leq(tmp_node->value_.first, reader_->const_value().first))
				{
					node_ = tmp_node;
					if (map_->__eq(node_->value_.first, reader_->const_value().first))
					{
						++i_external_;
						++(*reader_);
					}

					if (!node_->deleted())
					{
						key_ = node_->value_.first;
						source_ = hash_map_type::src_internal;
						goto end_search;	// just this once - I promise...
					}
					else
						tmp_node = tmp_node->next();	// continue search if internal value flaged as deleted
				}
				// otherwise external wins
				else
				{
					key_ = reader_->const_value().first;
					source_ = hash_map_type::src_external;
					goto end_search;
				}
			}
			// only external values left
			if (i_external_ < bucket.num_external_)
			{
				key_ = reader_->const_value().first;
				source_ = hash_map_type::src_external;
				goto end_search;
			}
			// only internal values left
			while (tmp_node)
			{
				node_ = tmp_node;
				if (!node_->deleted())
				{
					key_ = node_->_value.first;
					source_ = hash_map_type::src_internal;
					goto end_search;
				}
				else
					tmp_node = tmp_node->next();	// continue search
			}
			
			
			// at this point there are obviously no more values in the current bucket
			// let's try the next one (outer while-loop!)
			i_bucket_++;
			if (i_bucket_ == map_->buckets_.size())
			{
				end_ = true;
				reset_reader();
				goto end_search;
			}
			else
			{
				bucket = map_->buckets_[i_bucket_];
				i_external_ = 0;
				tmp_node = bucket.internal_list_;
				node_ = NULL;
				reader_->skip_to(map_->bids_.begin()+bucket.i_block_, bucket.i_first_subblock_);
			}
		}
		
	end_search:
		if (end_)
		{
			this->map_->iterator_map_.unregister_iterator(*this, i_bucket_old);
		}
		else if (i_bucket_old != i_bucket_)
		{
			this->map_->iterator_map_.unregister_iterator(*this, i_bucket_old);
			this->map_->iterator_map_.register_iterator(*this, i_bucket_);
		}
	}
		

	virtual ~hash_map_iterator_base()
	{
		STXXL_VERBOSE3("hash_map_iterator_base deconst "<<this);

		if (map_ && !end_) map_->iterator_map_.unregister_iterator(*this);
		reset_reader();
	}
};


#pragma mark -
#pragma mark Class iterator
#pragma mark 
template <class HashMap>
class hash_map_iterator : public hash_map_iterator_base<HashMap>
{
public:
	typedef HashMap 								  hash_map_type;
	typedef typename hash_map_type::size_type         size_type;
	typedef typename hash_map_type::value_type        value_type;
	typedef typename hash_map_type::key_type          key_type;
	typedef typename hash_map_type::reference         reference;
	typedef typename hash_map_type::const_reference   const_reference;
	typedef typename hash_map_type::node_type         node_type;
	typedef typename hash_map_type::bucket_type       bucket_type;
	typedef typename hash_map_type::bid_iterator_type bid_iterator_type;
	typedef typename hash_map_type::source_type       source_type;
	
	typedef bufferedreader_<typename hash_map_type::block_cache_type, bid_iterator_type> reader_type;

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
		if (this->source_ == hash_map_type::src_internal)
			return this->node_->value_;
		
		else
		{
			if (this->reader_ == NULL)
				base_type::init_reader();
			
			return this->reader_->value();
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
template <class HashMap>
class hash_map_const_iterator : public hash_map_iterator_base<HashMap>
{
public:
	typedef HashMap									  hash_map_type;
	typedef typename hash_map_type::size_type         size_type;
	typedef typename hash_map_type::value_type        value_type;
	typedef typename hash_map_type::key_type          key_type;
	typedef typename hash_map_type::reference         reference;
	typedef typename hash_map_type::const_reference   const_reference;
	typedef typename hash_map_type::node_type         node_type;
	typedef typename hash_map_type::bucket_type       bucket_type;
	typedef typename hash_map_type::bid_iterator_type bid_iterator_type;
	typedef typename hash_map_type::source_type       source_type;
	
	typedef bufferedreader_<typename hash_map_type::block_cache_type, bid_iterator_type> reader_type;

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
		if (this->source_ == hash_map_type::src_internal)
			return this->node_->value_;
		
		else
		{
			if (this->reader_ == NULL)
				base_type::init_reader();
			
			return this->reader_->const_value();
		}
	}
	
	//! \brief Increment iterator
	hash_map_const_iterator<hash_map_type> & operator ++ ()
	{
		base_type::find_next();
		return *this;
	}

};


} // end namespace hash_map


__STXXL_END_NAMESPACE


#endif /* STXXL_CONTAINERS_HASH_MAP__ITERATOR_H */
