/***************************************************************************
                          map_request.h  -  description
                             -------------------
    begin                : Son Mai 2 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file map_request.h
//! \brief This file contains the request classes for map query qeueu of \c stxxl::map implementation.
 
#include "smart_ptr.h"
#include "btree_request.h"

#ifndef __MAP_REQUEST_H
#define __MAP_REQUEST_H

__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	class map_request_base;

	//! Standard completon handler for map requests
	class map_completion_handler_impl
	{
		public:
		virtual void operator()( map_request_base* , map_lock_base* ) = 0;
		virtual map_completion_handler_impl * clone() const = 0;
		virtual ~map_completion_handler_impl() {}
	};

	class map_completion_handler
	{
		private:
			std::auto_ptr<map_completion_handler_impl> sp_impl_;
		public:
			map_completion_handler() : sp_impl_(0) {}
			map_completion_handler(const map_completion_handler & obj) : sp_impl_(obj.sp_impl_.get()->clone()) {}

			template <typename handler_type>
			map_completion_handler(const handler_type & handler__);

			map_completion_handler& operator = (const map_completion_handler & obj)
			{
				map_completion_handler copy(obj);
				map_completion_handler_impl* p = sp_impl_.release();
				sp_impl_.reset(copy.sp_impl_.release());
				copy.sp_impl_.reset(p);
				return *this;
			}

			void operator()(map_request_base * req, map_lock_base * p_lock ) { (*sp_impl_)( req, p_lock ); }
	};

	class map_request_base : public ref_counter< map_request_base >
	{
		friend class smart_ptr<map_request_base>;
		map_completion_handler on_complete;
	public:
		void completed( map_lock_base* p_lock ) { on_complete( this, p_lock );}
		map_request_base( map_completion_handler on_compl ) : on_complete( on_compl ) {}
    virtual ~map_request_base(){}
		virtual void wait () = 0;
		virtual bool poll() = 0;
	};



	struct default_map_completion_handler
	{
		void operator() ( map_request_base* ptr, map_lock_base* p_lock )
		{
			p_lock->unlock();
		}
	};


	struct help_completion_handler
	{
		smart_ptr<map_request_base> sp_help;

		private:

		help_completion_handler() {};

		public:

		help_completion_handler( map_request_base *p ) : sp_help( p )
		{
			//STXXL_MSG( "help_completion_handler::help_completion_handler(" << p << ") " << this );
		}
		help_completion_handler( const help_completion_handler& x ) : sp_help( x.sp_help )
		{
			//STXXL_MSG( "help_completion_handler::help_completion_handler" << this );
		}

		~help_completion_handler()
		{
			//STXXL_MSG( "help_completion_handler::~help_completion_handler" << this );
		}
		void operator() ( btree_request_base *ptr, map_lock_base* p_lock )
		{
			//if( !sp_help.ptr ) return;
			sp_help->completed( p_lock );
			sp_help.release();
		}
	};



 template<typename Map>
	class map_request : public map_request_base
	{
		friend class smart_ptr<map_request>;
		typedef typename Map::btree_type                        btree_type;
		typedef btree_request<btree_type>                       btree_request_type;

	public:
		static bt_counter              _count;
		smart_ptr<btree_request_type>  _sp_int_request;


		virtual ~map_request()
		{
			_count--;
		}
		virtual void wait ()   {        _sp_int_request->wait(); }
		virtual bool poll()    { return _sp_int_request->poll(); }
		        int type()     { return _sp_int_request->_type; }


		template <typename _InputIterator>
		map_request( _InputIterator begin, int type, Map* p_map, map_completion_handler on_compl ) :
			map_request_base( on_compl )
		{
			_count++;
			_sp_int_request = new btrequest_bulk_insert<btree_type,_InputIterator>( p_map->tree(), help_completion_handler(this) );
		}
		
		map_request( int type, Map* p_map, map_completion_handler on_compl ) :
			map_request_base( on_compl )
		{
			_count++;
			// help_completion_handler handler( this );
			switch( type )
			{
				case btree_request_base::CLEAR:
						_sp_int_request = new btrequest_clear<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::BEGIN:
						_sp_int_request = new btrequest_begin<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::LOWER_BOUND:
						_sp_int_request = new btrequest_lower_bound<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::UPPER_BOUND:
						_sp_int_request = new btrequest_upper_bound<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::FIND:
						_sp_int_request = new btrequest_find<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::NEXT:
						_sp_int_request = new btrequest_next<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::PREV:
						_sp_int_request = new btrequest_prev<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::EQUAL_RANGE:
						_sp_int_request = new btrequest_equal_range<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::INSERT:
						_sp_int_request = new btrequest_insert<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::BULK_INSERT:
						STXXL_ASSERT( 0 );
						break;

				case btree_request_base::ERASE:
						_sp_int_request = new btrequest_erase<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::ERASE_KEY:
						_sp_int_request = new btrequest_erase_key<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::ERASE_INTERVAL:
						_sp_int_request = new btrequest_erase_interval<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::DUMP:
						_sp_int_request = new btrequest_dump<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				default: STXXL_ASSERT( 0 );
			}
		}

		map_request( int type, const Map* p_map, map_completion_handler on_compl ) :
			map_request_base( on_compl )
		{

			//STXXL_MSG( "map_request::map_request " << this );
			_count++;
			// help_completion_handler handler( this );
			switch( type )
			{
				case btree_request_base::CONST_BEGIN:
						_sp_int_request = new btrequest_const_begin<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::CONST_LOWER_BOUND:
						_sp_int_request = new btrequest_const_lower_bound<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::CONST_UPPER_BOUND:
						_sp_int_request = new btrequest_const_upper_bound<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::CONST_FIND:
						_sp_int_request = new btrequest_const_find<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::CONST_NEXT:
						_sp_int_request = new btrequest_const_next<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::CONST_PREV:
						_sp_int_request = new btrequest_const_prev<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::CONST_EQUAL_RANGE:
						_sp_int_request = new btrequest_const_equal_range<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				case btree_request_base::SIZE:
						_sp_int_request = new btrequest_size<btree_type>( p_map->tree(), help_completion_handler(this) );
						break;

				default: STXXL_ASSERT( 0 );
			}
		}
	};

	template<typename Map>
	bt_counter map_request<Map>::_count(0);


	template <typename handler_type>
	class map_completion_handler1: public map_completion_handler_impl
	{
		private:
			handler_type handler_;
		public:
			map_completion_handler1(const handler_type & handler__): handler_(handler__) {}
			map_completion_handler1* clone() const
			{
				return new map_completion_handler1(*this);
			}
			void operator()( map_request_base * req, map_lock_base* p_lock ){ handler_( req, p_lock ); }
	};

	template <typename handler_type>
	map_completion_handler::map_completion_handler(const handler_type & handler__):
  sp_impl_(new map_completion_handler1<handler_type>(handler__)) {}

//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE


#endif
