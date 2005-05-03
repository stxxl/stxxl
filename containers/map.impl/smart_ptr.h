/***************************************************************************
                          smart_ptr.h  -  description
                             -------------------
    begin                : Die Mai 4 2004
    copyright            : (C) 2004 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

//! \file smart_ptr.h
//! \brief This file contains the classes for reference counting interfaces of \c stxxl::map implementation.

#ifndef ___SMART_PTR__H___
#define ___SMART_PTR__H___

__STXXL_BEGIN_NAMESPACE


namespace map_internal
{

//! \addtogroup stlcontinternals
//!
//! \{

	template<class Class_> class smart_ptr;

  //! \brief Reference counting template
  //! Class implements pointers to objects of classes, which supports reference counting
	template<class Class_> class ref_counter;

  //! \brief
	template<class Class_>
	bool operator == ( const smart_ptr<Class_>& x, const smart_ptr<Class_>& y );

  //! \brief
	template<class Class_>
	bool operator != ( const smart_ptr<Class_>& x, const smart_ptr<Class_>& y );


	template<class Class_>
	class ref_counter
	{
		public:
			#ifdef STXXL_BOOST_THREADS
			boost::mutex _mutex_ref;
			#else
			mutex _mutex_ref;
			#endif
			int _n_ref;
		public:

			typedef Class_ _Self;
			friend class smart_ptr<_Self>;

			int get_count()
			{
				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(_mutex_ref);
				return _n_ref;
				#else
				_mutex_ref.lock();
				int ret = _n_ref;
				_mutex_ref.unlock();
				return ret;
				#endif
			}

			_Self* add_ref()
			{
				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(_mutex_ref);
				STXXL_VERBOSE3( "add_ref " << this << " count=" << _n_ref );
				++_n_ref;
				_Self* ptr = (_Self*)this;
				#else
				_mutex_ref.lock();
				STXXL_VERBOSE3( "add_ref " << this << " count=" << _n_ref );
				++_n_ref;
				_Self* ptr = (_Self*)this;
				_mutex_ref.unlock();
				#endif
				return ptr;
			}

			bool sub_ref()
			{
				#ifdef STXXL_BOOST_THREADS
				boost::mutex::scoped_lock Lock(_mutex_ref);
				STXXL_VERBOSE3( "sub_ref " << this << " count=" << _n_ref );
				bool ret = !--_n_ref;
				#else
				_mutex_ref.lock();
				STXXL_VERBOSE3( "sub_ref " << this << " count=" << _n_ref );
				bool ret = !--_n_ref;
				_mutex_ref.unlock();
				#endif
				return ret;
			}

			ref_counter() : _n_ref(0) {};

			virtual ~ref_counter() { 
				#ifndef STXXL_BOOST_THREADS
				_mutex_ref.lock();  // To Thomas: why needed?
				#endif
				}
			
	}; // class ref_counter


  //! \brief Smart pointers
  //! Class implements pointers to objects of classes, which supports reference counting.
	template<class Class_>
	class smart_ptr
	{
		Class_* add_ref()
		{
			if ( ptr ) ptr->add_ref();
			return ptr;
		}

		bool sub_ref()
		{
			if( ptr && ptr->sub_ref() )
			{
				delete ptr;
				ptr = NULL;
				return true;
			}
			return false;
		}

  public:
    
    Class_* ptr;

    smart_ptr( Class_* ptr_ = NULL ) : ptr( ptr_ )  { add_ref(); }
    smart_ptr( const smart_ptr & p ) : ptr( p.ptr ) { add_ref(); }
    ~smart_ptr() { if( ptr && ptr->sub_ref() ) delete ptr; }
    smart_ptr & operator= (const smart_ptr & p)
    {
      return (*this = p.ptr);
    }
    smart_ptr & operator= (Class_ * p)
    {
      if(p != ptr)
      {
        sub_ref();
        ptr = p;
        add_ref();
      }
      return *this;
    }
    Class_& operator * () const
    {
      assert(ptr);
      return *ptr;
    }
    Class_* operator -> () const
    {
      assert(ptr);
      return ptr;
    }

		Class_* get() const
		{
      return ptr;
		}

    void release()
    {
			sub_ref();
			ptr = NULL;
		}

		bool free_last()
		{
			if( ptr && ptr->sub_ref() )
			{
				delete ptr;
				ptr = NULL;
				return true;
			}
			ptr->add_ref();
			return false;

		}

    bool valid() const { return ptr; }
    bool empty() const { return !ptr; }    
		bool operator == ( const smart_ptr& x ) { return ptr == x.ptr ; }
		bool operator != ( const smart_ptr& x ) { return ptr != x.ptr ; }

  }; //class smart_ptr

	template<class Class_>
	bool operator == ( const smart_ptr<Class_>& x, const smart_ptr<Class_>& y )
	{ return x.operator ==(y); }

	template<class Class_>
	bool operator != ( const smart_ptr<Class_>& x, const smart_ptr<Class_>& y )
	{ return x.operator ==(y); }

//! \}

} // namespace map_internal


__STXXL_END_NAMESPACE

#endif
