/***************************************************************************
 *            iterator.h
 *
 *  Tue Feb 14 19:11:00 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#ifndef _BTREEITERATOR_H
#define _BTREEITERATOR_H


#include <stxxl>

__STXXL_BEGIN_NAMESPACE

// TODO get dirty flag support enabled

namespace btree
{
	template <class BTreeType>
	class iterator_map;
	template <class KeyType_, class DataType_, class KeyCmp_, unsigned LogNElem_, class BTreeType>
	class normal_leaf;
	
	
	template <class BTreeType>
	class btree_iterator_base
	{
		public:
			typedef BTreeType btree_type;
			typedef typename btree_type::leaf_bid_type bid_type;
			typedef typename btree_type::value_type value_type;
			typedef typename btree_type::reference reference;
			typedef typename btree_type::const_reference const_reference;
		
			friend class iterator_map<btree_type>;
			template <class KeyType_, class DataType_, 
							class KeyCmp_, unsigned LogNElem_, class BTreeType__>
			friend class normal_leaf;
		
		protected:
			btree_type * btree_;
			bid_type bid;
			unsigned pos;
			
			btree_iterator_base()
			{
				STXXL_VERBOSE3("btree_iterator_base def contruct addr="<<this);
				make_invalid();
			}
			
			btree_iterator_base(
				btree_type * btree__, 
				const bid_type & b, 
				unsigned p
				): btree_(btree__), bid(b), pos(p)
			{
				STXXL_VERBOSE3("btree_iterator_base parameter contruct addr="<<this);
				btree_->iterator_map_.register_iterator(*this);
			}
			
			void make_invalid()
			{
				btree_ = NULL;
			}
			
			btree_iterator_base( const btree_iterator_base & obj)
			{
				STXXL_VERBOSE3("btree_iterator_base constr from"<<(&obj)<<" to "<<this);
				btree_ = obj.btree_;
				bid = obj.bid;
				pos = obj.pos;
				if(btree_) btree_->iterator_map_.register_iterator(*this);
			}
			
			btree_iterator_base & operator = (const btree_iterator_base & obj)
			{
				STXXL_VERBOSE3("btree_iterator_base copy from"<<(&obj)<<" to "<<this);
				if(&obj != this)
				{
					if(btree_) btree_->iterator_map_.unregister_iterator(*this);
					btree_ = obj.btree_;
					bid = obj.bid;
					pos = obj.pos;
					if(btree_) btree_->iterator_map_.register_iterator(*this);
				}
				return *this;
			}
			
			reference non_const_access()
			{
				assert(btree_);
				typename btree_type::leaf_type *Leaf = btree_->leaf_cache_.get_node(bid);
				assert(Leaf);
				return (reference)((*Leaf)[pos]);
			}
			
			const_reference const_access() const
			{
				return non_const_access();
			}
	
		public:	
			virtual ~btree_iterator_base()
			{
				STXXL_VERBOSE3("btree_iterator_base deconst "<<this);
				if(btree_) btree_->iterator_map_.unregister_iterator(*this);
			}
	};
	
	template <class BTreeType>
	class btree_iterator: public btree_iterator_base<BTreeType>
	{
		public:
			
			typedef BTreeType btree_type;
			typedef typename btree_type::leaf_bid_type bid_type;
			typedef typename btree_type::value_type value_type;
			typedef typename btree_type::reference reference;
			typedef typename btree_type::const_reference const_reference;
			typedef typename btree_type::pointer pointer;
		
			template <class KeyType_, class DataType_, 
							class KeyCmp_, unsigned LogNElem_, class BTreeType__>
			friend class normal_leaf;
		
			using btree_iterator_base<btree_type>::non_const_access;
		
			btree_iterator(): btree_iterator_base<btree_type>()
			{}
				
			btree_iterator(const btree_iterator & obj):
				btree_iterator_base<btree_type>(obj)
			{
			}
			
			btree_iterator & operator = (const btree_iterator & obj)
			{
				btree_iterator_base<btree_type>::operator =(obj);
				return *this;
			}
			
			reference operator * ()
			{
				return non_const_access();
			}
			
			pointer operator -> ()
			{
				return &(non_const_access());
			}
			

		private:
			btree_iterator(
				btree_type * btree__, 
				const bid_type & b, 
				unsigned p
				): btree_iterator_base<btree_type>(btree__,b,p)
			{
			}
			
			
	};
	
	template <class BTreeType>
	class btree_const_iterator : public btree_iterator_base<BTreeType>
	{
		public:
			
	};
	
}

__STXXL_END_NAMESPACE

#endif /* _ITERATOR_H */
