/***************************************************************************
 *            debug.h
 *
 *  Thu Oct 26 18:16:00 2006
 *  Copyright  2006  Roman Dementiev
 *  Email dementiev@ira.uka.de
 ****************************************************************************/
#ifndef STXXL_EXCEPTIONS_H_
#define STXXL_EXCEPTIONS_H_

#include <iostream>
#include <string>
#include <stdexcept>
#include "../common/utils.h"

__STXXL_BEGIN_NAMESPACE 

  class io_error : public std::ios_base::failure
  {
    public:
     io_error () throw(): std::ios_base::failure("") {}
     io_error (const std::string & msg_) throw(): 
      std::ios_base::failure(msg_)
     {
     }
  };


__STXXL_END_NAMESPACE 

#endif /* STXXL_EXCEPTIONS_H_*/
