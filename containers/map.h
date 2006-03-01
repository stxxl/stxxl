#ifndef STXXL_MAP_INCLUDE
#define STXXL_MAP_INCLUDE

#include "btree/btree.h"

__STXXL_BEGIN_NAMESPACE

namespace btree
{
	template <	class KeyType, 
						class DataType, 
						class CompareType, 
						unsigned LogNodeSize,
						unsigned LogLeafSize,
						class PDAllocStrategy
					>
	class btree;
}	
	
template <	class KeyType,
					class DataType,
					class CompareType,
					unsigned RawNodeSize = 16*1024, // 16 KBytes default
					unsigned RawLeafSize = 128*1024, // 128 KBytes default
					class PDAllocStrategy = stxxl::SR
				>
class map 
{
	enum {
		apx_node_size = RawNodeSize/sizeof(std::pair<KeyType,BID<1> >),
		apx_leaf_size = RawLeafSize/sizeof(std::pair<KeyType,DataType >) ,
		log_node_size = LOG<apx_node_size>::value,
		log_leaf_size = LOG<apx_leaf_size>::value
	};
	typedef btree::btree<KeyType,DataType,CompareType,log_node_size,log_leaf_size,PDAllocStrategy> impl_type;
	
	impl_type Impl;
	
	map(); // forbidden
	map(const map&); // forbidden
	map& operator=(const map&); // forbidden
	
public:
	typedef typename impl_type::node_block_type node_block_type;
	typedef typename impl_type::leaf_block_type leaf_block_type;

	typedef typename impl_type::key_type key_type;
	typedef typename impl_type::data_type data_type;
	typedef typename impl_type::data_type mapped_type;
	typedef typename impl_type::value_type value_type;
	typedef typename impl_type::key_compare key_compare;
	typedef typename impl_type::value_compare value_compare;
	typedef typename impl_type::pointer pointer;
	typedef typename impl_type::reference reference;
	typedef typename impl_type::const_reference const_reference;
	typedef typename impl_type::size_type size_type;
	typedef typename impl_type::difference_type difference_type;
	typedef typename impl_type::iterator iterator;
	typedef typename impl_type::const_iterator const_iterator;
	
	iterator begin() { return Impl.begin(); }
	iterator end() { return Impl.end(); }
	const_iterator begin() const { return Impl.begin(); }
	const_iterator end() const { return Impl.end(); }
	size_type size() const { return Impl.size(); }
	size_type max_size() const { return Impl.max_size(); }
	bool empty() const { return Impl.empty(); }
	key_compare key_comp() const { return Impl.key_comp(); }
	value_compare value_comp() const { return Impl.value_comp(); }
	
	map(	unsigned node_cache_size_in_bytes,
				unsigned leaf_cache_size_in_bytes
				) : Impl(node_cache_size_in_bytes,leaf_cache_size_in_bytes)
	{}
	
	map(	const key_compare & c_,
				unsigned node_cache_size_in_bytes,
				unsigned leaf_cache_size_in_bytes
				) : Impl(c_,node_cache_size_in_bytes,leaf_cache_size_in_bytes)
	{}		
	
	
	template <class InputIterator>
	map(	InputIterator b,
				InputIterator e,
				unsigned node_cache_size_in_bytes,
				unsigned leaf_cache_size_in_bytes,
				bool range_sorted = false
				) : Impl(b,e,node_cache_size_in_bytes,leaf_cache_size_in_bytes,range_sorted) 
	{
	}
	
	template <class InputIterator>
	map(	InputIterator b,
				InputIterator e,
				const key_compare & c_,
				unsigned node_cache_size_in_bytes,
				unsigned leaf_cache_size_in_bytes,
				bool range_sorted = false
			): Impl(b,e,c_,node_cache_size_in_bytes,leaf_cache_size_in_bytes,range_sorted)
	{
	}
	
	void swap(map  & obj) { std::swap(Impl,obj.Impl); }
	std::pair<iterator, bool> insert(const value_type& x)
	{
		return Impl.insert(x);
	}
	iterator insert(iterator pos, const value_type& x)
	{
		return Impl.insert(pos,x);
	}
	template <class InputIterator>
	void insert(InputIterator b, InputIterator e)
	{
		Impl.insert(b,e);
	}
	void erase(iterator pos)
	{
		Impl.erase(pos);
	}
	size_type erase(const key_type& k)
	{
		return Impl.erase(k);
	}
	void erase(iterator first, iterator last)
	{
		Impl.erase(first,last);
	}
	void clear()
	{
		Impl.clear();
	}
	iterator find(const key_type& k)
	{
		return Impl.find(k);
	}
	const_iterator find(const key_type& k) const
	{
		return Impl.find(k);
	}
	size_type count(const key_type& k)
	{
		return Impl.count(k);
	}
	iterator lower_bound(const key_type& k)
	{
		return Impl.lower_bound(k);
	}
	const_iterator lower_bound(const key_type& k) const
	{
		return Impl.lower_bound(k);
	}
	iterator upper_bound(const key_type& k)
	{
		return Impl.upper_bound(k);
	}
	const_iterator upper_bound(const key_type& k) const
	{
		return Impl.upper_bound(k);
	}
	std::pair<iterator, iterator> equal_range(const key_type& k)
	{
		return Impl.equal_range(k);
	}
	std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
	{
		return Impl.equal_range(k);
	}
	data_type & operator[](const key_type& k)
	{
		return Impl[k];
	}	
	
	//////////////////////////////////////////////////
	template <	class KeyType_,
					class DataType_,
					class CompareType_,
					unsigned RawNodeSize_,
					unsigned RawLeafSize_,
					class PDAllocStrategy_ >
	friend bool operator==(	const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & a, 
						const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & b);
	//////////////////////////////////////////////////
	template <	class KeyType_,
					class DataType_,
					class CompareType_,
					unsigned RawNodeSize_,
					unsigned RawLeafSize_,
					class PDAllocStrategy_ >
	friend bool operator <(	const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & a, 
						const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & b);
	//////////////////////////////////////////////////	
	template <	class KeyType_,
					class DataType_,
					class CompareType_,
					unsigned RawNodeSize_,
					unsigned RawLeafSize_,
					class PDAllocStrategy_ >
	friend bool operator >(	const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & a, 
						const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & b);						
	//////////////////////////////////////////////////
	template <	class KeyType_,
					class DataType_,
					class CompareType_,
					unsigned RawNodeSize_,
					unsigned RawLeafSize_,
					class PDAllocStrategy_ >
	friend bool operator !=(	const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & a, 
						const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & b);						
	//////////////////////////////////////////////////
	template <	class KeyType_,
					class DataType_,
					class CompareType_,
					unsigned RawNodeSize_,
					unsigned RawLeafSize_,
					class PDAllocStrategy_ >
	friend bool operator <=(	const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & a, 
						const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & b);						
	//////////////////////////////////////////////////
			template <	class KeyType_,
					class DataType_,
					class CompareType_,
					unsigned RawNodeSize_,
					unsigned RawLeafSize_,
					class PDAllocStrategy_ >
	friend bool operator >=(	const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & a, 
						const map<KeyType_,DataType_,CompareType_,RawNodeSize_,RawLeafSize_,PDAllocStrategy_> & b);						
	//////////////////////////////////////////////////
};
	
template <	class KeyType,
						class DataType,
						class CompareType,
						unsigned RawNodeSize,
						unsigned RawLeafSize,
						class PDAllocStrategy
				>
inline bool operator==(	const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & a, 
							const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & b)
{
	return a.Impl == b.Impl;
}

template <	class KeyType,
						class DataType,
						class CompareType,
						unsigned RawNodeSize,
						unsigned RawLeafSize,
						class PDAllocStrategy
				>
inline bool operator < (const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & a, 
						  const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & b)
{
	return a.Impl < b.Impl;
}

template <	class KeyType,
						class DataType,
						class CompareType,
						unsigned RawNodeSize,
						unsigned RawLeafSize,
						class PDAllocStrategy
				>
inline bool operator > (const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & a, 
						  const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & b)
{
	return a.Impl > b.Impl;
}

template <	class KeyType,
						class DataType,
						class CompareType,
						unsigned RawNodeSize,
						unsigned RawLeafSize,
						class PDAllocStrategy
				>
inline bool operator != (const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & a, 
						  const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & b)
{
	return a.Impl != b.Impl;
}

template <	class KeyType,
						class DataType,
						class CompareType,
						unsigned RawNodeSize,
						unsigned RawLeafSize,
						class PDAllocStrategy
				>
inline bool operator <= (const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & a, 
						  const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & b)
{
	return a.Impl <= b.Impl;
}

template <	class KeyType,
						class DataType,
						class CompareType,
						unsigned RawNodeSize,
						unsigned RawLeafSize,
						class PDAllocStrategy
				>
inline bool operator >= (const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & a, 
						  const map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & b)
{
	return a.Impl >= b.Impl;
}

__STXXL_END_NAMESPACE

namespace std
{
	template <	class KeyType,
						class DataType,
						class CompareType,
						unsigned RawNodeSize,
						unsigned RawLeafSize,
						class PDAllocStrategy
				>
	void swap(	stxxl::map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & a,
					stxxl::map<KeyType,DataType,CompareType,RawNodeSize,RawLeafSize,PDAllocStrategy> & b
				)
	{
		a.swap(b);
	}
}

#endif
