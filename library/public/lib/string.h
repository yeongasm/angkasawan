#pragma once
#ifndef LIB_STRING_H
#define LIB_STRING_H

#include <string_view>
#include <fmt/format.h>
#include "array.h"

namespace lib
{

template <is_char_type char_type>
struct null_terminator {};

template <>
struct null_terminator<char>
{
    inline static constexpr char val = '\0';
};

template <>
struct null_terminator<wchar_t>
{
    inline static constexpr wchar_t val = L'\0';
};

template <is_char_type T>
size_t string_length(const T* str)
{
    const T* ch = str;
    constexpr T nullTerminator = null_terminator<T>::val;
    size_t length = 0;
    for (; *ch != nullTerminator; ++ch, ++length);
    return length;
}

template <typename T>
inline static constexpr auto null_terminator_v = null_terminator<T>::val;

// TODO(Afiq):
// [x] 1. Remove C style string formatting.
// [x] 2. Add { fmt } as a thirdparty dependency.
// [x] 3. Encapsulate format function to return our own string type instead of std::string.

template <is_char_type char_type, provides_memory allocator = default_allocator>
struct fmt_allocator_interface : std::conditional_t<std::is_same_v<allocator, default_allocator>, system_memory_interface, allocator_memory_interface<allocator>>
{
    using super                 = std::conditional_t<std::is_same_v<allocator, default_allocator>, system_memory_interface, allocator_memory_interface<allocator>>;
    using allocator_interface   = super;
    using value_type            = char_type;
    using size_type             = size_t;
    using buffer                = fmt::basic_memory_buffer<value_type, fmt::inline_buffer_size, fmt_allocator_interface>;

    template<class U> 
    struct rebind 
    { 
        using other = fmt_allocator_interface<U, allocator>;
    };

    fmt_allocator_interface() requires std::same_as<allocator, default_allocator> {}
    fmt_allocator_interface(allocator_interface& allocator) requires !std::same_as<allocator, default_allocator> :
        super{ &allocator }
    {}

    value_type* allocate(size_type size)
    {
        return this->allocate_storage<value_type>({ .size = size });
    }

    void deallocate(value_type* pointer, [[maybe_unused]] size_t size)
    {
        super::free_storage(pointer);
    }
};


/**
* Small String Optimized BasicString class.
*
* Fast, lightweight and semi-fully featured.
* Any string that is less than 24 bytes (including the null terminator) would be placed on the stack instead of the heap.
* A decision was made to not revert the string class to a small string optimised version if the string is somehow altered to be shorter than before and less than 24 bytes.
* Reason being the cost of allocating memory on the heap outweighs the pros of putting the string on the stack.
*/
template <is_char_type char_type, provides_memory provided_allocator = default_allocator, std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<5>>
class basic_string final : 
    protected std::conditional_t<std::is_same_v<provided_allocator, default_allocator>, system_memory_interface, allocator_memory_interface<provided_allocator>>
{
public:

    using value_type        = char_type;
    using size_type         = uint32;
    using difference_type   = std::ptrdiff_t;
    using reference         = value_type&;
    using const_reference   = value_type const&;
    using pointer           = value_type*;
    using const_pointer     = value_type const*;
    using iterator          = array_iterator<basic_string>;
    using const_iterator    = array_const_iterator<basic_string>;

private:

    using allocator_interface   = std::conditional_t<std::is_same_v<provided_allocator, default_allocator>, system_memory_interface, allocator_memory_interface<provided_allocator>>;
    using allocator_type        = typename allocator_interface::allocator_type;
    using fmt_allocator         = fmt_allocator_interface<value_type, allocator_type>;
    using fmt_buffer            = fmt::basic_memory_buffer<value_type, fmt::inline_buffer_size, fmt_allocator>;

    static constexpr size_type msb_bit_flag = 0x80000000;
    static constexpr size_type SSO_MAX = 24 / sizeof(value_type) < 1 ? 1 : 24 / sizeof(value_type);

protected:

    union
    {
        value_type* m_data;
        value_type  m_buffer[SSO_MAX];
    };
    size_type m_len;
    size_type m_capacity;

    /**
    * Takes in a string literal as an argument and stores that literal into the buffer.
    */
    constexpr size_type write_fv(const_pointer str, size_t length)
    {
        if (length >= capacity())
        {
            alloc(growth_policy::new_capacity(length));
        }

        size_type len = static_cast<size_type>(length);
        m_len = 0;
        pointer ptr = pointer_to_string();

        while (m_len != len)
        {
            ptr[m_len] = str[m_len];
            ++m_len;
        }

        ptr[m_len] = null_terminator<value_type>::val;

        return m_len;
    }

    /**
    * Calls { fmt } to format string (non-allocator referenced version).
    */
    constexpr size_type write_fv(fmt::basic_string_view <char_type>str, fmt::format_args args) requires std::same_as<allocator_type, default_allocator>
    {
        //SystemAllocator systemAllocator;
        fmt_allocator fmtAlloc;
        fmt_buffer buffer{ fmtAlloc };

        fmt::vformat_to(std::back_inserter(buffer), str, args);

        return write_fv(buffer.data(), static_cast<size_type>(buffer.size()));
    }

    /**
    * Calls { fmt } to format string (allocator referenced version).
    */
    constexpr size_type write_fv(allocator_type& allocator, fmt::basic_string_view <char_type>str, fmt::format_args args) requires !std::same_as<allocator_type, default_allocator>
    {
        fmt_allocator fmtAlloc{ &allocator };
        fmt_buffer buffer{ fmtAlloc };

        fmt::vformat_to(std::back_inserter(buffer), str, args);

        return write_fv(buffer.data(), static_cast<size_type>(buffer.size()));
    }

    constexpr size_type append_internal(const_pointer str, size_t length)
    {
        if (m_len + length >= capacity())
        {
            alloc(growth_policy::new_capacity(m_len + length));
        }
        size_type len = static_cast<size_type>(length);
        size_type count = 0;
        pointer ptr = pointer_to_string();
        while (count != len)
        {
            ptr[m_len] = str[count];
            ++m_len;
            ++count;
        }
        ptr[m_len] = null_terminator<value_type>::val;
        return m_len;
    }

    constexpr size_type append_internal(fmt::basic_string_view<char_type> str, fmt::format_args args) requires std::same_as<allocator_type, default_allocator>
    {
        fmt_allocator fmtAlloc;
        fmt_buffer buffer{ fmtAlloc };

        fmt::vformat_to(std::back_inserter(buffer), str, args);

        return append_internal(buffer.data(), static_cast<size_type>(buffer.size()));
    }

    constexpr size_type append_internal(allocator_type& allocator, fmt::basic_string_view<char_type> str, fmt::format_args args) requires !std::same_as<allocator_type, default_allocator>
    {
        fmt_allocator fmtAlloc{ &allocator };
        fmt_buffer buffer{ fmtAlloc };

        fmt::vformat_to(std::back_inserter(buffer), str, args);

        return append_internal(buffer.data(), static_cast<size_type>(buffer.size()));
    }

private:

    constexpr pointer pointer_to_string()
    {
        return is_small_string() ? m_buffer : m_data;
    }

    /**
    * Returns a pointer to the current buffer that is in use for the string.
    */
    constexpr const_pointer pointer_to_string() const
    {
        return is_small_string() ? m_buffer : m_data;
    }

    /**
    * Stores the character into the buffer / pointer with perfect forwarding.
    */
    //template <typename... ForwardType>
    //constexpr void store_in_buffer(value_type* pointer, ForwardType&&... ch)
    //{
    //    new (pointer) value_type{ std::forward<ForwardType>(ch)... };
    //}

    constexpr void alloc(size_t length)
    {
        bool const isPreviouslySmallString = is_small_string();
        // Unset the msb in m_capacity to signify that it's no longer a SSO string.
        pointer ptr = this->allocate_storage<value_type>({ .size = length });

        m_capacity = static_cast<size_type>(length);

        if (!isPreviouslySmallString)
        {
            memmove(ptr, m_data, m_len * sizeof(value_type));
            this->free_storage(m_data);
        }
        else
        {
            memcopy(ptr, m_buffer, m_len * sizeof(value_type));
        }

        ASSERTION(ptr != nullptr && "Unable to allocate memory!");

        m_data = ptr;
    }

    /**
    * compares to see if the strings are the same.
    * Returns true if they are the same.
    */
    constexpr bool compare(basic_string const& rhs) const
    {
        // If the object is being compared with itself, then it definitely will always be the same.
        if (this != &rhs)
        {
            // If the length of both strings are not the same, then they definitely will not be the same.
            if (m_len != rhs.m_len)
            {
                return false;
            }

            value_type const* ptr = pointer_to_string();
            value_type const* rhsPointer = rhs.pointer_to_string();

            // If the length are the same, we have to check if the contents of the string matches one another.
            for (size_type i = 0; i < m_len; i++)
            {
                if (ptr[i] != rhsPointer[i])
                {
                    return false;
                }
            }
        }
        return true;
    }


    constexpr bool compare(const_pointer str) const
    {
        const_pointer ptr = pointer_to_string();

        if (ptr != str)
        {
            size_type length = static_cast<size_type>(string_length(str));

            if (m_len != length)
            {
                return false;
            }

            for (size_type i = 0; i < m_len; i++)
            {
                if (ptr[i] != str[i])
                {
                    return false;
                }
            }
        }
        return true;
    }

    constexpr allocator_type* get_allocator() const requires !std::same_as<allocator_type, default_allocator>
    {
        return this->get_allocator();
    }

public:

    constexpr basic_string() requires std::same_as<allocator_type, default_allocator> :
        m_buffer{ null_terminator<value_type>::val }, m_capacity{ msb_bit_flag | SSO_MAX }, m_len{ 0 }
    {}

    constexpr basic_string(allocator_type& allocator) requires !std::same_as<allocator_type, default_allocator> :
        m_buffer{ null_terminator<value_type>::val }, m_capacity{ msb_bit_flag | SSO_MAX }, m_len{ 0 }, allocator_interface{ &allocator }
    {}

    constexpr ~basic_string()
    {
        release();
    }

    constexpr basic_string(size_t size) requires std::same_as<allocator_type, default_allocator> :
        basic_string{}
    {
        alloc(size);
    }

    constexpr basic_string(allocator_type& allocator, size_t size) requires !std::same_as<allocator_type, default_allocator> :
        basic_string{ &allocator }
    {
        alloc(size);
    }

    constexpr basic_string(const_pointer str) requires std::same_as<allocator_type, default_allocator> :
        basic_string{}
    {
        write_fv(str, string_length(str));
    }

    template <typename... Args>
    constexpr basic_string(std::basic_string_view<char_type> str, Args&&... args) requires std::same_as<allocator_type, default_allocator> :
        basic_string{}
    {
        write_fv(fmt::basic_string_view<char_type>{ str.data(), str.size() }, fmt::make_format_args(args...));
    }

    template <typename... Args>
    constexpr basic_string(allocator_type& allocator, const_pointer str, Args&&... args) requires !std::same_as<allocator_type, default_allocator> :
        basic_string{ allocator }
    {
        write_fv(this->get_allocator(), fmt::basic_string_view<char_type>{ str }, fmt::make_format_args(args...));
    }

    constexpr basic_string(allocator_type& allocator, const_pointer str) requires !std::same_as<allocator_type, default_allocator> :
        basic_string{ &allocator }
    {
        write_fv(str, string_length(str));
    }

    constexpr basic_string(const_pointer str, size_type length) requires std::same_as<allocator_type, default_allocator> :
        basic_string{}
    {
        write_fv(str, length);
    }

    /**
    * Assigns the string literal into the string object.
    */
    constexpr basic_string& operator= (const_pointer str)
    {
        write_fv(str, string_length(str));
        return *this;
    }

    constexpr basic_string& operator=(std::basic_string_view<value_type> str)
    {
        write_fv(str.data(), str.size());
        return *this;
    }

    /**
    * Copy constructor.
    * Performs a deep copy.
    */
    constexpr basic_string(basic_string const& rhs) requires std::same_as<allocator_type, default_allocator> :
        basic_string{}
    { 
        *this = rhs; 
    }


    constexpr basic_string(basic_string const& rhs) requires !std::same_as<allocator_type, default_allocator> :
        basic_string{ *rhs.allocator() }
    {
        *this = rhs;
    }

    /**
    * Copy assignment operator.
    * Performs a deep copy.
    */
    constexpr basic_string& operator= (basic_string const& rhs)
    {
        if (this != &rhs)
        {
            pointer ptr = m_buffer;
            const_pointer rhsPointer = rhs.pointer_to_string();

            // Perform a deep copy if the copied string is not a small string.
            if (!rhs.is_small_string())
            {
                alloc(rhs.m_capacity);
                ptr = m_data;
            }

            m_len = rhs.m_len;
            m_capacity = rhs.m_capacity;

            for (size_type i = 0; i < m_len; i++)
            {
                ptr[i] = rhsPointer[i];
            }

            ptr[m_len] = null_terminator<value_type>::val;
        }
        return *this;
    }

    constexpr basic_string(basic_string&& rhs) :
        basic_string{}
    {
        *this = std::move(rhs);
    }

    constexpr basic_string& operator= (basic_string&& rhs)
    {
        if (this != &rhs)
        {
            pointer rhsPointer = rhs.pointer_to_string();

            if (!rhs.is_small_string())
            {
                bool move = true;
                release();

                if constexpr (!std::is_same_v<allocator_type, default_allocator>)
                {
                    if (get_allocator() != rhs.get_allocator())
                    {
                        reserve(rhs.size());
                        const size_type rhsLength = rhs.length();
                        for (size_type i = 0; i < rhsLength; ++i)
                        {
                            push(rhsPointer[i]);
                        }
                        rhs.release();
                        move = false;
                    }
                }

                if (move)
                {
                    m_data = rhsPointer;
                    m_capacity = rhs.m_capacity;
                    m_len = rhs.m_len;
                }
            }
            else
            {
                m_len = rhs.m_len;
                m_capacity = rhs.m_capacity;

                for (size_type i = 0; i < m_len; i++)
                {
                    m_buffer[i] = rhsPointer[i];
                }
                m_buffer[m_len] = null_terminator<value_type>::val;
            }

            if constexpr (!std::is_same_v<allocator_type, default_allocator>)
            {
                new (&rhs) basic_string{ *rhs.get_allocator() };
            }
            else
            {
                new (&rhs) basic_string{};
            }
        }
        return *this;
    }


    constexpr bool operator== (basic_string const& rhs) const
    {
        return compare(rhs);
    }


    constexpr bool operator== (const_pointer str) const
    {
        return compare(str);
    }

    /**
    * Square brace overloaded operator.
    * Retrieves an element from the specified index.
    */
    constexpr reference operator[] (size_type index)
    {
        //ASSERTION(index < size() && "The index specified exceeded the capacity of the string!");

        // Necessary for raw pointer accessed string.
        if (m_len < index)
        {
            m_len = (uint32)index + 1;
        }
        pointer ptr = pointer_to_string();

        return ptr[index];
    }


    /**
    * Square brace overloaded operator.
    * Retrieves an element from the specified index.
    */
    constexpr const_reference operator[] (size_type index) const
    {
        ASSERTION(index < length() && "Index specified exceeded the length of the string!");
        pointer ptr = pointer_to_string();
        return ptr[index];
    }

    constexpr explicit operator std::basic_string_view<char_type>() const
    {
        return std::basic_string_view<char_type>{ data(), size() };
    }

    /**
    * Clears all data from the string and releases it from memory.
    */
    constexpr void release()
    {
        if (!is_small_string())
        {
            if (m_data)
            {
                this->free_storage(m_data);
            }
        }
        m_data = nullptr;
        m_capacity = m_len = 0;
    }

    /**
    * Reserves [length] sized memory in the string container.
    */
    constexpr void reserve(size_type length)
    {
        if (length > SSO_MAX)
        {
            alloc(length);
        }
    }

    //constexpr void resize(size_type length)
    //{
    //    ASSERTION(length < m_capacity && "resized length cannot exceed the string's maximum capacity!");
    //    if (length < m_capacity)
    //    {
    //        m_len = length;
    //    }
    //}

    /**
    * Checks if the current string is or is not a small string.
    */
    constexpr bool is_small_string() const
    {
        //return m_capacity <= SSO_MAX;

        // If the MSB is still set to 1, it is definitely a small string.
        // This approach is considered "safe" because m_capacity with it's MSB set to 1 holds the value 2147483648 (approx 2GiB).
        // We will almost certainly never parse a string that's 2GiB in size.

        return m_capacity & msb_bit_flag;
    }

    /**
    * Assign a formatted string into the string object.
    * Uses { fmt } internally.
    */
    template <typename... Args>
    constexpr size_type format(std::basic_string_view<char_type> str, Args&&... args)
    {
        if constexpr (std::is_same_v<allocator_type, default_allocator>)
        {
            write_fv(fmt::basic_string_view<char_type>{ str.data(), str.size() }, fmt::make_format_args(args...));
        }
        else
        {
            write_fv(this->get_allocator(), fmt::basic_string_view<char_type>{ str.data(), str.size() }, fmt::make_format_args(args...));
        }

        return m_len;
    }

    static constexpr size_type sso_size()
    {
        return SSO_MAX;
    }

    /**
    * Assign a string literal into the string object.
    * It is the same as using the assignment operator to assign the string.
    */
    //constexpr size_type write(const_pointer str) { return write_fv(str, string_length(str)); }


    /**
    * Shrinks the total size capacity of the string to tightly fit the length of string +1 for the null terminator.
    */
    constexpr void shrink_to_fit()
    {
        if (!is_small_string())
        {
            alloc(m_len + 1);
        }
    }

    /**
    * Returns a new string with the character from [ start ] index and [ count] characters after that.
    */
    constexpr basic_string substr(size_type start, size_type count) const
    {
        ASSERTION(start + count < size());
        const_pointer ptr = pointer_to_string();
        return basic_string{ &ptr[start], count };
    }

    /**
    * Checks if a string is empty.
    */
    constexpr bool empty()
    {
        return m_len == 0;
    }

    /**
    * Clears data from the string container. Memory allocated still in-tact.
    */
    constexpr void clear()
    {
        pointer ptr = pointer_to_string();
        memzero(ptr, capacity());
        m_len = 0;
    }

    /**
    * Pushes a character to the back of the string.
    */
    constexpr size_type push(const_reference ch)
    {
        if (m_len == capacity() || !capacity())
        {
            alloc(growth_policy::new_capacity(capacity()));
        }
        pointer ptr = pointer_to_string();
        ptr[m_len] = ch;
        ptr[++m_len] = null_terminator<value_type>::val;

        return m_len;
    }

    /**
    * Pushes a character to the back of the string.
    */
    constexpr size_type push(value_type&& ch)
    {
        if (m_len == size())
        {
            alloc(growth_policy::new_capacity(size()));
        }
        pointer ptr = pointer_to_string();
        ptr += m_len;
        ptr[m_len] = std::move(ch);
        ptr[++m_len] = null_terminator<value_type>::val;

        return m_len++;
    }

    constexpr basic_string& append(std::basic_string_view<value_type> const& str)
    {
        append_internal(str.data(), str.size());
        return *this;
    }

    template <typename... Args>
    constexpr basic_string& append(std::basic_string_view<char_type> str, Args&&... args)
    {
        if constexpr (std::is_same_v<allocator_type, default_allocator>)
        {
            append_internal(fmt::basic_string_view<char_type>{ str.data(), str.size() }, fmt::make_format_args(args...));
        }
        else
        {
            append_internal(this->get_allocator(), fmt::basic_string_view<char_type>{ str.data(), str.size() }, fmt::make_format_args(args...));
        }
        return *this;
    }

    template <std::contiguous_iterator It>
    constexpr basic_string& append(It begin, It end)
    {
        if constexpr (std::is_pointer_v<It>)
        {
            using removed_pointer_type = std::remove_pointer_t<It>;
            using decayed_type = std::remove_reference_t<std::remove_cv_t<removed_pointer_type>>;

            static_assert(std::is_same_v<value_type, decayed_type>, "types provided do not match!");
            append_internal(begin, std::distance(begin, end));
        }
        else
        {
            using it_value_type = It::value_type;
            static_assert(std::is_same_v<value_type, it_value_type>, "types provided do not match!");
            append_internal(&(*begin), std::distance(begin, end));
        }
        return *this;
    }

    constexpr basic_string& append(basic_string const& other)
    {
        append_internal(other.data(), other.size());
        return *this;
    }

    constexpr pointer       data    ()  { return pointer_to_string(); }
    constexpr const_pointer data    ()  const { return pointer_to_string(); }
    constexpr const_pointer c_str   ()	const { return pointer_to_string(); }
    constexpr size_t        length  ()  const { return static_cast<size_t>(m_len); }
    constexpr size_t        size    ()  const { return static_cast<size_t>(m_len); }
    constexpr size_t        capacity()  const { return static_cast<size_t>(m_capacity & ~msb_bit_flag); }

    iterator		begin   ()          { return iterator(pointer_to_string(), *this); }
    iterator        end     ()          { return iterator(pointer_to_string() + m_len, *this); }
    const_iterator  begin   ()  const   { return const_iterator(const_cast<pointer>(pointer_to_string()), *this); }
    const_iterator  cbegin  ()  const   { return begin(); }
    const_iterator  end     ()	const   { return const_iterator(const_cast<pointer>(pointer_to_string()) + m_len, *this); }
    const_iterator  cend    ()	const   { return end(); }

};


/**
* BasicHashString data structure.
*
* Derived from BasicString.
* Strings contained within this class will be hashed on every single update.
*
* HashAlgorithm needs to be a functor object that accepts a const reference String or WString (depending on what was specified in the template argument) as an argument.
* The functor would then need to return the hash.
*/
//template <typename StringType, typename HashAlgorithm = MurmurHash<BasicString<StringType>>, size_t Slack = 2>
//class BasicHashString : public BasicString<StringType, Slack>
//{
//private:

//    using Type = StringType;
//    using Super = BasicString<Type, Slack>;

//    void UpdateHash()
//    {
//        HashAlgorithm Algorithm;
//        Hash = static_cast<uint32>(Algorithm(*this));
//    }

//public:

//    size_t Push(const Type& ch) = delete;
//    size_t Push(Type&& ch) = delete;

//    /**
//    * Default constructor.
//    */
//    BasicHashString() :
//        Super(), Hash(0) {}


//    /**
//    * Default destructor.
//    */
//    ~BasicHashString()
//    {
//        release();
//    }


//    /**
//    * Constructor to take in a string literal.
//    */
//    BasicHashString(const char* str) : Hash(0)
//    {
//        *this = str;
//    }


//    /**
//    * Assigns the string literal into the string object and hashes it.
//    */
//    BasicHashString& operator= (const char* str)
//    {
//        Super::operator=(str);
//        UpdateHash();

//        return *this;
//    }


//    /**
//    * Constructor to take in a String object.
//    */
//    BasicHashString(const Super& Source) : Super(), Hash(0)
//    {
//        *this = Source;
//    }


//    /**
//    * Overloaded assignment operator that assigns a String object into BasicHashString and updates it's Hash.
//    */
//    BasicHashString& operator= (const Super& Source)
//    {
//        Super::operator=(Source);
//        UpdateHash();

//        return *this;
//    }


//    /**
//    * Default copy constructor.
//    */
//    BasicHashString(const BasicHashString& rhs)
//    {
//        *this = rhs;
//    }


//    /**
//    * Default copy assignment operator.
//    */
//    BasicHashString& operator= (const BasicHashString& rhs)
//    {
//        ASSERTION(this != &rhs);

//        Hash = rhs.Hash;
//        Super::operator=(rhs);

//        return *this;
//    }


//    /**
//    * Default move constructor.
//    */
//    BasicHashString(BasicHashString&& rhs)
//    {
//        *this = std::move(rhs);
//    }


//    /**
//    * Default move assignment operator.
//    */
//    BasicHashString& operator= (BasicHashString&& rhs)
//    {
//        ASSERTION(this != &rhs);

//        Hash = rhs.Hash;
//        BasicString<Type, Slack>::operator=(std::move(rhs));
//        rhs.Hash = 0;

//        return *this;
//    }


//    /**
//    * Overloaded equality operator.
//    */
//    bool operator== (const BasicHashString& rhs) const
//    {
//        if (Hash != rhs.Hash)
//        {
//            return Super::operator==(rhs);
//        }

//        return true;
//    }


//    /**
//    * Overloaded inequality operator.
//    */
//    bool operator!= (const BasicHashString& rhs) const
//    {
//        return !(*this == rhs);
//    }


//    bool operator< (const BasicHashString& rhs) const
//    {
//        return Hash < rhs.Hash;
//    }


//    bool operator< (const Super& rhs) const
//    {
//        BasicHashString<Type> Temp = rhs;
//        return Hash < Temp.Hash;
//    }


//    /**
//    * Clears all data and the hash from the string and releases it from memory.
//    */
//    void release()
//    {
//        Hash = 0;
//        Super::release();
//    }


//    /**
//    * Clears all data and the hash from the string but does not release it from memory.
//    */
//    void Empty()
//    {
//        Hash = 0;
//        Super::Empty();
//    }


//    /**
//    * Assign a formatted string into the string object and updates it's hash.
//    */
//    size_t format(const char* format, ...)
//    {
//        va_list args;
//        va_start(args, format);
//        size_t length = Super::write_fv(format, args);
//        va_end(args);

//        UpdateHash();

//        return length;
//    }

//    /**
//    * Assign a string literal into the string object and updates it's hash.
//    * It is the same as using the assignment operator to assign a string.
//    */
//    size_t write(const char* str)
//    {
//        Super::write(str);
//        UpdateHash();

//        return Super::m_len;
//    }

//private:
//    uint32 m_hash;
//};

template <is_char_type char_type, size_t Capacity>
class base_static_string
{
public:
    using value_type        = char_type;
    using size_type         = size_t;
    using difference_type   = std::ptrdiff_t;
    using reference         = value_type&;
    using const_reference   = value_type const&;
    using pointer           = value_type*;
    using const_pointer     = value_type const*;
    using iterator          = array_iterator<base_static_string>;
    using const_iterator    = array_const_iterator<base_static_string>;

    constexpr base_static_string() :
        m_buf{}, m_len(0)
    {}

    constexpr ~base_static_string() { flush(); }

    constexpr base_static_string(base_static_string const& rhs) :
        base_static_string{}
    {
        *this = rhs;
    }

    constexpr base_static_string(base_static_string&& rhs) :
        base_static_string{}
    {
        *this = std::move(rhs);
    }

    constexpr base_static_string& operator=(base_static_string const& rhs)
    {
        if (this != &rhs)
        {
            memcopy(m_buf, rhs.m_buf, rhs.m_len * sizeof(value_type));
            m_len = rhs.m_len;
        }
        return *this;
    }

    constexpr base_static_string& operator=(base_static_string&& rhs)
    {
        if (this != &rhs)
        {
            memmove(m_buf, rhs.m_buf, rhs.m_len * sizeof(value_type));
            m_len = rhs.m_len;
            new (&rhs) base_static_string{};
        }
        return *this;
    }

    constexpr base_static_string(const_pointer text) :
        base_static_string{}
    {
        write(text);
    }

    constexpr base_static_string(std::basic_string_view<value_type> view) :
        base_static_string{}
    {
        write(view);
    }

    constexpr bool operator==(base_static_string const& rhs) const { return compare(rhs); }
    constexpr bool operator==(const_pointer str) const { return compare(str); }

    constexpr base_static_string& operator=(const_pointer str)
    {
        write(str);
        return *this;
    }

    constexpr void flush()
    {
        memzero(m_buf, m_len);
        m_len = 0;
    }

    constexpr void write(const_pointer str)
    {
        size_type len = string_length(str);
        ASSERTION((len + 1) <= Capacity);
        flush();
        while (m_len < len)
        {
            m_buf[m_len] = str[m_len];
            m_len++;
        }
        m_buf[m_len] = null_terminator<value_type>::val;
    }

    constexpr void write(const_pointer str, size_type len)
    {
        ASSERTION((len + 1) <= Capacity);
        flush();
        while (m_len < len)
        {
            m_buf[m_len] = str[m_len];
            m_len++;
        }
        m_buf[m_len] = null_terminator<value_type>::val;
    }

    constexpr void write(std::basic_string_view<value_type> str)
    {
        write(str.data(), str.length());
    }

    constexpr size_type		diff(base_static_string const& rhs)	    const { return static_cast<size_type>(std::strcmp(m_buf, rhs)); }
    constexpr size_type		diff(const_pointer str)			        const { return static_cast<size_type>(std::strcmp(m_buf, str)); }
    constexpr bool		    compare(base_static_string const& rhs)	const { return std::strcmp(m_buf, rhs.m_buf) == 0; }
    constexpr bool		    compare(const_pointer str)			    const { return std::strcmp(m_buf, str) == 0; }

    constexpr bool		    empty()  const { return !m_len; }
    constexpr size_type		size()  const { return m_len; }
    constexpr size_type     capacity()  const { return Capacity; }
    constexpr const_pointer c_str()  const { return m_buf; }

    template <typename... ForwardType>
    constexpr void add_ch(ForwardType&&... ch)
    {
        ASSERTION(m_len < Capacity);
        m_buf[m_len++] = value_type{ std::forward<ForwardType>(ch)... };
        m_buf[m_len] = null_terminator<value_type>::val;
    }

    iterator	    begin() { return iterator(m_buf, this); }
    iterator	    end() { return iterator(m_buf + m_len, this); }
    const_iterator  begin()  const { return const_iterator(m_buf, this); }
    const_iterator  end()  const { return const_iterator(m_buf + m_len, this); }

private:
    value_type	m_buf[Capacity];
    size_type	m_len;
};

template <is_char_type char_type, std::integral T = size_t>
class basic_hash_string_view : public std::basic_string_view<char_type, std::char_traits<char_type>>
{
public:
    using super = std::basic_string_view<char_type, std::char_traits<char_type>>;
    using size_type = typename super::size_type;
    using hash_value_type = T;
    using const_pointer = typename super::const_pointer;

    constexpr basic_hash_string_view() : super{}, m_hash{} {};
    constexpr basic_hash_string_view(basic_hash_string_view const& rhs) = default;
    constexpr basic_hash_string_view(const_pointer str) :
        super{ str }
    {
        m_hash = static_cast<T>((std::hash<super>{})(*this));
    }
    constexpr basic_hash_string_view(const_pointer str, size_type count) :
        super{ str, count }
    {
        m_hash = static_cast<T>((std::hash<super>{})(*this));
    };
    template <typename start_iterator, typename end_iterator>
    constexpr basic_hash_string_view(start_iterator first, end_iterator last) :
        super{ first, last }
    {
        m_hash = static_cast<T>((std::hash<super>{})(*this));
    }

    template <typename StringView_t = super>
    constexpr basic_hash_string_view(StringView_t&& strView) :
        super{ std::forward<StringView_t>(strView) }
    {
        m_hash = static_cast<T>((std::hash<super>{})(*this));
    }

    constexpr basic_hash_string_view& operator=(basic_hash_string_view const& rhs) = default;

    hash_value_type hash() const { return m_hash; }
    super           to_string_view() const { return *this; }

    // We no longer need to define operator!= & overloads for the other way round in C++20
    // https://brevzin.github.io/c++/2019/07/28/comparisons-cpp20/
    constexpr bool operator==(basic_hash_string_view other) noexcept
    {
        return m_hash == other.m_hash;
    }
    constexpr bool operator==(super other) noexcept
    {
        basic_hash_string_view temp{ other };
        return m_hash == temp.m_hash;
    }
private:
    hash_value_type m_hash;
};

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;

template <size_t Capacity>
using static_string = base_static_string<char, Capacity>;

template <size_t Capacity>
using static_wstring = base_static_string<wchar_t, Capacity>;

using string_16 = static_string<16>;
using string_32 = static_string<32>;
using string_64 = static_string<64>;
using string_128 = static_string<128>;
using string_256 = static_string<256>;

using wstring_16 = static_wstring<16>;
using wstring_32 = static_wstring<32>;
using wstring_64 = static_wstring<64>;
using wstring_128 = static_wstring<128>;
using wstring_256 = static_wstring<256>;

using hash_string_view = basic_hash_string_view<char>;
using hash_wstring_view = basic_hash_string_view<wchar_t>;

// Uses { fmt } internally to format string.
template <is_char_type char_type, typename... Args>
constexpr basic_string<char_type> format(char_type const* str, Args&&... arguments)
{
    using fmt_allocator = fmt_allocator_interface<char_type, default_allocator>;
    using string_type   = basic_string<char_type>;

    fmt_allocator fmtAlloc;
    fmt::basic_memory_buffer<char_type, fmt::inline_buffer_size, fmt_allocator> buffer{ fmtAlloc };

    fmt::vformat_to(std::back_inserter(buffer), fmt::basic_string_view<char_type>{ str }, fmt::make_format_args(arguments...));

    return string_type{ buffer.data(), static_cast<typename string_type::size_type>(buffer.size()) };
}

// Uses { fmt } internally to format string.
//template <is_char_type char_type, typename provided_allocator, typename... Args>
//constexpr basic_string<char_type> format(provided_allocator& allocator, const char_type* str, Args&&... arguments)
//{
//    using fmt_allocator = fmt_allocator_interface<char_type, provided_allocator>;
//    using fmt_buffer = typename fmt_allocator::buffer;
//
//    fmt_allocator fmtAlloc{ &allocator };
//    fmt::basic_memory_buffer<char_type, fmt::inline_buffer_size, fmt_allocator> buffer{ fmtAlloc };
//
//    fmt::vformat_to(std::back_inserter(buffer), fmt::basic_string_view<char_type>{ str }, fmt::make_format_args(arguments...));
//
//    return basic_string<char_type>{ buffer.data(), buffer.size() };
//}

/**
* TODO(afiq):
* 
* - Make this more robust by returning more information once a string has been appended into the pool i.e, which pool it's stored in and it's location in the pool.
* - Introduce the ability to clear pools.
* - Coalesce memory regions of released string data into a larger blocks to reduce fragmentation.
*/
template <is_char_type char_type, size_t POOL_SIZE = 32_KiB>
class basic_string_pool
{
protected:
    using string_type       = basic_string<char_type>;
    using string_view_type  = std::basic_string_view<char_type>;

    std::list<string_type>  m_pools = {};
    string_type             m_formatBuffer;
    string_type*            m_current = nullptr;

    void new_pool()
    {
        string_type _temp;
        _temp.reserve(POOL_SIZE);
        m_pools.push_back(std::move(_temp));
        m_current = &m_pools.back();
    }

    void try_allocate_new_pool(string_view_type str)
    {
        if (!m_pools.size())
        {
            new_pool();
        }
        else if (m_current && (str.size() + (*m_current).size() > (*m_current).capacity()))
        {
            new_pool();
        }
    }
public:
    string_view_type append(string_view_type str)
    {
        try_allocate_new_pool(str);

        string_type& current = *m_current;
        auto it = current.end();
        current.append(str);

        return string_view_type{ it.data(), str.size() };
    }

    template <typename... Args>
    string_view_type append(string_view_type str, Args&&... args)
    {
        m_formatBuffer.clear();
        m_formatBuffer.format(str, std::forward<Args>(args)...);

        try_allocate_new_pool((string_view_type)m_formatBuffer);

        string_type& current = *m_current;
        auto it = current.end();

        current.append(m_formatBuffer);

        return string_view_type{ it.data(), m_formatBuffer.size()};
    }

    template <typename T>
    string_view_type append(T const& value)
    {
        return append("{}", value);
    }
};

using string_pool   = basic_string_pool<char, 16_KiB>;
using wstring_pool  = basic_string_pool<wchar_t, 32_KiB>;

}

template<>
struct std::hash<lib::string>
{
    size_t operator()(const lib::string& str) const noexcept
    {
        using value_type = typename lib::string::value_type;
        std::hash<value_type> hasher;
        size_t result = 0;
        for (value_type const& c : str)
        {
            result += hasher(c);
        }
        return result;
    }
};

template <>
struct std::hash<lib::hash_string_view>
{
    size_t operator()(lib::hash_string_view const& strView) const noexcept
    {
        return (std::hash<std::string_view>{})(strView.to_string_view());
    }
};

#endif // !LIB_STRING_H
