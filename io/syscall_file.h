#ifndef SYSCALL_HEADER
#define SYSCALL_HEADER
/***************************************************************************
 *            syscall_file.h
 *
 *  Sat Aug 24 23:55:08 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "ufs_file.h"

__STXXL_BEGIN_NAMESPACE

  //! \addtogroup fileimpl
  //! \{

	//! \brief Implementation of file based on UNIX syscalls
	class syscall_file:public ufs_file_base
	{
	 protected:
	 public:
		//! \brief constructs file object
		//! \param filename path of file
		//! \attention filename must be resided at memory disk partition
		//! \param mode open mode, see \c stxxl::file::open_modes
		//! \param disk disk(file) identifier
		syscall_file(
				const std::string & filename, 
				int mode,
			  int disk = -1): ufs_file_base (filename, mode, disk)
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
	
	//! \brief Implementation of request based on UNIX syscalls
	class syscall_request: public ufs_request_base
	{
		friend class syscall_file;
	 protected:
		syscall_request(
				syscall_file * f, 
				void *buf, 
				off_t off,	
				size_t b, 
				request_type t,
				completion_handler on_cmpl):
						ufs_request_base(f,buf,off,b,t,on_cmpl)
		{
		};
		void serve ();
	 public:
		const char *io_type ()
		{
			return "syscall";
		};
   private:
    // Following methods are declared but not implemented 
    // intentionnaly to forbid their usage
		syscall_request(const syscall_request &);
    syscall_request & operator=(const syscall_request &);
		syscall_request();
	};

	void syscall_request::serve ()
	{
		if(nref() < 2)
		{
			STXXL_ERRMSG("WARNING: serious error, reference to the request is lost before serve (nref="<<nref()<<") "<<
			 " this="<<unsigned(this)<<" File descriptor="<<file->get_file_des()<< " offset="<<offset<<" buffer="<<buffer<<" bytes="<<bytes 
			 << " type=" <<(type == READ)?"READ":"WRITE" )
		}
		
		stxxl_ifcheck_i(::lseek (file->get_file_des (), offset, SEEK_SET),
			" this="<<unsigned(this)<<" File descriptor="<<file->get_file_des()<< " offset="<<offset<<" buffer="<<buffer<<" bytes="<<bytes
		    << " type=" <<(type == READ)?"READ":"WRITE" )
		else
		{
			if (type == READ)
			{
				stxxl_ifcheck_i(::read (file->get_file_des(), buffer, bytes),
					" this="<<unsigned(this)<<" File descriptor="<<file->get_file_des()<< " offset="<<offset<<" buffer="<<buffer<<" bytes="<<bytes<< " type=" <<(type == READ)?"READ":"WRITE")
			}
			else
			{
				stxxl_ifcheck_i(::write (file->get_file_des (), buffer, bytes),
					" this="<<unsigned(this)<<" File descriptor="<<file->get_file_des()<< " offset="<<offset<<" buffer="<<buffer<<" bytes="<<bytes<< " type=" <<(type == READ)?"READ":"WRITE");
			}
		}
		
		if(nref() < 2)
		{
			STXXL_ERRMSG("WARNING: reference to the request is lost after serve (nref="<<nref()<<") "<<
			 " this="<<unsigned(this)<<" File descriptor="<<file->get_file_des()<< " offset="<<offset<<" buffer="<<buffer<<" bytes="<<bytes << " type=" <<(type == READ)?"READ":"WRITE")
		}

		_state.set_to (DONE);

		waiters_mutex.lock ();
		// << notification >>
		std::for_each(
			waiters.begin(),
			waiters.end(),
			std::mem_fun(&onoff_switch::on) );
		
		
		waiters_mutex.unlock ();

		completed ();
		_state.set_to (READY2DIE);
	}

	request_ptr syscall_file::aread (
			void *buffer, 
			off_t pos, 
			size_t bytes,
			completion_handler on_cmpl)
	{
		request_ptr req = new syscall_request(this, 
					buffer, pos, bytes,
					request::READ, on_cmpl);
		
		if(!req.get())
			stxxl_function_error;
		
		#ifndef NO_OVERLAPPING
		disk_queues::get_instance ()->add_readreq(req,get_id());
		#endif
    return req;
	};
	request_ptr syscall_file::awrite (
			void *buffer, 
			off_t pos, 
			size_t bytes,
			completion_handler on_cmpl)
	{
		request_ptr req = new syscall_request(this, buffer, pos, bytes,
					   request::WRITE, on_cmpl);
    
		if(!req.get())
			stxxl_function_error;
		
		#ifndef NO_OVERLAPPING
		disk_queues::get_instance ()->add_writereq(req,get_id());
		#endif
		return req;
	};

  
  //! \}
  
__STXXL_END_NAMESPACE


#endif
