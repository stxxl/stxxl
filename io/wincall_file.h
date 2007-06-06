#ifndef WINCALL_HEADER
#define WINCALL_HEADER
/***************************************************************************
 *            wincall_file.h
 *
 *  Fri Aug 22 17:00:00 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@ira.uka.de
 ****************************************************************************/

#include "wfs_file.h"
#include "../common/debug.h"

#ifdef BOOST_MSVC

__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

//! \brief Implementation of file based on Windows native I/O calls
class wincall_file : public wfs_file_base
{
protected:
public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \attention filename must be resided at memory disk partition
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    wincall_file(
        const std::string & filename,
        int mode,
        int disk = -1);
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

//! \brief Implementation of request based on UNIX syscalls
class wincall_request : public wfs_request_base
{
    friend class wincall_file;
protected:
    wincall_request(
        wincall_file * f,
        void * buf,
        stxxl::int64 off,
        size_t b,
        request_type t,
        completion_handler on_cmpl);
    void serve ();
public:
    const char * io_type ();
private:
    // Following methods are declared but not implemented
    // intentionnaly to forbid their usage
    wincall_request(const wincall_request &);
    wincall_request & operator=(const wincall_request &);
    wincall_request();
};


//! \}

__STXXL_END_NAMESPACE

#endif // BOOST_MSVC

#endif // WINCALL_HEADER
