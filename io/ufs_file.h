#ifndef UFSFILEBASE_HEADER
#define UFSFILEBASE_HEADER

/***************************************************************************
 *            ufs_file.h
 *  UNIX file system file base
 *  Sat Aug 24 23:55:13 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/


#include "iobase.h"

__STXXL_BEGIN_NAMESPACE

  //! \addtogroup fileimpl
  //! \{

	class ufs_request_base;

	//! \brief Base for UNIX file system implementations
	class ufs_file_base:public file
	{
	protected:
		int file_des;	// file descriptor
		
		ufs_file_base (const std::string & filename, int mode, int disk);
	private:
		ufs_file_base ();
	public:
		int get_file_des()
		{
			return file_des;
		};
		~ufs_file_base();
		off_t size ();
		void set_size(off_t newsize);
	};

	//! \brief Base for UNIX file system implementations
	class ufs_request_base:public request
	{
		friend class ufs_file_base;
	protected:
		// states of request
		enum { OP = 0, DONE = 1, READY2DIE = 2 };	// OP - operating, DONE - request served, 
																							// READY2DIE - can be destroyed
		ufs_file_base *file;
		void *buffer;
		off_t offset;
		size_t bytes;
		request_type type;
		
		state _state;
		mutex waiters_mutex;
		std::set < onoff_switch * > waiters;

		ufs_request_base (
				ufs_file_base * f, 
				void *buf, 
				off_t off,
				size_t b, 
				request_type t,
				completion_handler on_cmpl):
					request (on_cmpl),
					file (f), 
					buffer (buf), 
					offset (off), 
					bytes (b),
					type(t),
					_state (OP)
		{
#ifdef STXXL_CHECK_BLOCK_ALIGNING
			// Linux direct I/O requires filsystem block size alighnment for file offsets,
			// memory buffer adresses, and transfer(buffer) size must be multiple
			// of the filesystem block size
			check_aligning ();
#endif
		};/*
		void enqueue(request_ptr & req)
		{
			if (type == READ)
				disk_queues::get_instance ()->add_readreq(req,file->get_id());
			else
				disk_queues::get_instance ()->add_writereq(req,file->get_id());
		};*/
		bool add_waiter (onoff_switch * sw)
		{
			waiters_mutex.lock ();

			if (poll ()) // request already finished
			{
				waiters_mutex.unlock ();
				return true;
			}

			waiters.insert (sw);
			waiters_mutex.unlock ();

			return false;
		};
		void delete_waiter (onoff_switch * sw)
		{
			waiters_mutex.lock ();
			waiters.erase (sw);
			waiters_mutex.unlock ();
		}
		int nwaiters () // returns number of waiters
		{
			waiters_mutex.lock ();
			int size = waiters.size ();
			waiters_mutex.unlock ();
			return size;
		}
		void check_aligning ()
		{
			if (offset % BLOCK_ALIGN)
				STXXL_ERRMSG ("Offset is not aligned: modulo "
					      << BLOCK_ALIGN << " = " <<
					      offset % BLOCK_ALIGN);
			if (bytes % BLOCK_ALIGN)
				STXXL_ERRMSG ("Size is multiple of " <<
					      BLOCK_ALIGN << ", = " << bytes % BLOCK_ALIGN);
			if (unsigned (buffer) % BLOCK_ALIGN)
				STXXL_ERRMSG ("Buffer is not aligned: modulo "
					      << BLOCK_ALIGN << " = " <<
					      unsigned (buffer) % BLOCK_ALIGN << " (" << 
								std::hex << buffer << std::dec << ")");
		}
		
		unsigned size() const { return bytes; }
	public:
		virtual ~ufs_request_base ()
		{
      		STXXL_VERBOSE3("ufs_request_base "<< unsigned(this) <<": deletion, cnt: "<<ref_cnt)
	
			assert(_state() == DONE || _state() == READY2DIE);
			
			// if(_state() != DONE && _state()!= READY2DIE )
			//	STXXL_ERRMSG("WARNING: serious stxxl error requiest being deleted while I/O did not finish "<<
			//		"! Please report it to the stxxl author(s) <dementiev@mpi-sb.mpg.de>")
			
			// _state.wait_for (READY2DIE); // does not make sense ?
		};

		void wait ()
		{
			STXXL_VERBOSE3("ufs_request_base : "<< unsigned(this) <<" wait ")
			
			START_COUNT_WAIT_TIME 
			
			#ifdef NO_OVERLAPPING
			enqueue();
			#endif
			
			_state.wait_for (READY2DIE);
			
			END_COUNT_WAIT_TIME
		};
		bool poll()
		{
			#ifdef NO_OVERLAPPING
			/*if(_state () < DONE)*/ wait();
			#endif
			
			return (_state () >= DONE);
		};
		const char *io_type ()
		{
			return "ufs_base";
		};
	};

	ufs_file_base::ufs_file_base (
			const std::string & filename, 
			int mode, 
			int disk): file (disk), file_des (-1)
	{
		int fmode = 0;
		#ifndef STXXL_DIRECT_IO_OFF
		if (mode & DIRECT)
			fmode |= O_SYNC | O_RSYNC | O_DSYNC | O_DIRECT;
		#endif
		if (mode & RDONLY)
			fmode |= O_RDONLY;
		if (mode & WRONLY)
			fmode |= O_WRONLY;
		if (mode & RDWR)
			fmode |= O_RDWR;
		if (mode & CREAT)
			fmode |= O_CREAT;
		if (mode & TRUNC)
			fmode |= O_TRUNC;


		stxxl_ifcheck_i ((file_des =::open (filename.c_str(), fmode,
				      S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP)),
				"Filedescriptor="<<file_des<<" filename="<<filename<< " fmode="<<fmode);
	};
	ufs_file_base::~ufs_file_base ()
	{
		int res =::close (file_des);

		// if successful, reset file descriptor
		if (res >= 0)
			file_des = -1;
		else
			stxxl_function_error;
	};
	off_t ufs_file_base::size ()
	{
		struct stat st;
		stxxl_ifcheck (fstat (file_des, &st));
		return st.st_size;
	};
	void ufs_file_base::set_size (off_t newsize)
	{
		if (newsize > size ())
		{
			stxxl_ifcheck (::lseek (file_des, newsize - 1,SEEK_SET));
		}
	};
  
  //! \}

__STXXL_END_NAMESPACE

#endif
