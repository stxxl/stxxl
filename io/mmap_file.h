#ifndef MMAP_HEADER
#define MMAP_HEADER

/***************************************************************************
 *            mmap_file.h
 *
 *  Sat Aug 24 23:54:54 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "ufs_file.h"

#ifdef BOOST_MSVC
// mmap call does not exist in Windows
#else

#include <sys/mman.h>

__STXXL_BEGIN_NAMESPACE

  //! \weakgroup fileimpl File implementations
  //! \ingroup iolayer
  //! Implemantations of \c stxxl::file and \c stxxl::request 
  //! for various file access methods
  //! \{

	//! \brief Implementation of memory mapped access file
	class mmap_file:public ufs_file_base
	{
		
	public:
		//! \brief constructs file object
		//! \param filename path of file
		//! \param mode open mode, see \c stxxl::file::open_modes
		//! \param disk disk(file) identifier
		mmap_file (const std::string & filename, int mode, int disk = -1):
			ufs_file_base (filename, mode, disk)
		{
		};
		request_ptr aread(
				void *buffer, 
				stxxl::int64 pos, 
				size_t bytes,
				completion_handler on_cmpl);
		request_ptr awrite(
				void *buffer,
				stxxl::int64 pos,
				size_t bytes,
				completion_handler on_cmpl);
	};
	
	//! \brief Implementation of memory mapped access file request
	class mmap_request:public ufs_request_base
	{
		friend class mmap_file;
	  protected:
		  mmap_request (mmap_file * f,
				void *buf, stxxl::int64 off, size_t b,
				request_type t,
				completion_handler on_cmpl):
				ufs_request_base (f, buf, off, b, t, on_cmpl)
		{
		};
		void serve();
		
	public:
		const char *io_type()
		{
			return "mmap";
		};
  private:
    // Following methods are declared but not implemented 
    // intentionnaly to forbid their usage
		mmap_request(const mmap_request &);
    mmap_request & operator=(const mmap_request &);
		mmap_request();
	};

	void mmap_request::serve()
	{
		stats * iostats = stats::get_instance();
		if (type == READ)
		{
			#ifdef STXXL_IO_STATS
				iostats->read_started (size());
			#endif
		}
		else
		{
			#ifdef STXXL_IO_STATS
				iostats->write_started (size());
			#endif
		}
		// static_cast<syscall_file*>(file_)->set_size(offset+bytes);
		#ifdef STXXL_MMAP_EXPERIMENT1
		int prot = (type == READ)? PROT_READ: PROT_WRITE;

		void *mem = mmap (buffer, bytes,prot, MAP_SHARED|MAP_FIXED , static_cast<syscall_file*>(file_)->get_file_des (), offset);
		//STXXL_MSG("Mmaped to "<<mem<<" , buffer suggested at "<<((void*)buffer));
		if (mem == MAP_FAILED)
		{
			STXXL_ERRMSG("Mapping failed.")
			STXXL_ERRMSG("Page size: " << sysconf (_SC_PAGESIZE) << " offset modulo page size " << 
				(offset % sysconf(_SC_PAGESIZE)))
			abort ();
		}
		else 
		if (mem == 0)
		{ 
			stxxl_function_error
		}
		else
		{
				if (type == READ)
				{
					//stxxl_ifcheck (memcpy (buffer, mem, bytes))
					//else
					//stxxl_ifcheck (munmap ((char *) mem, bytes))
				}
				else
				{
					//stxxl_ifcheck (memcpy (mem, buffer, bytes))
					//else
					//stxxl_ifcheck (munmap ((char *) mem, bytes))
				}
		}
		#else
		int prot = (type == READ)? PROT_READ: PROT_WRITE;
		void *mem = mmap (NULL, bytes, prot , MAP_SHARED, static_cast<syscall_file*>(file_)->get_file_des (), offset);
		// void *mem = mmap (buffer, bytes, prot , MAP_SHARED|MAP_FIXED , static_cast<syscall_file*>(file_)->get_file_des (), offset);
		// STXXL_MSG("Mmaped to "<<mem<<" , buffer suggested at "<<((void*)buffer));
		if (mem == MAP_FAILED)
		{
			STXXL_ERRMSG("Mapping failed.")
			STXXL_ERRMSG("Page size: " << sysconf (_SC_PAGESIZE) << " offset modulo page size " << 
				(offset % sysconf(_SC_PAGESIZE)))
			abort ();
		}
		else 
		if (mem == 0)
		{ 
			stxxl_function_error
		}
		else
		{
				if (type == READ)
				{
					stxxl_ifcheck (memcpy (buffer, mem, bytes))
					else
					stxxl_ifcheck (munmap ((char *) mem, bytes))
				}
				else
				{
					stxxl_ifcheck (memcpy (mem, buffer, bytes))
					else
					stxxl_ifcheck (munmap ((char *) mem, bytes))
				}
		}
		#endif

		if (type == READ)
		{
			#ifdef STXXL_IO_STATS
				iostats->read_finished ();
			#endif
		}
		else
		{
			#ifdef STXXL_IO_STATS
				iostats->write_finished ();
			#endif
		}
		
		_state.set_to (DONE);

		#ifdef STXXL_BOOST_THREADS
		boost::mutex::scoped_lock Lock(waiters_mutex);
		#else
		waiters_mutex.lock ();
		#endif
		
		// << notification >>
		std::for_each(
			waiters.begin (),
			waiters.end(),
			std::mem_fun(&onoff_switch::on) );
		
		#ifdef STXXL_BOOST_THREADS
		Lock.unlock();
		#else
		waiters_mutex.unlock ();
		#endif

		completed();
		_state.set_to(READY2DIE);
	}

	request_ptr mmap_file::aread(
			void *buffer, 
			stxxl::int64 pos, 
			size_t bytes,
			completion_handler on_cmpl)
	{
		request_ptr req = new mmap_request (
					this, 
					buffer, 
					pos, 
					bytes,
					request::READ, 
					on_cmpl);
		
		if (!req.get())
			stxxl_function_error;
		
		#ifndef NO_OVERLAPPING
		disk_queues::get_instance ()->add_readreq(req,get_id());
		#endif
    
    return req;
	};
	request_ptr mmap_file::awrite (
			void *buffer, 
			stxxl::int64 pos, 
			size_t bytes,
			completion_handler on_cmpl)
	{
		request_ptr req = new mmap_request(
				this, 
				buffer, 
				pos, 
				bytes,
				request::WRITE, 
				on_cmpl);
		
		if (!req.get())
			stxxl_function_error;
		
		#ifndef NO_OVERLAPPING
		disk_queues::get_instance ()->add_writereq(req,get_id());
		#endif
    
    return req;
	};

//! \}
  
__STXXL_END_NAMESPACE

#endif

#endif
