/***************************************************************************
 *            deque.h
 *
 *  Mon Sep 18 16:18:16 2006
 *  Copyright  2006  Roman Dementiev
 *  Email 
 ****************************************************************************/

#ifndef _STXXL_DEQUE_H
#define _STXXL_DEQUE_H

#include "vector.h"

__STXXL_BEGIN_NAMESPACE



template <class T, class VectorType>
class deque;

template <class DequeType>
class const_deque_iterator;

template <class DequeType>
class deque_iterator
{
	typedef typename DequeType::size_type size_type;
	typedef typename DequeType::vector_type vector_type;
	typedef deque_iterator<DequeType>	_Self;
	DequeType * Deque;
	size_type Offset;
	
	deque_iterator(DequeType * Deque_, size_type Offset_): 
		Deque(Deque_), Offset(Offset_)
	{ }
	
public:
	typedef typename DequeType::value_type  value_type;
	typedef typename DequeType::pointer pointer;
	typedef typename DequeType::const_pointer const_pointer;
	typedef typename DequeType::reference reference;
	typedef typename DequeType::const_reference const_reference;
	typedef deque_iterator<DequeType> iterator;
	typedef const_deque_iterator<DequeType> const_iterator;
	friend class deque<value_type,vector_type>;
	friend class const_deque_iterator<DequeType>;
	typedef std::random_access_iterator_tag iterator_category;
	typedef size_type difference_type;
		
	deque_iterator(): Deque(NULL),Offset(0) {}
		
	size_type operator - (const _Self & a) const
	{
		size_type SelfAbsOffset = (Offset >= Deque->begin_o)?
			Offset:(Deque->Vector.size() + Offset);
		size_type aAbsOffset = (a.Offset >= Deque->begin_o)?
			a.Offset:(Deque->Vector.size() + a.Offset);
		
		return SelfAbsOffset-aAbsOffset;
	}
	
	_Self operator - (size_type op) const
	{
		return _Self(Deque,(Offset + Deque->Vector.size() - op)%Deque->Vector.size());
	}
	
	_Self operator + (size_type op) const
	{
		return _Self(Deque,(Offset + op)%Deque->Vector.size());
	}
	
	_Self & operator -= (size_type op)
	{
		Offset = (Offset + Deque->Vector.size() - op)%Deque->Vector.size();
		return *this;
	}
	
	_Self & operator += (size_type op)
	{
		Offset = (Offset + op)%Deque->Vector.size();
		return *this;
	}
	
	reference operator *()
	{
		return Deque->Vector[Offset];
	}
	
	pointer operator ->()
	{
		return &(Deque->Vector[Offset]);
	}

	const_reference operator *() const
	{
		return Deque->Vector[Offset];
	}
	
	const_pointer operator ->() const
	{
		return &(Deque->Vector[Offset]);
	}
	
	_Self & operator ++()
	{
		Offset = (Offset + 1)%Deque->Vector.size();
		return *this;
	}
	_Self operator ++(int)
	{
		_Self __tmp = *this;
		Offset = (Offset + 1)%Deque->Vector.size();
		return __tmp;
	}
	_Self & operator --()
	{
		Offset = (Offset + Deque->Vector.size() - 1)%Deque->Vector.size();
		return *this;
	}
	_Self operator --(int)
	{
		_Self __tmp = *this;
		Offset = (Offset + Deque->Vector.size() - 1)%Deque->Vector.size();
		return __tmp;
	}
	bool operator == (const _Self &a) const
	{
		assert(Deque == a.Deque);
		return Offset == a.Offset;
	}
	bool operator != (const _Self &a) const
	{
		assert(Deque == a.Deque);
		return Offset != a.Offset;
	}
	
	bool operator < (const _Self &a) const
	{
		assert(Deque == a.Deque);
		return (a - (*this))>0;
	}
};

template <class DequeType>
class const_deque_iterator
{
	typedef const_deque_iterator<DequeType>	_Self;
	typedef typename DequeType::size_type size_type;
	typedef typename DequeType::vector_type vector_type;
	const DequeType * Deque;
	size_type Offset;
	
	const_deque_iterator(const DequeType * Deque_, size_type Offset_): 
		Deque(Deque_), Offset(Offset_)
	{ }
	
public:
	typedef typename DequeType::value_type  value_type;
	typedef typename DequeType::pointer pointer;
	typedef typename DequeType::const_pointer const_pointer;
	typedef typename DequeType::reference reference;
	typedef typename DequeType::const_reference const_reference;
	typedef deque_iterator<DequeType> iterator;
	typedef const_deque_iterator<DequeType> const_iterator;
	friend class deque<value_type,vector_type>;
		
	typedef std::random_access_iterator_tag iterator_category;
	typedef size_type difference_type;
		
	const_deque_iterator() : Deque(NULL),Offset(0){}
	const_deque_iterator(const deque_iterator<DequeType> & it ): 
		Deque(it.Deque),Offset(it.Offset) 
	{
	}
	
	size_type operator - (const _Self & a) const
	{
		size_type SelfAbsOffset = (Offset >=Deque->begin_o)?
			Offset:(Deque->Vector.size() + Offset);
		size_type aAbsOffset = (a.Offset >=Deque->begin_o)?
			a.Offset:(Deque->Vector.size() + a.Offset);
		
		return SelfAbsOffset-aAbsOffset;
	}
	
	_Self operator - (size_type op) const
	{
		return _Self(Deque,(Offset + Deque->Vector.size() - op)%Deque->Vector.size());
	}
	
	_Self operator + (size_type op) const
	{
		return _Self(Deque,(Offset + op)%Deque->Vector.size());
	}
	
	_Self & operator -= (size_type op)
	{
		Offset = (Offset + Deque->Vector.size() - op)%Deque->Vector.size();
		return *this;
	}
	
	_Self & operator += (size_type op)
	{
		Offset = (Offset + op)%Deque->Vector.size();
		return *this;
	}
	
	const_reference operator *() const
	{
		return Deque->Vector[Offset];
	}
	
	const_pointer operator ->() const
	{
		return &(Deque->Vector[Offset]);
	}
	
	_Self & operator ++()
	{
		Offset = (Offset + 1)%Deque->Vector.size();
		return *this;
	}
	_Self operator ++(int)
	{
		_Self __tmp = *this;
		Offset = (Offset + 1)%Deque->Vector.size();
		return __tmp;
	}
	_Self & operator --()
	{
		Offset = (Offset + Deque->Vector.size() - 1)%Deque->Vector.size();
		return *this;
	}
	_Self operator --(int)
	{
		_Self __tmp = *this;
		Offset = (Offset + Deque->Vector.size() - 1)%Deque->Vector.size();
		return __tmp;
	}
	bool operator == (const _Self &a) const
	{
		assert(Deque == a.Deque);
		return Offset == a.Offset;
	}
	bool operator != (const _Self &a) const
	{
		assert(Deque == a.Deque);
		return Offset != a.Offset;
	}

	bool operator < (const _Self &a) const
	{
		assert(Deque == a.Deque);
		return (a - (*this))>0;
	}
		
};

//! \addtogroup stlcont
//! \{

//! \brief A deque container
//!
//! It is an adaptor of the \c VectorType.
//! The implementation wraps the elements around 
//! the end of the \c VectorType circularly.
//! Template parameters:
//! - \c T the element type
//! - \c VectorType the type of the underlying vector container, 
//! the default is \c stxxl::vector<T>
template <class T, class VectorType = stxxl::vector<T> >
class deque
{
		typedef deque<T,VectorType>	Self_;
public:
		typedef typename VectorType::size_type size_type;
		typedef typename VectorType::difference_type difference_type;
		typedef VectorType	vector_type;
		typedef T  value_type;
		typedef T * pointer;
		typedef const value_type *const_pointer;
		typedef T & reference;
		typedef const T & const_reference;
		typedef deque_iterator<Self_> iterator;
		typedef const_deque_iterator<Self_> const_iterator;

		friend class deque_iterator<Self_>;
		friend class const_deque_iterator<Self_>;
private:
		VectorType Vector;
		size_type begin_o, end_o,size_;

		deque(const deque&)// Copying external deques is discouraged
                               					// (and not implemented)
    	{
      		STXXL_FORMAT_ERROR_MSG(msg,"deque::deque, stxxl::deque copy constructor is not implemented yet");
      		throw std::runtime_error(msg.str());
    	}
		
		deque& operator=(const deque&); // not implemented, and forbidden
		
		void double_array()
		{
				const size_type old_size = Vector.size();
				Vector.resize(2*old_size);
				if(begin_o > end_o)
				{ // copy data to the new end of the vector
					const size_type new_begin_o = old_size + begin_o;
					std::copy(	Vector.begin() + begin_o, 
										Vector.begin() + old_size, 
										Vector.begin() + new_begin_o);
					begin_o = new_begin_o;
				}
		}
public:
	
		deque(): Vector((STXXL_DEFAULT_BLOCK_SIZE(T))/sizeof(T)), begin_o(0),end_o(0),size_(0)
		{
		}
		
		deque(size_type n)
		: Vector(std::max((STXXL_DEFAULT_BLOCK_SIZE(T))/sizeof(T),2*n)), begin_o(0),end_o(n),size_(n)
		{
		}
		
		~deque()
		{ // empty so far
		}
		
		iterator begin()
		{
			return iterator(this,begin_o);
		}
		iterator end()
		{
			return iterator(this,end_o);
		}
		const_iterator begin() const
		{
			return const_iterator(this,begin_o);
		}
		const_iterator end() const
		{
			return const_iterator(this,end_o);
		}
		
		size_type size() const
		{
			return size_;
		}
		
		size_type max_size() const
		{
			return (std::numeric_limits<size_type>::max)()/2 - 1;
		}
		
		bool empty() const
		{
			return size_  == 0;
		}
		
		reference operator [] (size_type n)
		{
			assert(n < size());
			return Vector[(begin_o+n)%Vector.size()];
		}
		
		const_reference operator[](size_type n) const
		{
			assert(n < size());
			return Vector[(begin_o+n)%Vector.size()];
		}
		
		reference front()
		{
			assert(!empty());
			return Vector[begin_o];
		}
		
		const_reference front() const
		{
			assert(!empty());
			return Vector[begin_o];
		}
		
		reference back()
		{
			assert(!empty());
			return Vector[(end_o+Vector.size()-1)%Vector.size()];
		}
		
		const_reference back() const
		{
			assert(!empty());
			return Vector[(end_o+Vector.size()-1)%Vector.size()];
		}
		
		void push_front(const T& el)
		{
			
			if((begin_o+Vector.size()-1)%Vector.size() == end_o)
			{
				// an overflow will occur: resize the array
				double_array();
			}
			
			begin_o = (begin_o+Vector.size()-1)%Vector.size();
			Vector[begin_o] = el;
			++size_;
		}
		
		void push_back(const T& el)
		{
			
			if((end_o +1)%Vector.size() == begin_o)
			{
				// an overflow will occur: resize the array
				double_array();
			}
			Vector[end_o] = el;
			end_o = (end_o +1)%Vector.size();
			++size_;
		}
		
		void pop_front()
		{
			assert(!empty());
			begin_o = (begin_o + 1)%Vector.size();
			--size_;
		}
		
		void pop_back()
		{
			assert(!empty());
			end_o = (end_o + Vector.size() - 1)%Vector.size();
			--size_;
		}
		
		void swap(deque & obj)
		{
			std::swap(Vector,obj.Vector);
			std::swap(begin_o, obj.begin_o);
			std::swap(end_o,obj.end_o);
			std::swap(size_,obj.size_);
		}
		
		void clear()
		{
			Vector.clear();
			Vector.resize((STXXL_DEFAULT_BLOCK_SIZE(T))/sizeof(T));
			begin_o = 0;
			end_o = 0;
			size_ = 0;
		}
		
		void resize(size_type n)
		{
			if(n < size())
			{
				do
				{
					pop_back();
				} while(n < size());
			}
			else
			{
				if(n+1 > Vector.size())
				{ // need to resize
					const size_type old_size = Vector.size();
					Vector.resize(2*n);
					if(begin_o > end_o)
					{ // copy data to the new end of the vector
						const size_type new_begin_o = Vector.size() - old_size + begin_o;
						std::copy(	Vector.begin() + begin_o, 
										Vector.begin() + old_size, 
										Vector.begin() + new_begin_o);
						begin_o = new_begin_o;
					}
				}
				end_o = (end_o + n - size()) % Vector.size();
				size_ = n;
			}
		}
		
};

template <class T, class VectorType>
bool operator == (const deque<T,VectorType> & a, const deque<T,VectorType> & b)
{
	return std::equal(a.begin(),a.end(),b.begin());
}

template <class T, class VectorType>
bool operator < (const deque<T,VectorType> & a, const deque<T,VectorType> & b)
{
	return std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end());
}

//! \}

__STXXL_END_NAMESPACE

namespace std
{
	template < 
    	typename T,
    	typename VT >
	void swap( 	stxxl::deque<T,VT> & a,
							stxxl::deque<T,VT> & b )
	{
		a.swap(b);
	}
}



#endif /* _STXXL_DEQUE_H */
