#include "wincall_file.h"

#ifdef BOOST_MSVC

__STXXL_BEGIN_NAMESPACE

		 wincall_file::wincall_file(
				const std::string & filename, 
				int mode,
			  int disk): wfs_file_base (filename, mode, disk)
		{
		}
	
		 wincall_request::wincall_request(
				wincall_file * f, 
				void *buf, 
				stxxl::int64 off,	
				size_t b, 
				request_type t,
				completion_handler on_cmpl):
						wfs_request_base(f,buf,off,b,t,on_cmpl)
		{
		}
		const char *wincall_request::io_type ()
		{
			return "syscall";
		}

	void wincall_request::serve ()
	{
		stats * iostats = stats::get_instance();
		if(nref() < 2)
		{
			STXXL_ERRMSG("WARNING: serious error, reference to the request is lost before serve (nref="
				<<nref()<<") "<<
			 " this="<<long(this)<< " offset="<<offset<<" buffer="<<buffer<<" bytes="<<bytes 
			 << " type=" <<((type == READ)?"READ":"WRITE") )
		}
		STXXL_VERBOSE2("wincall_request::serve(): Buffer at "<<((void*)buffer)
			<<" offset: "<<offset<<" bytes: "<<bytes<<((type== READ)?" READ":" WRITE")
			);
		
    try {
		
		LARGE_INTEGER desired_pos;
		desired_pos.QuadPart = offset;
		if(!SetFilePointerEx(static_cast<wincall_file*>(file_)->get_file_des(),
				desired_pos,NULL,FILE_BEGIN))
			stxxl_win_lasterror_exit("SetFilePointerEx in wincall_request::serve() offset="<<offset
				<<" this="<<long(this)<<" buffer="<<
				buffer<<" bytes="<<bytes
		    	<< " type=" <<((type == READ)?"READ":"WRITE"))

		{
			if (type == READ)
			{
				#ifdef STXXL_IO_STATS
					iostats->read_started (size());
				#endif
				
				debugmon::get_instance()->io_started((char*)buffer);
				DWORD NumberOfBytesRead = 0;
				if(!ReadFile(static_cast<wincall_file*>(file_)->get_file_des(),
					buffer,bytes,&NumberOfBytesRead,NULL))
				{
					stxxl_win_lasterror_exit("ReadFile this="<<long(this)<<
						" offset="<<offset<<
						" buffer="<<buffer<<" bytes="<<bytes<< " type=" <<
						((type == READ)?"READ":"WRITE")<<" nref= "<<nref()<<
						" NumberOfBytesRead= "<<NumberOfBytesRead)
				}
				
				debugmon::get_instance()->io_finished((char*)buffer);
				
				#ifdef STXXL_IO_STATS
					iostats->read_finished ();
				#endif
			}
			else
			{
				#ifdef STXXL_IO_STATS
					iostats->write_started (size());
				#endif
				
				debugmon::get_instance()->io_started((char*)buffer);
				
				DWORD NumberOfBytesWritten = 0;
				if(!WriteFile(static_cast<wincall_file*>(file_)->get_file_des(),buffer,bytes,
					&NumberOfBytesWritten,NULL))
				{
					stxxl_win_lasterror_exit("WriteFile this="<<long(this)<<
						" offset="<<offset<<
						" buffer="<<buffer<<" bytes="<<bytes<< " type=" <<
						((type == READ)?"READ":"WRITE")<<" nref= "<<nref()<<
						" NumberOfBytesWritten= "<<NumberOfBytesWritten)
				}

				debugmon::get_instance()->io_finished((char*)buffer);
				
				#ifdef STXXL_IO_STATS
					iostats->write_finished ();
				#endif
			}
		}
		
    
    }
    catch(const io_error & ex)
    {
      error_occured(ex.what());
    }
    
		if(nref() < 2)
		{
			STXXL_ERRMSG("WARNING: reference to the request is lost after serve (nref="<<nref()<<") "<<
			 " this="<<long(this)<< 
			" offset="<<offset<<" buffer="<<buffer<<" bytes="<<bytes << 
			" type=" <<((type == READ)?"READ":"WRITE"))
		}

		_state.set_to (DONE);

		#ifdef STXXL_BOOST_THREADS
		boost::mutex::scoped_lock Lock(waiters_mutex);
		#else
		waiters_mutex.lock ();
		#endif
		// << notification >>
		std::for_each(
			waiters.begin(),
			waiters.end(),
			std::mem_fun(&onoff_switch::on) );
		
		#ifdef STXXL_BOOST_THREADS
		Lock.unlock();
		#else
		waiters_mutex.unlock ();
		#endif

		completed ();
		_state.set_to (READY2DIE);
	}

	request_ptr wincall_file::aread (
			void *buffer, 
			stxxl::int64 pos, 
			size_t bytes,
			completion_handler on_cmpl)
	{
		request_ptr req = new wincall_request(this, 
					buffer, pos, bytes,
					request::READ, on_cmpl);
		
		if(!req.get())
			stxxl_function_error;
		
		#ifndef NO_OVERLAPPING
		disk_queues::get_instance ()->add_readreq(req,get_id());
		#endif
    return req;
	}
	request_ptr wincall_file::awrite (
			void *buffer, 
			stxxl::int64 pos, 
			size_t bytes,
			completion_handler on_cmpl)
	{
		request_ptr req = new wincall_request(this, buffer, pos, bytes,
					   request::WRITE, on_cmpl);
    
		if(!req.get())
			stxxl_function_error;
		
		#ifndef NO_OVERLAPPING
		disk_queues::get_instance ()->add_writereq(req,get_id());
		#endif
		return req;
	}

__STXXL_END_NAMESPACE

#endif // BOOST_MSVC

