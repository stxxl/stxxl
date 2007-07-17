#ifndef MMAP_HEADER
#define MMAP_HEADER

/***************************************************************************
 *            mmap_file.h
 *
 *  Sat Aug 24 23:54:54 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/bits/io/ufs_file.h"

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

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
class mmap_file : public ufs_file_base
{
public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    inline mmap_file (const std::string & filename, int mode, int disk = -1) :
        ufs_file_base (filename, mode, disk)
    { }
    request_ptr aread(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
    request_ptr awrite(
        void * buffer,
        stxxl::int64 pos,
        size_t bytes,
        completion_handler on_cmpl);
};

//! \brief Implementation of memory mapped access file request
class mmap_request : public ufs_request_base
{
    friend class mmap_file;
protected:
    inline mmap_request (mmap_file * f,
                         void * buf, stxxl::int64 off, size_t b,
                         request_type t,
                         completion_handler on_cmpl) :
        ufs_request_base (f, buf, off, b, t, on_cmpl)
    { }
    void serve();

public:
    inline const char * io_type()
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

//! \}

__STXXL_END_NAMESPACE

#endif

#endif
