/***************************************************************************
                          btree.h  -  description
                             -------------------
    begin                : Fre Apr 30 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file btree.h
//! \brief Implemets the intern struct B-Tree for \c stxxl::map container.

#ifndef __STXXL_BTREE_H
#define __STXXL_BTREE_H

#include <containers/vector.h>

#include "btnode.h"
#include "btree_iterator.h"
#include "btree_request.h"
#include "btree_queue.h"

__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	struct empty_btree_completion_handler
	{
		void operator() ( btree_request_base*, map_lock_base* p ) {}
	};

	//! \brief The realy implementation of B-Tree.
	/*!
	This class implements the B-Tree structure for extern memory.
	The implementation uses the class stxxl::map_internal::btnode as internal nodes and leafs in the tree.
	*/
	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	class btree : public ref_counter<btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_> >
	{
		typedef btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_> _Self;

	public:

		typedef btree_request<_Self>               request_type;
		typedef btree_queue<_Self>                 worker_queue_type;
		typedef BID<BlkSize_>                         bid_type;

		typedef stxxl::vector< std::pair<Key_,bid_type > > node_vector;
		typedef typename node_vector::iterator node_vector_iterator;

		typedef btnode<Key_,BID<BlkSize_>,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>  _Node;
		typedef btnode<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>       _Leaf;

		typedef async_btnode_manager<_Node,_Self,BlkSize_,PrefCacheBlkSize_> node_manager_type;
		typedef async_btnode_manager<_Leaf,_Self,BlkSize_,PrefCacheBlkSize_> leaf_manager_type;

		typedef leaf_manager_type                     leaf_manager;
		typedef node_manager_type                     node_manager;

		typedef smart_ptr<btnode<Key_,BID<BlkSize_>,Compare_,BlkSize_,unique_,PrefCacheBlkSize_> > node_ptr;
		typedef smart_ptr<btnode<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_> > leaf_ptr;

		typedef typename _Node::const_iterator	const_node_iterator;
		typedef typename _Node::iterator				node_iterator;
		typedef typename _Node::value_type			node_value_type;
		typedef typename _Node::store_type			node_store_type;
		typedef typename _Leaf::const_iterator	const_leaf_iterator;
		typedef typename _Leaf::iterator 				leaf_iterator;
		typedef typename _Leaf::key_type				key_type;
		typedef typename _Leaf::mapped_type			mapped_type;
		typedef typename _Leaf::value_type			value_type;
		typedef typename _Leaf::store_type			store_type;
		typedef typename _Leaf::key_compare			key_compare;
		typedef typename _Leaf::reference				reference;
		typedef typename _Leaf::const_reference const_reference;
		typedef typename _Leaf::size_type				size_type;
		typedef typename _Leaf::difference_type	difference_type;
		typedef typename _Leaf::pointer					pointer;
		typedef typename _Leaf::const_pointer		const_pointer;

		//typedef typename 	Leaf_::reverse_iterator				reverse_iterator;
		//typedef typename 	Leaf_::const_reverse_iterator 	const_reverse_iterator;

		typedef 	btree_iterator<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>      iterator;
		typedef 	const_btree_iterator<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_> const_iterator;

		static btree_queue<_Self> _btree_queue;
//	static async_btnode_menager<_Node,_Self>* _p_node_manager ;
//	static async_btnode_menager<_Leaf,_Self>* _p_leaf_manager ;


	static void MrProper()
	{
		// async_btnode_menager<_Node,_Self>::get_instance()->MrProper();
		// async_btnode_menager<_Leaf,_Self>::get_instance()->MrProper();
	}

	class value_compare
	{
		protected:

			Compare_ 	_comp;
			value_compare() {};
			value_compare(Compare_ comp) : _comp(comp) {};

		public:

			bool 		operator()	(const value_type& x, const value_type& y) const
			{
				return _comp(x.first, y.first);
			}

			bool 		operator()	(const key_type& x, const key_type& y) const
			{
				return _comp(x, y);
			}
			friend 		class btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>;
	};

	value_compare 		_value_compare;
	key_compare				key_comp() const 			{ return _value_compare._comp; }
	value_compare			value_comp() const 		{ return _value_compare; }

	private:

	leaf_ptr _sp_leaf_root;
	node_ptr _sp_node_root;

	unsigned			_height;
	size_type 		_tree_size;

	// ***************************************************************************
	// This function will be called only by bulk constructor.
	// The first level of B-tree will be created.
	// ***************************************************************************
	template <typename InputIterator_ >
	void _insert_into_empty_tree( InputIterator_ first, InputIterator_ last )
	{

		// *************************************************************************
		// create simply leaf
		// *************************************************************************
			if ( (size_type)(last - first) < _Leaf::MAX_SIZE() )
			{
				leaf_manager::get_instance()->GetNewNode( first, last, &_sp_leaf_root.ptr );
				_tree_size += _sp_leaf_root->size() ;
				return ;
			}

		// *************************************************************************
		// We becomes at least 2 leafs.
		// *************************************************************************

			node_vector insertvector;
			InputIterator_ help;

			smart_ptr<_Leaf> sp_node1;
			smart_ptr<_Leaf> sp_node2;
			BID<BlkSize_> bid;

		// *************************************************************************
		// Create leafs as long as entries gives.
		// *************************************************************************

			while( first != last )
			{

				// All entries fit in a container.
				if ( (size_type)( last - first ) <= _Leaf::MAX_SIZE() )
					help = last;

				// All entries fit in in two a container.
				// They must be distributed evenly.
				else if ( (size_type)( last - first ) < 2 * _Leaf::MAX_SIZE() )
					help = first + (last - first)/2 ;

				// There are entries for more than two leafs.
				else
					help = first + _Leaf::MAX_SIZE();

				// create a leaf
				leaf_manager::get_instance()->GetNewNode( first, help, &sp_node1.ptr );

				// Concatenate leafs with one another.
				if ( sp_node2.get() != NULL )
				{
					sp_node2->bid_next( sp_node1->bid() );
					sp_node1->bid_prev( sp_node2->bid() );
					sp_node2.release();
				}

				// Produce entry for the next level.
				Key_ key = (*sp_node1->begin()).first;
				bid = sp_node1->bid();
				std::pair<Key_,BID<BlkSize_> > mypair( key, bid );
				insertvector.push_back( mypair );

				// Increase the size of the B-Tree.
				_tree_size += sp_node1->size() ;

				// Go to the next step
				sp_node2 = sp_node1;
				sp_node1.release();
				first = help;
			}

			// Concatenate leafs with one another.
			if ( sp_node2.get() )
			{
				BID<BlkSize_> bid;
				sp_node2->bid_next( bid ) ;
				sp_node2.release();
			}

		// *************************************************************************
		// Go to the next level
		// *************************************************************************

			_height ++;
			_insert_into_empty_tree( insertvector.begin(), insertvector.end(), true );
	}

	// ***************************************************************************
	// This function will be called only by bulk constructor.
	// The second and follow levels of B-tree will be created.
	// the vector we used for inserting created by previous call.
	// ***************************************************************************
	void _insert_into_empty_tree( node_vector_iterator first, node_vector_iterator last, bool b_bid )
	{

		// *************************************************************************
		// create simply node
		// *************************************************************************

			if ( last - first > 0 && (size_type)(last - first) <= _Node::MAX_SIZE() )
			{
				node_manager::get_instance()->GetNewNode( first, last, &_sp_node_root.ptr );
				return ;
			}

		// *************************************************************************
		// We becomes at least 2 nodes.
		// *************************************************************************
			node_vector insertvector;
			node_vector_iterator help;
			smart_ptr<_Node> sp_node1;
			smart_ptr<_Node> sp_node2;
			BID<BlkSize_> bid;

			while( first != last )
			{
				// All entries fit in a container.
				if ( last - first > 0 && (size_type)(last - first) <= _Node::MAX_SIZE() )
					help = last;

				// All entries fit in in two a container.
				// They must be distributed evenly.
				else if ( (size_type)(last - first) < 2 * _Node::MAX_SIZE() )
					help = first + (last - first)/2 ;

				// There are entries for more than two leafs.
				else
					help = first + _Node::MAX_SIZE();


				// new node will be created
				node_manager::get_instance()->GetNewNode( first, help, &sp_node1.ptr );

				// Concatenate nodes with one another.
				if ( sp_node2.get() )
				{
					sp_node2->bid_next( sp_node1->bid() );
					sp_node1->bid_prev( sp_node2->bid() );
					sp_node2.release();
				}

				// Produce entry for the next level.
				Key_ key = (*sp_node1->begin()).first;
				bid = sp_node1->bid();
				std::pair<Key_,BID<BlkSize_> > mypair( key, bid );
				insertvector.push_back( mypair );

				// Go to next step
				sp_node2 = sp_node1;
				sp_node1.release();
				first = help;
			}

			// Concatenate nodes with one another.
			if ( sp_node2.get() )
			{
				BID<BlkSize_> bid;
				sp_node2->bid_next( bid );
				sp_node2.release();
			}

			// Go to next level
			_height ++;
			_insert_into_empty_tree( insertvector.begin(), insertvector.end(), true );
	}

	public:

	// **********************************************************************
	// An iterator to the first entry will be created.
	// **********************************************************************
	iterator begin()
	{
		// If B-Tree is empty return end
		if ( !_tree_size ) return end();

		// New iterator
		iterator iter( this );

		// If B-Tree don't contain internal nodes we are ready.
		if ( _height == 1 )
		{
			iter._leaf_iterator = _sp_leaf_root->begin();
			return iter;
		}

		// we create an entry for current level
		node_iterator n_iter = _sp_node_root->begin() ;
		const_node_iterator cn_iter = n_iter ;
		iter._node_iterators.push_back( n_iter ) ;

		// we go to the next level
		return _begin( iter, (*cn_iter).second, _height -1 ) ;
	}

	// **********************************************************************
	// _tree_size contins the right number of entries
	// **********************************************************************
	void asize( btrequest_size<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::SIZE );
		*param->_p_size = _tree_size;
		param->set_to( btree_request<_Self>::DONE );
	}

	/***************************************************************************************
	****************************************************************************************

	FIND functions

	****************************************************************************************
	***************************************************************************************/

public:

	// ***********************************************************************
	// main find function
	// ***********************************************************************
	bool afind( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::FIND );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
			if ( param->_height < 0 )
			{
				STXXL_ASSERT( param->_pp_iterator );
				STXXL_ASSERT( !*param->_pp_iterator );

				if ( !_tree_size )
				{
					*param->_pp_iterator = new iterator( end() );
					(*param->_pp_iterator)->add_ref();
					return true;
				}

				*param->_pp_iterator = new iterator( this );
				(*param->_pp_iterator)->add_ref();
				if ( _height == 1 ) param->_sp_cur_leaf = _sp_leaf_root;
				else param->_sp_cur_node = _sp_node_root;
				param->_height = _height;
			}

		// *********************************************************************
		// choose the current phase
		// *********************************************************************
			switch( param->_phase )
			{
			case btrequest_find<_Self>::PHASE_SEARCH :
				return _afind_search( param );
				break;

			case btrequest_find<_Self>::PHASE_MOVE_NEXT :
				return _afind_move_next( param );
				break;

			case btrequest_find<_Self>::PHASE_MOVE_PREV :
				return _afind_move_prev( param );
				break;

			default:
				STXXL_ASSERT( 0 );
				break;
			}
			return false;
	}

	// ***********************************************************************
	// main find function ( const case )
	// ***********************************************************************
	bool afind( btrequest_const_find<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_FIND );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
			if ( param->_height < 0 )
			{
				STXXL_ASSERT( param->_pp_const_iterator );
				STXXL_ASSERT( !*param->_pp_const_iterator );

				if ( !_tree_size )
				{
					*param->_pp_const_iterator = new const_iterator( end() );
					(*param->_pp_const_iterator)->add_ref();
					return true;
				}

				*param->_pp_const_iterator = new const_iterator( this );
				(*param->_pp_const_iterator)->add_ref();
				if ( _height == 1 ) param->_sp_cur_leaf = _sp_leaf_root;
				else param->_sp_cur_node = _sp_node_root;
				param->_height = _height;
			}

		// **********************************************************************
		// The current phase will be choosed
		// **********************************************************************

			switch( param->_phase )
			{
			case btrequest_const_find<_Self>::PHASE_SEARCH :
				return _afind_search( param );
				break;

			case btrequest_const_find<_Self>::PHASE_MOVE_NEXT :
				return _afind_move_next( param );
				break;

			default:
				STXXL_ASSERT ( 0 ); break;
			}
			return false;
	}

private:

	bool _afind_search( btrequest_search<_Self>* param );
	bool _afind_move_next( btrequest_search<_Self>* param );
	bool _afind_move_prev( btrequest_search<_Self>* param );
	bool _afind_search( btrequest_const_search<_Self>* param ) const;
	bool _afind_move_next( btrequest_const_search<_Self>* param ) const;

	/***************************************************************************************
	****************************************************************************************

	UPPER_BOUND

	****************************************************************************************
	***************************************************************************************/

public:

	bool aupper_bound( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::UPPER_BOUND ||
		// *********************************************************************
		// init params for the first call
		// *********************************************************************
			param->_type == btree_request<_Self>::EQUAL_RANGE );

		if ( param->_height < 0 )
		{
			STXXL_ASSERT( param->_pp_iterator );
			STXXL_ASSERT( !*param->_pp_iterator );

			if ( !_tree_size )
			{
				*param->_pp_iterator = new iterator( end() );
				(*param->_pp_iterator)->add_ref();
				return true;
			}

			*param->_pp_iterator = new iterator( this );
			(*param->_pp_iterator)->add_ref();
			if ( _height == 1 ) param->_sp_cur_leaf = _sp_leaf_root;
			else param->_sp_cur_node = _sp_node_root;
			param->_height = _height;
		}

		switch( param->_phase )
		{
			case btrequest_upper_bound<_Self>::PHASE_SEARCH:
				return _aupper_bound_search( param );
				break;

			case btrequest_upper_bound<_Self>::PHASE_MOVE_NEXT:
				return _aupper_bound_move_next( param );
				break;

			default:
				STXXL_ASSERT( 0 );
				return false;
				break;
		}
	}

	bool aupper_bound( btrequest_const_search<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_UPPER_BOUND ||
		// *********************************************************************
		// init params for the first call
		// *********************************************************************
			param->_type == btree_request<_Self>::CONST_EQUAL_RANGE );

		if ( param->_height < 0 )
		{
			STXXL_ASSERT( param->_pp_const_iterator );
			STXXL_ASSERT( !*param->_pp_const_iterator );

			if ( !_tree_size )
			{
				*param->_pp_const_iterator = new const_iterator( end() );
				(*param->_pp_const_iterator)->add_ref();
				return true;
			}

			*param->_pp_const_iterator = new const_iterator( this );
			(*param->_pp_const_iterator)->add_ref();
			if ( _height == 1 ) param->_sp_cur_leaf = _sp_leaf_root;
			else param->_sp_cur_node = _sp_node_root;
			param->_height = _height;
		}

		switch( param->_phase )
		{
			case btrequest_const_upper_bound<_Self>::PHASE_SEARCH:
				return _aupper_bound_search( param );
				break;

			case btrequest_const_upper_bound<_Self>::PHASE_MOVE_NEXT:
				return _aupper_bound_move_next( param );
				break;

			default:
				STXXL_ASSERT( 0 );
				return false;
				break;
		}
	}

private:

	bool _aupper_bound_search( btrequest_search<_Self>* param );
	bool _aupper_bound_move_next( btrequest_search<_Self>* param );
	bool _aupper_bound_search( btrequest_const_search<_Self>* param ) const;
	bool _aupper_bound_move_next( btrequest_const_search<_Self>* param ) const;

	/***************************************************************************************
	****************************************************************************************

	LOWER_BOUND

	****************************************************************************************
	***************************************************************************************/

public:
	bool alower_bound( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT(
			param->_type == btree_request<_Self>::LOWER_BOUND ||
			param->_type == btree_request<_Self>::EQUAL_RANGE
		);

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
		if ( param->_height < 0 )
		{
			STXXL_ASSERT( param->_pp_iterator );
      STXXL_ASSERT( !*param->_pp_iterator );

			if ( !_tree_size )
			{
				*param->_pp_iterator = new iterator( end() );
				(*param->_pp_iterator)->add_ref();
				return true;
			}

			*param->_pp_iterator = new iterator( this );
			(*param->_pp_iterator)->add_ref();
			if ( _height == 1 ) param->_sp_cur_leaf = _sp_leaf_root;
			else param->_sp_cur_node = _sp_node_root;
			param->_height = _height;
		}

		switch( param->_phase )
		{
			case btrequest_lower_bound<_Self>::PHASE_SEARCH:
				return _alower_bound_search( param );
				break;

			case btrequest_lower_bound<_Self>::PHASE_MOVE_NEXT:
				return _alower_bound_move_next( param );
				break;

			default:
				STXXL_ASSERT( 0 );
				return false;
				break;
		}
		
	}

	bool alower_bound( btrequest_const_search<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_LOWER_BOUND ||
			param->_type == btree_request<_Self>::CONST_EQUAL_RANGE );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
		if ( param->_height < 0 )
		{
			STXXL_ASSERT( param->_pp_const_iterator );
      STXXL_ASSERT( !*param->_pp_const_iterator );

			if ( !_tree_size )
			{
				*param->_pp_const_iterator = new const_iterator( end() );
				(*param->_pp_const_iterator)->add_ref();
				return true;
			}

			*param->_pp_const_iterator = new const_iterator( this );
			(*param->_pp_const_iterator)->add_ref();
			if ( _height == 1 ) param->_sp_cur_leaf = _sp_leaf_root;
			else param->_sp_cur_node = _sp_node_root;
			param->_height = _height;
		}

		switch( param->_phase )
		{
			case btrequest_const_lower_bound<_Self>::PHASE_SEARCH:
				return _alower_bound_search( param );
				break;

			case btrequest_const_lower_bound<_Self>::PHASE_MOVE_NEXT:
				return _alower_bound_move_next( param );
				break;

			default:
				STXXL_ASSERT( 0 );
				return false;
				break;
		}
	}

private:

	bool _alower_bound_search( btrequest_search<_Self>* param );
	bool _alower_bound_move_next( btrequest_search<_Self>* param );
	bool _alower_bound_search( btrequest_const_search<_Self>* param ) const;
	bool _alower_bound_move_next( btrequest_const_search<_Self>* param ) const;

	/***************************************************************************************
	****************************************************************************************

	ERASE INTERVAL

	****************************************************************************************
	***************************************************************************************/

public:

	void aerase( btrequest_erase_interval<_Self>* param )
	{

		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
					if ( param->_height < 0 )
					{
						if( *param->_p_first == end() || !_tree_size )
						{
							param->DisposeParam();
							param->set_to( btree_request<_Self>::DONE );
							return;
						}
						param->_sp_cur_leaf = param->_p_first->_leaf_iterator.node();
						param->_sp_cur_leaf2 = param->_p_last->_leaf_iterator.node();
						param->_height = 1;
					}


		// ******************************************************************
		//
		// ******************************************************************

					STXXL_ASSERT(
						(unsigned) param->_height <= _height ||
						btrequest_erase_interval<_Self>::CLEAR_UP == param->_phase );

					switch( param->_phase )
					{
						case btrequest_erase_interval<_Self>::PHASE_ERASE:
							_aerase_phase_erase(param);                           break;

						case btrequest_erase_interval<_Self>::REORDER_NODES:
							_aerase_phase_rebalance( param );                     break;

						case btrequest_erase_interval<_Self>::REORDER_WITH_NEXT:
							_aerase_phase_reorder_with_next( param );             break;

						case btrequest_erase_interval<_Self>::REORDER_WITH_PREV:
							_aerase_phase_reorder_with_prev( param );             break;

						case btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT_AFTER_REORDER:
							_aerase_phase_set_next_next_after_reorder( param );   break;

						case btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT:
							_aerase_phase_set_next_next( param );                 break;

						case btrequest_erase_interval<_Self>::CLEAR_UP:
							_aerase_phase_clear_up( param );                      break;

						case btrequest_erase_interval<_Self>::MOVE_NEXT_AFTER_REODER:
							_aerase_phase_move_next_after_reorder( param );       break;

						default: STXXL_ASSERT( 0 );
					}
	}

	void _aerase_phase_clear_up(btrequest_erase_interval<_Self>* param )
	{
		// *************************************************************************
		// we have delted the right leafs and nodes and we become a new root
		// other nodes have to be deleted.
		// WORNING: noe is the param->_height then count until to root
		// *************************************************************************

		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height );

		// only the current node is needed
		STXXL_ASSERT( !param->_sp_cur_leaf.get() );
		STXXL_ASSERT( !param->_sp_cur_leaf2.get() );
		STXXL_ASSERT( !param->_sp_cur_node2.get() );

		// *************************************************************************
		// current node have to be release and the next one will be find
		// *************************************************************************

					bid_type bid = param->_sp_cur_node->bid();              // store the bid

		// *********************************************
		// next node is Nil, take the parents
		// *********************************************

					if ( !param->_sp_cur_node->bid_next().valid() )
					{
						node_manager::get_instance()->ReleaseNode( bid );
						if ( --param->_height )
						{
							param->_sp_cur_node = param->_p_first->_node_iterators[param->_height -1].node();

							// this node have to be most left node in this height
							STXXL_ASSERT( !param->_sp_cur_node->bid_prev().valid() );

							aerase( param );
						}
						else
						{
							param->DisposeParam();
							param->set_to( btree_request<_Self>::DONE );
						}
						return;
					}

		// *********************************************
		// next node is not Nil
		// *********************************************

					bid_type next_bid = param->_sp_cur_node->bid_next();
					param->_sp_cur_node.release();
					node_manager::get_instance()->ReleaseNode( bid );
					node_manager::get_instance()->GetNode( next_bid, &param->_sp_cur_node.ptr, param );
	}


	void _aerase_phase_next( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );

		if ( ((unsigned) param->_height2) == _height )
		{
			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf2->begin();
		}
		else
		{
			(*param->_pp_iterator)->_node_iterators[param->_height2-1] = param->_sp_cur_node->begin();
		}

		smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[param->_height-2].node();
		if ( !(*param->_pp_iterator)->_node_iterators[param->_height-2].next() )
		{
			param->DisposeParam();
			sp_node.release();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		--param->_height;
		bid_type bid = sp_node->bid_next();
		//if ( !bid.valid() ) sp_node->dump( std::cout );
		STXXL_ASSERT( bid.valid() );
		param->_sp_cur_node.release();
		sp_node.release();
		node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
	}

	void _aerase_phase_erase_leaf( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height == 1 );

		if ( param->_sp_cur_leaf->bid() == param->_sp_cur_leaf2->bid() )
			_aerase_phase_erase_in_same_leaf( param );
		else
			_aerase_phase_erase_in_diff_leaf( param );
	}

	void _aerase_phase_erase_in_same_leaf( btrequest_erase_interval<_Self>* param )
	{

		// *************************************************************************
		// Leafs are the same
		// *************************************************************************
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height == 1 );
		STXXL_ASSERT( param->_sp_cur_leaf->bid() == param->_sp_cur_leaf2->bid() );
		STXXL_ASSERT( param->_sp_cur_leaf->bid() == param->_p_first->_leaf_iterator.node()->bid() );

		_tree_size -= param->_sp_cur_leaf->size();
		param->_sp_cur_leaf->erase( param->_p_first->_leaf_iterator, param->_p_last->_leaf_iterator );
		_tree_size += param->_sp_cur_leaf->size();

		param->_phase = btrequest_erase_interval<_Self>::REORDER_NODES;
		aerase( param );
		return;
	}

	void _aerase_phase_erase_in_diff_leaf( btrequest_erase_interval<_Self>* param )
	{

		// *************************************************************************
		// Leafs are not the same
		// *************************************************************************

		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height == 1 );
		STXXL_ASSERT( !(param->_sp_cur_leaf->bid() == param->_sp_cur_leaf2->bid()) );

		bid_type next_bid = param->_sp_cur_leaf->bid_next();
		bid_type bid = param->_sp_cur_leaf->bid();

		// *************************************************************************
		// if we are in the first leaf delete or release leafs
		// *************************************************************************

					if ( bid == param->_p_first->_leaf_iterator.node()->bid() )
					{
						_tree_size -= param->_sp_cur_leaf->size();
						param->_sp_cur_leaf->erase( param->_p_first->_leaf_iterator, param->_sp_cur_leaf->end() );
						_tree_size += param->_sp_cur_leaf->size();
					}
					else
					{
						_tree_size -= param->_sp_cur_leaf->size();
						leaf_manager::get_instance()->ReleaseNode( bid );
					}

		// *************************************************************************
		//
		// *************************************************************************

					if ( !next_bid.valid() )
					{
						bid_type bid;
						param->_sp_cur_leaf = param->_p_first->_leaf_iterator.node();
						param->_sp_cur_leaf->bid_next( bid );
						param->_sp_cur_leaf2 = &_Leaf::nil;
						param->_phase = btrequest_erase_interval<_Self>::REORDER_NODES;
						aerase( param );
					}

					else if ( next_bid == param->_p_last->_leaf_iterator.node()->bid() )
					{
						_tree_size -= param->_sp_cur_leaf2->size();
						param->_sp_cur_leaf2->erase( param->_sp_cur_leaf2->begin(), param->_p_last->_leaf_iterator );
						_tree_size += param->_sp_cur_leaf2->size();

						param->_sp_cur_leaf = param->_p_first->_leaf_iterator.node();
						param->_phase = btrequest_erase_interval<_Self>::REORDER_NODES;
						aerase( param );
					}
					else
					{
						param->_sp_cur_leaf.release();
						leaf_manager::get_instance()->GetNode( next_bid, &param->_sp_cur_leaf.ptr, param );
					}
	}


	void _aerase_phase_erase_node( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height > 1 );

		if ( param->_sp_cur_node->bid() == param->_sp_cur_node2->bid() )
			_aerase_phase_erase_in_same_node( param );
		else
			_aerase_phase_erase_in_diff_node( param );
	}


	void _aerase_phase_erase_in_same_node( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height > 1 );
		STXXL_ASSERT( param->_sp_cur_node->bid() == param->_sp_cur_node2->bid() );
		int ix = _height - param->_height;

		STXXL_ASSERT( param->_sp_cur_node->bid() == param->_p_first->_node_iterators[ix].node()->bid() );
		STXXL_ASSERT( param->_p_last->_node_iterators[ix].node()->end() != param->_p_last->_node_iterators[ix] );

				param->_p_last->_node_iterators[ix]++;
				param->_sp_cur_node->erase( param->_p_first->_node_iterators[ix], param->_p_last->_node_iterators[ix] );
				int count = param->_node_vector.size();
				int pos = 0;

				while( pos < count )
				{
					if( !param->_sp_cur_node->full() )
						param->_sp_cur_node->insert( param->_node_vector[pos] );
					else
						param->_sp_cur_node2->insert( param->_node_vector[pos] );
						pos++;
				}
				param->_node_vector.clear();
				param->_phase = btrequest_erase_interval<_Self>::REORDER_NODES;
				aerase( param );
	}

	void _aerase_phase_erase_in_diff_node( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height > 1 );
		STXXL_ASSERT( !(param->_sp_cur_node->bid() == param->_sp_cur_node2->bid()) );

					// the right index in the array with node_iterators
					unsigned ix = _height - param->_height;
					STXXL_ASSERT( param->_p_first->_node_iterators.size() > ix );

					bid_type next_bid = param->_sp_cur_node->bid_next();
					bid_type bid = param->_sp_cur_node->bid();

		// **************************************************************************************
		// delete the keys in currunt node if there is the first one
		// or release a complete node if it is the next one
		// **************************************************************************************

					if ( bid == param->_p_first->_node_iterators[ix].node()->bid() )
					{
						param->_sp_cur_node->erase( param->_p_first->_node_iterators[ix], param->_sp_cur_node->end() );
		    	}
					else
					{
						node_manager::get_instance()->ReleaseNode( bid );
					}

		// **************************************************************************************
		// if there is no next nodes reorder the nodes
		// **************************************************************************************

					if ( !next_bid.valid() )
					{
						bid_type bid;

						param->_sp_cur_node = param->_p_first->_node_iterators[ix].node();
						param->_sp_cur_node->bid_next( bid );

						int count = param->_node_vector.size();
						int pos = 0;

						while( pos < count )
						{
							if( !param->_sp_cur_node->full() )
								param->_sp_cur_node->insert( param->_node_vector[pos] );
							else
								param->_sp_cur_node2->insert( param->_node_vector[pos] );
							pos ++;
						}
						param->_node_vector.clear();

						param->_sp_cur_node2 = &_Node::nil;
						param->_phase = btrequest_erase_interval<_Self>::REORDER_NODES;
						aerase( param );
					}

		// ****************************************************************************
		// deletete in last node if the next is one or go to next node
		// ****************************************************************************

					else if( param->_p_last->_leaf_iterator != _Leaf::nil.end() &&
							next_bid == param->_p_last->_node_iterators[ix].node()->bid() )
					{
						param->_sp_cur_node = param->_p_first->_node_iterators[ix].node();
						param->_sp_cur_node2 = param->_p_last->_node_iterators[ix].node();

						param->_p_last->_node_iterators[ix]++;
						param->_sp_cur_node2->erase( param->_sp_cur_node2->begin(), param->_p_last->_node_iterators[ix] );
						int count = param->_node_vector.size();
						int pos = 0;

						while( pos < count )
						{
							if( !param->_sp_cur_node->full() )
								param->_sp_cur_node->insert( param->_node_vector[pos] );
							else
								param->_sp_cur_node2->insert( param->_node_vector[pos] );
							pos ++;
						}
						param->_node_vector.clear();
						param->_phase = btrequest_erase_interval<_Self>::REORDER_NODES;
						aerase( param );
					}

					else
					{
						param->_sp_cur_node.release();
						node_manager::get_instance()->GetNode( next_bid, &param->_sp_cur_node.ptr, param );
					}
	}

	void _aerase_phase_erase( btrequest_erase_interval<_Self>* param )
	{
		if ( param->_height == 1 )
			_aerase_phase_erase_leaf( param );
		else
			_aerase_phase_erase_node( param );
	}

	void _aerase_phase_rebalance( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		if ( param->_height == 1 )
			_aerase_phase_rebalance_leaf( param );
		else
			_aerase_phase_rebalance_node( param );
	}

	void _aerase_phase_rebalance_leaf( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height == 1 );

		if ( !param->_sp_cur_leaf2->bid().valid() )
			_aerase_phase_rebalance_leaf_end( param );
		else if ( param->_sp_cur_leaf->bid() == param->_sp_cur_leaf2->bid() )
			_aerase_phase_rebalance_leaf_same_leaf( param );
		else
			_aerase_phase_rebalance_leaf_different_leaf( param );
	}

	void _aerase_phase_rebalance_leaf_end( btrequest_erase_interval<_Self>* param )
	{
		// ******************************************************************
		// we are deleting a interval XX - end
		// ******************************************************************

		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height == 1 );
		STXXL_ASSERT( !param->_sp_cur_leaf2->bid().valid() );

		bid_type bid;
		param->_sp_cur_leaf->bid_next( bid );

		// ******************************************************************
		// leaf has less then half entries
		// OR
		// the are only entries for one leaf - we become new root
		// *******************************************************************

				if ( param->_sp_cur_leaf->less_half() || !param->_sp_cur_leaf->bid_prev().valid() )
				{
					param->_sp_cur_leaf2.release();

					// ******************************************************************
					// we can rebalance the leaf with the previos leaf
					// ******************************************************************

					if( param->_sp_cur_leaf->bid_prev().valid() )
					{
						STXXL_ASSERT( param->_sp_cur_leaf->less_half() );
						bid = param->_sp_cur_leaf->bid_prev();
						param->_phase = btrequest_erase_interval<_Self>::REORDER_WITH_PREV;
						param->_sp_cur_leaf2.release();
						leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf2.ptr, param );
						return;
					}


					// ******************************************************************
					// we can clear up no other node exists
					// ******************************************************************

					else
					{
						param->_height = _height - param->_height;
						_height = 1;
						_sp_node_root.release();
						_sp_leaf_root = param->_sp_cur_leaf;

						if ( param->_height )
						{
							param->_phase = btrequest_erase_interval<_Self>::CLEAR_UP;
							param->_sp_cur_node = param->_p_first->_node_iterators[param->_height-1].node();
							param->_sp_cur_node2.release();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							aerase( param );
							return;
						}
						else
						{
							param->DisposeParam();
							param->set_to( btree_request<_Self>::DONE );
							return;
						}
					}
				}


		// *****************************************************************
		// leaf has a right size
    // *****************************************************************

				STXXL_ASSERT( !param->_sp_cur_leaf->less_half() );


				/*
				-- it doesn't wort if we delete the enties in the last two leafs

				const_leaf_iterator cl_iter = param->_p_first->_leaf_iterator;
				_replace_key( *param->_p_first, 1, (*cl_iter).first );
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );*/


				key_type key = param->_sp_cur_leaf->begin()->first;
				bid = param->_sp_cur_leaf->bid();
				param->_node_vector.push_back( node_store_type(key,bid) );

				param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
				param->_sp_cur_leaf.release();
				param->_sp_cur_leaf2.release();
				param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height-1].node();
				if( param->_p_last->_leaf_iterator != _Leaf::nil.end() )
					param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height-1].node();
				else
					param->_sp_cur_node2 = &_Node::nil ;
				param->_height++;
				aerase( param );
	}

	void _aerase_phase_rebalance_leaf_same_leaf( btrequest_erase_interval<_Self>* param )
	{
		// ******************************************************************
		// begin and end are in the same leaf
		// ******************************************************************

		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height == 1 );
		STXXL_ASSERT( param->_sp_cur_leaf2->bid().valid() );
    STXXL_ASSERT( param->_sp_cur_leaf->bid() == param->_sp_cur_leaf2->bid() );

		// ******************************************************************
		// leaf has less then half entries
		// ******************************************************************

				if ( param->_sp_cur_leaf->less_half() )
				{
					bid_type bid ;
					param->_sp_cur_leaf2.release();

					// ******************************************************************
					// we can rebalance the leaf with the previos leaf
					// ******************************************************************

					if( param->_sp_cur_leaf->bid_prev().valid() )
					{
						bid = param->_sp_cur_leaf->bid_prev();
						param->_phase = btrequest_erase_interval<_Self>::REORDER_WITH_PREV;
						leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf2.ptr, param );
						return;
					}

					// ******************************************************************
					// we can rebalance the leaf with the next leaf
					// ******************************************************************

					else if(  param->_sp_cur_leaf->bid_next().valid() )
					{
						param->_sp_cur_node2.release();
						param->_height2 = param->_height + 1;
						param->_phase = btrequest_erase_interval<_Self>::MOVE_NEXT_AFTER_REODER;
						aerase( param );
						return;
					}

					// ******************************************************************
					// we can clear up no other node exists
					// ******************************************************************

					param->_height = _height - param->_height;
					_height = 1;
					_sp_node_root.release();
					_sp_leaf_root = param->_sp_cur_leaf;

					if ( param->_height )
					{
						param->_phase = btrequest_erase_interval<_Self>::CLEAR_UP;
						param->_sp_cur_node = param->_p_first->_node_iterators[param->_height-1].node();
						param->_sp_cur_node2.release();
						param->_sp_cur_leaf.release();
						param->_sp_cur_leaf2.release();
						aerase( param );
					}
					else
					{
						param->DisposeParam();
						param->set_to( btree_request<_Self>::DONE );
					}
					return;
				}

		// *****************************************************************
		// leaf has a right size
    // *****************************************************************

				const_leaf_iterator cl_iter = param->_p_first->_leaf_iterator;
				_replace_key( *param->_p_first, 1, (*cl_iter).first );
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
	}

	void _aerase_phase_rebalance_leaf_different_leaf( btrequest_erase_interval<_Self>* param )
	{
		// ******************************************************************
		// begin and end are in the diferent leafs
		// ******************************************************************

		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height == 1 );
		STXXL_ASSERT( param->_sp_cur_leaf2->bid().valid() );
    STXXL_ASSERT( !(param->_sp_cur_leaf->bid() == param->_sp_cur_leaf2->bid() ));


		// *********************************************************************************
		// in both leafs are no more entries then the maximum posible nummber of entiries
		// *********************************************************************************

				if ( 	param->_sp_cur_leaf->size() + param->_sp_cur_leaf2->size()
							<= param->_sp_cur_leaf->max_size() )
				{
					bid_type next_next_bid = param->_sp_cur_leaf2->bid_next();
					bid_type bid = param->_sp_cur_leaf2->bid();

					param->_sp_cur_leaf->bid_next( next_next_bid );
					param->_sp_cur_leaf->merge( *param->_sp_cur_leaf2 );
					param->_sp_cur_leaf2.release();
					leaf_manager::get_instance()->ReleaseNode( bid ) ;

					param->_phase = btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT_AFTER_REORDER;

					if ( next_next_bid.valid() )
					{
						param->_bid = param->_sp_cur_leaf->bid();
						leaf_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_leaf2.ptr, param ) ;
						return;
					}
					else
					{
						param->_bid = bid_type();
						aerase( param );
						return;
					}
				}

		// *********************************************************************************
		// in one of both leaf are not enought entries,
		// but there are enought entries for two leafs
		// *********************************************************************************

				else if ( param->_sp_cur_leaf->less_half() )
				{
					param->_sp_cur_leaf->steal_right( *param->_sp_cur_leaf2 );
				}
				else if(  param->_sp_cur_leaf2->less_half() )
				{
					param->_sp_cur_leaf2->steal_left( *param->_sp_cur_leaf );
				}

		// *********************************************************************************
		// now in both leaf are enought entries
		// *********************************************************************************

				key_type key = param->_sp_cur_leaf->begin()->first;
				bid_type bid = param->_sp_cur_leaf->bid();
				param->_sp_cur_leaf2->bid_prev( bid );
				param->_node_vector.push_back( node_store_type(key,bid) );

				key = param->_sp_cur_leaf2->begin()->first;
				bid = param->_sp_cur_leaf2->bid();
				param->_sp_cur_leaf->bid_next( bid );
				param->_node_vector.push_back( node_store_type(key,bid) );

				param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
				param->_sp_cur_leaf.release();
				param->_sp_cur_leaf2.release();
				param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height-1].node();
				param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height-1].node();
				param->_height++;
				aerase( param );
	}

	void _aerase_phase_rebalance_node( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height > 1 );

		if ( !param->_sp_cur_node2->bid().valid() )
			_aerase_phase_rebalance_node_end( param );
		else if ( param->_sp_cur_node->bid() == param->_sp_cur_node2->bid() )
			_aerase_phase_rebalance_node_same_node( param );
		else
			_aerase_phase_rebalance_node_different_node( param );
	}

	void _aerase_phase_rebalance_node_end( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height > 1 );
		STXXL_ASSERT( !param->_sp_cur_node2->bid().valid() );

		bid_type bid;
		param->_sp_cur_node->bid_next( bid );

		// **********************************************************************
		// curent node has not got enoght entries
		// OR
		// there are no other nodes
		// **********************************************************************


				if ( param->_sp_cur_node->less_half() || !param->_sp_cur_node->bid_prev().valid() )
				{
					param->_sp_cur_node2.release();

					// ****************************************************************
					// we can rebalance current node with previous node
					// ****************************************************************

					if( param->_sp_cur_node->bid_prev().valid() )
					{

						bid = param->_sp_cur_node->bid_prev();
						param->_phase = btrequest_erase_interval<_Self>::REORDER_WITH_PREV;
						node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node2.ptr, param );
					}

					// ****************************************************************
					// the are no other node - we can clean up
					// ****************************************************************

					else
					{
						int neu_height = param->_height;
						param->_height = _height - param->_height;
						_height = neu_height;
						_sp_node_root = param->_sp_cur_node;

						if ( param->_height )
						{
							param->_phase = btrequest_erase_interval<_Self>::CLEAR_UP;
							param->_sp_cur_node = param->_p_first->_node_iterators[param->_height-1].node();
							param->_sp_cur_node2.release();
							aerase( param );
						}
						else
						{
							param->DisposeParam();
							param->set_to( btree_request<_Self>::DONE );
						}
					}

					return;
				}

		// **********************************************************************
		// curent node has got enoght entries
		// **********************************************************************

				/*STXXL_ASSERT( !param->_sp_cur_node->less_half() );
				const_node_iterator cl_iter = param->_p_first->_node_iterators[param->_height];
				_replace_key( *param->_p_first, param->_height, (*cl_iter).first );
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				it dusn't wor for the last leaf
				*/


				if( _height - param->_height == 0 )
				{
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
					return;
				}

				STXXL_ASSERT( !param->_sp_cur_node->less_half() );
				key_type key = param->_sp_cur_node->begin()->first;
				bid = param->_sp_cur_node->bid();
				param->_node_vector.push_back( node_store_type(key,bid) );

				param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
				param->_sp_cur_node.release();
				param->_sp_cur_node2.release();
				param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height-1].node();
				if( param->_p_last->_leaf_iterator != _Leaf::nil.end() )
					param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height-1].node();
				else
					param->_sp_cur_node2 = &_Node::nil ;
				param->_height++;
				aerase( param );
	}

	void _aerase_phase_rebalance_node_same_node( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height > 1 );
		STXXL_ASSERT( param->_sp_cur_node2->bid().valid() );
		STXXL_ASSERT( param->_sp_cur_node->bid() == param->_sp_cur_node2->bid() );

				if ( param->_sp_cur_node->less_half() )
				{
					bid_type bid ;
					param->_sp_cur_node2.release();

					if( param->_sp_cur_node->bid_prev().valid() )
					{
						bid = param->_sp_cur_node->bid_prev();
						param->_phase = btrequest_erase_interval<_Self>::REORDER_WITH_PREV;
						node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node2.ptr, param );
						return;
					}

					else if(  param->_sp_cur_node->bid_next().valid() )
					{
						param->_sp_cur_node2.release();
						param->_height2 = param->_height + 1;
						param->_phase = btrequest_erase_interval<_Self>::MOVE_NEXT_AFTER_REODER;
						aerase( param );
						return;
					}

        	else
					{
						if ( param->_sp_cur_node->bid() == _sp_node_root->bid() );
						else
						{
							int neu_height = param->_height;
							param->_height = _height - param->_height;
							_height = neu_height;
							_sp_node_root = param->_sp_cur_node;

							if ( param->_height )
							{
								param->_phase = btrequest_erase_interval<_Self>::CLEAR_UP;
								param->_sp_cur_node = param->_p_first->_node_iterators[param->_height-1].node();
								param->_sp_cur_node2.release();
								param->_sp_cur_leaf.release();
								param->_sp_cur_leaf2.release();
								aerase( param );
								return;
							}
						}
						param->DisposeParam();
						param->set_to( btree_request<_Self>::DONE );
						return;
					}
				}

		// **************************************************************************
		// node has got enought entries
		// **************************************************************************

				const_node_iterator cn_iter = param->_p_first->_node_iterators[_height-param->_height];
				_replace_key( *param->_p_first, param->_height, (*cn_iter).first );
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
	}

	void _aerase_phase_rebalance_node_different_node( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height > 1 );
		STXXL_ASSERT( param->_sp_cur_node2->bid().valid() );
		STXXL_ASSERT( !(param->_sp_cur_node->bid() == param->_sp_cur_node2->bid() ));

		// ***************************************************************************
		// both nodes have got not enought entries,
		// we merge the nodes
		// ***************************************************************************

				if ( param->_sp_cur_node->size() + param->_sp_cur_node2->size() <= param->_sp_cur_node->max_size() )
				{
					bid_type next_next_bid = param->_sp_cur_node2->bid_next();
					bid_type bid = param->_sp_cur_node2->bid();

					param->_sp_cur_node->bid_next( next_next_bid );
					param->_sp_cur_node->merge( *param->_sp_cur_node2 );
					param->_sp_cur_node2.release();
					node_manager::get_instance()->ReleaseNode( bid ) ;

					param->_phase = btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT_AFTER_REORDER;

					if ( next_next_bid.valid() )
					{
						param->_bid = param->_sp_cur_node->bid();
						node_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_node2.ptr, param ) ;
					}
					else
					{
						param->_bid = bid_type();
						aerase( param );
					}
					return;
				}

		// **************************************************************
		// one of nodes has got not enoght entries,
		// but there exists enoght nodes for two nodes
		// **************************************************************

				if ( param->_sp_cur_node->less_half() )
				{
					param->_sp_cur_node->steal_right( *param->_sp_cur_node2 );
				}
				else if ( param->_sp_cur_node2->less_half() )
				{
					param->_sp_cur_node2->steal_left( *param->_sp_cur_node );
				}

		// **************************************************************
		// now both nodes has anought entries
		// **************************************************************

				key_type key = param->_sp_cur_node->begin()->first;
				bid_type bid = param->_sp_cur_node->bid();
				param->_sp_cur_node2->bid_prev( bid );
				param->_node_vector.push_back( node_store_type(key,bid) );

				key = param->_sp_cur_node2->begin()->first;
				bid = param->_sp_cur_node2->bid();
				param->_sp_cur_node->bid_next( bid );
				param->_node_vector.push_back( node_store_type(key,bid) );

				param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
				param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height-1].node();
				param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height-1].node();
				param->_height++;
				aerase( param );
	}

	void _aerase_phase_move_next_after_reorder( btrequest_erase_interval<_Self>* param )
	{

		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );

		// **************************************************************************
		// this function will be called only if there is no previus node
		// we delere at the begining of tree
		// **************************************************************************

		if ( param->_sp_cur_node2.get() )
		{
			param->_p_last->_node_iterators[_height-param->_height2] = param->_sp_cur_node2->begin();
			param->_sp_cur_node2.release();
			param->_height2++;
		}

		bid_type bid = param->_p_last->_node_iterators[_height-param->_height2].node()->bid_next();

		if ( !param->_p_last->_node_iterators[_height-param->_height2].next() )
		{
			if( param->_height == 1 )
			{
				STXXL_ASSERT(  param->_sp_cur_leaf->bid_next().valid() );
				bid_type bid = param->_sp_cur_leaf->bid_next();
				param->_phase = btrequest_erase_interval<_Self>::REORDER_WITH_NEXT;
				param->_sp_cur_leaf2.release();
				leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf2.ptr, param );
				return;
			}
			else
			{
				STXXL_ASSERT(  param->_sp_cur_node->bid_next().valid() );
				bid_type bid = param->_sp_cur_node->bid_next();
				param->_phase = btrequest_erase_interval<_Self>::REORDER_WITH_NEXT;
				node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node2.ptr, param );
				return;
			}
		}

		STXXL_ASSERT( bid.valid() );
		param->_sp_cur_node2.release();
		node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node2.ptr, param );
	}

	void _aerase_phase_set_next_next_after_reorder( btrequest_erase_interval<_Self>* param )
  {
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_phase == btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT_AFTER_REORDER );

		if( param->_height == 1 )
			_aerase_phase_set_next_next_after_reorder_leaf( param );
		else
			_aerase_phase_set_next_next_after_reorder_node( param );
	}

	void _aerase_phase_set_next_next_after_reorder_leaf( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_phase == btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT_AFTER_REORDER );
		STXXL_ASSERT( param->_height == 1 );

		bid_type bid;

		if( param->_sp_cur_leaf2.get() )
		{
			param->_sp_cur_leaf2->bid_prev( param->_bid );
			param->_sp_cur_leaf2->set_dirty( true );
			param->_sp_cur_leaf2.release();
		}

		if ( param->_sp_cur_leaf->less_half() )
		{
			if( param->_sp_cur_leaf->bid_prev().valid() )
			{
				bid = param->_sp_cur_leaf->bid_prev();
				param->_phase = btrequest_erase_interval<_Self>::REORDER_WITH_PREV;
				leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf2.ptr, param );
				return;
			}
			else if(  param->_sp_cur_leaf->bid_next().valid() )
			{
				param->_sp_cur_node2.release();
				param->_height2 = param->_height + 1;
				param->_phase = btrequest_erase_interval<_Self>::MOVE_NEXT_AFTER_REODER;
				aerase( param );
				return;
			}

			STXXL_ASSERT( !param->_sp_cur_leaf->bid_prev().valid() );
			STXXL_ASSERT( !param->_sp_cur_leaf->bid_next().valid() );

			param->_height = _height - param->_height;
			_height = 1;

			_sp_node_root.release();
			_sp_leaf_root = param->_sp_cur_leaf;

			if ( param->_height )
			{
				param->_phase = btrequest_erase_interval<_Self>::CLEAR_UP;
				param->_sp_cur_node = param->_p_first->_node_iterators[param->_height-1].node();
				param->_sp_cur_node2.release();
				param->_sp_cur_leaf.release();
				param->_sp_cur_leaf2.release();
				aerase( param );
				return;
			}
			else
			{
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}
		}
		else
		{
			key_type key = param->_sp_cur_leaf->begin()->first;
			bid_type bid = param->_sp_cur_leaf->bid();
			param->_node_vector.push_back( node_store_type(key,bid) );
			param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
			param->_sp_cur_leaf.release();
			param->_sp_cur_leaf2.release();
			param->_sp_cur_node = param->_p_first->_node_iterators[ _height - param->_height -1].node();
			param->_sp_cur_node2 = param->_p_last->_node_iterators[ _height - param->_height -1].node();
			param->_height++;
			aerase( param );
			return;
		}
	}


	void _aerase_phase_set_next_next_after_reorder_node( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_phase == btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT_AFTER_REORDER );
		STXXL_ASSERT( param->_height > 1 );

		bid_type bid;

		if( param->_sp_cur_node2.get() )
		{
			param->_sp_cur_node2->bid_prev( param->_bid );
			param->_sp_cur_node2->set_dirty( true );
			param->_sp_cur_node2.release();
		}

		if ( param->_sp_cur_node->less_half() )
		{
			if( param->_sp_cur_node->bid_prev().valid() )
			{
				bid = param->_sp_cur_node->bid_prev();
				param->_phase = btrequest_erase_interval<_Self>::REORDER_WITH_PREV;
				node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node2.ptr, param );
				return;
			}
			else if(  param->_sp_cur_node->bid_next().valid() )
			{
				param->_sp_cur_node2.release();
				param->_height2 = param->_height + 1;
				param->_phase = btrequest_erase_interval<_Self>::MOVE_NEXT_AFTER_REODER;
				aerase( param );
				return;
			}

			STXXL_ASSERT( !param->_sp_cur_node->bid_prev().valid() );
			STXXL_ASSERT( !param->_sp_cur_node->bid_next().valid() );

			int help = param->_height;
			param->_height = _height - param->_height;
			_height = help ;

			_sp_node_root = param->_sp_cur_node;
			STXXL_ASSERT( !_sp_leaf_root.get() );

			if ( param->_height )
			{
				param->_phase = btrequest_erase_interval<_Self>::CLEAR_UP;

				STXXL_ASSERT( param->_height <= (int) param->_p_first->_node_iterators.size() );
				STXXL_ASSERT( param->_height > 0 );

				param->_sp_cur_node = param->_p_first->_node_iterators[param->_height-1].node();
				param->_sp_cur_node2.release();
				param->_sp_cur_leaf.release();
				param->_sp_cur_leaf2.release();
				aerase( param );
				return;
			}
			else
			{
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}
		}
		else
		{
			key_type key = param->_sp_cur_node->begin()->first;
			bid_type bid = param->_sp_cur_node->bid();
			param->_node_vector.push_back( node_store_type(key,bid) );

			param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
			param->_sp_cur_leaf.release();
			param->_sp_cur_leaf2.release();

			STXXL_ASSERT( _height-param->_height-1 < param->_p_first->_node_iterators.size() );
			STXXL_ASSERT( _height-param->_height-1 < param->_p_last->_node_iterators.size() );
			STXXL_ASSERT( _height-param->_height-1 >= 0 );

			param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height-1].node();
			param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height-1].node();
			param->_height++;
			aerase( param );
			return;
		}
	}


	void _aerase_phase_reorder_with_prev( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );

		if( param->_height == 1 )
			_aerase_phase_reorder_with_prev_leaf( param );
		else
			_aerase_phase_reorder_with_prev_node( param );
	}


	void _aerase_phase_reorder_with_prev_leaf( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height == 1 );


				if ( param->_sp_cur_leaf->size() + param->_sp_cur_leaf2->size() <= param->_sp_cur_leaf->max_size() )
				{
					bid_type next_next_bid = param->_sp_cur_leaf->bid_next();
					param->_sp_cur_leaf2->merge( *param->_sp_cur_leaf );
					param->_sp_cur_leaf2->bid_next( next_next_bid );
					leaf_manager::get_instance()->ReleaseNode( param->_sp_cur_leaf->bid() ) ;
					param->_phase = btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT;

					if ( next_next_bid.valid() )
					{
						param->_bid = param->_sp_cur_leaf2->bid();
						param->_sp_cur_leaf.release();
						param->_sp_cur_leaf2.release();
						leaf_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_leaf2.ptr, param ) ;
						return;
					}
					else
					{
						param->_bid = bid_type();
						param->_sp_cur_leaf.release();
						param->_sp_cur_leaf2.release();
						aerase( param );
						return;
					}
				}
				else
				{
					STXXL_ASSERT( param->_sp_cur_leaf->less_half() || param->_sp_cur_leaf2->less_half() );
					param->_sp_cur_leaf->steal_left( *param->_sp_cur_leaf2 );

					key_type key = param->_sp_cur_leaf->begin()->first;
					bid_type bid = param->_sp_cur_leaf->bid();
					param->_node_vector.push_back( node_store_type(key,bid) );

					param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
					param->_sp_cur_leaf.release();
					param->_sp_cur_leaf2.release();
					param->_height++;

					if( _height-param->_height >= 0 )
					{
						param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height].node();
						if( param->_p_last->_node_iterators.size() != 0 )
							param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height].node();
						else
							param->_sp_cur_node2 = &_Node::nil;
						aerase( param );
						return;
					}
					else
					{
						param->DisposeParam();
						param->set_to( btree_request<_Self>::DONE );
						return;
					}
				}
	}

	void _aerase_phase_reorder_with_prev_node( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );
		STXXL_ASSERT( param->_height > 1 );

		// ***************************************************************************************
		// two nodes can be meged
		// ***************************************************************************************
					if ( param->_sp_cur_node->size() + param->_sp_cur_node2->size() <= param->_sp_cur_node->max_size() )
					{
						bid_type next_next_bid = param->_sp_cur_node->bid_next() ;
						param->_sp_cur_node2->merge( *param->_sp_cur_node ) ;
						param->_sp_cur_node2->bid_next( next_next_bid ) ;

						node_manager::get_instance()->ReleaseNode( param->_sp_cur_node->bid() ) ;
						param->_sp_cur_node.release() ;
						param->_phase = btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT ;

						if ( next_next_bid.valid() )
						{
							param->_bid = param->_sp_cur_node2->bid();
							param->_sp_cur_node2.release() ;
							node_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_node2.ptr, param ) ;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_node2.release() ;
							aerase( param );
						}
					}


		// ******************************************************************************************
		// the nodes have got together to much keys but on have less half nodes
		// ******************************************************************************************

					else
					{
						STXXL_ASSERT( param->_sp_cur_node->less_half() || param->_sp_cur_node2->less_half() );
						param->_sp_cur_node->steal_left( *param->_sp_cur_node2 );
						key_type key = param->_sp_cur_node->begin()->first;
						bid_type bid = param->_sp_cur_node->bid();
						param->_node_vector.push_back( node_store_type(key,bid) );
						param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
						param->_sp_cur_node.release();
						param->_sp_cur_node2.release();
						param->_height++;

						if ( _height-param->_height >=  0)
						{
							param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height].node();
							if( param->_p_last->_node_iterators.size() != 0 )
								param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height].node();
							else
								param->_sp_cur_node2 = &_Node::nil;
							aerase( param );
						}
						else
						{
							param->DisposeParam();
							param->set_to( btree_request<_Self>::DONE );
						}
					}
	}

	void _aerase_phase_reorder_with_next( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );

//		STXXL_MSG( "" );
//		STXXL_MSG( "" );
//		STXXL_MSG( "PHASE REORDER WITH NEXT" );
		if( param->_height == 1 )
		{
			if ( param->_sp_cur_leaf->size() + param->_sp_cur_leaf2->size() <= param->_sp_cur_leaf->max_size() )
			{
//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "MERGE" );
//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "befor merge" );
//				param->_sp_cur_leaf->dump( std::cout );
//				param->_sp_cur_leaf2->dump( std::cout );

				bid_type next_next_bid = param->_sp_cur_leaf2->bid_next();
				param->_sp_cur_leaf->merge( *param->_sp_cur_leaf2 );
				param->_sp_cur_leaf->bid_next( next_next_bid );
				leaf_manager::get_instance()->ReleaseNode( param->_sp_cur_leaf2->bid() ) ;
				param->_phase = btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT;

				key_type key = param->_sp_cur_leaf->begin()->first;
				bid_type bid = param->_sp_cur_leaf->bid();
				param->_node_vector.push_back( node_store_type(key,bid) );

//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "after merge" );
//				param->_sp_cur_leaf->dump( std::cout );
//				STXXL_MSG( "******************************************" );

				if ( next_next_bid.valid() )
				{
					param->_bid = param->_sp_cur_leaf->bid();
					param->_sp_cur_leaf.release();
					param->_sp_cur_leaf2.release();
					leaf_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_leaf2.ptr, param ) ;
					return;
				}
				else
				{
					param->_bid = bid_type();
					param->_sp_cur_leaf.release();
					param->_sp_cur_leaf2.release();
					aerase( param );
					return;
				}
			}
			else // steal
			{
				STXXL_ASSERT( param->_sp_cur_leaf->less_half() || param->_sp_cur_leaf2->less_half() );

//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "STEAL" );
//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "befor steal" );
//				param->_sp_cur_leaf->dump( std::cout );
//				param->_sp_cur_leaf2->dump( std::cout );

				param->_sp_cur_leaf->steal_right( *param->_sp_cur_leaf2 );
				key_type key = param->_sp_cur_leaf->begin()->first;
				bid_type bid = param->_sp_cur_leaf->bid();
				param->_node_vector.push_back( node_store_type(key,bid) );

				key = param->_sp_cur_leaf2->begin()->first;
				bid = param->_sp_cur_leaf2->bid();
				param->_node_vector.push_back( node_store_type(key,bid) );

//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "after steal" );
//				param->_sp_cur_leaf->dump( std::cout );
//				param->_sp_cur_leaf2->dump( std::cout );
//				STXXL_MSG( "******************************************" );

				param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
				param->_sp_cur_leaf.release();
				param->_sp_cur_leaf2.release();
				param->_height++;
				if( _height-param->_height >= 0 )
				{
					param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height].node();
					if( param->_p_last->_node_iterators.size() != 0 )
						param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height].node();
					else
						param->_sp_cur_node2 = &_Node::nil;
					aerase( param );
					return;
				}
				else
				{
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
					return;
				}
			}
		}
		else
		{
			if ( param->_sp_cur_node->size() + param->_sp_cur_node2->size() <= param->_sp_cur_node->max_size() )
			{
//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "MERGE" );
//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "befor merge" );
//				param->_sp_cur_node->dump( std::cout );
//				param->_sp_cur_node2->dump( std::cout );

				bid_type next_next_bid = param->_sp_cur_node2->bid_next();
				param->_sp_cur_node->merge( *param->_sp_cur_node2 );
				param->_sp_cur_node->bid_next( next_next_bid );
				node_manager::get_instance()->ReleaseNode( param->_sp_cur_node2->bid() ) ;
				param->_phase = btrequest_erase_interval<_Self>::PHASE_SET_NEXT_NEXT;

				key_type key = param->_sp_cur_node->begin()->first;
				bid_type bid = param->_sp_cur_node->bid();
				param->_node_vector.push_back( node_store_type(key,bid) );

//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "after merge" );
//				param->_sp_cur_node->dump( std::cout );
//				param->_sp_cur_node2->dump( std::cout );
//				STXXL_MSG( "******************************************" );

				if ( next_next_bid.valid() )
				{
					param->_bid = param->_sp_cur_node->bid();
					param->_sp_cur_node.release();
					param->_sp_cur_node2.release();
					node_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_node2.ptr, param ) ;
					return;
				}
				else
				{
					param->_bid = bid_type();
					param->_sp_cur_node.release();
					param->_sp_cur_node2.release();
					aerase( param );
					return;
				}
			}
			else // steal
			{
				STXXL_ASSERT( param->_sp_cur_node->less_half() || param->_sp_cur_node2->less_half() );

//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "STEAL" );
//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "befor steal" );
//				param->_sp_cur_node->dump( std::cout );
//				param->_sp_cur_node2->dump( std::cout );

				param->_sp_cur_node->steal_right( *param->_sp_cur_node2 );

				key_type key = param->_sp_cur_node->begin()->first;
				bid_type bid = param->_sp_cur_node->bid();
				param->_node_vector.push_back( node_store_type(key,bid) );

				key = param->_sp_cur_node2->begin()->first;
				bid = param->_sp_cur_node2->bid();
				param->_node_vector.push_back( node_store_type(key,bid) );

//				STXXL_MSG( "******************************************" );
//				STXXL_MSG( "after steal" );
//				param->_sp_cur_node->dump( std::cout );
//				param->_sp_cur_node2->dump( std::cout );
//				STXXL_MSG( "******************************************" );

				param->_height++;
				if( _height-param->_height >= 0 )
				{
					param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
					param->_sp_cur_node.release();
					param->_sp_cur_node2.release();
					param->_sp_cur_node = param->_p_first->_node_iterators[_height-param->_height].node();
					if( param->_p_last->_node_iterators.size() != 0 )
						param->_sp_cur_node2 = param->_p_last->_node_iterators[_height-param->_height].node();
					else
						param->_sp_cur_node2 = &_Node::nil;
					aerase( param );
					return;
				}
				else
				{
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
					return;
				}
			}
		}
	}

	void _aerase_phase_set_next_next( btrequest_erase_interval<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_INTERVAL );

		// param->bid           - the BID of prev node
		// param->_sp_cur_leaf2 - the next node, which becomes new prev BID

		if( param->_height == 1 )
		{
			if( param->_sp_cur_leaf2.get() )
			{
				param->_sp_cur_leaf2->bid_prev( param->_bid );
				param->_sp_cur_leaf2->set_dirty( true );
			}
		}
		else
		{
			if( param->_sp_cur_node2.get() )
			{
				param->_sp_cur_node2->bid_prev( param->_bid );
				param->_sp_cur_node2->set_dirty( true );
			}
		}

		param->_sp_cur_leaf2.release();
		param->_sp_cur_node2.release();

		param->_phase = btrequest_erase_interval<_Self>::PHASE_ERASE;
		param->_height++;
		int ix = _height - param->_height;

		if( ix >= 0 )
		{
			param->_sp_cur_node = param->_p_first->_node_iterators[ix].node();
			if( param->_p_last->_node_iterators.size() != 0 )
				param->_sp_cur_node2 = param->_p_last->_node_iterators[ix].node();
			else
				param->_sp_cur_node2 = &_Node::nil;
			aerase( param );
		}
		else
		{
			// it must be  the new root
			STXXL_ASSERT( 0 );
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
		}
	}

	// **************************************************************************************
	void aerase( btrequest_erase<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
		if ( param->_height < 0 )
		{
			if( *param->_p_iter == end() || !_tree_size )
			{
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}
			param->_height = 1;
		}

		switch( param->_phase )
		{
			case btrequest_erase<_Self>::PHASE_ERASE :
				_aerase_phase_erase( param );
				break;

			case btrequest_erase<_Self>::PHASE_NEXT_AND_ERASE  :
				_aerase_phase_next_and_erase( param );
				break;

			case btrequest_erase<_Self>::PHASE_REPLACE_KEY  :
				_aerase_phase_replace_key( param );
				break;

			case btrequest_erase<_Self>::PHASE_NEXT_AND_REPLACE_KEY  :
				_aerase_phase_next_and_replace_key( param );
				break;

			case btrequest_erase<_Self>::PHASE_SET_NEXT_NEXT:
				_aerase_phase_set_next_next( param );
				break;

			default:
				STXXL_ASSERT( 0 );
				break;
		}
	}

	void _aerase_phase_erase(  btrequest_erase<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE );

		if ( param->_height == 1 )
		{

			smart_ptr<_Leaf> sp_leaf = param->_p_iter->_leaf_iterator.node();
			bid_type bid;
			const_leaf_iterator cl_iter;

			if( param->_erase_in_tree == btrequest_erase<_Self>::NO_ERASED )
			{

				_tree_size--;
				if( param->_p_iter->_leaf_iterator.offset() == 0 && sp_leaf->size() > 1 )
				{
					cl_iter = param->_p_iter->_leaf_iterator;
					++cl_iter;
					_replace_key( *param->_p_iter, 1, (*cl_iter).first );
				}

				if ( !sp_leaf->half() || !sp_leaf->bid_next().valid() && !sp_leaf->bid_prev().valid() )
				{
					sp_leaf->erase_in_tree( param->_p_iter->_leaf_iterator, NULL, _Leaf::SIMPLY_ERASED );
					param->DisposeParam();
					sp_leaf.release();
					param->set_to( btree_request<_Self>::DONE );
					return;
				}

				else if ( sp_leaf->bid_next().valid() )
				{
					param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED_NEXT;
					bid_type bid = sp_leaf->bid_next();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_leaf2.release();
					sp_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf2.ptr, param ) ;
					return;
				}

				else
				{
					STXXL_ASSERT( sp_leaf->bid_prev().valid() );
					param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED_PREV;
					bid_type bid = sp_leaf->bid_prev();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_leaf.release();
					sp_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
					return;
				}
			}

			else if( param->_erase_in_tree == btrequest_erase<_Self>::NO_ERASED_NEXT )
			{
				if ( param->_sp_cur_leaf2->half() )
				{
					bid_type bid = sp_leaf->bid_prev();
					if( bid.valid() )
					{
						param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED_PREV;
						param->_sp_cur_leaf.release() ;
						sp_leaf.release();
						leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
						return;
					}
					else
					{
						bid_type next_next_bid = param->_sp_cur_leaf2->bid_next();
						sp_leaf->erase_in_tree(
							param->_p_iter->_leaf_iterator,
							param->_sp_cur_leaf2.get(),
							_Leaf::MERGED_NEXT );

						sp_leaf->bid_next( next_next_bid );
						leaf_manager::get_instance()->ReleaseNode( param->_sp_cur_leaf2->bid() ) ;
						param->_phase = btrequest_erase<_Self>::PHASE_SET_NEXT_NEXT;
						if ( next_next_bid.valid() )
						{
							param->_bid = sp_leaf->bid();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							sp_leaf.release();
							leaf_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_leaf2.ptr, param ) ;
							return;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							aerase( param );
							return;
						}
					}
				}
				else
				{
					sp_leaf->erase_in_tree( param->_p_iter->_leaf_iterator, param->_sp_cur_leaf2.get(), _Leaf::STEALED_NEXT );
					param->_sp_cur_leaf = param->_sp_cur_leaf2;
					param->_height2 = _height ;
					param->_phase = btrequest_erase<_Self>::PHASE_NEXT_AND_REPLACE_KEY;
					param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED;
					param->_sp_cur_leaf2.release();
					sp_leaf.release();
					aerase( param );
					return;
				}
			}

			else if( param->_erase_in_tree == btrequest_erase<_Self>::NO_ERASED_PREV )
			{
				if ( param->_sp_cur_leaf->half() )
				{
					if( sp_leaf->bid_next().valid() )
					{
						bid_type next_next_bid = param->_sp_cur_leaf2->bid_next();
						sp_leaf->erase_in_tree( param->_p_iter->_leaf_iterator, param->_sp_cur_leaf2.get(), _Leaf::MERGED_NEXT );
						sp_leaf->bid_next( next_next_bid );
						leaf_manager::get_instance()->ReleaseNode( param->_sp_cur_leaf2->bid() ) ;
						param->_phase = btrequest_erase<_Self>::PHASE_SET_NEXT_NEXT;
						if ( next_next_bid.valid() )
						{
							param->_bid = sp_leaf->bid();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							sp_leaf.release();
							leaf_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_leaf2.ptr, param ) ;
							return;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							aerase( param );
							return;
						}
					}
					else
					{
						bid_type next_next_bid = sp_leaf->bid_next();
						bid_type cur_bid       = sp_leaf->bid();

						STXXL_ASSERT( !next_next_bid.valid() );
						STXXL_ASSERT( cur_bid.valid() );

						sp_leaf->erase_in_tree( param->_p_iter->_leaf_iterator, param->_sp_cur_leaf.get(), _Leaf::MERGED_PREV );
						param->_sp_cur_leaf->bid_next( next_next_bid );
						sp_leaf.release();
						leaf_manager::get_instance()->ReleaseNode( cur_bid ) ;
						param->_height++;
						param->_sp_cur_leaf.release();
						param->_sp_cur_leaf2.release();
						param->_phase = btrequest_erase<_Self>::PHASE_ERASE;
					}
				}
				else
				{
					sp_leaf->erase_in_tree( param->_p_iter->_leaf_iterator, param->_sp_cur_leaf.get(), _Leaf::STEALED_PREV );
					param->_phase = btrequest_erase<_Self>::PHASE_REPLACE_KEY;
				}
			}
			param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED;
			param->_sp_cur_leaf.release();
			param->_sp_cur_leaf2.release();
			aerase( param );
		}
		else
		{
			const_node_iterator cn_iter;
			node_iterator node_iter = param->_p_iter->_node_iterators[ _height - param->_height ];
			smart_ptr<_Node> sp_node( node_iter.node() );

			bid_type bid;
			if( param->_erase_in_tree == btrequest_erase<_Self>::NO_ERASED )
			{

				if( node_iter.offset() == 0 )
				{
					cn_iter = node_iter;
					++cn_iter;
					_replace_key( *param->_p_iter, param->_height, (*cn_iter).first );
				}

				if ( !sp_node->half() || !sp_node->bid_next().valid() && !sp_node->bid_prev().valid() )
				{
					sp_node->erase_in_tree( node_iter, NULL, _Node::SIMPLY_ERASED );
					if ( sp_node->size() == 1 )
					{
						node_ptr sp_old_root = _sp_node_root;
						if ( param->_height == 2 )
						{
							_sp_leaf_root = param->_p_iter->_leaf_iterator.node();
							STXXL_ASSERT( !_sp_leaf_root->bid_next().valid() );
							STXXL_ASSERT( !_sp_leaf_root->bid_prev().valid() );
						}
						else
						{
							_sp_node_root = param->_p_iter->_node_iterators[ _height - param->_height + 1 ].node();
							STXXL_ASSERT( !_sp_node_root->bid_next().valid() );
							STXXL_ASSERT( !_sp_node_root->bid_prev().valid() );
						}
						node_manager::get_instance()->ReleaseNode( sp_old_root->bid() ) ;
						STXXL_VERBOSE2( "B-Tree is shrinking." );
						_height --;
					}
					param->DisposeParam();
					sp_node.release();
					param->set_to( btree_request<_Self>::DONE );
					return;
				}

				else if ( sp_node->bid_next().valid() )
				{
					param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED_NEXT;
					bid_type bid = sp_node->bid_next();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_node2.release();
					sp_node.release();
					node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node2.ptr, param ) ;
					return;
				}

				else
				{
					STXXL_ASSERT( sp_node->bid_prev().valid() );
					param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED_PREV;
					bid_type bid = sp_node->bid_prev();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_node = NULL;
					sp_node.release();
					node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
					return;
				}
			}

			else if( param->_erase_in_tree == btrequest_erase<_Self>::NO_ERASED_NEXT )
			{
				if ( param->_sp_cur_node2->half() )
				{
					bid_type bid = sp_node->bid_prev();
					if( bid.valid() )
					{
						param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED_PREV;
						param->_sp_cur_node = NULL;
						sp_node.release();
						node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
						return;
					}
					else
					{
						bid_type next_next_bid = param->_sp_cur_node2->bid_next();
						sp_node->erase_in_tree(
							node_iter,
							param->_sp_cur_node2.get(),
							_Node::MERGED_NEXT );
						sp_node->bid_next( next_next_bid );
						node_manager::get_instance()->ReleaseNode( param->_sp_cur_node2->bid() ) ;
						param->_phase = btrequest_erase<_Self>::PHASE_SET_NEXT_NEXT;
						if ( next_next_bid.valid() )
						{
							param->_bid = sp_node->bid();
							param->_sp_cur_node.release();
							param->_sp_cur_node2.release();
							sp_node.release();
							node_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_node2.ptr, param ) ;
							return;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_node.release();
							param->_sp_cur_node2.release();
							aerase( param );
							return;
						}
					}
				}
				else
				{
					sp_node->erase_in_tree(
						node_iter,
						param->_sp_cur_node2.get(),
						_Node::STEALED_NEXT );

					param->_sp_cur_node = param->_sp_cur_node2;
					param->_height2 = _height - param->_height + 1;
					param->_phase = btrequest_erase<_Self>::PHASE_NEXT_AND_REPLACE_KEY;
					param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED;
					param->_sp_cur_node2.release();
					aerase( param );
					return;
				}
			}

			else if( param->_erase_in_tree == btrequest_erase<_Self>::NO_ERASED_PREV )
			{
				if ( param->_sp_cur_node->half() )
				{
					if( sp_node->bid_next().valid() )
					{
						bid_type next_next_bid = param->_sp_cur_node2->bid_next();
						sp_node->erase_in_tree(
							node_iter,
							param->_sp_cur_node2.get(),
							_Node::MERGED_NEXT );

						sp_node->bid_next( next_next_bid );
						node_manager::get_instance()->ReleaseNode( param->_sp_cur_node2->bid() ) ;

						param->_phase = btrequest_erase<_Self>::PHASE_SET_NEXT_NEXT;
						if ( next_next_bid.valid() )
						{
							param->_bid = sp_node->bid();
							param->_sp_cur_node = NULL;
							param->_sp_cur_node2 = NULL;
							sp_node.release();
							node_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_node2.ptr, param ) ;
							return;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_node = NULL;
							param->_sp_cur_node2 = NULL;
							aerase( param );
							return;
						}
					}
					else
					{
						bid_type next_next_bid = sp_node->bid_next();
						sp_node->erase_in_tree( node_iter, param->_sp_cur_node.get(), _Node::MERGED_PREV );
						STXXL_ASSERT( !next_next_bid.valid() );
						param->_sp_cur_node->bid_next( next_next_bid );
						node_manager::get_instance()->ReleaseNode( sp_node->bid() ) ;
						param->_p_iter->_node_iterators[ _height - param->_height ] = node_iter;
						param->_height++;
						param->_phase = btrequest_erase<_Self>::PHASE_ERASE;
					}
				}
				else
				{
					sp_node->erase_in_tree( node_iter, param->_sp_cur_node.get(), _Node::STEALED_PREV );
					param->_phase = btrequest_erase<_Self>::PHASE_REPLACE_KEY;
				}
			}
			param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED;
			aerase( param );
		}
	}

	void _aerase_phase_set_next_next(  btrequest_erase<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE );

		if( param->_height == 1 )
		{
			if( param->_sp_cur_leaf2.get() )
			{
				param->_sp_cur_leaf2->bid_prev( param->_bid );
				param->_sp_cur_leaf2->set_dirty( true );
				param->_sp_cur_leaf2.release();
			}
		}
		else
		{
			if( param->_sp_cur_node2.get() )
			{
				param->_sp_cur_node2->bid_prev( param->_bid );
				param->_sp_cur_node2->set_dirty( true );
				param->_sp_cur_node2.release();
			}
		}

		if ( !param->_p_iter->_node_iterators[ _height - param->_height - 1 ].next() )
		{
			param->_phase = btrequest_erase<_Self>::PHASE_ERASE;
			param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED;
			param->_height++;
			param->_sp_cur_node.release();
			aerase( param );
		}
		else
		{
			param->_phase = btrequest_erase<_Self>::PHASE_NEXT_AND_ERASE;
			param->_height2 = _height - param->_height - 1;
			smart_ptr<_Node> sp_node = param->_p_iter->_node_iterators[ param->_height2 ].node();
			bid_type bid = sp_node->bid_next();
			param->_sp_cur_node.release();
			sp_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _aerase_phase_next_and_erase(  btrequest_erase<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE );
		STXXL_ASSERT( (size_type) param->_height2 != _height );

		param->_p_iter->_node_iterators[param->_height2] = param->_sp_cur_node->begin();
		--param->_height2;

		if ( !param->_p_iter->_node_iterators[param->_height2].next() )
		{
			param->_phase = btrequest_erase<_Self>::PHASE_ERASE;
			param->_erase_in_tree = btrequest_erase<_Self>::NO_ERASED;
			param->_sp_cur_node.release();
			param->_height++;
			aerase( param );
		}
		else
		{
			smart_ptr<_Node> sp_node = param->_p_iter->_node_iterators[param->_height2].node();
			bid_type bid = sp_node->bid_next();
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _aerase_phase_next_and_replace_key(  btrequest_erase<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE );

		if ( ( (size_type) param->_height2 ) == _height )
		{
			param->_p_iter->_leaf_iterator = param->_sp_cur_leaf->begin();
		}
		else
		{
			param->_p_iter->_node_iterators[param->_height2-1] = param->_sp_cur_node->begin();
		}

		if ( !param->_p_iter->_node_iterators[param->_height2-2].next() )
		{
			param->_phase = btrequest_erase<_Self>::PHASE_REPLACE_KEY;
			param->_sp_cur_node.release();
			aerase( param );
		}
		else
		{
			smart_ptr<_Node> sp_node = param->_p_iter->_node_iterators[param->_height2-2].node();
			--param->_height2;
			bid_type bid = sp_node->bid_next();
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _aerase_phase_replace_key(  btrequest_erase<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE );

		if ( param->_height == 1 )
		{
			param->_p_iter->_leaf_iterator = param->_p_iter->_leaf_iterator.node()->begin();
			const_leaf_iterator cl_iter = param->_p_iter->_leaf_iterator ;
			_replace_key( *param->_p_iter, 1, (*cl_iter).first );
		}
		else
		{
			param->_p_iter->_node_iterators[ _height - param->_height ] =
			param->_p_iter->_node_iterators[ _height - param->_height ].node()->begin();
			const_node_iterator cn_iter = param->_p_iter->_node_iterators[ _height - param->_height ];
			_replace_key( *param->_p_iter, param->_height, (*cn_iter).first );
		}
		param->DisposeParam();
		param->set_to( btree_request<_Self>::DONE );
	}

	/***************************************************************************************
	****************************************************************************************

	ERASE KEY

	****************************************************************************************
	***************************************************************************************/

	void aerase( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
		if ( param->_height < 0 )
		{
			if ( !_tree_size )
			{
				*param->_p_size = 0;
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			param->_sp_iter = new iterator( this );
			if ( _height == 1 )
			{
				param->_sp_cur_leaf = _sp_leaf_root;
			}
			else
			{
				param->_sp_cur_node = _sp_node_root;
			}
			param->_height = _height;
		}

		switch( param->_phase )
		{
			case btrequest_erase_key<_Self>::PHASE_SEARCH :
				_aerase_search( param );
				break;

			case btrequest_erase_key<_Self>::PHASE_MOVE_NEXT :
				_aerase_move_next( param );
				break;

			case btrequest_erase_key<_Self>::PHASE_MOVE_PREV :
				_aerase_move_prev( param );
				break;

			case btrequest_erase_key<_Self>::PHASE_ERASE :
				_aerase_phase_erase( param );
				break;

			case btrequest_erase_key<_Self>::PHASE_NEXT_AND_ERASE  :
				_aerase_phase_next_and_erase( param );
				break;

			case btrequest_erase_key<_Self>::PHASE_REPLACE_KEY  :
				_aerase_phase_replace_key( param );
				break;

			case btrequest_erase_key<_Self>::PHASE_NEXT_AND_REPLACE_KEY  :
				_aerase_phase_next_and_replace_key( param );
				break;

			case btrequest_erase_key<_Self>::PHASE_SET_NEXT_NEXT:
				_aerase_phase_set_next_next( param );
				break;

			default:
				STXXL_ASSERT( 0 );
				break;
		}
	}

	void _aerase_search( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );

		node_iterator n_iter;

		if ( !--param->_height )
		{

			// **************************************************************************************
			// we in a leaf
			// and we have to find the right leaf

			param->_sp_iter->_leaf_iterator = param->_sp_cur_leaf->lower_bound( param->_key );
			if ( param->_sp_iter->_leaf_iterator == param->_sp_cur_leaf->end() )
			{
				smart_ptr<_Leaf> sp_leaf = param->_sp_cur_leaf;
				if( !sp_leaf->bid_next().valid() )
				{
					// there are no next leaf and no lower_bound
					// lower_bound = end()
					*param->_p_size = 0;
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
					return;
				}
				// We have to find the begin of next leaf
				param->_phase = btrequest_erase_key<_Self>::PHASE_MOVE_NEXT;
				param->_height = _height;
				bid_type bid = sp_leaf->bid_next();
				STXXL_ASSERT( bid.valid() );
				param->_sp_cur_leaf.release();
				leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
				return;
			}

			const_leaf_iterator cn_iter( param->_sp_iter->_leaf_iterator );
			if( (*cn_iter).first != param->_key )
			{
					*param->_p_size = 0;
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
					return;
			}
			param->DisposeParam();
			param->_height = 1;
			*param->_p_size = 1;
			param->_phase = btrequest_erase_key<_Self>::PHASE_ERASE;
			aerase( param );
			return;
		}
		else
		{
			// ***********************************************************************************
			// we are in internal nodes
			// ***********************************************************************************

			n_iter = param->_sp_cur_node->lower_bound( param->_key );

			if ( n_iter == param->_sp_cur_node->end() )

				// ***********************************************************************************
				// all elemenets are less then key -> search in the last node
				// ***********************************************************************************
				n_iter = param->_sp_cur_node->last();

			else if ( n_iter != param->_sp_cur_node->begin() )

				// ***********************************************************************************
				// it is not the first element
				// ***********************************************************************************
				--n_iter;

			else
			{
				// the first element in tree is it.
				STXXL_ASSERT( !param->_sp_cur_node->bid_prev().valid() );
			}
			param->_sp_iter->_node_iterators.push_back( n_iter );
		}

		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

		const_node_iterator cn_iter = n_iter;

		if( param->_height == 1 )
		{
			// ***************************************************************************************
			// the next one is a leaf
			// ***************************************************************************************

			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_leaf.release();
			leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;

		}
		else
		{
			// ***************************************************************************************
			// the next one is steel a node
			// ***************************************************************************************

			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _aerase_move_next( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btrequest_erase_key<_Self>::ERASE_KEY );

		if ( (unsigned)param->_height == _height )
		{
			param->_sp_iter->_leaf_iterator = param->_sp_cur_leaf->begin();
			const_leaf_iterator cn_iter = param->_sp_iter->_leaf_iterator;
			if( (*cn_iter).first != param->_key )
			{
				*param->_p_size = 0;
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}
		}
		else
		{
			param->_sp_iter->_node_iterators[param->_height-1] = param->_sp_cur_node->begin();
		}

		smart_ptr<_Node> sp_node = param->_sp_iter->_node_iterators[param->_height-2].node();
		if ( !param->_sp_iter->_node_iterators[param->_height-2].next() )
		{
			param->DisposeParam();
			param->_height = 1;
			*param->_p_size = 1;
			param->_phase = btrequest_erase_key<_Self>::PHASE_ERASE;
			aerase( param );
			return;
		}

		--param->_height;
		bid_type bid = sp_node->bid_next();
		STXXL_ASSERT( bid.valid() );
		param->_sp_cur_node.release();
		node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
	}

	void _aerase_move_prev( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btrequest_erase_key<_Self>::ERASE_KEY );

		if ( ( (size_type) param->_height2 ) == _height )
		{
			STXXL_ASSERT( 0 );
		}
		else
		{
			param->_sp_iter->_node_iterators[param->_height2-1] = param->_sp_cur_node->last();
		}

		if ( !param->_sp_iter->_node_iterators[param->_height2-2].prev() )
		{
			param->_phase = btrequest_erase_key<_Self>::PHASE_SEARCH ;
			const_node_iterator cn_iter = param->_sp_iter->_node_iterators[param->_height-1];
			bid_type bid = (*cn_iter).second;
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		else
		{
			smart_ptr<_Node> sp_node = param->_sp_iter->_node_iterators[param->_height2-2].node();
			--param->_height2;
			bid_type bid = sp_node->bid_prev();
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _aerase_phase_erase( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );

		if ( param->_height == 1 )
			_aerase_phase_erase_leaf( param );
		else
			_aerase_phase_erase_node( param );
	}

	void _aerase_phase_erase_leaf( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );
		STXXL_ASSERT( param->_height == 1 );

		// ******************************************************************************
		// locale variales:
		// ******************************************************************************


		smart_ptr<_Leaf> sp_leaf = param->_sp_iter->_leaf_iterator.node(); // curent leaf
		bid_type bid;
		const_leaf_iterator cl_iter;



			if( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED )
			{

				_tree_size--;
				if( param->_sp_iter->_leaf_iterator.offset() == 0 && sp_leaf->size() > 1 )
				{
					cl_iter = param->_sp_iter->_leaf_iterator;
					++cl_iter;
					_replace_key( *param->_sp_iter, 1, (*cl_iter).first );
				}

				if ( !sp_leaf->half() || !sp_leaf->bid_next().valid() && !sp_leaf->bid_prev().valid() )
				{
					sp_leaf->erase_in_tree( param->_sp_iter->_leaf_iterator, NULL, _Leaf::SIMPLY_ERASED );
					param->DisposeParam();
					sp_leaf.release();
					param->set_to( btree_request<_Self>::DONE );
					return;
				}

				else if ( sp_leaf->bid_next().valid() )
				{
					param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED_NEXT;
					bid_type bid = sp_leaf->bid_next();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_leaf2.release();
					sp_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf2.ptr, param ) ;
					return;
				}

				else
				{
					STXXL_ASSERT( sp_leaf->bid_prev().valid() );
					param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED_PREV;
					bid_type bid = sp_leaf->bid_prev();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_leaf.release();
					sp_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
					return;
				}
			}

			else if( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED_NEXT )
			{
				if ( param->_sp_cur_leaf2->half() )
				{
					bid_type bid = sp_leaf->bid_prev();
					if( bid.valid() )
					{
						param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED_PREV;
						param->_sp_cur_leaf.release() ;
						sp_leaf.release();
						leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
						return;
					}
					else
					{
						bid_type next_next_bid = param->_sp_cur_leaf2->bid_next();
						sp_leaf->erase_in_tree(
							param->_sp_iter->_leaf_iterator,
							param->_sp_cur_leaf2.get(),
							_Leaf::MERGED_NEXT );
						sp_leaf->bid_next( next_next_bid );
						leaf_manager::get_instance()->ReleaseNode( param->_sp_cur_leaf2->bid() ) ;
						param->_phase = btrequest_erase_key<_Self>::PHASE_SET_NEXT_NEXT;

						if ( next_next_bid.valid() )
						{
							param->_bid = sp_leaf->bid();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							sp_leaf.release();
							leaf_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_leaf2.ptr, param ) ;
							return;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							sp_leaf.release();
							aerase( param );
							return;
						}
					}
				}
				else
				{
					sp_leaf->erase_in_tree( param->_sp_iter->_leaf_iterator, param->_sp_cur_leaf2.get(), _Leaf::STEALED_NEXT );
					param->_sp_cur_leaf = param->_sp_cur_leaf2;
					param->_height2 = _height ;
					param->_phase = btrequest_erase_key<_Self>::PHASE_NEXT_AND_REPLACE_KEY;
					param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED;
					param->_sp_cur_leaf2.release();
					sp_leaf.release();
					aerase( param );
					return;
				}
			}

			else if( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED_PREV )
			{
				if ( param->_sp_cur_leaf->half() )
				{
					if( sp_leaf->bid_next().valid() )
					{
						bid_type next_next_bid = param->_sp_cur_leaf2->bid_next();
						sp_leaf->erase_in_tree( param->_sp_iter->_leaf_iterator, param->_sp_cur_leaf2.get(), _Leaf::MERGED_NEXT );
						sp_leaf->bid_next( next_next_bid );
						leaf_manager::get_instance()->ReleaseNode( param->_sp_cur_leaf2->bid() ) ;
						param->_phase = btrequest_erase_key<_Self>::PHASE_SET_NEXT_NEXT;
						if ( next_next_bid.valid() )
						{
							param->_bid = sp_leaf->bid();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							sp_leaf.release();
							leaf_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_leaf2.ptr, param ) ;
							return;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_leaf.release();
							param->_sp_cur_leaf2.release();
							sp_leaf.release();
							aerase( param );
							return;
						}
					}
					else
					{
						bid_type next_next_bid = sp_leaf->bid_next();
						bid_type bid = sp_leaf->bid();
						STXXL_ASSERT( !next_next_bid.valid() );
						sp_leaf->erase_in_tree( param->_sp_iter->_leaf_iterator, param->_sp_cur_leaf.get(), _Leaf::MERGED_PREV );
						param->_sp_cur_leaf->bid_next( next_next_bid );
						sp_leaf.release();
						leaf_manager::get_instance()->ReleaseNode( bid ) ;
						param->_height++;
						param->_sp_cur_leaf.release();
						param->_sp_cur_leaf2.release();
						param->_phase = btrequest_erase_key<_Self>::PHASE_ERASE;
					}
				}
				else
				{
					sp_leaf->erase_in_tree( param->_sp_iter->_leaf_iterator, param->_sp_cur_leaf.get(), _Leaf::STEALED_PREV );
					param->_phase = btrequest_erase_key<_Self>::PHASE_REPLACE_KEY;
				}
			}
			param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED;
			param->_sp_cur_leaf.release();
			param->_sp_cur_leaf2.release();
			sp_leaf.release();
			aerase( param );
		}


	void _aerase_phase_erase_node ( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );
		STXXL_ASSERT( param->_height > 1 );

		if( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED )
			_aerase_phase_erase_node_no_erased( param );
		else if( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED_NEXT )
			_aerase_phase_erase_node_no_erased_next( param );
		else if( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED_PREV )
			_aerase_phase_erase_node_no_erased_prev( param );
	}


	void _aerase_phase_erase_node_no_erased ( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );
		STXXL_ASSERT( param->_height > 1 );
		STXXL_ASSERT( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED );

		// ******************************************************************************
		// locale variales:
		// ******************************************************************************

		const_node_iterator cn_iter;
		node_iterator node_iter = param->_sp_iter->_node_iterators[ _height - param->_height ];
		smart_ptr<_Node> sp_node( node_iter.node() );
		bid_type bid;


				if( node_iter.offset() == 0 )
				{
					cn_iter = node_iter;
					++cn_iter;
					_replace_key( *param->_sp_iter, param->_height, (*cn_iter).first );
				}

		// ******************************************************************************************
		// node has right key count or it is the root
		// we can delete direct
		// ******************************************************************************************

				if ( !sp_node->half() || !sp_node->bid_next().valid() && !sp_node->bid_prev().valid() )
				{
					sp_node->erase_in_tree( node_iter, NULL, _Node::SIMPLY_ERASED );
					STXXL_ASSERT( sp_node->size() > 0 );
					if ( sp_node->size() == 1 )
					{
						node_ptr sp_old_root = _sp_node_root;
						if ( param->_height == 2 )
						{
							_sp_leaf_root = param->_sp_iter->_leaf_iterator.node();
							STXXL_ASSERT( !_sp_leaf_root->bid_next().valid() );
							STXXL_ASSERT( !_sp_leaf_root->bid_prev().valid() );
						}
						else
						{
							_sp_node_root = param->_sp_iter->_node_iterators[ _height - param->_height + 1 ].node();
							STXXL_ASSERT( !_sp_node_root->bid_next().valid() );
							STXXL_ASSERT( !_sp_node_root->bid_prev().valid() );
						}
						sp_node.release();
						node_manager::get_instance()->ReleaseNode( sp_old_root->bid() ) ;
						STXXL_VERBOSE2( "B-Tree is shrinking." );
						_height --;
					}
					param->DisposeParam();
					sp_node.release();
					param->set_to( btree_request<_Self>::DONE );
				}

		// ******************************************************************************************
		// node has no right and it is not the root
		// we have to load other node for delete
		// ******************************************************************************************

				else if ( sp_node->bid_next().valid() )
				{
					param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED_NEXT;
					bid_type bid = sp_node->bid_next();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_node2.release();
					sp_node.release();
					node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node2.ptr, param ) ;
				}
				else
				{
					STXXL_ASSERT( sp_node->bid_prev().valid() );
					param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED_PREV;
					bid_type bid = sp_node->bid_prev();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_node.release();
					sp_node.release();
					node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
				}
	}

	void _aerase_phase_erase_node_no_erased_next( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );
		STXXL_ASSERT( param->_height > 1 );
		STXXL_ASSERT( param->_erase_in_tree != btrequest_erase_key<_Self>::NO_ERASED );
		STXXL_ASSERT( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED_NEXT );

		// ******************************************************************************
		// locale variales:
		// ******************************************************************************

		const_node_iterator cn_iter;
		node_iterator node_iter = param->_sp_iter->_node_iterators[ _height - param->_height ];
		smart_ptr<_Node> sp_node( node_iter.node() );
		bid_type bid;

				if ( param->_sp_cur_node2->half() )
				{
					bid_type bid = sp_node->bid_prev();
					if( bid.valid() )
					{
						param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED_PREV;
						param->_sp_cur_node = NULL;
						sp_node.release();
						node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
					}
					else
					{
						bid_type next_next_bid = param->_sp_cur_node2->bid_next();
						sp_node->erase_in_tree(
							node_iter,
							param->_sp_cur_node2.get(),
							_Node::MERGED_NEXT );
						sp_node->bid_next( next_next_bid );
						node_manager::get_instance()->ReleaseNode( param->_sp_cur_node2->bid() ) ;
						param->_phase = btrequest_erase_key<_Self>::PHASE_SET_NEXT_NEXT;
						if ( next_next_bid.valid() )
						{
							param->_bid = sp_node->bid();
							param->_sp_cur_node.release();
							param->_sp_cur_node2.release();
							sp_node.release();
							node_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_node2.ptr, param ) ;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_node.release();
							param->_sp_cur_node2.release();
							sp_node.release();
							aerase( param );
						}
					}
				}
				else
				{
					sp_node->erase_in_tree(
						node_iter,
						param->_sp_cur_node2.get(),
						_Node::STEALED_NEXT );

					param->_sp_cur_node = param->_sp_cur_node2;
					param->_height2 = _height - param->_height + 1;
					param->_phase = btrequest_erase_key<_Self>::PHASE_NEXT_AND_REPLACE_KEY;
					param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED;
					param->_sp_cur_node2.release();
					sp_node.release();
					aerase( param );
				}
	}

	void _aerase_phase_erase_node_no_erased_prev( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );
		STXXL_ASSERT( param->_height > 1 );
		STXXL_ASSERT( param->_erase_in_tree != btrequest_erase_key<_Self>::NO_ERASED );
		STXXL_ASSERT( param->_erase_in_tree != btrequest_erase_key<_Self>::NO_ERASED_NEXT );
		STXXL_ASSERT( param->_erase_in_tree == btrequest_erase_key<_Self>::NO_ERASED_PREV );

		// ******************************************************************************
		// locale variales:
		// ******************************************************************************

		const_node_iterator cn_iter;
		node_iterator node_iter = param->_sp_iter->_node_iterators[ _height - param->_height ];
		smart_ptr<_Node> sp_node( node_iter.node() );
		bid_type bid;

				if ( param->_sp_cur_node->half() )
				{
					if( sp_node->bid_next().valid() )
					{
						bid_type next_next_bid = param->_sp_cur_node2->bid_next();
						sp_node->erase_in_tree(
							node_iter,
							param->_sp_cur_node2.get(),
							_Node::MERGED_NEXT );

						sp_node->bid_next( next_next_bid );
						node_manager::get_instance()->ReleaseNode( param->_sp_cur_node2->bid() ) ;

						param->_phase = btrequest_erase_key<_Self>::PHASE_SET_NEXT_NEXT;
						if ( next_next_bid.valid() )
						{
							param->_bid = sp_node->bid();
							param->_sp_cur_node = NULL;
							param->_sp_cur_node2 = NULL;
							sp_node.release();
							node_manager::get_instance()->GetNode( next_next_bid, &param->_sp_cur_node2.ptr, param ) ;
							return;
						}
						else
						{
							param->_bid = bid_type();
							param->_sp_cur_node = NULL;
							param->_sp_cur_node2 = NULL;
							aerase( param );
							return;
						}
					}
					else
					{
						bid_type next_next_bid = sp_node->bid_next();
						sp_node->erase_in_tree( node_iter, param->_sp_cur_node.get(), _Node::MERGED_PREV );
						STXXL_ASSERT( !next_next_bid.valid() );
						param->_sp_cur_node->bid_next( next_next_bid );
						node_manager::get_instance()->ReleaseNode( sp_node->bid() ) ;
						param->_sp_iter->_node_iterators[ _height - param->_height ] = node_iter;
						param->_height++;
						param->_phase = btrequest_erase_key<_Self>::PHASE_ERASE;
					}
				}
				else
				{
					sp_node->erase_in_tree( node_iter, param->_sp_cur_node.get(), _Node::STEALED_PREV );
					param->_phase = btrequest_erase_key<_Self>::PHASE_REPLACE_KEY;
				}
			param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED;
			aerase( param );
	}

	void _aerase_phase_set_next_next( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );

		if( param->_height == 1 )
		{
			if( param->_sp_cur_leaf2.get() )
			{
				param->_sp_cur_leaf2->bid_prev( param->_bid );
				param->_sp_cur_leaf2->set_dirty( true );
				param->_sp_cur_leaf2.release();
			}
		}
		else
		{
			if( param->_sp_cur_node2.get() )
			{
				param->_sp_cur_node2->bid_prev( param->_bid );
				param->_sp_cur_node2->set_dirty( true );
				param->_sp_cur_node2.release();
			}
		}

		if ( !param->_sp_iter->_node_iterators[ _height - param->_height - 1 ].next() )
		{
			param->_phase = btrequest_erase_key<_Self>::PHASE_ERASE;
			param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED;
			param->_height++;
			param->_sp_cur_node.release();
			aerase( param );
		}
		else
		{
			param->_phase = btrequest_erase_key<_Self>::PHASE_NEXT_AND_ERASE;
			param->_height2 = _height - param->_height - 1;
			smart_ptr<_Node> sp_node = param->_sp_iter->_node_iterators[ param->_height2 ].node();
			bid_type bid = sp_node->bid_next();
			param->_sp_cur_node.release();
			sp_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _aerase_phase_next_and_erase( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );
		STXXL_ASSERT( (size_type) param->_height2 != _height );

		param->_sp_iter->_node_iterators[param->_height2] = param->_sp_cur_node->begin();
		--param->_height2;

		if ( !param->_sp_iter->_node_iterators[param->_height2].next() )
		{
			param->_phase = btrequest_erase_key<_Self>::PHASE_ERASE;
			param->_erase_in_tree = btrequest_erase_key<_Self>::NO_ERASED;
			param->_sp_cur_node.release();
			param->_height++;
			aerase( param );
		}
		else
		{
			smart_ptr<_Node> sp_node = param->_sp_iter->_node_iterators[param->_height2].node();
			bid_type bid = sp_node->bid_next();
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _aerase_phase_next_and_replace_key( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );

		if ( ( (size_type) param->_height2 ) == _height )
		{
			param->_sp_iter->_leaf_iterator = param->_sp_cur_leaf->begin();
		}
		else
		{
			param->_sp_iter->_node_iterators[param->_height2-1] = param->_sp_cur_node->begin();
		}

		if ( !param->_sp_iter->_node_iterators[param->_height2-2].next() )
		{
			param->_phase = btrequest_erase_key<_Self>::PHASE_REPLACE_KEY;
			param->_sp_cur_node.release();
			aerase( param );
		}
		else
		{
			smart_ptr<_Node> sp_node = param->_sp_iter->_node_iterators[param->_height2-2].node();
			--param->_height2;
			bid_type bid = sp_node->bid_next();
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _aerase_phase_replace_key( btrequest_erase_key<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::ERASE_KEY );

		if ( param->_height == 1 )
		{
			param->_sp_iter->_leaf_iterator = param->_sp_iter->_leaf_iterator.node()->begin();
			const_leaf_iterator cl_iter = param->_sp_iter->_leaf_iterator ;
			_replace_key( *param->_sp_iter, 1, (*cl_iter).first );
		}
		else
		{
			param->_sp_iter->_node_iterators[ _height - param->_height ] =
			param->_sp_iter->_node_iterators[ _height - param->_height ].node()->begin();
			const_node_iterator cn_iter = param->_sp_iter->_node_iterators[ _height - param->_height ];
			_replace_key( *param->_sp_iter, param->_height, (*cn_iter).first );
		}
		param->DisposeParam();
		param->set_to( btree_request<_Self>::DONE );
	}

	/***************************************************************************************
	****************************************************************************************

	BULK INSERT

	****************************************************************************************
	***************************************************************************************/

	//! \brief Implementing of inseration of an interval
	template <typename InputIterator_>
	void abulk_insert( btrequest_bulk_insert<_Self,InputIterator_>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BULK_INSERT );

		// ***********************************************************************************
		// we check if ainsert already calleed
		// ***********************************************************************************

				if ( param->_height < 0 )
				{

		// *********************************************************************************
		// the tree is empty
		// we can insert directly and leave the function if all entries are added
		// *********************************************************************************

					if ( param->_first == param->_last )
					{
						param->DisposeParam();
						param->set_to( btree_request<_Self>::DONE );
						return;
					}

					if ( !_tree_size )
					{
						while( param->_first != param->_last && _sp_leaf_root->size() < _sp_leaf_root->max_size() )
						{
							std::pair<leaf_iterator,bool> ret = _sp_leaf_root->insert( *param->_first );
							param->_first++;
							if( ret.second ) _tree_size++;
						}
						if( param->_first == param->_last  )
						{
							param->DisposeParam();
							param->set_to( btree_request<_Self>::DONE );
							return;
						}
					}

		// *********************************************************************************
		// tree is not empty we init param
		// *********************************************************************************

					param->_sp_iterator = new iterator(this);
					param->_height = _height;
					if ( _height == 1 )
						param->_sp_cur_leaf = _sp_leaf_root;
					else
						param->_sp_cur_node = _sp_node_root;
				}

		// *********************************************************************************
		// CASES
		// *********************************************************************************

				switch( param->_phase )
				{

					case btrequest_bulk_insert<_Self,InputIterator_>::PHASE_SEARCH:
						_abulk_insert_search( param );                           break;

					case btrequest_bulk_insert<_Self,InputIterator_>::PHASE_MOVE_NEXT:
						_abulk_insert_move_next( param );                        break;

					case btrequest_bulk_insert<_Self,InputIterator_>::PHASE_MOVE_PREV:
						_abulk_insert_move_prev( param );                        break;

					case btrequest_bulk_insert<_Self,InputIterator_>::PHASE_INSERTING:
						_abulk_insert_inserting( param );                        break;

					case btrequest_bulk_insert<_Self,InputIterator_>::PHASE_REPLACE_BID_IN_NEXT_NODE:
						_abulk_insert_replace_bid_in_next_node( param );         break;

					default : STXXL_ASSERT( 0 );
				}
	}


	template <typename InputIterator_>
	void _abulk_insert_search( btrequest_bulk_insert<_Self,InputIterator_>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BULK_INSERT );
		STXXL_ASSERT( param->_height > 0 );

		//************************************************************************************
		// param->_height contains previusly height, the current height have to decrease
		// param->_height = 0 : means we are in a leaf
		// param->_height > 0 : means we are in the node
		//************************************************************************************

		if ( !--param->_height )
			_abulk_insert_search_leaf( param );
		else
			_abulk_insert_search_node( param );
	}


	template <typename InputIterator_>
	void _abulk_insert_search_leaf( btrequest_bulk_insert<_Self,InputIterator_>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BULK_INSERT );
		STXXL_ASSERT( !param->_height );

		//************************************************************************************
		// LEAF
		//************************************************************************************
		// we test if insert and which position we have to use for insert
		//************************************************************************************

					std::pair<leaf_iterator,bool> test = param->_sp_cur_leaf->test_insert( *param->_first );
					param->_sp_iterator->_leaf_iterator = test.first;


		//************************************************************************************
		// test failed
		//************************************************************************************
					if( !test.second )
					{
						if( ++param->_first == param->_last )
						{
							param->DisposeParam();
							param->set_to( btree_request<_Self>::DONE );
						}
						else
						{
							param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_SEARCH;
							param->_height = _height;
							param->_sp_iterator = new iterator(this);
							if ( _height == 1 )
							{
								param->_sp_cur_leaf = _sp_leaf_root;
							}
							else
							{
								param->_sp_cur_leaf.release();
								param->_sp_cur_node = _sp_node_root;
							}
							abulk_insert( param );
						}
						return;
					}

		//************************************************************************************
		// test was sucess
		//************************************************************************************


					STXXL_ASSERT( test.second );
					if ( param->_sp_iterator->_leaf_iterator == param->_sp_cur_leaf->end() )
					{
						if( !param->_sp_cur_leaf->bid_next().valid() )
						{
							param->_sp_iterator->_leaf_iterator = param->_sp_cur_leaf->last();
						}
						else
						{
							// We have to find the begin of next leaf
							param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_MOVE_NEXT;
							bid_type bid = param->_sp_cur_leaf->bid_next();
							STXXL_ASSERT( bid.valid() );
							param->_sp_cur_leaf.release();
							STXXL_ASSERT( !param->_sp_cur_node.get() ); // param->_sp_cur_node.release();
							leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
							return;
						}
					}

					param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_INSERTING;
					abulk_insert(param);
	}


	template <typename InputIterator_>
	void _abulk_insert_search_node( btrequest_bulk_insert<_Self,InputIterator_>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BULK_INSERT );
		STXXL_ASSERT( param->_height > 0 );

		//************************************************************************************
		// param->_height contains previusly height, the current height have to decrease
		// param->_height = 0 : means we are in a leaf
		// param->_height > 0 : means we are in the node
		//************************************************************************************


		//************************************************************************************
		// NODE
		//************************************************************************************
		// we stil search for key
		//************************************************************************************

				node_iterator n_iter = param->_sp_cur_node->lower_bound( (*param->_first).first );

				// ********************************************************
				// all keys are less
				// ********************************************************

				if ( n_iter == param->_sp_cur_node->end() )
					n_iter = param->_sp_cur_node->last();


				// ********************************************************
				// there is a key is less
				// ********************************************************
				else if ( n_iter != param->_sp_cur_node->begin() )
					--n_iter;


				else if ( param->_sp_cur_node->bid_prev().valid() )
				{
					// the key is in the previosly node
					param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_MOVE_PREV;
					param->_height2 = param->_height;
					bid_type bid = param->_sp_cur_node->bid_prev();
					STXXL_ASSERT( param->_sp_cur_leaf.get() ); // param->_sp_cur_leaf.release();
					param->_sp_cur_node.release();
					node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
					return;
				}

				param->_sp_iterator->_node_iterators.push_back( n_iter );


		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

					const_node_iterator cn_iter = n_iter;
					bid_type bid = (*cn_iter).second;
					STXXL_ASSERT( bid.valid() );
					STXXL_ASSERT( !param->_sp_cur_leaf.get() ); // param->_sp_cur_leaf.release();
					param->_sp_cur_node.release();

					if( param->_height == 1 )
						leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
					else
						node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
	}

	template <typename InputIterator_>
	void _abulk_insert_move_next( btrequest_bulk_insert<_Self,InputIterator_>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BULK_INSERT );

		int index = _height - param->_height - 1;
		if ( !param->_height )
		{
			std::pair<leaf_iterator,bool> test = param->_sp_cur_leaf->test_insert( *param->_first );
			STXXL_ASSERT( test.first == param->_sp_cur_leaf->begin() );
			param->_sp_iterator->_leaf_iterator = test.first;
			if( !test.second )
			{
				if( ++param->_first == param->_last )
				{
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
				}
				else
				{
					param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_SEARCH;
					param->_height = _height;
					param->_sp_iterator = new iterator(this);
					if ( _height == 1 )
						param->_sp_cur_leaf = _sp_leaf_root;
					else
					{
						param->_sp_cur_leaf.release();
						param->_sp_cur_node = _sp_node_root;
					}
					abulk_insert( param );
				}
				return;
			}
		}
		else
		{
			param->_sp_iterator->_node_iterators[index] = param->_sp_cur_node->begin();
		}

		++param->_height;
		--index;

		// smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[index].node();
		if ( !param->_sp_iterator->_node_iterators[index].next() )
		{
			param->_height = 0;
			param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_INSERTING;
			abulk_insert(param);
			return;
		}
		else
		{
			bid_type bid = param->_sp_iterator->_node_iterators[index].node()->bid_next();
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
		}
	}

	template <typename InputIterator_>
	void _abulk_insert_move_prev( btrequest_bulk_insert<_Self,InputIterator_>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BULK_INSERT );

		int index = param->_height2 - 1;


		if ( ( (size_type) param->_height2 ) == _height )
		{
			STXXL_ASSERT( 0 );
		}
		else
		{
			param->_sp_iterator->_node_iterators[index] = param->_sp_cur_node->last();
		}

		++param->_height2;
		++index;

		if ( !param->_sp_iterator->_node_iterators[index].prev() )
		{
			param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_SEARCH;
			const_node_iterator cn_iter = param->_sp_iterator->_node_iterators[index];
			bid_type bid = (*cn_iter).second;
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		else
		{
			smart_ptr<_Node> sp_node = param->_sp_iterator->_node_iterators[index].node();
			bid_type bid = sp_node->bid_prev();
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	template <typename InputIterator_>
	void _abulk_insert_inserting( btrequest_bulk_insert<_Self,InputIterator_>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BULK_INSERT );

		if( !param->_height )
		{
			smart_ptr<_Leaf> sp_cur_leaf = param->_sp_iterator->_leaf_iterator.node();
			if ( !sp_cur_leaf->full() )
			{
				std::pair<leaf_iterator,bool> iter_pair = sp_cur_leaf->insert( *param->_first );

				_tree_size++;
				STXXL_ASSERT( iter_pair.second );
				param->_sp_iterator->_leaf_iterator = iter_pair.first;
				if( param->_sp_iterator->_leaf_iterator == sp_cur_leaf->begin() )
					_replace_key( *param->_sp_iterator, param->_height + 1, (*param->_first).first );

				++param->_first;
				if( param->_first == param->_last )
				{
					sp_cur_leaf.release();
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
				}
        else
				{
					param->_height = _height;
					param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_SEARCH;
					param->_sp_iterator = new iterator(this);
					if ( _height == 1 )
						param->_sp_cur_leaf = _sp_leaf_root;
					else
					{
						param->_sp_cur_leaf.release();
						param->_sp_cur_node = _sp_node_root;
					}
					abulk_insert( param );
				}
				return;
			}

			else   // Splittinig
			{
				smart_ptr<_Leaf> sp_new_leaf;

				leaf_manager::get_instance()->GetNewNode( &sp_new_leaf.ptr );
				sp_cur_leaf->splitt( sp_new_leaf.ptr );

				const_leaf_iterator cl_iter( sp_new_leaf.get(), 0); // begin
				node_value_type pair( ( *cl_iter ).first, sp_new_leaf->bid() );

				std::pair<leaf_iterator,bool> iter_pair;
				if ( _value_compare( ( *cl_iter ).first, (*param->_first).first ) )
					iter_pair = sp_new_leaf->insert( *param->_first );
				else
					iter_pair = sp_cur_leaf->insert( *param->_first );
				_tree_size++;
				STXXL_ASSERT( iter_pair.second );

				param->_sp_iterator->_leaf_iterator = iter_pair.first;
				if( param->_sp_iterator->_leaf_iterator == sp_cur_leaf->begin() )
					_replace_key( *param->_sp_iterator, param->_height + 1, (*param->_first).first );

				param->_node_pair = pair;

				bid_type next_bid = sp_cur_leaf->bid_next();
				sp_cur_leaf->bid_next( sp_new_leaf->bid() );
				sp_new_leaf->bid_next( next_bid );
				sp_new_leaf->bid_prev( sp_cur_leaf->bid() );

				if ( next_bid.valid() )
				{
					param->_bid = sp_new_leaf->bid();
					param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_REPLACE_BID_IN_NEXT_NODE;
					param->_sp_cur_leaf = NULL;
					leaf_manager::get_instance()->GetNode( next_bid, &param->_sp_cur_leaf.ptr, param ) ;
					return;
				}
				else
				{
					param->_height++;
					abulk_insert( param );
					return;
				}
			}
		}
		else
		{
			smart_ptr<_Node> sp_cur_node;
			if( _height == (unsigned)param->_height )
			{
				node_manager::get_instance()->GetNewNode( &sp_cur_node.ptr );

				if( _height == 1 )
					sp_cur_node->insert( node_value_type( _sp_leaf_root->begin()->first, _sp_leaf_root->bid() ));
				else
					sp_cur_node->insert( node_value_type( _sp_node_root->begin()->first, _sp_node_root->bid() ));

				std::pair<node_iterator,bool> iter_pair = sp_cur_node->insert( param->_node_pair );

				_sp_leaf_root = NULL;
				_sp_node_root = sp_cur_node;
				_height++;

				STXXL_ASSERT( iter_pair.second );
				param->_sp_iterator->_node_iterators.insert( param->_sp_iterator->_node_iterators.begin(),iter_pair.first );

				if( ++param->_first == param->_last )
				{
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
				}
				else
				{
					param->_height = _height;
					param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_SEARCH;
					param->_sp_iterator = new iterator(this);
					if ( _height == 1 )
						param->_sp_cur_leaf = _sp_leaf_root;
					else
					{
						param->_sp_cur_leaf.release();
						param->_sp_cur_node = _sp_node_root;
					}
					abulk_insert( param );
				}
				return;
			}

			sp_cur_node = param->_sp_iterator->_node_iterators[ _height - param->_height-1 ].node();

			if ( !sp_cur_node->full() )
			{
				std::pair<node_iterator,bool> iter_pair = sp_cur_node->insert( param->_node_pair );
				STXXL_ASSERT( iter_pair.second );
				param->_sp_iterator->_node_iterators[_height - param->_height-1] = iter_pair.first;
				if( param->_sp_iterator->_node_iterators[_height - param->_height-1] == sp_cur_node->begin() )
					_replace_key( *param->_sp_iterator, param->_height + 1, (*param->_first).first );

				if( ++param->_first == param->_last )
				{
					param->DisposeParam();
					param->set_to( btree_request<_Self>::DONE );
				}
				else
				{
					param->_height = _height;
					param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_SEARCH;
					param->_sp_iterator = new iterator(this);
					if ( _height == 1 )
						param->_sp_cur_leaf = _sp_leaf_root;
					else
					{
						param->_sp_cur_leaf.release();
						param->_sp_cur_node = _sp_node_root;
					}
					abulk_insert( param );
				}
				return;
			}
			else // Splitting
			{
				node_ptr sp_new_node = NULL;
				node_manager::get_instance()->GetNewNode( &sp_new_node.ptr );
				sp_cur_node->splitt( sp_new_node.get() );

				const_node_iterator cl_iter( sp_new_node.get(), 0);
				node_value_type pair( ( *cl_iter ).first, sp_new_node->bid() );

				std::pair<node_iterator,bool> iter_pair;
				if ( _value_compare( ( *cl_iter ).first, param->_node_pair.first ) )
					iter_pair = sp_new_node->insert( param->_node_pair );
				else
					iter_pair = sp_cur_node->insert( param->_node_pair );
				STXXL_ASSERT( iter_pair.second );

				param->_sp_iterator->_node_iterators[_height - param->_height-1] = iter_pair.first;
				if( param->_sp_iterator->_node_iterators[_height - param->_height-1] == sp_cur_node->begin() )
					_replace_key( *param->_sp_iterator, param->_height + 1, param->_node_pair.first );

				param->_node_pair = pair;

				bid_type next_bid = sp_cur_node->bid_next();
				sp_cur_node->bid_next( sp_new_node->bid() );
				sp_new_node->bid_next( next_bid );
				sp_new_node->bid_prev( sp_cur_node->bid() );

				if ( next_bid.valid() )
				{
					param->_bid = sp_new_node->bid();
					sp_new_node.release();
					sp_cur_node.release();
					param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_REPLACE_BID_IN_NEXT_NODE;
					param->_sp_cur_node.release();
					node_manager::get_instance()->GetNode( next_bid, &param->_sp_cur_node.ptr, param ) ;
				}
				else
				{
					param->_height++;
					abulk_insert( param );
				}
			}
		}
	}

	template <typename InputIterator_>
	void _abulk_insert_replace_bid_in_next_node( btrequest_bulk_insert<_Self,InputIterator_>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BULK_INSERT );

		if( !param->_height )
		{
			param->_sp_cur_leaf->bid_prev( param->_bid );
			param->_sp_cur_leaf->set_dirty( true );
		}
		else
		{
			param->_sp_cur_node->bid_prev( param->_bid );
			param->_sp_cur_node->set_dirty( true );
		}
		param->_phase = btrequest_bulk_insert<_Self,InputIterator_>::PHASE_INSERTING;
		param->_height++;
		abulk_insert( param );
	}


	/***************************************************************************************
	****************************************************************************************

	INSERT A PAIR

	****************************************************************************************
	***************************************************************************************/

	//! \brief Implementing of inseration of a pair
	void ainsert( btrequest_insert<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::INSERT );

		// ***********************************************************************************
		// we check if ainsert already calleed
		// ***********************************************************************************
				if ( param->_height < 0 )
				{
					STXXL_ASSERT( param->_pp_iterator );
					STXXL_ASSERT( !*param->_pp_iterator );

		// *********************************************************************************
		// the tree is empty
		// we can insert directly and leave the function
		// *********************************************************************************
					if ( !_tree_size )
					{
						_tree_size++;
						std::pair<leaf_iterator,bool> ret = _sp_leaf_root->insert( param->_pair );

						*param->_pp_iterator = new iterator( this );
						(*param->_pp_iterator)->add_ref();

						if( param->_p_bool ) *param->_p_bool = ret.second;
						(*param->_pp_iterator)->_leaf_iterator = ret.first ;

						STXXL_ASSERT( ret.second );
						param->DisposeParam();
						param->set_to( btree_request<_Self>::DONE );
						return;
					}

		// *********************************************************************************
		// tree is not empty we init param
		// *********************************************************************************

					*param->_pp_iterator = new iterator( this );
					(*param->_pp_iterator)->add_ref();
					param->_height = _height;

					if ( _height == 1 )
						param->_sp_cur_leaf = _sp_leaf_root;
					else
						param->_sp_cur_node = _sp_node_root;
				}

		// *********************************************************************************
		//
		// *********************************************************************************

				switch( param->_phase )
				{

					case btrequest_insert<_Self>::PHASE_SEARCH:
						_ainsert_search( param );                           break;

					case btrequest_insert<_Self>::PHASE_MOVE_NEXT:
						_ainsert_move_next( param );                        break;

					case btrequest_insert<_Self>::PHASE_MOVE_PREV:
						_ainsert_move_prev( param );                        break;

					case btrequest_insert<_Self>::PHASE_INSERTING:
						_ainsert_inserting( param );                        break;

					case btrequest_insert<_Self>::PHASE_REPLACE_BID_IN_NEXT_NODE:
						_ainsert_replace_bid_in_next_node( param );         break;

					default : STXXL_ASSERT( 0 );
				}
	}


	void _ainsert_search( btrequest_insert<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::INSERT );
		STXXL_ASSERT( param->_height > 0 );

		//************************************************************************************
		// param->_height contains previusly height, the current height have to decrease
		// param->_height = 0 : means we are in a leaf
		// param->_height > 0 : means we are in the node
		//************************************************************************************

		if ( !--param->_height )
			_ainsert_search_leaf( param );
		else
			_ainsert_search_node( param );
	}


	void _ainsert_search_leaf( btrequest_insert<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::INSERT );
		STXXL_ASSERT( !param->_height );

		//************************************************************************************
		// LEAF
		//************************************************************************************
		// we test if insert and which position we have to use for insert
		//************************************************************************************

					std::pair<leaf_iterator,bool> test = param->_sp_cur_leaf->test_insert( param->_pair );
					(*param->_pp_iterator)->_leaf_iterator = test.first;


		//************************************************************************************
		// test failed
		//************************************************************************************
					if( !test.second )
					{
						if( param->_p_bool ) *param->_p_bool = false;
						param->DisposeParam();
						param->set_to( btree_request<_Self>::DONE );
						return;
					}

		//************************************************************************************
		// test was sucess
		//************************************************************************************


					STXXL_ASSERT( test.second );

					if ( (*param->_pp_iterator)->_leaf_iterator == param->_sp_cur_leaf->end() )
					{
						//param->_sp_cur_leaf->dump( std::cout );
						if( !param->_sp_cur_leaf->bid_next().valid() )
						{
							(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->last();
						}
						else
						{

							// We have to find the begin of next leaf
							param->_phase = btrequest_insert<_Self>::PHASE_MOVE_NEXT;
							bid_type bid = param->_sp_cur_leaf->bid_next();
							STXXL_ASSERT( bid.valid() );
							param->_sp_cur_leaf.release();
							STXXL_ASSERT( !param->_sp_cur_node.get() ); // param->_sp_cur_node.release();
							leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
							return;
						}
					}

					param->_phase = btrequest_insert<_Self>::PHASE_INSERTING;
					ainsert(param);
	}


	void _ainsert_search_node( btrequest_insert<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::INSERT );
		STXXL_ASSERT( param->_height > 0 );

		//************************************************************************************
		// param->_height contains previusly height, the current height have to decrease
		// param->_height = 0 : means we are in a leaf
		// param->_height > 0 : means we are in the node
		//************************************************************************************


		//************************************************************************************
		// NODE
		//************************************************************************************
		// we stil search for key
		//************************************************************************************

				node_iterator n_iter = param->_sp_cur_node->lower_bound( param->_pair.first );

				// ********************************************************
				// all keys are less
				// ********************************************************

				if ( n_iter == param->_sp_cur_node->end() )
					n_iter = param->_sp_cur_node->last();


				// ********************************************************
				// there is a key is less
				// ********************************************************
				else if ( n_iter != param->_sp_cur_node->begin() )
					--n_iter;


				else if ( param->_sp_cur_node->bid_prev().valid() )
				{
					// the key is in the previosly node
					param->_phase = btrequest_insert<_Self>::PHASE_MOVE_PREV;
					param->_height2 = param->_height;
					bid_type bid = param->_sp_cur_node->bid_prev();
					STXXL_ASSERT( param->_sp_cur_leaf.get() ); // param->_sp_cur_leaf.release();
					param->_sp_cur_node.release();
					node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
					return;
				}

				(*param->_pp_iterator)->_node_iterators.push_back( n_iter );


		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

					const_node_iterator cn_iter = n_iter;
					bid_type bid = (*cn_iter).second;
					STXXL_ASSERT( bid.valid() );
					STXXL_ASSERT( !param->_sp_cur_leaf.get() ); // param->_sp_cur_leaf.release();
					param->_sp_cur_node.release();

					if( param->_height == 1 )
						leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
					else
						node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
	}

	void _ainsert_move_next( btrequest_insert<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::INSERT );

		int index = _height - param->_height - 1;

		if ( !param->_height )
		{
			std::pair<leaf_iterator,bool> test = param->_sp_cur_leaf->test_insert( param->_pair );

			STXXL_ASSERT( test.first == param->_sp_cur_leaf->begin() );
			(*param->_pp_iterator)->_leaf_iterator = test.first;
			if( !test.second )
			{
				if( param->_p_bool )*param->_p_bool = false;
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}
		}
		else
		{
			(*param->_pp_iterator)->_node_iterators[index] = param->_sp_cur_node->begin();
		}

		++param->_height;
		--index;

		// smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[index].node();
		if ( !(*param->_pp_iterator)->_node_iterators[index].next() )
		{
			param->_height = 0;
			param->_phase = btrequest_insert<_Self>::PHASE_INSERTING;
			ainsert(param);
			return;
		}
		else
		{
			bid_type bid = (*param->_pp_iterator)->_node_iterators[index].node()->bid_next();
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
		}
	}

	void _ainsert_move_prev( btrequest_insert<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::INSERT );

		int index = param->_height2 - 1;


		if ( ( (size_type) param->_height2 ) == _height )
		{
			STXXL_ASSERT( 0 );
		}
		else
		{
			(*param->_pp_iterator)->_node_iterators[index] = param->_sp_cur_node->last();
		}

		++param->_height2;
		++index;

		if ( !(*param->_pp_iterator)->_node_iterators[index].prev() )
		{
			param->_phase = btrequest_insert<_Self>::PHASE_SEARCH;
			const_node_iterator cn_iter = (*param->_pp_iterator)->_node_iterators[index];
			bid_type bid = (*cn_iter).second;
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		else
		{
			smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[index].node();
			bid_type bid = sp_node->bid_prev();
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
	}

	void _ainsert_inserting( btrequest_insert<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::INSERT );

		if( !param->_height )
		{
			smart_ptr<_Leaf> sp_cur_leaf = (*param->_pp_iterator)->_leaf_iterator.node();
			if ( !sp_cur_leaf->full() )
			{
				std::pair<leaf_iterator,bool> iter_pair = sp_cur_leaf->insert( param->_pair );

				_tree_size++;
				STXXL_ASSERT( iter_pair.second );
				(*param->_pp_iterator)->_leaf_iterator = iter_pair.first;
				if( param->_p_bool )*param->_p_bool = iter_pair.second;
				if( (*param->_pp_iterator)->_leaf_iterator == sp_cur_leaf->begin() )
					_replace_key( **param->_pp_iterator, param->_height + 1, param->_pair.first );

				sp_cur_leaf.release();
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );

				return;
			}

			else   // Splittinig
			{
				smart_ptr<_Leaf> sp_new_leaf;

				leaf_manager::get_instance()->GetNewNode( &sp_new_leaf.ptr );
				sp_cur_leaf->splitt( sp_new_leaf.ptr );

				const_leaf_iterator cl_iter( sp_new_leaf.get(), 0); // begin
				node_value_type pair( ( *cl_iter ).first, sp_new_leaf->bid() );

				std::pair<leaf_iterator,bool> iter_pair;
				if ( _value_compare( ( *cl_iter ).first, param->_pair.first ) )
					iter_pair = sp_new_leaf->insert( param->_pair );
				else
					iter_pair = sp_cur_leaf->insert( param->_pair );
				_tree_size++;
				STXXL_ASSERT( iter_pair.second );

				(*param->_pp_iterator)->_leaf_iterator = iter_pair.first;
				if( param->_p_bool )*param->_p_bool = iter_pair.second;
				if( (*param->_pp_iterator)->_leaf_iterator == sp_cur_leaf->begin() )
					_replace_key( **param->_pp_iterator, param->_height + 1, param->_pair.first );

				param->_node_pair = pair;

				bid_type next_bid = sp_cur_leaf->bid_next();
				sp_cur_leaf->bid_next( sp_new_leaf->bid() );
				sp_new_leaf->bid_next( next_bid );
				sp_new_leaf->bid_prev( sp_cur_leaf->bid() );

				if ( next_bid.valid() )
				{
					param->_bid = sp_new_leaf->bid();
					param->_phase = btrequest_insert<_Self>::PHASE_REPLACE_BID_IN_NEXT_NODE;
					param->_sp_cur_leaf = NULL;
					leaf_manager::get_instance()->GetNode( next_bid, &param->_sp_cur_leaf.ptr, param ) ;
					return;
				}
				else
				{
					param->_height++;
					ainsert( param );
					return;
				}
			}
		}
		else
		{
			smart_ptr<_Node> sp_cur_node;
			if( _height == (unsigned)param->_height )
			{
				node_manager::get_instance()->GetNewNode( &sp_cur_node.ptr );

				if( _height == 1 )
					sp_cur_node->insert( node_value_type( _sp_leaf_root->begin()->first, _sp_leaf_root->bid() ));
				else
					sp_cur_node->insert( node_value_type( _sp_node_root->begin()->first, _sp_node_root->bid() ));

				std::pair<node_iterator,bool> iter_pair = sp_cur_node->insert( param->_node_pair );

				_sp_leaf_root = NULL;
				_sp_node_root = sp_cur_node;
				_height++;

				STXXL_ASSERT( iter_pair.second );
				(*param->_pp_iterator)->_node_iterators.insert(
					(*param->_pp_iterator)->_node_iterators.begin(),
					iter_pair.first );

				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			sp_cur_node = (*param->_pp_iterator)->_node_iterators[ _height - param->_height-1 ].node();

			if ( !sp_cur_node->full() )
			{

				std::pair<node_iterator,bool> iter_pair = sp_cur_node->insert( param->_node_pair );
				STXXL_ASSERT( iter_pair.second );
				(*param->_pp_iterator)->_node_iterators[_height - param->_height-1] = iter_pair.first;
				if( (*param->_pp_iterator)->_node_iterators[_height - param->_height-1] == sp_cur_node->begin() )
					_replace_key( **param->_pp_iterator, param->_height + 1, param->_pair.first );

				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );

				return;
			}
			else // Splitting
			{

//				STXXL_MSG( "INSERT (1)" );
//				sp_cur_node->dump( std::cout );


				node_ptr sp_new_node = NULL;
				node_manager::get_instance()->GetNewNode( &sp_new_node.ptr );
				sp_cur_node->splitt( sp_new_node.get() );

				const_node_iterator cl_iter( sp_new_node.get(), 0);
				node_value_type pair( ( *cl_iter ).first, sp_new_node->bid() );

				std::pair<node_iterator,bool> iter_pair;
				if ( _value_compare( ( *cl_iter ).first, param->_node_pair.first ) )
					iter_pair = sp_new_node->insert( param->_node_pair );
				else
					iter_pair = sp_cur_node->insert( param->_node_pair );
				STXXL_ASSERT( iter_pair.second );

				(*param->_pp_iterator)->_node_iterators[_height - param->_height-1] = iter_pair.first;
				if( (*param->_pp_iterator)->_node_iterators[_height - param->_height-1] == sp_cur_node->begin() )
					_replace_key( **param->_pp_iterator, param->_height + 1, param->_node_pair.first );

				param->_node_pair = pair;


				bid_type next_bid = sp_cur_node->bid_next();
				sp_cur_node->bid_next( sp_new_node->bid() );
				sp_new_node->bid_next( next_bid );
				sp_new_node->bid_prev( sp_cur_node->bid() );

//				STXXL_MSG( "INSERT (2)" );
				//sp_cur_node->dump( std::cout );

//				STXXL_MSG( "INSERT (3)" );
				//sp_new_node->dump( std::cout );

				if ( next_bid.valid() )
				{
					param->_bid = sp_new_node->bid();
					sp_new_node.release();
					sp_cur_node.release();
					param->_phase = btrequest_insert<_Self>::PHASE_REPLACE_BID_IN_NEXT_NODE;
					param->_sp_cur_node.release();
					node_manager::get_instance()->GetNode( next_bid, &param->_sp_cur_node.ptr, param ) ;
				}
				else
				{
					param->_height++;
					ainsert( param );
				}
			}
		}
	}

	void _ainsert_replace_bid_in_next_node( btrequest_insert<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::INSERT );

		if( !param->_height )
		{
			param->_sp_cur_leaf->bid_prev( param->_bid );
			param->_sp_cur_leaf->set_dirty( true );
		}
		else
		{
			param->_sp_cur_node->bid_prev( param->_bid );
			param->_sp_cur_node->set_dirty( true );
		}
		param->_phase = btrequest_insert<_Self>::PHASE_INSERTING;
		param->_height++;
		ainsert( param );
	}


	// *********************************************************************************
	// *********************************************************************************
	// CLEAR
	// *********************************************************************************
	// *********************************************************************************



	void aclear( btrequest_clear<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CLEAR );

		// **********************************************************************************
		// all nodes and leafs will be release, but root
		// **********************************************************************************
					_tree_size = 0;


		// *********************************************************************
		// init params for the first call
		// *********************************************************************
					if ( param->_height < 0 )
					{
						STXXL_ASSERT( param->_phase == btrequest_clear<_Self>::PHASE_VERTICAL );
						if ( _height == 1 )
						{
							_sp_leaf_root->clear();
							param->DisposeParam();
							param->set_to( btree_request<_Self>::DONE ) ; // set to DONE
							return;
						}
						else
						{
							param->_p_sp_node = new smart_ptr<_Node>[ _height - 1 ];
							param->_height = _height - 2;

							// set the first node for vertikal
							param->_p_sp_node[ param->_height ] = _sp_node_root;
							_sp_node_root.release();

							// now is the tree empty
							leaf_manager::get_instance()->GetNewNode( &_sp_leaf_root.ptr );
						}
					}

		// **********************************************************************************
		// Right operation woll be found
		// **********************************************************************************

					switch( param->_phase )
					{
						case btrequest_clear<_Self>::PHASE_HORIZONTAL:
							_aclear_horizontal(param); break;

						case btrequest_clear<_Self>::PHASE_VERTICAL:
							_aclear_vertical( param ); break;

						default :
							STXXL_ASSERT( 0 ); break;
					}
	}

	void _aclear_vertical( btrequest_clear<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CLEAR );
		STXXL_ASSERT( param->_phase == btrequest_clear<_Self>::PHASE_VERTICAL );

		// ********************************************************************************
		// initialize the node on the path to begin of tree
		// ********************************************************************************

					if ( !param->_height )
					{
						// now the most left nodes are in our array, we can begin release of nodes/leafs
						param->_phase = btrequest_clear<_Self>::PHASE_HORIZONTAL;
						aclear( param );
					}
					else
					{
						const_node_iterator iter = param->_p_sp_node[param->_height]->begin();
						--param->_height;
						bid_type bid( (*iter).second );
						STXXL_ASSERT( bid.valid() );
						param->_p_sp_node[param->_height].release();
						node_manager::get_instance()->GetNode( bid, &param->_p_sp_node[param->_height].ptr, param );
					}
	}

	void _aclear_horizontal( btrequest_clear<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CLEAR );
		STXXL_ASSERT( param->_phase == btrequest_clear<_Self>::PHASE_HORIZONTAL );


		// ********************************************************************************
		// release all nodes or leaf, childs of curent node
		// ********************************************************************************

					const_node_iterator iter = param->_p_sp_node[param->_height]->begin();
					while( iter != (const_node_iterator) param->_p_sp_node[param->_height]->end() )
					{
						if ( !param->_height )
							leaf_manager::get_instance()->ReleaseNode( (*iter).second );
						else
							node_manager::get_instance()->ReleaseNode( (*iter).second );
						++iter;
					}

		// ********************************************************************************
		// take the next node in curren hight
		// ********************************************************************************

					if( param->_p_sp_node[param->_height]->bid_next().valid() )
					{
						bid_type bid = param->_p_sp_node[param->_height]->bid_next();
						param->_p_sp_node[param->_height].release();
						node_manager::get_instance()->GetNode( bid, &param->_p_sp_node[param->_height].ptr, param );
						return;
					}

		// ********************************************************************************
		// no next nodes in current hight
		// take the parents to releasing
		// ********************************************************************************

					if ( param->_height == (int)_height - 2 )
					{
						// we are in the old root end we can finish
						node_manager::get_instance()->ReleaseNode( param->_p_sp_node[param->_height]->bid() );
						_height = 1;
						param->DisposeParam();
						param->set_to( btree_request<_Self>::DONE ) ; // set to DONE
					}
					else
					{
						param->_height++;
						aclear( param );
					}
	};

	/***************************************************************************************
	****************************************************************************************

	PREV

	****************************************************************************************
	***************************************************************************************/

	void aprev( btrequest_prev<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::PREV );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
		if ( param->_height < 0 )
		{
			if ( !_tree_size )
			{
				STXXL_ASSERT( !*param->_pp_iterator );
				**param->_pp_iterator = end();
				(*param->_pp_iterator)->add_ref();
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			if ( _height == 1 )
			{
				--(*param->_pp_iterator)->_leaf_iterator;
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			if ( !(*param->_pp_iterator)->_leaf_iterator.prev() )
			{
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			smart_ptr<_Leaf> sp_leaf = (*param->_pp_iterator)->_leaf_iterator.node();
			if( !sp_leaf->bid_prev().valid() )
			{
				(*param->_pp_iterator)->_leaf_iterator = _Leaf::nil.end();
				(*param->_pp_iterator)->_node_iterators.clear();
				param->DisposeParam();
				sp_leaf.release();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			param->_height = _height;
			param->_sp_cur_leaf.release();
			leaf_manager::get_instance()->GetNode( sp_leaf->bid_prev(), &param->_sp_cur_leaf.ptr, param ) ;
			return;
		}
		_aprev( param );
	}

	void _aprev( btrequest_prev<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::PREV );

		if ( ((size_type) param->_height) == _height )
		{
			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->last();
		}
		else
		{
			(*param->_pp_iterator)->_node_iterators[param->_height-1] = param->_sp_cur_node->last();
		}

		if ( !(*param->_pp_iterator)->_node_iterators[param->_height-2].prev() )
		{
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		bid_type bid = (*param->_pp_iterator)->_node_iterators[param->_height-2].node()->bid_prev();
		STXXL_ASSERT( bid.valid() );
		--param->_height;
		param->_sp_cur_node.release();
		node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
	}

	void aprev( btrequest_const_prev<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_PREV );
		STXXL_ASSERT( param->_pp_const_iterator );
		STXXL_ASSERT( *param->_pp_const_iterator );

		if ( param->_next )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->last();
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		if ( !_tree_size )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = _Leaf::nil.end();
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		if ( (*param->_pp_const_iterator)->_leaf_iterator == _Leaf::nil.end() )
		{
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		if ( _height == 1 ||
		(*param->_pp_const_iterator)->_leaf_iterator !=
		(const_leaf_iterator) (*param->_pp_const_iterator)->_leaf_iterator.node()->begin() )
		{
			--(*param->_pp_const_iterator)->_leaf_iterator;
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		if( !(*param->_pp_const_iterator)->_leaf_iterator.node()->bid_prev().valid() )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = _Leaf::nil.end();
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		bid_type bid = (*param->_pp_const_iterator)->_leaf_iterator.node()->bid_prev();
		STXXL_ASSERT( bid.valid() );
		param->_next = true;
		param->_sp_cur_leaf.release();
		leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param );
	}

	/***************************************************************************************
	****************************************************************************************

	NEXT

	****************************************************************************************
	***************************************************************************************/

	void anext( btrequest_next<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::NEXT );
		STXXL_ASSERT( param->_pp_iterator );
		STXXL_ASSERT( *param->_pp_iterator );


		// *********************************************************************
		// init params for the first call
		// *********************************************************************
		if ( param->_height < 0 )
		{
			if ( !_tree_size )
			{
				(*param->_pp_iterator)->_leaf_iterator == _Leaf::nil.end();
				(*param->_pp_iterator)->_node_iterators.clear();

				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			if ( (*param->_pp_iterator)->_leaf_iterator == _Leaf::nil.end() )
			{
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			smart_ptr<_Leaf> sp_leaf = (*param->_pp_iterator)->_leaf_iterator.node();
			++(*param->_pp_iterator)->_leaf_iterator;

			if ( (*param->_pp_iterator)->_leaf_iterator != sp_leaf->end() )
			{
				param->DisposeParam();
				sp_leaf.release();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			if( !sp_leaf->bid_next().valid() )
			{
				(*param->_pp_iterator)->_leaf_iterator = _Leaf::nil.end();
				(*param->_pp_iterator)->_node_iterators.clear();

				param->DisposeParam();
				sp_leaf.release();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			bid_type bid = sp_leaf->bid_next();
			STXXL_ASSERT( bid.valid() );
			param->_height = _height;
			param->_sp_cur_leaf.release();
			sp_leaf.release();
			leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param );
			return;
		}
		_anext( param );
	}

	void _anext( btrequest_next<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::NEXT );

		if ( ((unsigned) param->_height) == _height )
		{
			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin();
		}
		else
		{
			(*param->_pp_iterator)->_node_iterators[param->_height-1] = param->_sp_cur_node->begin();
		}

		smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[param->_height-2].node();
		if ( !(*param->_pp_iterator)->_node_iterators[param->_height-2].next() )
		{
			param->DisposeParam();
			sp_node.release();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		--param->_height;
		bid_type bid = sp_node->bid_next();
//		if ( !bid.valid() ) sp_node->dump( std::cout );
		STXXL_ASSERT( bid.valid() );
		param->_sp_cur_node.release();
		sp_node.release();
		node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
	}

	void anext( btrequest_const_next<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_NEXT );
		STXXL_ASSERT( param->_pp_const_iterator );
		STXXL_ASSERT( *param->_pp_const_iterator );

		if ( param->_next )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin();
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		if ( !_tree_size )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = _Leaf::nil.end();
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}


  if ( (*param->_pp_const_iterator)->_leaf_iterator == _Leaf::nil.end() )
		{
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		smart_ptr<_Leaf> sp_leaf = (*param->_pp_const_iterator)->_leaf_iterator.node();
		++(*param->_pp_const_iterator)->_leaf_iterator;

		const_leaf_iterator iter1( (*param->_pp_const_iterator)->_leaf_iterator );
		const_leaf_iterator iter2( sp_leaf->end() );

		if ( iter1 != iter2 )
		{
			param->DisposeParam();
			sp_leaf.release();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		if( !sp_leaf->bid_next().valid() )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = _Leaf::nil.end();
			param->DisposeParam();
			sp_leaf.release();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		bid_type bid = sp_leaf->bid_next();
		STXXL_ASSERT( bid.valid() );
		param->_next = true;
		param->_sp_cur_leaf.release();
		sp_leaf.release();
		leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param );
		return;
	}

	/***************************************************************************************
	****************************************************************************************

	BEGIN

	****************************************************************************************
	***************************************************************************************/

	void abegin( btrequest_begin<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BEGIN );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
		if ( param->_height < 0 )
		{
			if ( !_tree_size )
			{
				STXXL_ASSERT( !*param->_pp_iterator );
				*param->_pp_iterator = new iterator( end());
				(*param->_pp_iterator)->add_ref();
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			STXXL_ASSERT( !*param->_pp_iterator );
			*param->_pp_iterator = new iterator( this );
			(*param->_pp_iterator)->add_ref();
			if ( _height == 1 )
			{
				param->_sp_cur_leaf = _sp_leaf_root;
			}
			else
			{
				param->_sp_cur_node = _sp_node_root;
			}
			param->_height = _height;
		}
		_abegin( param );
	}

	void _abegin( btrequest_begin<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::BEGIN );
		if ( !--param->_height )
		{
			STXXL_ASSERT( param );
			STXXL_ASSERT( (*param->_pp_iterator) );
			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin() ;
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		node_iterator n_iter( param->_sp_cur_node->begin() );
		(*param->_pp_iterator)->_node_iterators.push_back( n_iter );

		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

		const_node_iterator cn_iter = n_iter;
		if( param->_height == 1 )
		{
			param->_sp_cur_node.release();
			param->_sp_cur_leaf.release();
			leaf_manager::get_instance()->GetNode( (*cn_iter).second, &param->_sp_cur_leaf.ptr, param ) ;
		}
		else
		{
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( (*cn_iter).second, &param->_sp_cur_node.ptr, param );
		}
	}

	void abegin( btrequest_const_begin<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_BEGIN );

		// *********************************************************************
		// init params for the first call
		// *********************************************************************
		if ( param->_height < 0 )
		{
			if ( !_tree_size )
			{
				STXXL_ASSERT( !*param->_pp_const_iterator );
				*param->_pp_const_iterator = new const_iterator( end());
				(*param->_pp_const_iterator)->add_ref();
				param->DisposeParam();
				param->set_to( btree_request<_Self>::DONE );
				return;
			}

			STXXL_ASSERT( !*param->_pp_const_iterator );
			*param->_pp_const_iterator = new const_iterator( this );
			(*param->_pp_const_iterator)->add_ref();
			if ( _height == 1 )
			{
				param->_sp_cur_leaf = _sp_leaf_root;
			}
			else
			{
				param->_sp_cur_node = _sp_node_root;
			}
			param->_height = _height;
		}
		_abegin( param );
	}

	void _abegin( btrequest_const_begin<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_BEGIN );

		if ( !--param->_height )
		{
			STXXL_ASSERT( param );
			STXXL_ASSERT( (*param->_pp_const_iterator) );
			(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin() ;
			param->DisposeParam();
			param->set_to( btree_request<_Self>::DONE );
			return;
		}

		const_node_iterator c_iter( param->_sp_cur_node->begin() );

		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

		if( param->_height == 1 )
		{
			param->_sp_cur_node.release();
			param->_sp_cur_leaf.release();
			leaf_manager::get_instance()->GetNode( (*c_iter).second, &param->_sp_cur_leaf.ptr, param ) ;
		}
		else
		{
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( (*c_iter).second, &param->_sp_cur_node.ptr, param );
		}
	}

  public:

	// *******************************************************************************
	// Constructors for btree
	// *******************************************************************************

	btree() : _height(1), _tree_size(0)
	{
		leaf_manager::get_instance()->GetNewNode( &_sp_leaf_root.ptr );
	}

	btree(const Compare_& comp) : _height(1), _tree_size(0)
	{
		leaf_manager::get_instance()->GetNewNode( &_sp_leaf_root.ptr );
	}

	btree(const btree& x) : _height(1), _tree_size(0)
	{
		_insert_into_empty_tree( x.begin(), x.end() );
	}

	template <typename InputIterator_>
	btree( InputIterator_ first, InputIterator_ last ) : _height(1), _tree_size(0)
	{
		_insert_into_empty_tree( first, last );
	}

	~btree()
	{
		STXXL_ASSERT( _sp_leaf_root->bid().valid() );

		bid_type bid = _sp_leaf_root->bid();
		leaf_manager::get_instance()->ReleaseNode( bid );
		_sp_leaf_root.release();
	}

	_Self& operator=( const _Self& tree )
	{
		STXXL_ASSERT( 0 );
		return *this;
	}

	iterator end()
	{
		iterator iter( this );
		iter._leaf_iterator = _Leaf::nil.end();
		return iter;
	}

	const_iterator end() const
	{
		return const_iterator( this, _Leaf::nil.end() );
	}

	bool operator == ( const _Self& tree ) const
	{
		const_iterator iter1 = begin();
		const_iterator iter2 = tree.begin();
		if ( _tree_size != tree._tree_size ) return false;
		while( iter1 != end() && iter2 != tree.end() && *iter1 == *iter2 )
		{
			++iter1 ;
			++iter2 ;
		}
		if ( iter1 != end() || iter2 != tree.end() ) return false;
		else return true;
	}

	bool operator < ( const _Self& tree ) const
	{
		const_iterator iter1 = begin();
		const_iterator iter2 = tree.begin();
		while( iter1 != end() && iter2 != tree.end() && *iter1 == *iter2 )
		{
			++iter1 ;
			++iter2 ;
		}
		if ( iter1 != end() && iter2 != tree.end() ) return _value_compare( *iter1, *iter2 );
		else if ( iter1 == end() ) return true;
		else return false;
	}

	// NO STD FUNKTIONS

	void _replace_key( iterator& iter, unsigned height, Key_ key );

	void _dump( btrequest_dump<_Self>* param )
	{
		if( param->_height )
		{
			if ( param->_height == 1 )
			{
				param->_sp_leaf->dump( std::cout );
				bid_type bid = param->_sp_leaf->bid_next();
				if( bid.valid() )
				{
					param->_sp_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_leaf.ptr, param );
					return;
				}
				param->_height--;
			}
			else
			{
				param->_sp_node->dump( std::cout );
				bid_type bid = param->_sp_node->bid_next();


				if( param->_bid_first_next == param->_sp_node->bid() )
					param->_bid_first_next = (*param->_sp_node->begin()).second;

				if( bid.valid() )
				{
					param->_sp_node.release();
					node_manager::get_instance()->GetNode( bid, &param->_sp_node.ptr, param );
					return;
				}

				assert( param->_bid_first_next.valid() );
				param->_height --;

				if( param->_height == 1 )
				{
					param->_sp_leaf.release();
					leaf_manager::get_instance()->GetNode(
						param->_bid_first_next, &param->_sp_leaf.ptr, param );
					return;
				}
				else
				{
					param->_sp_node.release();
					node_manager::get_instance()->GetNode(
						param->_bid_first_next, &param->_sp_node.ptr, param );
					return;
				}
			}
		}

		param->_sp_node.release();
		param->_sp_leaf.release();
		param->set_to( btree_request<_Self>::DONE );
	}

	static void dump_managers()
	{
		STXXL_MSG( "Nodes:" );
		node_manager::get_instance()->dump();
		STXXL_MSG( "Leafs:" );
		leaf_manager::get_instance()->dump();
	}

	void dump( btrequest_dump<_Self>* param )
	{
		if( param->_height == -1 )
		{
			param->_height = _height;
			if( _height == 1 )
			{
				param->_sp_leaf = _sp_leaf_root;
			}
			else
			{
				param->_sp_node = _sp_node_root;
				param->_bid_first_next = (*param->_sp_node->begin()).second;
			}
		}
		_dump( param );
	}
};

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	inline
	bool operator==( const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& x, const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& y )
	{ return x.operator==( y ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	inline
	bool operator!=( const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& x, const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& y )
	{ return !( x.operator==( y) ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	inline
	bool operator<( const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& x, const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& y )
	{ return !( x.operator<( y) ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	inline
	bool operator>( const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& x, const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& y )
	{ return y.operator<(x); }

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	inline
	bool operator>=( const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& x, const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& y )
	{ return !( x.operator<( y) ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	inline
	bool operator<=( const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& x, const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& y )
	{ return ! y.operator<(x) ; }

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	std::ostream& operator << ( std::ostream & _str , const btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>& _tree )
	{ return _tree.operator<<( _str ); }

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	btree_queue<btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_> > btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_btree_queue;

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	void btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_replace_key( 
		typename btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::iterator& iter, unsigned height, Key_ key )
	{
		if ( iter == end() || height > _height ) return;
		if ( height == 1 )
		{
			if (  iter._leaf_iterator.offset() == 0 && height < _height )
			{
				leaf_iterator leaf_iter = iter._leaf_iterator;
				_replace_key( iter, height + 1, key );
				++leaf_iter; // then next place to inserting will be moved
			}
		}
		else if ( height != 1 )
		{
			node_iterator node_iter = iter._node_iterators[ _height - height ];
			node_ptr sp_node = node_iter.node() ;
			sp_node->replace_key( node_iter, key );
			if ( node_iter.offset() == 0 && height < _height  )
			{
				_replace_key( iter, height + 1, key );
			}
		}
	}

	/***************************************************************************************
	****************************************************************************************

	FIND

	****************************************************************************************
	***************************************************************************************/

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_afind_search( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::FIND );

		node_iterator n_iter;

			// **************************************************************************************
			// we are in a leaf
			// and we have to find the right leaf
			// **************************************************************************************

				if ( !--param->_height )
				{
					(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->lower_bound( param->_key );
					if ( (*param->_pp_iterator)->_leaf_iterator == param->_sp_cur_leaf->end() )
					{
						if( !param->_sp_cur_leaf->bid_next().valid() )
						{
							// there are no next leaf and no lower_bound
							// lower_bound = end()
							STXXL_ASSERT( *param->_pp_iterator );
							**param->_pp_iterator = end();
							return true;
						}

						// We have to find the begin of next leaf
						param->_phase = btrequest_find<_Self>::PHASE_MOVE_NEXT;
						param->_height = _height;

						bid_type bid = param->_sp_cur_leaf->bid_next();
						STXXL_ASSERT( bid.valid() );

						param->_sp_cur_leaf.release();
						STXXL_ASSERT( !param->_sp_cur_node.ptr ); // allready released
						leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
						return false;
					}

					const_leaf_iterator cn_iter((*param->_pp_iterator)->_leaf_iterator);
					if( (*cn_iter).first != param->_key )
					{
						STXXL_ASSERT( *param->_pp_iterator );
						**param->_pp_iterator = end();
						( *param->_pp_iterator )->add_ref();
					}
					return true;
				}

		// ***********************************************************************************
		// we are in internal nodes
		// ***********************************************************************************

				else
				{
					n_iter = param->_sp_cur_node->lower_bound( param->_key );

		// ***********************************************************************************
		// all elemenets are less then key -> search in the last node
		// ***********************************************************************************

					if ( n_iter == param->_sp_cur_node->end() )
						n_iter = param->_sp_cur_node->last();

		// ***********************************************************************************
		// it is not the first element
		// ***********************************************************************************

					else if ( n_iter != param->_sp_cur_node->begin() )
						--n_iter;

		// ***********************************************************************************
		// the first element in tree is it.
		// ***********************************************************************************
					else
					{
						STXXL_ASSERT( !param->_sp_cur_node->bid_prev().valid() );
					}
					(*param->_pp_iterator)->_node_iterators.push_back( n_iter );

				}

		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

				const_node_iterator cn_iter = n_iter;

		// ***************************************************************************************
		// the next one is a leaf
		// ***************************************************************************************

				if( param->_height == 1 )
				{
					bid_type bid = (*cn_iter).second;
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_node.release();
					STXXL_ASSERT( !param->_sp_cur_leaf.ptr ); // unused until jet
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
				}

		// ***************************************************************************************
		// the next one is steel a node
		// ***************************************************************************************

				else
				{
					bid_type bid = (*cn_iter).second;
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_node.release();
					node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
				}
				return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_afind_move_next( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::FIND );

		if ( (unsigned)param->_height == _height )
		{
			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin();
			param->_sp_cur_leaf.release();
			const_leaf_iterator cn_iter = (*param->_pp_iterator)->_leaf_iterator;
			if( (*cn_iter).first != param->_key )
			{
				STXXL_ASSERT( *param->_pp_iterator );
				**param->_pp_iterator = end();
				// ( *param->_pp_iterator )->add_ref();
				return true;
			}
		}
		else
		{
			(*param->_pp_iterator)->_node_iterators[param->_height-1] = param->_sp_cur_node->begin();
		}

		smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[param->_height-2].node();
		if ( !(*param->_pp_iterator)->_node_iterators[param->_height-2].next() )
		{
			sp_node.release();
			return true;
		}

		--param->_height;
		bid_type bid = sp_node->bid_next();
		STXXL_ASSERT( bid.valid() );
		param->_sp_cur_node.release();
		sp_node.release();
		node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_afind_move_prev( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::FIND );
		STXXL_ASSERT( (size_type) param->_height2 == _height );


		(*param->_pp_iterator)->_node_iterators[param->_height2-1] = param->_sp_cur_node->last();

		if ( !(*param->_pp_iterator)->_node_iterators[param->_height2-2].prev() )
		{
			param->_phase = btrequest_find<_Self>::PHASE_SEARCH ;
			const_node_iterator cn_iter = (*param->_pp_iterator)->_node_iterators[param->_height-1];
			bid_type bid = (*cn_iter).second;
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		else
		{
			smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[param->_height2-2].node();
			--param->_height2;
			bid_type bid = sp_node->bid_prev();
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_afind_search( btrequest_const_search<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_FIND );

		const_node_iterator cn_iter;
		const_node_iterator cn_begin;

		if ( !--param->_height )
		{

			// **************************************************************************************
			// we in a leaf
			// and we have to find the right leaf

			(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->lower_bound( param->_key );
			if ( (*param->_pp_const_iterator)->_leaf_iterator == param->_sp_cur_leaf->end() )
			{
				smart_ptr<_Leaf> sp_leaf = param->_sp_cur_leaf;
				if( !sp_leaf->bid_next().valid() )
				{
					// there are no next leaf and no lower_bound
					// lower_bound = end()
					STXXL_ASSERT( *param->_pp_const_iterator );
					**param->_pp_const_iterator = end();
					( *param->_pp_const_iterator )->add_ref();
					return true;
				}
				// We have to find the begin of next leaf
				param->_phase = btrequest_const_find<_Self>::PHASE_MOVE_NEXT;
				param->_height = _height;
				bid_type bid = sp_leaf->bid_next();
				STXXL_ASSERT( bid.valid() );
				param->_sp_cur_leaf.release();
				leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
				return false;
			}

			const_leaf_iterator cn_iter((*param->_pp_const_iterator)->_leaf_iterator);
			if( (*cn_iter).first != param->_key )
			{
				STXXL_ASSERT( *param->_pp_const_iterator );
				**param->_pp_const_iterator = end();
				( *param->_pp_const_iterator )->add_ref();
			}
			return true;
		}
		else
		{
			// ***********************************************************************************
			// we are in internal nodes
			// ***********************************************************************************

			cn_iter = param->_sp_cur_node->lower_bound( param->_key );
			cn_begin = param->_sp_cur_node->begin();

			if ( cn_iter == param->_sp_cur_node->end() )

				// ***********************************************************************************
				// all elemenets are less then key -> search in the last node
				// ***********************************************************************************
				cn_iter = param->_sp_cur_node->last();

			else if ( cn_iter != cn_begin )

				// ***********************************************************************************
				// it is not the first element
				// ***********************************************************************************
				--cn_iter;

			else
			{
				// the first element in tree is it.
				STXXL_ASSERT( !param->_sp_cur_node->bid_prev().valid() );
			}
		}

		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

		if( param->_height == 1 )
		{
			// ***************************************************************************************
			// the next one is a leaf
			// ***************************************************************************************

			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_leaf.release();
			leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;

		}
		else
		{
			// ***************************************************************************************
			// the next one is steel a node
			// ***************************************************************************************

			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_afind_move_next( btrequest_const_search<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_FIND );

		if ( (unsigned)param->_height == _height )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin();
			const_leaf_iterator cn_iter = (*param->_pp_const_iterator)->_leaf_iterator;
			if( (*cn_iter).first != param->_key )
			{
				STXXL_ASSERT( *param->_pp_const_iterator );
				**param->_pp_const_iterator = end();
				( *param->_pp_const_iterator )->add_ref();
			}
			return true;
		}
		else
		{
			STXXL_ASSERT( 0 );
		}
		return false;
	}

	/***************************************************************************************
	****************************************************************************************

	UPPER_BOUND

	****************************************************************************************
	***************************************************************************************/

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_aupper_bound_search( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::UPPER_BOUND ||
			param->_type == btree_request<_Self>::EQUAL_RANGE );

		node_iterator n_iter;

		if ( !--param->_height )
		{
			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->upper_bound( param->_key );

			if ( (*param->_pp_iterator)->_leaf_iterator == param->_sp_cur_leaf->end() )
			{
				// smart_ptr<_Leaf> sp_leaf = param->_sp_cur_leaf;
				if( !param->_sp_cur_leaf->bid_next().valid() )
				{
					STXXL_ASSERT( *param->_pp_iterator );
					**param->_pp_iterator = end();
					( *param->_pp_iterator )->add_ref();
					return true;
				}
				else
				{
					param->_phase = btrequest_upper_bound<_Self>::PHASE_MOVE_NEXT;
					param->_height = _height;
					bid_type bid = param->_sp_cur_leaf->bid_next();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
					return false;
				}
			}
			return true;
		}
		else
		{
			n_iter = param->_sp_cur_node->upper_bound( param->_key );

			if ( n_iter == param->_sp_cur_node->end() )
			{
				n_iter = param->_sp_cur_node->last();
			}
			else if ( n_iter != param->_sp_cur_node->begin() )
			{
				--n_iter;
			}
			else
			{
				STXXL_ASSERT( !param->_sp_cur_node->bid_prev().valid() );
			}
			(*param->_pp_iterator)->_node_iterators.push_back( n_iter );
		}

		const_node_iterator cn_iter( n_iter );
		bid_type bid = (*cn_iter).second;

		if( param->_height == 1 )
		{
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_leaf.release();
			param->_sp_cur_node.release();
			leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
		}
		else
		{
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_aupper_bound_move_next( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::UPPER_BOUND ||
			param->_type == btree_request<_Self>::EQUAL_RANGE );

		if ( (unsigned)param->_height == _height )
		{
			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin();
			param->_sp_cur_leaf.release();
		}
		else
		{
			(*param->_pp_iterator)->_node_iterators[param->_height-1] = param->_sp_cur_node->begin();
		}

		bid_type bid = (*param->_pp_iterator)->_node_iterators[param->_height-2].node()->bid_next();
		// bid_type bid = sp_node->bid_next();
		// smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[param->_height-2].node();
		if ( !(*param->_pp_iterator)->_node_iterators[param->_height-2].next() )
			return true;

		--param->_height;
		STXXL_ASSERT( bid.valid() );
		param->_sp_cur_node.release();
		node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_aupper_bound_search( btrequest_const_search<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_UPPER_BOUND ||
			param->_type == btree_request<_Self>::CONST_EQUAL_RANGE );

		const_node_iterator cn_iter;
		const_node_iterator cn_begin;

		if ( !--param->_height )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->upper_bound( param->_key );
			if ( (*param->_pp_const_iterator)->_leaf_iterator == param->_sp_cur_leaf->end() )
			{
				smart_ptr<_Leaf> sp_leaf = param->_sp_cur_leaf;
				if( !sp_leaf->bid_next().valid() )
				{
					STXXL_ASSERT( *param->_pp_const_iterator );
					**param->_pp_const_iterator = end();
					( *param->_pp_const_iterator )->add_ref();
					return true;
				}
				else
				{
					param->_phase = btrequest_const_upper_bound<_Self>::PHASE_MOVE_NEXT;
					param->_height = _height;
					bid_type bid = sp_leaf->bid_next();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
					return false;
				}
			}
			return true;
		}
		else
		{
			cn_iter = param->_sp_cur_node->upper_bound( param->_key );
			cn_begin = param->_sp_cur_node->begin();
			if ( cn_iter == param->_sp_cur_node->end() )
			{
				cn_iter = param->_sp_cur_node->last();
			}
			else if ( cn_iter != cn_begin )
			{
				--cn_iter;
			}
			else
			{
				STXXL_ASSERT( !param->_sp_cur_node->bid_prev().valid() );
			}
		}

		if( param->_height == 1 )
		{
			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_leaf.release();
			param->_sp_cur_node.release();
			leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
		}
		else
		{
			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_aupper_bound_move_next( btrequest_const_search<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_UPPER_BOUND ||
			param->_type == btree_request<_Self>::CONST_EQUAL_RANGE );

		if ( (unsigned)param->_height == _height )
		{
			(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin();
			param->_sp_cur_leaf.release();
		}
		else STXXL_ASSERT( 0 );
		return true;
	}

	/***************************************************************************************
	****************************************************************************************

	LOWER_BOUND

	****************************************************************************************
	***************************************************************************************/

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_alower_bound_search( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::LOWER_BOUND ||
			param->_type == btree_request<_Self>::EQUAL_RANGE );

		node_iterator n_iter;

		if ( !--param->_height )
		{
			// **************************************************************************************
			// we in a leaf
			// and we have to find the right leaf

			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->lower_bound( param->_key );
			if ( (*param->_pp_iterator)->_leaf_iterator == param->_sp_cur_leaf->end() )
			{
				if( !param->_sp_cur_leaf->bid_next().valid() )
				{
					STXXL_ASSERT( *param->_pp_iterator );
					**param->_pp_iterator = end();
					( *param->_pp_iterator )->add_ref();
					return true;
				}
				else
				{
					param->_phase = btrequest_lower_bound<_Self>::PHASE_MOVE_NEXT;
					param->_height = _height;
					bid_type bid = param->_sp_cur_leaf->bid_next();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
					return false;
				}
			}
			return true;

      // ***********************************************************************************

		}
		else
		{
      // ***********************************************************************************
			// we are in internal nodes
      // ***********************************************************************************
			n_iter = param->_sp_cur_node->less_bound( param->_key );
			if ( n_iter == param->_sp_cur_node->end() )
			{
				// ***********************************************************************************
				// all elements are greater then key
				// ***********************************************************************************
				n_iter = param->_sp_cur_node->begin();
			}
			(*param->_pp_iterator)->_node_iterators.push_back( n_iter );
		}

		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

		const_node_iterator cn_iter = n_iter;
		if( param->_height == 1 )
		{
			// ***************************************************************************************
			// the next one is a leaf
			// ***************************************************************************************

			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_leaf.release();
			param->_sp_cur_node.release();
			leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;

		}
		else
		{
			// ***************************************************************************************
			// the next one is steel a node
			// ***************************************************************************************

			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_alower_bound_move_next( btrequest_search<_Self>* param )
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::LOWER_BOUND ||
			param->_type == btree_request<_Self>::EQUAL_RANGE );


		if ( (unsigned)param->_height == _height )
		{
			(*param->_pp_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin();
			param->_sp_cur_leaf.release();
		}
		else
		{
			(*param->_pp_iterator)->_node_iterators[param->_height-1] = param->_sp_cur_node->begin();
		}

		smart_ptr<_Node> sp_node = (*param->_pp_iterator)->_node_iterators[param->_height-2].node();
		if ( !(*param->_pp_iterator)->_node_iterators[param->_height-2].next() )
			return true;

		--param->_height;
		bid_type bid = sp_node->bid_next();
		STXXL_ASSERT( bid.valid() );
		param->_sp_cur_node.release();
		node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param );
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_alower_bound_search( btrequest_const_search<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_LOWER_BOUND  ||
			param->_type == btree_request<_Self>::CONST_EQUAL_RANGE
		 );

		const_node_iterator cn_iter;

		if ( !--param->_height )
		{
			// **************************************************************************************
			// we in a leaf
			// and we have to find the right leaf

			(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->lower_bound( param->_key );
			if ( (*param->_pp_const_iterator)->_leaf_iterator == param->_sp_cur_leaf->end() )
			{
				if( !param->_sp_cur_leaf->bid_next().valid() )
				{
					STXXL_ASSERT( *param->_pp_const_iterator );
					**param->_pp_const_iterator = end();
					( *param->_pp_const_iterator )->add_ref();
					return true;
				}
				else
				{
					param->_phase = btrequest_const_lower_bound<_Self>::PHASE_MOVE_NEXT;
					param->_height = _height;
					bid_type bid = param->_sp_cur_leaf->bid_next();
					STXXL_ASSERT( bid.valid() );
					param->_sp_cur_leaf.release();
					leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;
					return false;
				}
			}
			return true;

      // ***********************************************************************************

		}
		else
		{
      // ***********************************************************************************
			// we are in internal nodes
      // ***********************************************************************************
			cn_iter = param->_sp_cur_node->less_bound( param->_key );
			if ( cn_iter == param->_sp_cur_node->end() )
			{
				// ***********************************************************************************
				// all elements are greater then key
				// ***********************************************************************************
				cn_iter = param->_sp_cur_node->begin();
			}
		}

		// ***************************************************************************************
		// next Data load
		// ***************************************************************************************

		if( param->_height == 1 )
		{
			// ***************************************************************************************
			// the next one is a leaf
			// ***************************************************************************************

			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_leaf.release();
			param->_sp_cur_node.release();
			leaf_manager::get_instance()->GetNode( bid, &param->_sp_cur_leaf.ptr, param ) ;

		}
		else
		{
			// ***************************************************************************************
			// the next one is steel a node
			// ***************************************************************************************

			bid_type bid = (*cn_iter).second;
			STXXL_ASSERT( bid.valid() );
			param->_sp_cur_node.release();
			node_manager::get_instance()->GetNode( bid, &param->_sp_cur_node.ptr, param ) ;
		}
		return false;
	}

	template<typename Key_,typename Data_, typename Compare_, unsigned BlkSize_,bool unique_,unsigned PrefCacheBlkSize_>
	bool btree<Key_,Data_,Compare_,BlkSize_,unique_,PrefCacheBlkSize_>::_alower_bound_move_next( btrequest_const_search<_Self>* param ) const
	{
		STXXL_ASSERT( param->_type == btree_request<_Self>::CONST_LOWER_BOUND ||
			param->_type == btree_request<_Self>::CONST_EQUAL_RANGE );

		STXXL_ASSERT( (unsigned)param->_height == _height );
		(*param->_pp_const_iterator)->_leaf_iterator = param->_sp_cur_leaf->begin();
		return true;
	}

//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE

#endif
