#ifndef IOBASE_HEADER
#define IOBASE_HEADER
/***************************************************************************
 *            iobase.h
 *
 *  Sat Aug 24 23:54:44 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#define STXXL_IO_STATS


#if defined(__linux__)
#define STXXL_CHECK_BLOCK_ALIGNING
#endif

//#if defined(__linux__)
//# if !defined(O_DIRECT) && (defined(__alpha__) || defined(__i386__))
//#  define O_DIRECT 040000 /* direct disk access */
//# endif
//#endif


//#ifdef __sun__
//#define O_DIRECT 0
//#endif


#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

// STL includes
#include <algorithm>
#include <string>
#include <queue>
#include <map>
#include <set>



#if defined(__linux__)
//# include <asm/fcntl.h>
# if !defined(O_DIRECT) && (defined(__alpha__) || defined(__i386__))
#  define O_DIRECT 040000	/* direct disk access */
# endif
#endif


#ifndef O_DIRECT
#define O_DIRECT O_SYNC
#endif


#include "iostats.h"
#include "../common/utils.h"
#include "../common/semaphore.h"
#include "../common/mutex.h"
//#include "../common/rwlock.h"
#include "../common/switch.h"
#include "../common/state.h"
#include "completion_handler.h"

__STXXL_BEGIN_NAMESPACE

#define BLOCK_ALIGN 4096

	typedef void *(*thread_function_t) (void *);
	typedef long long int DISKID;

	class request;
  class request_ptr;
	
	//! \brief Default completion handler class
	
	struct default_completion_handler
	{
		//! \brief An operator that does nothing
		void operator() (request *) { };
	};

	//! \brief Defines interface of file
	
	//! It is a base class for different implementations that might
	//! base on various file systems or even remote storage interfaces
	class file
	{
		//! \brief private constructor
		//! \remark instantiation of file without id is forbidden
		file ()
		{
		};
		
	protected:
		int id;
		
		//! \brief Initializes file object
		//! \param _id file identifier
		//! \remark Called in implementations of file
		file (int _id):id (_id) { };
		
	public:
		//! \brief Definition of acceptable file open modes
		
		//! Various open modes in a file system must be
		//! converted to this set of acceptable modes
		enum open_mode
		{ 
				RDONLY = 1, //!< only reading of the file is allowed
				WRONLY = 2, //!< only writing of the file is allowed
				RDWR = 4,   //!< read and write of the file are allowed
				CREAT = 8,  //!< in case file does not exist no error occurs and file is newly created
				DIRECT = 16,//!< I/Os proceed bypassing file system buffers, i.e. unbuffered I/O
				TRUNC = 32  //!< once file is opened its length becomes zero
		};

		//! \brief Schedules asynchronous read request to the file
		//! \param buffer pointer to memory buffer to read into
		//! \param pos starting file position to read
		//! \param bytes number of bytes to transfer
		//! \param on_cmpl I/O completion handler
		//! \return \c request_ptr object, that can be used to track the status of the operation
		virtual request_ptr aread (void *buffer, off_t pos, size_t bytes,
				    completion_handler on_cmpl ) = 0;
		//! \brief Schedules asynchronous write request to the file
		//! \param buffer pointer to memory buffer to write from
		//! \param pos starting file position to write
		//! \param bytes number of bytes to transfer
		//! \param on_cmpl I/O completion handler
		//! \return \c request_ptr object, that can be used to track the status of the operation
		virtual request_ptr awrite (void *buffer, off_t pos, size_t bytes, 
            completion_handler on_cmpl ) = 0;

		//! \brief Changes the size of the file
		//! \param newsize value of the new file size
		virtual void set_size (off_t newsize) = 0;
		//! \brief Returns size of the file
		//! \return file size in bytes
		virtual off_t size () = 0;
		//! \brief depricated, use \c stxxl::file::get_id() instead
		int get_disk_number () 
		{
			return id;
		};
		//! \brief Returns file's identifier
		//! \remark might be used as disk's id in case disk to file mapping
		//! \return integer file identifier, passed as constructor parameter
		int get_id()
		{
			return id;
		};
		virtual ~ file () { };
	};

	class mc;
	class disk_queue;
	class disk_queues;

	//! \brief Defines interface of request
	
	//! Since all library I/O operations are asynchronous,
	//! one needs to keep track of their status: whether
	//! an I/O completed or not.
	class request
	{
    friend int wait_any(request_ptr req_array[], int count);
		friend class file;
		friend class disk_queue;
		friend class disk_queues;
    friend class request_ptr;
		
	protected:
		virtual bool add_waiter (onoff_switch * sw) = 0;
		virtual void delete_waiter (onoff_switch * sw) = 0;
		//virtual void enqueue () = 0;
		virtual void serve () = 0;
	
		completion_handler on_complete;
		int ref_cnt;
    mutex ref_cnt_mutex;
    
		enum request_type { READ, WRITE };
		
		void completed ()
		{
			on_complete(this);
		};
		
	public:
		request(completion_handler on_compl):on_complete(on_compl),ref_cnt(0) {};
		//! \brief Suspends calling thread until completion of the request
		virtual void wait () = 0;
		//! \brief Polls the status of the request
		//! \return \c true if request is comleted, otherwise \c false
		virtual bool poll () = 0;
		//! \brief Identifies the type of request I/O implementation
		//! \return pointer to null terminated string of characters, containing the name of I/O implementation
		virtual const char * io_type ()
		{
			return "none";
		};
		virtual ~ request() {};
	private:
    // Following methods are declared but not implemented 
    // intentionnaly to forbid their usage
		request(const request &);
    request & operator=(const request &);
		request();
    
    void add_ref()
    {
      ref_cnt_mutex.lock();
      ref_cnt++;
      ref_cnt_mutex.unlock();
    }
    bool sub_ref()
    {
      ref_cnt_mutex.lock();
      ref_cnt--;
      ref_cnt_mutex.unlock();
      return (ref_cnt<=0);
    }
	};

  //! \brief A smart wrapper for \c request pointer.
  
  //! Implemented as reference counting smart pointer.
  class request_ptr
  {
    request * ptr;
    void add_ref()
    {
      
      if(ptr)
      {
        ptr->add_ref();
      }
    }
    void sub_ref()
    {
      if(ptr)
      {
        if(ptr->sub_ref())
        {
          delete ptr;
          ptr = NULL;
        }
      }
    }
  public:
    //! \brief Constucts an \c request_ptr from \c request pointer
    request_ptr(request *ptr_=NULL):ptr(ptr_) { add_ref(); }
    //! \brief Constucts an \c request_ptr from a \c request_ptr object
    request_ptr(const request_ptr & p): ptr(p.ptr) { add_ref(); }
    //! \brief Destructor
    ~request_ptr() { sub_ref(); }
    //! \brief Assignment operator from \c request_ptr object
    //! \return reference to itself
    request_ptr & operator= (const request_ptr & p)
    {
      return (*this = p.ptr);
    }
    //! \brief Assignment operator from \c request pointer
    //! \return reference to itself
    request_ptr & operator= (request * p)
    {
      if(p != ptr)
      {
        sub_ref();
        ptr = p;
        add_ref();
      }
      return *this;
    }
    //! \brief "Star" operator
    //! \return reference to owned \c request object 
    request & operator * () const
    {
      return *ptr;
    }
    //! \brief "Arrow" operator
    //! \return pointer to owned \c request object 
    request * operator -> () const
    {
      return ptr;
    }
    //! \brief Access to owned \c request object (sinonym for \c operator->() )
    //! \return reference to owned \c request object 
    //! \warning Creation another \c request_ptr from the returned \c request or deletion 
    //!  causes unpredictable behaviour. Do not do that!
    request * get() { return ptr; }
  };
  
	//! \brief Collection of functions to track statuses of a number of requests
	
  //! \brief Suspends calling thread until \b any of requests is completed
  //! \param req_array array of \c request_ptr objects
  //! \param count size of req_array
  //! \return index in req_array pointing to the \b first completed request
  inline int wait_any(request_ptr req_array[], int count);
  //! \brief Suspends calling thread until \b all requests are completed
  //! \param req_array array of request_ptr objects
  //! \param count size of req_array
  inline void wait_all(request_ptr req_array[], int count);
  //! \brief Polls requests
  //! \param req_array array of request_ptr objects
  //! \param count size of req_array
  //! \param index contains index of the \b first completed request if any
  //! \return \c true if any of requests is completed, then index contains valid value, otherwise \c false
  inline bool poll_any (request_ptr req_array[], int count,int &index);
  
  
  void wait_all(request_ptr req_array[], int count)
	{
		for (int i = 0; i < count; i++)
		{
			req_array[i]->wait ();
		}
	}

  
	bool poll_any (request_ptr req_array[], int count, int &index)
	{
		index = -1;
		for (int i = 0; i < count; i++)
		{
			if (req_array[i]->poll())
			{
				index = i;
				return true;
			}
		};
		return false;
	}

	int wait_any (request_ptr req_array[], int count)
	{
		START_COUNT_WAIT_TIME
		onoff_switch sw;
		int i = 0, index = -1;

		for (; i < count; i++)
		{
			if (req_array[i]->add_waiter (&sw))
			{
				// already done
				index = i;

				while (--i >= 0)
					req_array[i]->delete_waiter (&sw);

				END_COUNT_WAIT_TIME
				return index;
			}
		}

		sw.wait_for_on ();

		for (i = 0; i < count; i++)
		{
			req_array[i]->delete_waiter (&sw);
			if (index < 0 && req_array[i]->poll ())
				index = i;
		}

		END_COUNT_WAIT_TIME
		return index;
	}

	class disk_queue
	{
	public:
		enum priority_op { READ, WRITE, NONE };
	private:
		mutex write_mutex;
		mutex read_mutex;
		std::queue < request_ptr > write_queue;
		std::queue < request_ptr > read_queue;
		semaphore sem;
		pthread_t thread;
		priority_op _priority_op;

#ifdef STXXL_IO_STATS
		stats *iostats;
#endif

		static void *worker (void *arg);
	public:
		disk_queue (int n = 1);	// max number of requests simultainenously submitted to disk
		
		void set_priority_op (priority_op op)
		{
			_priority_op = op;
		};
		void add_readreq (request_ptr & req);
		void add_writereq (request_ptr & req);
		~disk_queue ();
	};

	//! \brief Encapsulates disk queues
	//! \remark is a singleton
	class disk_queues
	{
	protected:
		std::map < DISKID, disk_queue * > queues;
		disk_queues ()
		{
		};
	public:
		void add_readreq (request_ptr & req, DISKID disk)
		{
			if (queues.find (disk) == queues.end ())
			{
				// create new disk queue
				queues[disk] = new disk_queue ();
			}
			queues[disk]->add_readreq (req);
		};
		void add_writereq (request_ptr & req, DISKID disk)
		{
			if (queues.find (disk) == queues.end ())
			{
				// create new disk queue
				queues[disk] = new disk_queue ();
			}
			queues[disk]->add_writereq (req);
		};
		~disk_queues ()
		{
			// deallocate all queues
			for (std::map < DISKID, disk_queue * >::iterator i =
			     queues.begin (); i != queues.end (); i++)
				delete (*i).second;
		};
		static disk_queues *get_instance ()
		{
			if (!instance)
				instance = new disk_queues ();

			return instance;
		};
		//! \brief Changes requests priorities
		//! \param op one of:
		//!                 - READ, read requests are served before write requests within a disk queue
		//!                 - WRITE, write requests are served before read requests within a disk queue
		//!                 - NONE, read and write requests are served by turns, alternately
		void set_priority_op(disk_queue::priority_op op)
		{
			for (std::map < DISKID, disk_queue * >::iterator i =
			     queues.begin (); i != queues.end (); i++)
				i->second->set_priority_op (op);
		};
	private:
		static disk_queues *instance;
	};

#ifdef COUNT_WAIT_TIME
	extern double wait_time_counter;
#endif

__STXXL_END_NAMESPACE


#endif
