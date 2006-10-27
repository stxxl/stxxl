#include "boostfd_file.h"
#include "../common/debug.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/fstream.hpp"


#ifdef STXXL_BOOST_CONFIG

__STXXL_BEGIN_NAMESPACE


    boost::iostreams::file_descriptor 
      boostfd_file::get_file_des() const
    {
      return file_des;
    }

    boostfd_request::boostfd_request (
        boostfd_file * f, 
        void * buf, 
        stxxl::int64 off,
        size_t b, 
        request_type t,
        completion_handler on_cmpl):
          request (on_cmpl,f,buf,off,b,t),
          _state (OP)
    {
    }

    bool boostfd_request::add_waiter (onoff_switch * sw)
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
    }
    void boostfd_request::delete_waiter (onoff_switch * sw)
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
    int boostfd_request::nwaiters () // returns number of waiters
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
    void boostfd_request::check_aligning ()
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
  
    boostfd_request::~boostfd_request ()
    {
          STXXL_VERBOSE3("boostfd_request "<< unsigned(this) <<": deletion, cnt: "<<ref_cnt)
  
      assert(_state() == DONE || _state() == READY2DIE);
    }

    void boostfd_request::wait ()
    {
      STXXL_VERBOSE3("boostfd_request : "<< unsigned(this) <<" wait ")
      
      START_COUNT_WAIT_TIME 
      
      #ifdef NO_OVERLAPPING
      enqueue();
      #endif
      
      _state.wait_for (READY2DIE);
      
      END_COUNT_WAIT_TIME
      
      check_errors();
    }
    bool boostfd_request::poll()
    {
      #ifdef NO_OVERLAPPING
      /*if(_state () < DONE)*/ wait();
      #endif
      
      const bool s = _state() >= DONE;
      
      check_errors();
      
      return s;
    }
    const char *boostfd_request::io_type ()
    {
      return "boostfd";
    }

  boostfd_file::boostfd_file (
      const std::string & filename, 
      int mode, 
      int disk): file (disk),mode_(mode)
  {
    BOOST_IOS::openmode boostfd_mode;


    #ifndef STXXL_DIRECT_IO_OFF
    if (mode & DIRECT)
    {
      // direct mode not supported in Boost
    }
    #endif

    if (mode & RDONLY)
    {
      boostfd_mode = BOOST_IOS::in;
    }
      
    if (mode & WRONLY)
    {
      boostfd_mode = BOOST_IOS::out;
    }

    if (mode & RDWR)
    {
      boostfd_mode = BOOST_IOS::out | BOOST_IOS::in;
    }

    
	  const boost::filesystem::path fspath(filename,
		  boost::filesystem::native);
    
    if (mode & TRUNC)
    {
      if(boost::filesystem::exists(fspath))
      {
        boost::filesystem::remove(fspath);
        boost::filesystem::ofstream f(fspath);
        f.close();
        assert(boost::filesystem::exists(fspath));
      }
    }

    if (mode & CREAT)
    {
      // need to be emulated:
      if(!boost::filesystem::exists(fspath))
      {
        boost::filesystem::ofstream f(fspath);
        f.close();
        assert(boost::filesystem::exists(fspath));
      }
    }

    
    file_des.open(filename,boostfd_mode,boostfd_mode);
       
    
  }
  
  boostfd_file::~boostfd_file ()
  {
    file_des.close();    
  }
  stxxl::int64 boostfd_file::size ()
  {
    stxxl::int64 size_ = file_des.seek(0,BOOST_IOS::end); 
    return size_;
  }
  void boostfd_file::set_size (stxxl::int64 newsize)
  {
    stxxl::int64 oldsize = size();
    file_des.seek(newsize,BOOST_IOS::beg);
    file_des.seek(0,BOOST_IOS::beg); // not important ?
    assert(size() >= oldsize);
  }
  
  void boostfd_request::serve ()
  {
    stats * iostats = stats::get_instance();
    if(nref() < 2)
    {
      STXXL_ERRMSG("WARNING: serious error, reference to the request is lost before serve (nref="
        <<nref()<<") "<<
       " this="<<long(this)<< " offset="<<offset<<" buffer="<<buffer<<" bytes="<<bytes 
       << " type=" <<((type == READ)?"READ":"WRITE") )
    }
    STXXL_VERBOSE2("boostfd_request::serve(): Buffer at "<<((void*)buffer)
      <<" offset: "<<offset<<" bytes: "<<bytes<<((type== READ)?" READ":" WRITE")
      );
    
    boostfd_file::fd_type fd = static_cast<boostfd_file*>(file_)->get_file_des();
    
    try
    {
      fd.seek(offset,BOOST_IOS::beg);
    }
    catch(const std::exception & ex)
    {
      STXXL_FORMAT_ERROR_MSG(msg,"seek() in boostfd_request::serve() offset="<<offset
        <<" this="<<long(this)<<" buffer="<<
        buffer<<" bytes="<<bytes
        << " type=" <<((type == READ)?"READ":"WRITE")<< " : "<<ex.what() )
     
      error_occured(msg.str());
    }
   
    {
      if (type == READ)
      {
        #ifdef STXXL_IO_STATS
          iostats->read_started (size());
        #endif
        
        debugmon::get_instance()->io_started((char*)buffer);
        
        try
        {
          fd.read((char*)buffer,bytes);
        }
        catch(const std::exception & ex)
        {
          STXXL_FORMAT_ERROR_MSG(msg,"read() in boostfd_request::serve() offset="<<offset
            <<" this="<<long(this)<<" buffer="<<
            buffer<<" bytes="<<bytes
            << " type=" <<((type == READ)?"READ":"WRITE")<< 
            " nref= "<<nref() <<" : "<<ex.what() )
     
           error_occured(msg.str());
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
     
        try
        {   
          fd.write((char*)buffer,bytes);
        }
        catch(const std::exception & ex)
        {
          STXXL_FORMAT_ERROR_MSG(msg,"write() in boostfd_request::serve() offset="<<offset
            <<" this="<<long(this)<<" buffer="<<
            buffer<<" bytes="<<bytes
            << " type=" <<((type == READ)?"READ":"WRITE")<< 
            " nref= "<<nref() <<" : "<<ex.what() )
     
           error_occured(msg.str());
        }

        debugmon::get_instance()->io_finished((char*)buffer);
        
        #ifdef STXXL_IO_STATS
          iostats->write_finished ();
        #endif
      }
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

  request_ptr boostfd_file::aread (
      void *buffer, 
      stxxl::int64 pos, 
      size_t bytes,
      completion_handler on_cmpl)
  {
    request_ptr req = new boostfd_request(this, 
          buffer, pos, bytes,
          request::READ, on_cmpl);
    
    if(!req.get())
      stxxl_function_error(io_error);
    
    #ifndef NO_OVERLAPPING
    disk_queues::get_instance ()->add_readreq(req,get_id());
    #endif
    return req;
  }
  request_ptr boostfd_file::awrite (
      void *buffer, 
      stxxl::int64 pos, 
      size_t bytes,
      completion_handler on_cmpl)
  {
    request_ptr req = new boostfd_request(this, buffer, pos, bytes,
             request::WRITE, on_cmpl);
    
    if(!req.get())
      stxxl_function_error(io_error);
    
    #ifndef NO_OVERLAPPING
    disk_queues::get_instance ()->add_writereq(req,get_id());
    #endif
    return req;
  }
  
  

__STXXL_END_NAMESPACE

#endif // BOOST_VERSION

