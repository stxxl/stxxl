/***************************************************************************
 *  include/stxxl/bits/common/binary_buffer.h
 *
 *  Classes binary_buffer and binary_reader to construct data blocks with
 *  variable length content. Programs construct blocks using
 *  binary_buffer::put<type>() and read them using
 *  binary_reader::get<type>(). The operation sequences should match.
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_BINARY_BUFFER_HEADER
#define STXXL_COMMON_BINARY_BUFFER_HEADER

#include <tlx/define.hpp>

#include <foxxll/common/types.hpp>
#include <foxxll/common/utils.hpp>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace stxxl {

//! \addtogroup support
//! \{

/*!
 * binary_buffer represents a dynamically growable area of memory, which can be
 * modified by appending integral data types via put() and other basic
 * operations.
 */
class binary_buffer
{
protected:
    //! Allocated buffer pointer.
    char* m_data;

    //! Size of valid data.
    size_t m_size;

    //! Total capacity of buffer.
    size_t m_capacity;

public:
    //! Create a new empty object
    binary_buffer()
        : m_data(nullptr), m_size(0), m_capacity(0)
    { }

    //! Copy-Constructor, duplicates memory content.
    binary_buffer(const binary_buffer& other)
        : m_data(nullptr), m_size(0), m_capacity(0)
    {
        assign(other);
    }

    //! Constructor, copy memory area.
    binary_buffer(const void* data, size_t n)
        : m_data(nullptr), m_size(0), m_capacity(0)
    {
        assign(data, n);
    }

    //! Constructor, create object with n bytes pre-allocated.
    explicit binary_buffer(size_t n)
        : m_data(nullptr), m_size(0), m_capacity(0)
    {
        alloc(n);
    }

    //! Constructor from std::string, copies string content.
    explicit binary_buffer(const std::string& str)
        : m_data(nullptr), m_size(0), m_capacity(0)
    {
        assign(str.data(), str.size());
    }

    //! Destroys the memory space.
    ~binary_buffer()
    {
        dealloc();
    }

    //! Return a pointer to the currently kept memory area.
    const char * data() const
    {
        return m_data;
    }

    //! Return a writeable pointer to the currently kept memory area.
    char * data()
    {
        return m_data;
    }

    //! Return the currently used length in bytes.
    size_t size() const
    {
        return m_size;
    }

    //! Return the currently allocated buffer capacity.
    size_t capacity() const
    {
        return m_capacity;
    }

    //! Explicit conversion to std::string (copies memory of course).
    std::string str() const
    {
        return std::string(reinterpret_cast<const char*>(m_data), m_size);
    }

    //! Set the valid bytes in the buffer, use if the buffer is filled
    //! directly.
    binary_buffer & set_size(size_t n)
    {
        assert(n <= m_capacity);
        m_size = n;

        return *this;
    }

    //! Make sure that at least n bytes are allocated.
    binary_buffer & alloc(size_t n)
    {
        if (m_capacity < n)
        {
            m_capacity = n;
            m_data = static_cast<char*>(realloc(m_data, m_capacity));
        }

        return *this;
    }

    //! Deallocates the kept memory space (we use dealloc() instead of free()
    //! as a name, because sometimes "free" is replaced by the preprocessor)
    binary_buffer & dealloc()
    {
        if (m_data) free(m_data);
        m_data = nullptr;
        m_size = m_capacity = 0;

        return *this;
    }

    //! Detach the memory from the object, returns the memory pointer.
    const char * detach()
    {
        const char* data = m_data;
        m_data = nullptr;
        m_size = m_capacity = 0;
        return data;
    }

    //! Clears the memory contents, does not deallocate the memory.
    binary_buffer & clear()
    {
        m_size = 0;
        return *this;
    }

    //! Copy a memory range into the buffer, overwrites all current
    //! data. Roughly equivalent to clear() followed by append().
    binary_buffer & assign(const void* data, size_t len)
    {
        if (len > m_capacity) alloc(len);

        if (TLX_LIKELY(len > 0)) {
            memcpy(m_data, data, len);
        }
        m_size = len;

        return *this;
    }

    //! Copy the contents of another buffer object into this buffer, overwrites
    //! all current data. Roughly equivalent to clear() followed by append().
    binary_buffer & assign(const binary_buffer& other)
    {
        if (&other != this)
            assign(other.data(), other.size());

        return *this;
    }

    //! Assignment operator: copy other's memory range into buffer.
    binary_buffer& operator = (const binary_buffer& other)
    {
        if (&other != this)
            assign(other.data(), other.size());

        return *this;
    }

    //! Align the size of the buffer to a multiple of n. Fills up with 0s.
    binary_buffer & align(size_t n)
    {
        assert(n > 0);
        size_t rem = m_size % n;
        if (rem != 0)
        {
            size_t add = n - rem;
            if (m_size + add > m_capacity) dynalloc(m_size + add);
            memset(m_data + m_size, 0, add);
            m_size += add;
        }
        assert((m_size % n) == 0);

        return *this;
    }

    //! Dynamically allocate more memory. At least n bytes will be available,
    //! probably more to compensate future growth.
    binary_buffer & dynalloc(size_t n)
    {
        if (m_capacity < n)
        {
            // place to adapt the buffer growing algorithm as need.
            size_t newsize = m_capacity;

            while (newsize < n) {
                if (newsize < 256) newsize = 512;
                else if (newsize < 1024 * 1024) newsize = 2 * newsize;
                else newsize += 1024 * 1024;
            }

            alloc(newsize);
        }

        return *this;
    }

    // *** Appending Write Functions ***

    //! Append a memory range to the buffer
    binary_buffer & append(const void* data, size_t len)
    {
        if (m_size + len > m_capacity) dynalloc(m_size + len);

        memcpy(m_data + m_size, data, len);
        m_size += len;

        return *this;
    }

    //! Append the contents of a different buffer object to this one.
    binary_buffer & append(const class binary_buffer& bb)
    {
        return append(bb.data(), bb.size());
    }

    //! Append to contents of a std::string, excluding the null (which isn't
    //! contained in the string size anyway).
    binary_buffer & append(const std::string& s)
    {
        return append(s.data(), s.size());
    }

    //! Put (append) a single item of the template type T to the buffer. Be
    //! careful with implicit type conversions!
    template <typename Type>
    binary_buffer & put(const Type item)
    {
        if (m_size + sizeof(Type) > m_capacity) dynalloc(m_size + sizeof(Type));

        *reinterpret_cast<Type*>(m_data + m_size) = item;
        m_size += sizeof(Type);

        return *this;
    }

    //! Append a varint to the buffer.
    binary_buffer & put_varint(uint32_t v)
    {
        if (v < 128) {
            put<uint8_t>(uint8_t(v));
        }
        else if (v < 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 0) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 7) & 0x7F));
        }
        else if (v < 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 0) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 7) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 14) & 0x7F));
        }
        else if (v < 128 * 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 0) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 7) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 21) & 0x7F));
        }
        else {
            put<uint8_t>((uint8_t)(((v >> 0) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 7) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 21) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 28) & 0x7F));
        }

        return *this;
    }

    //! Append a varint to the buffer.
    binary_buffer & put_varint(int v)
    {
        return put_varint((uint32_t)v);
    }

    //! Append a varint to the buffer.
    binary_buffer & put_varint(uint64_t v)
    {
        if (v < 128) {
            put<uint8_t>(uint8_t(v));
        }
        else if (v < 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 07) & 0x7F));
        }
        else if (v < 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 07) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 14) & 0x7F));
        }
        else if (v < 128 * 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 07) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 21) & 0x7F));
        }
        else if (v < ((uint64_t)128) * 128 * 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 07) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 21) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 28) & 0x7F));
        }
        else if (v < ((uint64_t)128) * 128 * 128 * 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 07) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 21) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 28) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 35) & 0x7F));
        }
        else if (v < ((uint64_t)128) * 128 * 128 * 128 * 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 07) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 21) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 28) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 35) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 42) & 0x7F));
        }
        else if (v < ((uint64_t)128) * 128 * 128 * 128 * 128 * 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 07) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 21) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 28) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 35) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 42) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 49) & 0x7F));
        }
        else if (v < ((uint64_t)128) * 128 * 128 * 128 * 128 * 128 * 128 * 128 * 128) {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 07) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 21) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 28) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 35) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 42) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 49) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 56) & 0x7F));
        }
        else {
            put<uint8_t>((uint8_t)(((v >> 00) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 07) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 14) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 21) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 28) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 35) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 42) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 49) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)(((v >> 56) & 0x7F) | 0x80));
            put<uint8_t>((uint8_t)((v >> 63) & 0x7F));
        }

        return *this;
    }

    //! Put a string by saving it's length followed by the data itself.
    binary_buffer & put_string(const char* data, size_t len)
    {
        return put_varint((uint32_t)len).append(data, len);
    }

    //! Put a string by saving it's length followed by the data itself.
    binary_buffer & put_string(const std::string& str)
    {
        return put_string(str.data(), str.size());
    }

    //! Put a binary_buffer by saving it's length followed by the data itself.
    binary_buffer & put_string(const binary_buffer& bb)
    {
        return put_string(bb.data(), bb.size());
    }
};

/*!
 * binary_buffer_ref represents a memory area as pointer and valid length. It
 * is not deallocated or otherwise managed. This class can be used to pass
 * around references to binary_buffer objects.
 */
class binary_buffer_ref
{
protected:
    //! Allocated buffer pointer.
    const char* m_data;

    //! Size of valid data.
    size_t m_size;

public:
    //! implicit conversion, assign memory area from binary_buffer.
    binary_buffer_ref(const binary_buffer& bb)  // NOLINT
        : m_data(bb.data()), m_size(bb.size())
    { }

    //! Constructor, assign memory area from pointer and length.
    binary_buffer_ref(const void* data, size_t n)
        : m_data(reinterpret_cast<const char*>(data)), m_size(n)
    { }

    //! Constructor, assign memory area from string, does NOT copy.
    explicit binary_buffer_ref(const std::string& str)
        : m_data(str.data()), m_size(str.size())
    { }

    //! Return a pointer to the currently kept memory area.
    const void * data() const
    { return m_data; }

    //! Return the currently valid length in bytes.
    size_t size() const
    { return m_size; }

    //! Explicit conversion to std::string (copies memory of course).
    std::string str() const
    { return std::string(reinterpret_cast<const char*>(m_data), m_size); }

    //! Compare contents of two binary_buffer_refs.
    bool operator == (const binary_buffer_ref& br) const
    {
        if (m_size != br.m_size) return false;
        return memcmp(m_data, br.m_data, m_size) == 0;
    }

    //! Compare contents of two binary_buffer_refs.
    bool operator != (const binary_buffer_ref& br) const
    {
        if (m_size != br.m_size) return true;
        return memcmp(m_data, br.m_data, m_size) != 0;
    }
};

/*!
 * binary_reader represents a binary_buffer_ref with an additional cursor with which
 * the memory can be read incrementally.
 */
class binary_reader : public binary_buffer_ref
{
protected:
    //! Current read cursor
    size_t m_curr;

public:
    //! Constructor, assign memory area from binary_buffer.
    explicit binary_reader(const binary_buffer_ref& br)
        : binary_buffer_ref(br), m_curr(0)
    { }

    //! Constructor, assign memory area from pointer and length.
    binary_reader(const void* data, size_t n)
        : binary_buffer_ref(data, n), m_curr(0)
    { }

    //! Constructor, assign memory area from string, does NOT copy.
    explicit binary_reader(const std::string& str)
        : binary_buffer_ref(str), m_curr(0)
    { }

    //! Return the current read cursor.
    size_t curr() const
    {
        return m_curr;
    }

    //! Reset the read cursor.
    binary_reader & rewind()
    {
        m_curr = 0;
        return *this;
    }

    //! Check that n bytes are available at the cursor.
    bool cursor_available(size_t n) const
    {
        return (m_curr + n <= m_size);
    }

    //! Throws a std::underflow_error unless n bytes are available at the
    //! cursor.
    void check_available(size_t n) const
    {
        if (!cursor_available(n))
            throw std::underflow_error("binary_reader underrun");
    }

    //! Return true if the cursor is at the end of the buffer.
    bool empty() const
    {
        return (m_curr == m_size);
    }

    //! Advance the cursor given number of bytes without reading them.
    binary_reader & skip(size_t n)
    {
        check_available(n);
        m_curr += n;

        return *this;
    }

    //! Fetch a number of unstructured bytes from the buffer, advancing the
    //! cursor.
    binary_reader & read(void* outdata, size_t datalen)
    {
        check_available(datalen);
        memcpy(outdata, m_data + m_curr, datalen);
        m_curr += datalen;

        return *this;
    }

    //! Fetch a number of unstructured bytes from the buffer as std::string,
    //! advancing the cursor.
    std::string read(size_t datalen)
    {
        check_available(datalen);
        std::string out(m_data + m_curr, datalen);
        m_curr += datalen;
        return out;
    }

    //! Fetch a single item of the template type Type from the buffer,
    //! advancing the cursor. Be careful with implicit type conversions!
    template <typename Type>
    Type get()
    {
        check_available(sizeof(Type));

        Type ret = *reinterpret_cast<const Type*>(m_data + m_curr);
        m_curr += sizeof(Type);

        return ret;
    }

    //! Fetch a varint with up to 32-bit from the buffer at the cursor.
    uint32_t get_varint()
    {
        uint32_t u, v = get<uint8_t>();
        if (!(v & 0x80)) return v;
        v &= 0x7F;
        u = get<uint8_t>(), v |= (u & 0x7F) << 7;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 14;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 21;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>();
        if (u & 0xF0)
            throw std::overflow_error("Overflow during varint decoding.");
        v |= (u & 0x7F) << 28;
        return v;
    }

    //! Fetch a 64-bit varint from the buffer at the cursor.
    uint64_t get_varint64()
    {
        uint64_t u, v = get<uint8_t>();
        if (!(v & 0x80)) return v;
        v &= 0x7F;
        u = get<uint8_t>(), v |= (u & 0x7F) << 7;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 14;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 21;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 28;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 35;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 42;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 49;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>(), v |= (u & 0x7F) << 56;
        if (!(u & 0x80)) return v;
        u = get<uint8_t>();
        if (u & 0xFE)
            throw std::overflow_error("Overflow during varint64 decoding.");
        v |= (u & 0x7F) << 63;
        return v;
    }

    //! Fetch a string which was put via put_string().
    std::string get_string()
    {
        uint32_t len = get_varint();
        return read(len);
    }

    //! Fetch a binary_buffer_ref to a binary string or blob which was put via
    //! put_string(). Does NOT copy the data.
    binary_buffer_ref get_binary_buffer_ref()
    {
        uint32_t len = get_varint();
        // save object
        binary_buffer_ref br(m_data + m_curr, len);
        // skip over sub block data
        skip(len);
        return br;
    }
};

//! \}

} // namespace stxxl

#endif // !STXXL_COMMON_BINARY_BUFFER_HEADER
