#ifndef WFSFILEBASE_HEADER
#define WFSFILEBASE_HEADER

/***************************************************************************
 *            wfs_file.h
 *  Windows file system file base
 *  Fri Jul 22 15:39:59 2005
 *  Copyright  2005  Roman Dementiev
 *  dementiev@ira.uka.de
 ****************************************************************************/

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef BOOST_MSVC

#include "stxxl/bits/io/iobase.h"


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

class wfs_request_base;

//! \brief Base for Windows file system implementations
class wfs_file_base : public file
{
protected:
    HANDLE file_des;             // file descriptor
    int mode_;             // open mode
    wfs_file_base (const std::string & filename, int mode, int disk);
public:
    HANDLE get_file_des() const;
    ~wfs_file_base();
    stxxl::int64 size ();
    void set_size(stxxl::int64 newsize);
    void lock();
};

//! \brief Base for UNIX file system implementations
class wfs_request_base : public request
{
    friend class wfs_file_base;
protected:
    // states of request
    enum { OP = 0, DONE = 1, READY2DIE = 2 };                   // OP - operating, DONE - request served,
    // READY2DIE - can be destroyed
    /*
       wfs_file_base *file;
       void *buffer;
       stxxl::int64 offset;
       size_t bytes;
       request_type type;
     */

    state _state;
 #ifdef STXXL_BOOST_THREADS
    boost::mutex waiters_mutex;
 #else
    mutex waiters_mutex;
 #endif
    std::set < onoff_switch * > waiters;

    wfs_request_base (
        wfs_file_base * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t,
        completion_handler on_cmpl);

    bool add_waiter (onoff_switch * sw);
    void delete_waiter (onoff_switch * sw);
    int nwaiters ();             // returns number of waiters
    void check_aligning ();
public:
    virtual ~wfs_request_base ();
    void wait ();
    bool poll();
    const char * io_type ();
};

//! \}

__STXXL_END_NAMESPACE

#endif // BOOST_MSVC

#endif // WFSFILEBASE_HEADER
