
#include "wfs_file.h"

#ifdef BOOST_MSVC

__STXXL_BEGIN_NAMESPACE


		HANDLE wfs_file_base::get_file_des() const
		{
			return file_des;
		}

		wfs_request_base::wfs_request_base (
				wfs_file_base * f, 
				void *buf, 
				stxxl::int64 off,
				size_t b, 
				request_type t,
				completion_handler on_cmpl):
					request (on_cmpl,f,buf,off,b,t),
			/*		file (f), 
					buffer (buf), 
					offset (off), 
					bytes (b),
					type(t), */
					_state (OP)
		{
#ifdef STXXL_CHECK_BLOCK_ALIGNING
			// Direct I/O requires filsystem block size alighnment for file offsets,
			// memory buffer adresses, and transfer(buffer) size must be multiple
			// of the filesystem block size
			check_aligning ();
#endif
		}

		bool wfs_request_base::add_waiter (onoff_switch * sw)
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(waiters_mutex);
			#else
			waiters_mutex.lock ();
			#endif

			if (poll ()) // request already finished
			{
				#ifndef STXXL_BOOST_THREADS
				waiters_mutex.unlock ();
				#endif
				return true;
			}

			waiters.insert (sw);
			#ifndef STXXL_BOOST_THREADS
			waiters_mutex.unlock ();
			#endif

			return false;
		};
		void wfs_request_base::delete_waiter (onoff_switch * sw)
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(waiters_mutex);
			waiters.erase (sw);
			#else
			waiters_mutex.lock ();
			waiters.erase (sw);
			waiters_mutex.unlock ();
			#endif
		}
		int wfs_request_base::nwaiters () // returns number of waiters
		{
			#ifdef STXXL_BOOST_THREADS
			boost::mutex::scoped_lock Lock(waiters_mutex);
			return waiters.size();
			#else
			waiters_mutex.lock ();
			int size = waiters.size ();
			waiters_mutex.unlock ();
			return size;
			#endif
		}
		void wfs_request_base::check_aligning ()
		{
			if (offset % BLOCK_ALIGN)
				STXXL_ERRMSG ("Offset is not aligned: modulo "
					      << BLOCK_ALIGN << " = " <<
					      offset % BLOCK_ALIGN);
			if (bytes % BLOCK_ALIGN)
				STXXL_ERRMSG ("Size is multiple of " <<
					      BLOCK_ALIGN << ", = " << bytes % BLOCK_ALIGN);
			if (long (buffer) % BLOCK_ALIGN)
				STXXL_ERRMSG ("Buffer is not aligned: modulo "
					      << BLOCK_ALIGN << " = " <<
					      long (buffer) % BLOCK_ALIGN << " (" << 
								std::hex << buffer << std::dec << ")");
		}
		
		unsigned wfs_request_base::size() const { return bytes; }
	
		wfs_request_base::~wfs_request_base ()
		{
      		STXXL_VERBOSE3("wfs_request_base "<< unsigned(this) <<": deletion, cnt: "<<ref_cnt)
	
			assert(_state() == DONE || _state() == READY2DIE);
			
			// if(_state() != DONE && _state()!= READY2DIE )
			//	STXXL_ERRMSG("WARNING: serious stxxl error requiest being deleted while I/O did not finish "<<
			//		"! Please report it to the stxxl author(s) <dementiev@mpi-sb.mpg.de>")
			
			// _state.wait_for (READY2DIE); // does not make sense ?
		}

		void wfs_request_base::wait ()
		{
			STXXL_VERBOSE3("wfs_request_base : "<< unsigned(this) <<" wait ")
			
			START_COUNT_WAIT_TIME 
			
			#ifdef NO_OVERLAPPING
			enqueue();
			#endif
			
			_state.wait_for (READY2DIE);
			
			END_COUNT_WAIT_TIME
		}
		bool wfs_request_base::poll()
		{
			#ifdef NO_OVERLAPPING
			/*if(_state () < DONE)*/ wait();
			#endif
			
			return (_state () >= DONE);
		}
		const char *wfs_request_base::io_type ()
		{
			return "wfs_base";
		}

	wfs_file_base::wfs_file_base (
			const std::string & filename, 
			int mode, 
			int disk): file (disk), file_des (INVALID_HANDLE_VALUE),mode_(mode)
	{
		DWORD dwDesiredAccess = 0;
		DWORD dwShareMode = 0;
		DWORD dwCreationDisposition = 0;
		DWORD dwFlagsAndAttributes = 0;

		#ifndef STXXL_DIRECT_IO_OFF
		if (mode & DIRECT)
		{
			dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
			// TODO: try also FILE_FLAG_WRITE_THROUGH option ?
		}
		#endif

		if (mode & RDONLY)
		{
			dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
			dwDesiredAccess |= GENERIC_READ;
		}
			
		if (mode & WRONLY)
		{
			dwDesiredAccess |= GENERIC_WRITE;
		}

		if (mode & RDWR)
		{
			dwDesiredAccess |= (GENERIC_READ|GENERIC_WRITE);
		}

		if (mode & CREAT)
		{
			// ignored
		}
		

		if (mode & TRUNC)
		{
			dwCreationDisposition |= TRUNCATE_EXISTING;
		}
		else
		{
			dwCreationDisposition |= OPEN_ALWAYS;
		}

		file_des = ::CreateFile(filename.c_str(),dwDesiredAccess,dwShareMode,NULL,
			dwCreationDisposition,dwFlagsAndAttributes,NULL);

		if(file_des == INVALID_HANDLE_VALUE)
			stxxl_win_lasterror_exit("CreateFile  filename="<<filename)


	};
	wfs_file_base::~wfs_file_base ()
	{
		if(!CloseHandle(file_des))
			stxxl_win_lasterror_exit("closing file (call of ::CloseHandle) ")
	
		file_des = INVALID_HANDLE_VALUE;
	};
	stxxl::int64 wfs_file_base::size ()
	{
		LARGE_INTEGER result;
		if(!GetFileSizeEx(file_des,&result))
			stxxl_win_lasterror_exit("GetFileSizeEx ")

		return result.QuadPart;
	};
	void wfs_file_base::set_size (stxxl::int64 newsize)
	{
		stxxl::int64 cur_size = size();
		
		LARGE_INTEGER desired_pos;
		desired_pos.QuadPart = newsize;

		if(!SetFilePointerEx(file_des,desired_pos,NULL,FILE_BEGIN))
			stxxl_win_lasterror_exit("SetFilePointerEx in wfs_file_base::set_size(..) oldsize="<<cur_size<<
			" newsize="<<newsize<<" ")
		
		if(!SetEndOfFile(file_des))
			stxxl_win_lasterror_exit("SetEndOfFile oldsize="<<cur_size<<
			" newsize="<<newsize<<" ")

	};
  

__STXXL_END_NAMESPACE

#endif // BOOST_MSVC

