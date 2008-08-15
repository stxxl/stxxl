/***************************************************************************
 *            iterator_map.h
 *
 *  Jan 2008, Markus Westphal
 ****************************************************************************/

#ifndef STXXL_CONTAINERS_HASH_MAP__ITERATOR_MAP_H
#define STXXL_CONTAINERS_HASH_MAP__ITERATOR_MAP_H

#include "stxxl/bits/containers/hash_map/iterator.h"


__STXXL_BEGIN_NAMESPACE

namespace hash_map {


template <class HashMap>
class iterator_map
{	
	
public:
	typedef HashMap  hash_map_type;
	typedef typename hash_map_type::size_type   size_type;
	typedef typename hash_map_type::node_type   node_type;
	typedef typename hash_map_type::source_type source_type;
	typedef typename hash_map_type::key_type	key_type;
	
	typedef hash_map_iterator_base<hash_map_type> iterator_base;

private:
	struct hasher
	{
		size_t operator () (const key_type & key) const
		{
			return longhash1(key);
		}
	};

	typedef __gnu_cxx::hash_multimap<size_type, iterator_base *, hasher> multimap_type;	// store iterators by bucket-index

	typedef typename multimap_type::value_type     pair_type;	/* bucket-index and pointer to iterator_base */
	typedef typename multimap_type::iterator       mmiterator_type;
	typedef typename multimap_type::const_iterator const_mmiterator_type;

	multimap_type  it_map_;
	hash_map_type *map_;

	// forbidden
	iterator_map();
	iterator_map(const iterator_map &);
	iterator_map & operator = (const iterator_map &);    


public:	
	iterator_map(hash_map_type * map) :
		map_(map),
		it_map_(10)		// we don't expect too many iterators at the same time
	{ }
	
	
	~iterator_map() { it_map_.clear(); }
	

	void register_iterator(iterator_base & it)
	{
		register_iterator(it, it.i_bucket_);
	}

	
	void register_iterator(iterator_base & it, size_type i_bucket)
	{
		STXXL_VERBOSE2("hash_map::iterator_map register_iterator addr=" << &it << " bucket=" << i_bucket)
		it_map_.insert(pair_type(i_bucket, &it));
	}
	
	
	void unregister_iterator(iterator_base & it)
	{
		unregister_iterator(it, it.i_bucket_);
	}

	
	void unregister_iterator(iterator_base & it, size_type i_bucket)
	{
		STXXL_VERBOSE2("hash_map::iterator_map unregister_iterator addr=" << &it << " bucket=" << i_bucket)

		std::pair<mmiterator_type,mmiterator_type> range = it_map_.equal_range(i_bucket);
		assert(range.first != range.second);
		
		for (mmiterator_type i = range.first; i != range.second; ++i)
		{
			if (i->second == &it)
			{
				it_map_.erase(i);
				return;
			}
		}
  
		STXXL_FORMAT_ERROR_MSG(msg,"unregister_iterator Panic in hash_map::iterator_map, can not find and unregister iterator")
		throw std::runtime_error(msg.str());
	}
	
	
	//! \brief Update iterators with given key and bucket and make them point to the specified location in external memory (will be called during re-hashing)
	void fix_iterators_2ext(size_type i_bucket_old, const key_type & key, size_type i_bucket_new, size_type i_ext)
	{
		STXXL_VERBOSE2("hash_map::iterator_map fix_iterators_2ext i_bucket=" << i_bucket_old << " key=" << key << ", new_i_ext=" << i_ext)
		
		std::vector<iterator_base *> its2fix;
		__find(i_bucket_old, its2fix);
		
		typename std::vector<iterator_base *>::iterator it2fix = its2fix.begin();
		for (; it2fix != its2fix.end(); ++it2fix)
		{
			if (!map_->__eq(key, (**it2fix).key_))
				continue;

			if (i_bucket_old != i_bucket_new)
			{
				unregister_iterator(**it2fix);
				register_iterator(**it2fix, i_bucket_new);
			}
		
			(**it2fix).i_bucket_ = i_bucket_new;
			(**it2fix).node_ = NULL;
			(**it2fix).i_external_ = i_ext;
			(**it2fix).source_ = hash_map_type::src_external;
			(**it2fix).ext_valid_ = true;	// external position is now known (i_ext) and therefore valid
			(**it2fix).reset_reader();
			(**it2fix).reader_ = NULL;
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
			if (!map_->__eq((**it2fix).key_, key))
				continue;
		
			assert((**it2fix).source_ == hash_map_type::src_external);

			(**it2fix).source_ = hash_map_type::src_internal;
			(**it2fix).node_ = node;
			(**it2fix).i_external_++;
			if ((**it2fix).reader_)
				(**it2fix).reader_->operator++();
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
			if (!map_->__eq(key, (**it2fix).key_))
				continue;
			
			(**it2fix).end_ = true;
			(**it2fix).reset_reader();
			unregister_iterator(**it2fix);
		}
	}

	
	//! \brief Update all iterators and make them point to the end of the hash-map (used by clear())
	void fix_iterators_all2end()
	{
		mmiterator_type it2fix = it_map_.begin();
		for (; it2fix != it_map_.end(); ++it2fix)
		{
			(*it2fix).second->end_ = true;
			(*it2fix).second->reset_reader();
		}
		it_map_.clear();
	}


private:
	//! \brief Find all iterators registered with given bucket and add them to outc
	template <class OutputContainer>
	void __find(size_type i_bucket, OutputContainer & outc)
	{
		std::pair<mmiterator_type,mmiterator_type> range = it_map_.equal_range(i_bucket);
		
		for (mmiterator_type i = range.first; i != range.second; ++i)
		{
			outc.push_back((*i).second);			
		}
	}
	
	
	// changes hash_map pointer in all contained iterators
	void change_hash_map_pointers(hash_map_type * map) {
		for (mmiterator_type it = it_map_.begin(); it != it_map_.end(); ++it) {
			((*it).second)->map_ = map;
		}
	}


public:
	void swap(iterator_map<HashMap> & obj)
	{
		std::swap(it_map_, obj.it_map_);
		std::swap(map_, obj.map_);
		
		change_hash_map_pointers(map_);
		obj.change_hash_map_pointers(obj.map_);
	}


	void print_statistics(std::ostream & o = std::cout) const
	{
		o << "Registered iterators: " << it_map_.size() << "\n";
		const_mmiterator_type i = it_map_.begin();
		for (; i != it_map_.end(); ++i)
		{
			o << "  Address=" << (*i).second << ", Bucket=" << (*i).second->i_bucket_
			  << ", Node=" << (*i).second->node_ << ", i_ext=" << (*i).second->i_external_
			  << ", " << ( ((*i).source_ == hash_map_type::src_external) ? "external" : "internal" ) << std::endl;
		}
	}
};


} // end namespace hash_map


__STXXL_END_NAMESPACE


namespace std
{
	template <class HashMapType>
	void swap( stxxl::hash_map::iterator_map<HashMapType> & a,
					stxxl::hash_map::iterator_map<HashMapType> & b)
	{
		if (&a != &b)
			a.swap(b);
	}
}


#endif /* STXXL_CONTAINERS_HASH_MAP__ITERATOR_MAP_H */
