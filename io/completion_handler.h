 /***************************************************************************
 *            completion_handler.h
 *
 *  Mon Apr 21 02:45:45 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 *  Loki-style completion handler (functors)
 ****************************************************************************/
#ifndef COMPLETION_HANDLER_HEADER
#define COMPLETION_HANDLER_HEADER

#include "../common/namespace.h"
#include <memory>

__STXXL_BEGIN_NAMESPACE

class request;

class completion_handler_impl
{
public:
  virtual void operator()(request * ) = 0;
  virtual completion_handler_impl * clone() const = 0;
  virtual ~completion_handler_impl() {}
};

//! \brief Completion handler class (Loki-style)

//! In some situations one needs to execute
//! some actions after completion of an I/O
//! request. In these cases one can use
//! an I/O completion handler - a function
//! object that can be passed as a parameter
//! to asynchronous I/O calls \c stxxl::file::aread
//! and \c stxxl::file::awrite .
//! For an example of use see \link mng/test_mng.cpp mng/test_mng.cpp \endlink
class completion_handler
{
public:
  completion_handler():sp_impl_(0) {}
  completion_handler(const completion_handler & obj):sp_impl_(obj.sp_impl_.get()->clone()){}
  completion_handler& operator = (const completion_handler & obj)
  {
    completion_handler copy(obj);
    completion_handler_impl* p = sp_impl_.release();
    sp_impl_.reset(copy.sp_impl_.release());
    copy.sp_impl_.reset(p);
    return *this; 
  }
  void operator()(request * req)
  {
    (*sp_impl_)(req);
  }
  template <typename handler_type>
  completion_handler(const handler_type & handler__);
private:
  std::auto_ptr<completion_handler_impl> sp_impl_;
};

template <typename handler_type>
class completion_handler1: public completion_handler_impl
{
private:
  handler_type handler_;
public:
  completion_handler1(const handler_type & handler__): handler_(handler__) {}
  completion_handler1* clone() const
  {
    return new completion_handler1(*this);
  }
  void operator()(request * req)
  {
    handler_(req);
  }
};

template <typename handler_type>
completion_handler::completion_handler(const handler_type & handler__): 
  sp_impl_(new completion_handler1<handler_type>(handler__))
{
}

__STXXL_END_NAMESPACE

#endif
