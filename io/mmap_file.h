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
				off_t pos, 
				size_t bytes,
				completion_handler on_cmpl);
		request_ptr awrite(
				void *buffer,
				off_t pos,
				size_t bytes,
				completion_handler on_cmpl);
	};
	
	//! \brief Implementation of memory mapped access file request
	class mmap_request:public ufs_request_base
	{
		friend class mmap_file;
	  protected:
		  mmap_request (mmap_file * f,
				void *buf, off_t off, size_t b,
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
		// file->set_size(offset+bytes);
		void *mem = mmap (NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, file->get_file_des (), offset);
		if (mem == MAP_FAILED)
		{
			STXXL_ERRMSG("Mapping failed.")
			STXXL_ERRMSG("Page size: " << sysconf (_SC_PAGESIZE) << " offset modulo page size" << 
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
				stxxl_ifcheck (munmap ((char *) mem, bytes))}
				else
				{
					stxxl_ifcheck (memcpy (mem, buffer, bytes))
					else
					stxxl_ifcheck (munmap ((char *) mem, bytes))
				}
		}

		_state.set_to (DONE);

		waiters_mutex.lock ();
		
		// << notification >>
		std::for_each(
			waiters.begin (),
			waiters.end(),
			std::mem_fun(&onoff_switch::on) );
		
		waiters_mutex.unlock ();

		completed();
		_state.set_to(READY2DIE);
	}

	request_ptr mmap_file::aread(
			void *buffer, 
			off_t pos, 
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
			off_t pos, 
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
