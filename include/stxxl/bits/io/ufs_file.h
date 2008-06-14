/***************************************************************************
 *            ufs_file.h
 *  UNIX file system file base
 *  Sat Aug 24 23:55:13 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#ifndef STXXL_UFSFILEBASE_HEADER
#define STXXL_UFSFILEBASE_HEADER

#include "stxxl/bits/io/iobase.h"


__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

class ufs_request_base;

//! \brief Base for UNIX file system implementations
class ufs_file_base : public file
{
protected:
    int file_des;               // file descriptor
    int mode_;             // open mode
    ufs_file_base (const std::string & filename, int mode, int disk);

public:
    int get_file_des() const;
    ~ufs_file_base();
    stxxl::int64 size ();
    void set_size(stxxl::int64 newsize);
    void lock();
};

//! \brief Base for UNIX file system implementations
class ufs_request_base : public request
{
    friend class ufs_file_base;

protected:
    // states of request
    enum { OP = 0, DONE = 1, READY2DIE = 2 };                   // OP - operating, DONE - request served,
    // READY2DIE - can be destroyed
    /*
       ufs_file_base *file;
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

    ufs_request_base (
        ufs_file_base * f,
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
    virtual ~ufs_request_base ();
    void wait ();
    bool poll();
    const char * io_type ();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_UFSFILEBASE_HEADER
