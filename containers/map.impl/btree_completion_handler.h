/***************************************************************************
                          btree_completion_handler.h  -  description
                             -------------------
    begin                : Son Jul 18 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file btree_completion_handler.h
//! \brief Implemets the completition handler form requests in intern struct B-Tree for \c stxxl::map container.

#ifndef BTREE_COMPLETION_HANDLER_H
#define BTREE_COMPLETION_HANDLER_H


__STXXL_BEGIN_NAMESPACE

namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	class btree_request_base;
	class map_lock_base;
	class btree_completion_handler_impl;
	class btree_completion_handler;
	template <typename handler_type> class btree_completion_handler1;



	class map_lock_base
	{
		protected:
		state    _state;
		map_lock_base( int l) : _state( l )
		{
		}

		~map_lock_base()
		{
		}

		public:
		enum {
			NO = 0,
			S = 1, // read  lock
			X = 2, // write lock
		};

		void unlock()
		{
			_state.set_to( NO );
		}
	};

	struct default_btree_completion_handler
	{
		//! \brief An operator that does nothing
		void operator() ( btree_request_base*, map_lock_base* p ) { p->unlock(); }
	};

	class btree_completion_handler_impl
	{

	public:
		virtual void operator()( btree_request_base *, map_lock_base *p ) = 0;
		virtual btree_completion_handler_impl * clone() const = 0;
		virtual ~btree_completion_handler_impl() {}

	};


	class btree_completion_handler
	{

	private:

		std::auto_ptr<btree_completion_handler_impl> sp_impl_;

	public:

		btree_completion_handler() : sp_impl_(0) {}
		btree_completion_handler(const btree_completion_handler & obj) : sp_impl_(obj.sp_impl_.get()->clone()) {}

		template <typename handler_type> btree_completion_handler(const handler_type & handler__);
		btree_completion_handler& operator = (const btree_completion_handler & obj )
		{
			btree_completion_handler copy(obj);
			btree_completion_handler_impl* p = sp_impl_.release();
			sp_impl_.reset(copy.sp_impl_.release());
			copy.sp_impl_.reset(p);
			return *this;
			}

		void operator()(btree_request_base * req, map_lock_base* p_lock )
		{
			return (*sp_impl_)( req, p_lock );
		}
	}; // class btree_completion_handler


	template <typename handler_type>
	class btree_completion_handler1: public btree_completion_handler_impl
	{

	private:
		handler_type handler_;

	public:
		btree_completion_handler1(const handler_type & handler__): handler_(handler__) {}
		btree_completion_handler1* clone() const
		{
			return new btree_completion_handler1( *this );
		}

		void operator()(btree_request_base * req, map_lock_base* p_lock)
		{
			return handler_( req, p_lock );
		}
	};  // class btree_completion_handler1


	template <typename handler_type>
	btree_completion_handler::btree_completion_handler(const handler_type & handler__):
  	sp_impl_(new btree_completion_handler1<handler_type>(handler__)) {}

//! \}

} // namespace map_internal

__STXXL_END_NAMESPACE

#endif
