/***************************************************************************
 *  include/stxxl/bits/common/counting_ptr.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *  Copyright (C) 2013-2017 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_COUNTING_PTR_HEADER
#define STXXL_COMMON_COUNTING_PTR_HEADER

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <ostream>
#include <type_traits>

namespace stxxl {

//! \addtogroup support
//! \{

//! default deleter for counting_ptr
class default_counting_ptr_deleter
{
public:
    template <typename Type>
    void operator () (Type* ptr) const noexcept
    {
        delete ptr;
    }
};

/*!
 * High-performance smart pointer used as a wrapping reference counting pointer.
 *
 * This smart pointer class requires two functions in the template type: void
 * inc_reference() and void dec_reference(). These must increment and decrement
 * a reference count inside the templated object. When initialized, the type
 * must have reference count zero. Each new object referencing the data calls
 * inc_reference() and each destroying holder calls del_reference(). When the
 * data object determines that it's internal count is zero, then it must destroy
 * itself.
 *
 * Accompanying the counting_ptr is a class reference_count, from which
 * reference counted classes may be derive from. The class reference_count
 * implement all methods required for reference counting.
 *
 * The whole method is more similar to boost's instrusive_ptr, but also yields
 * something resembling std::shared_ptr. However, compared to std::shared_ptr,
 * this class only contains a single pointer, while shared_ptr contains two
 * which are only related if constructed with std::make_shared.
 */
template <typename Type,
          typename Deleter = default_counting_ptr_deleter>
class counting_ptr
{
public:
    //! contained type.
    using element_type = Type;

private:
    //! the pointer to the currently referenced object.
    Type* ptr_;

    //! increment reference count for current object.
    void inc_reference() noexcept
    { inc_reference(ptr_); }

    //! increment reference count of other object.
    void inc_reference(Type* o) noexcept
    { if (o) o->inc_reference(); }

    //! decrement reference count of current object and maybe delete it.
    void dec_reference() noexcept
    { if (ptr_ && ptr_->dec_reference()) Deleter()(ptr_); }

public:
    //! all counting_ptr are friends such that they may steal pointers.
    template <typename Other, typename OtherDeleter>
    friend class counting_ptr;

    //! default constructor: contains a nullptr pointer.
    counting_ptr() noexcept
        : ptr_(nullptr) { }

    //! implicit construction from nullptr_t: contains a nullptr pointer.
    counting_ptr(std::nullptr_t) noexcept // NOLINT
        : ptr_(nullptr) { }

    //! constructor from pointer: initializes new reference to ptr.
    explicit counting_ptr(Type* ptr) noexcept
        : ptr_(ptr) { inc_reference(); }

    //! copy-constructor: also initializes new reference to ptr.
    counting_ptr(const counting_ptr& other) noexcept
        : ptr_(other.ptr_) { inc_reference(); }

    //! copy-constructor: also initializes new reference to ptr.
    template <typename Down,
              typename = typename std::enable_if<
                  std::is_convertible<Down*, Type*>::value, void>::type>
    counting_ptr(const counting_ptr<Down, Deleter>& other) noexcept
        : ptr_(other.ptr_) { inc_reference(); }

    //! move-constructor: just moves pointer, does not change reference counts.
    counting_ptr(counting_ptr&& other) noexcept
        : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    //! move-constructor: just moves pointer, does not change reference counts.
    template <typename Down,
              typename = typename std::enable_if<
                  std::is_convertible<Down*, Type*>::value, void>::type>
    counting_ptr(counting_ptr<Down, Deleter>&& other) noexcept
        : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    //! copy-assignment operator: acquire reference on new one and dereference
    //! current object.
    counting_ptr& operator = (const counting_ptr& other) noexcept
    {
        if (&other == this) return *this;
        inc_reference(other.ptr_);
        dec_reference();
        ptr_ = other.ptr_;
        return *this;
    }

    //! copy-assignment operator: acquire reference on new one and dereference
    //! current object.
    template <typename Down,
              typename = typename std::enable_if<
                  std::is_convertible<Down*, Type*>::value, void>::type>
    counting_ptr& operator = (
        const counting_ptr<Down, Deleter>& other) noexcept
    {
        if (&other == this) return *this;
        inc_reference(other.ptr_);
        dec_reference();
        ptr_ = other.ptr_;
        return *this;
    }

    //! move-assignment operator: move reference of other to current object.
    counting_ptr& operator = (counting_ptr&& other) noexcept
    {
        if (&other == this) return *this;
        dec_reference();
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
        return *this;
    }

    //! move-assignment operator: move reference of other to current object.
    template <typename Down,
              typename = typename std::enable_if<
                  std::is_convertible<Down*, Type*>::value, void>::type>
    counting_ptr& operator = (counting_ptr<Down, Deleter>&& other) noexcept
    {
        if (get() == this->get()) return *this;
        dec_reference();
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
        return *this;
    }

    //! destructor: decrements reference count in ptr.
    ~counting_ptr()
    { dec_reference(); }

    //! return the enclosed object as reference.
    Type& operator * () const noexcept
    {
        assert(ptr_);
        return *ptr_;
    }

    //! return the enclosed pointer.
    Type* operator -> () const noexcept
    {
        assert(ptr_);
        return ptr_;
    }

    //! return the enclosed pointer.
    Type * get() const noexcept
    { return ptr_; }

    //! test equality of only the pointer values.
    bool operator == (const counting_ptr& other) const noexcept
    { return ptr_ == other.ptr_; }

    //! test inequality of only the pointer values.
    bool operator != (const counting_ptr& other) const noexcept
    { return ptr_ != other.ptr_; }

    //! test equality of only the address pointed to
    bool operator == (Type* other) const noexcept
    { return ptr_ == other; }

    //! test inequality of only the address pointed to
    bool operator != (Type* other) const noexcept
    { return ptr_ != other; }

    //! cast to bool check for a nullptr pointer
    operator bool () const noexcept
    { return valid(); }

    //! test for a non-nullptr pointer
    bool valid() const noexcept
    { return (ptr_ != nullptr); }

    //! test for a nullptr pointer
    bool empty() const noexcept
    { return (ptr_ == nullptr); }

    //! if the object is referred by this counting_ptr only
    bool unique() const noexcept
    { return ptr_ && ptr_->unique(); }

    //! make and refer a copy if the original object was shared.
    void unify()
    {
        if (ptr_ && !ptr_->unique())
            operator = (counting_ptr(new Type(*ptr_)));
    }

    //! release contained pointer
    void reset()
    {
        dec_reference();
        ptr_ = nullptr;
    }

    //! swap enclosed object with another counting pointer (no reference counts
    //! need change)
    void swap(counting_ptr& b) noexcept
    { std::swap(ptr_, b.ptr_); }
};

template <typename Type, typename... Args>
counting_ptr<Type> make_counting(Args&& ... args)
{
    return counting_ptr<Type>(new Type(std::forward<Args>(args) ...));
}

//! swap enclosed object with another counting pointer (no reference counts need
//! change)
template <typename A>
void swap(counting_ptr<A>& a1, counting_ptr<A>& a2) noexcept
{
    a1.swap(a2);
}

//! print pointer
template <typename A>
std::ostream& operator << (std::ostream& os, const counting_ptr<A>& c)
{
    return os << c.get();
}

/*!
 * Provides reference counting abilities for use with counting_ptr.
 *
 * Use as superclass of the actual object, this adds a reference_count
 * value. Then either use counting_ptr as pointer to manage references and
 * deletion, or just do normal new and delete.
 */
class reference_count
{
private:
    //! the reference count is kept mutable for counting_ptr<const Type> to
    //! change the reference count.
    mutable std::atomic<size_t> reference_count_;

public:
    //! new objects have zero reference count
    reference_count() noexcept
        : reference_count_(0) { }

    //! coping still creates a new object with zero reference count
    reference_count(const reference_count&) noexcept
        : reference_count_(0) { }

    //! assignment operator, leaves pointers unchanged
    reference_count& operator = (const reference_count&) noexcept
    { return *this; } // changing the contents leaves pointers unchanged

    ~reference_count()
    { assert(reference_count_ == 0); }

public:
    //! Call whenever setting a pointer to the object
    void inc_reference() const noexcept
    { ++reference_count_; }

    /*!
     * Call whenever resetting (i.e. overwriting) a pointer to the object.
     * IMPORTANT: In case of self-assignment, call AFTER inc_reference().
     *
     * \return if the object has to be deleted (i.e. if it's reference count
     * dropped to zero)
     */
    bool dec_reference() const noexcept
    { assert(reference_count_ > 0); return (--reference_count_ == 0); }

    //! Test if the reference_count is referenced by only one counting_ptr.
    bool unique() const noexcept
    { return (reference_count_ == 1); }

    //! Return the number of references to this object (for debugging)
    size_t get_reference_count() const noexcept
    { return reference_count_; }
};

//! \}

} // namespace stxxl

#endif // !STXXL_COMMON_COUNTING_PTR_HEADER
