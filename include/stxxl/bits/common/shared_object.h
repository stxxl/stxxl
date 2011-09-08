/***************************************************************************
 *  include/stxxl/bits/common/shared_object.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_SHARED_OBJECT_HEADER
#define STXXL_SHARED_OBJECT_HEADER

#include <cassert>
#include <stxxl/bits/namespace.h>

__STXXL_BEGIN_NAMESPACE

//! \brief Behaves much like a Pointer, plus deleteing the referred object with it's last reference
//!    and ability to unify, i.e. make and refer a copy if the original object was shared.
//!
//! Use with objects derived from shared_object.
//! Similar to boost/shared_ptr.
template <class C>
class shared_object_pointer
{
    C * ptr;

    void new_reference()
    { new_reference(ptr); }

    void new_reference(C * o)
    { if (o) o->new_reference(); }

    void delete_reference()
    { if (ptr && ptr->delete_reference()) delete ptr; }

public:
    shared_object_pointer()
        : ptr(0) {}

    shared_object_pointer(C * pointer)
        : ptr(pointer)
    { new_reference(); }

    shared_object_pointer(const shared_object_pointer & shared_pointer)
        : ptr(shared_pointer.ptr)
    { new_reference(); }

    shared_object_pointer & operator = (const shared_object_pointer & shared_pointer)
    { return operator = (shared_pointer.ptr); }

    shared_object_pointer & operator = (C * pointer)
    {
        new_reference(pointer);
        delete_reference();
        ptr = pointer;
        return *this;
    }

    ~shared_object_pointer()
    { delete_reference(); }

    C & operator * () const
    {
        assert (ptr);
        return *ptr;
    }

    C * operator -> () const
    {
        assert(ptr);
        return ptr;
    }

    operator C * () const
    { return ptr; }

    C * get() const
    { return ptr; }

    bool operator == (const shared_object_pointer & shared_pointer) const
    { return ptr == shared_pointer.ptr; }

    operator bool () const
    { return ptr; }

    bool valid() const
    { return ptr; }

    bool empty() const
    { return ! ptr; }

    //! \brief if the object is referred by this shared_object_pointer only
    bool unique() const
    { return ptr && ptr->unique(); }

    //! \brief Make and refer a copy if the original object was shared.
    void unify()
    {
        if (ptr && ! ptr->unique())
            operator = (new C(*ptr));
    }
};

//! \brief Behaves much like a Pointer, plus deleteing the referred object with it's last reference
//!    and ability to unify, i.e. make and refer a copy if the original object was shared.
//!
//! Use with objects derived from shared_object.
//! Similar to boost/shared_ptr.
template <class C>
class const_shared_object_pointer
{
    const C * ptr;

    void new_reference()
    { new_reference(ptr); }

    void new_reference(const C * o)
    { if (o) o->new_reference(); }

    void delete_reference()
    { if (ptr && ptr->delete_reference()) delete ptr; }

public:
    const_shared_object_pointer(const shared_object_pointer<C> & shared_pointer)
        : ptr(shared_pointer.ptr)
    { new_reference(); }

    const_shared_object_pointer()
        : ptr(0) {}

    const_shared_object_pointer(const C * pointer)
        : ptr(pointer)
    { new_reference(); }

    const_shared_object_pointer(const const_shared_object_pointer & shared_pointer)
        : ptr(shared_pointer.ptr)
    { new_reference(); }

    const_shared_object_pointer & operator = (const const_shared_object_pointer & shared_pointer)
    { return operator = (shared_pointer.ptr); }

    const_shared_object_pointer & operator = (const C * pointer)
    {
        new_reference(pointer);
        delete_reference();
        ptr = pointer;
        return *this;
    }

    ~const_shared_object_pointer()
    { delete_reference(); }

    const C & operator * () const
    {
        assert (ptr);
        return *ptr;
    }

    const C * operator -> () const
    {
        assert(ptr);
        return ptr;
    }

    operator const C * () const
    { return ptr; }

    const C * get() const
    { return ptr; }

    bool operator == (const const_shared_object_pointer & shared_pointer) const
    { return ptr == shared_pointer.ptr; }

    operator bool () const
    { return ptr; }

    bool valid() const
    { return ptr; }

    bool empty() const
    { return ! ptr; }

    //! \brief if the object is referred by this shared_object_pointer only
    bool unique() const
    { return ptr && ptr->unique(); }
};

//! \brief Provides reference counting abilities for use with shared_object_pointer.
//!
//! Use as superclass of the actual object. Then either use shared_object_pointer as pointer
//!   to manage references and deletion, or just do normal new and delete.
class shared_object
{
    mutable int reference_count;

public:
    shared_object()
        : reference_count(0) {} // new objects have no reference

    shared_object(const shared_object &)
        : reference_count(0) {} // coping still creates a new object

    const shared_object & operator = (const shared_object &) const
    { return *this; } // changing the contents leaves pointers unchanged

    shared_object & operator = (const shared_object &)
    { return *this; } // changing the contents leaves pointers unchanged

    ~shared_object()
    { assert (! reference_count); }

private:
    template <class C> friend class shared_object_pointer;

    //! \brief Call whenever setting a pointer to the object
    void new_reference() const
    { ++reference_count; }

    //! \brief Call whenever resetting (i.e. overwriting) a pointer to the object.
    //! IMPORTANT: In case of self-assignment, call AFTER new_reference().
    //! \return if the object has to be deleted (i.e. if it's reference count dropped to zero)
    bool delete_reference() const
    { return (! --reference_count); }

    //! \brief if the shared_object is referenced by only one shared_object_pointer
    bool unique() const
    { return reference_count == 1; }
};

__STXXL_END_NAMESPACE

#endif // STXXL_SHARED_OBJECT_HEADER
