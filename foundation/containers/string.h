#pragma once
#ifndef FOUNDATION_STRING_H
#define FOUNDATION_STRING_H

#include "array.h"
#include <fmt/format.h>
#include <string_view>

FTLBEGIN

template <typename T>
concept IsCharType = std::same_as<T, char> || std::same_as<T, wchar_t>;

template <IsCharType T>
consteval T null_terminator()
{
    if constexpr (sizeof(T) == sizeof(char))
    {
        return '\0';
    }
    else
    {
        return L'\0';
    }
}

#if _WINDOWS
inline constexpr char const* endl = "\r\n";
#else
inline constexpr char const* endl = "\n";
#endif

template <IsCharType T>
size_t string_length(const T* str)
{
    const T* ch = str;
    constexpr T nullTerminator = null_terminator<T>();
    size_t length = 0;
    for (; *ch != nullTerminator; ++ch, ++length);
    return length;
}

// TODO(Afiq):
// [x] 1. Remove C style string formatting.
// [x] 2. Add { fmt } as a thirdparty dependency.
// [x] 3. Encapsulate format function to return our own string type instead of std::string.

template <IsCharType T, ProvidesMemory Allocator_t = SystemMemory>
class FmtAllocatorInterface : std::conditional_t<std::is_same_v<Allocator_t, SystemMemory>, SystemMemoryInterface<T>, AllocatorMemoryInterface<T, Allocator_t>>
{
private:
    using Allocator = std::conditional_t<std::is_same_v<Allocator_t, SystemMemory>, SystemMemoryInterface<T>, AllocatorMemoryInterface<T, Allocator_t>>;
public:
    using Buffer = fmt::basic_memory_buffer<T, fmt::inline_buffer_size, FmtAllocatorInterface>;
    using value_type = T;

    template<class U> 
    struct rebind 
    { 
        using other = FmtAllocatorInterface<U, Allocator>;
    };

    FmtAllocatorInterface() requires std::same_as<Allocator_t, SystemMemory> {}
    FmtAllocatorInterface(Allocator& allocator) requires !std::same_as<Allocator_t, SystemMemory> :
        Allocator{ &allocator }
    {}

    T* allocate(size_t size)
    {
        return Allocator::allocate_storage(size);
        //return reinterpret_cast<T*>(m_allocator->allocate_memory(size));
    }

    void deallocate(T* pointer, size_t size)
    {
        size;
        Allocator::free_storage(pointer);
        //m_allocator->release_memory(pointer);
    }
};

struct StringTypeEmptyBase {};


/**
* Small String Optimized BasicString class.
*
* Fast, lightweight and semi-fully featured.
* Any string that is less than 24 bytes (including the null terminator) would be placed on the stack instead of the heap.
* A decision was made to not revert the string class to a small string optimised version if the string is somehow altered to be shorter than before and less than 24 bytes.
* Reason being the cost of allocating memory on the heap outweighs the pros of putting the string on the stack.
*/
template <IsCharType StringType, ProvidesMemory AllocatorType = SystemMemory, std::derived_from<ContainerGrowthPolicy> GrowthPolicy = ShiftGrowthPolicy<2>>
class BasicString final : 
    private StringTypeEmptyBase,
    protected std::conditional_t<std::is_same_v<AllocatorType, SystemMemory>, SystemMemoryInterface<StringType>, AllocatorMemoryInterface<StringType, AllocatorType>>
{
public:
    using Type = StringType;
    using ConstType = const Type;
    using Iterator = ArrayIterator<Type>;
    using ConstIterator = ArrayIterator<ConstType>;
private:

    using Allocator = std::conditional_t<std::is_same_v<AllocatorType, SystemMemory>, SystemMemoryInterface<StringType>, AllocatorMemoryInterface<StringType, AllocatorType>>;
    using FmtAllocator = FmtAllocatorInterface<Type, typename Allocator::Type>;
    using FmtBuffer = fmt::basic_memory_buffer<Type, fmt::inline_buffer_size, FmtAllocator>;

    static consteval uint32 msb_bit_flag()
    {
        return 0x80000000;
    }

    static constexpr size_t SSO_MAX = 24 / sizeof(Type) < 1 ? 1 : 24 / sizeof(Type);

protected:

    union
    {
        Type* m_data;
        Type  m_buffer[SSO_MAX];
    };

    uint32 m_len;
    uint32 m_capacity;

    /**
    * Takes in a string literal as an argument and stores that literal into the buffer.
    */
    constexpr size_t write_fv(const Type* str, size_t length)
    {
        if (length >= size())
        {
            alloc(GrowthPolicy::new_capacity(length));
        }

        m_len = 0;
        Type* pointer = pointer_to_string();

        while (m_len != length)
        {
            store_in_buffer(&pointer[m_len], str[m_len]);
            m_len++;
        }

        pointer[m_len] = null_terminator<Type>();

        return m_len;
    }

    /**
    * Calls { fmt } to format string (non-allocator referenced version).
    */
    constexpr size_t write_fv(fmt::string_view str, fmt::format_args args) requires std::same_as<AllocatorType, SystemMemory>
    {
        //SystemAllocator systemAllocator;
        FmtAllocator fmtAlloc;
        FmtBuffer buffer{ fmtAlloc };

        fmt::vformat_to(std::back_inserter(buffer), str, args);

        return write_fv(buffer.data(), buffer.size());
    }

    /**
    * Calls { fmt } to format string (allocator referenced version).
    */
    constexpr size_t write_fv(AllocatorType& allocator, fmt::string_view str, fmt::format_args args) requires !std::same_as<AllocatorType, SystemMemory>
    {
        FmtAllocator fmtAlloc{ &allocator };
        FmtBuffer buffer{ fmtAlloc };

        fmt::vformat_to(std::back_inserter(buffer), str, args);

        return write_fv(buffer.data(), buffer.size());
    }

private:

    /**
    * Returns a pointer to the current buffer that is in use for the string.
    */
    constexpr Type* pointer_to_string()
    {
        return is_small_string() ? m_buffer : m_data;
    }

    /**
    * Returns a const pointer to the current buffer that is in use for the string.
    */
    constexpr const Type* pointer_to_string() const
    {
        return is_small_string() ? m_buffer : m_data;
    }

    /**
    * Stores the character into the buffer / pointer with perfect forwarding.
    */
    template <typename... ForwardType>
    constexpr void store_in_buffer(Type* pointer, ForwardType&&... ch)
    {
        new (pointer) Type{ std::forward<ForwardType>(ch)... };
    }

    constexpr void alloc(size_t length)
    {
        check_allocator_referenced();
        // Check if string was previously a small string.
        const bool isSmallString = is_small_string();

        // Unset the msb in m_capacity to signify that it's no longer a SSO string.
        // m_capacity &= ~msb_bit_flag();

        m_capacity = static_cast<uint32>(length);

        Type* pointer = Allocator::allocate_storage(m_capacity);

        if (!isSmallString)
        {
            ftl::memmove(pointer, m_data, m_len * sizeof(Type));
            Allocator::free_storage(m_data);
        }

        ASSERTION(pointer != nullptr && "Unable to allocate memory!");

        m_data = pointer;
    }

    /**
    * compares to see if the strings are the same.
    * Returns true if they are the same.
    */
    constexpr bool compare(const BasicString& rhs) const
    {
        // If the object is being compared with itself, then it definitely will always be the same.
        if (this != &rhs)
        {
            // If the length of both strings are not the same, then they definitely will not be the same.
            if (m_len != rhs.m_len)
            {
                return false;
            }

            const Type* pointer = pointer_to_string();
            const Type* rhsPointer = rhs.pointer_to_string();

            // If the length are the same, we have to check if the contents of the string matches one another.
            for (size_t i = 0; i < m_len; i++)
            {
                if (pointer[i] != rhsPointer[i])
                {
                    return false;
                }
            }
        }

        return true;
    }


    constexpr bool compare(const char* str) const
    {
        const Type* pointer = pointer_to_string();

        if (pointer != str)
        {
            size_t length = string_length(str);

            if (m_len != length)
            {
                return false;
            }


            for (size_t i = 0; i < m_len; i++)
            {
                if (pointer[i] != str[i])
                {
                    return false;
                }
            }
        }

        return true;
    }

    constexpr void check_allocator_referenced() const
    {
        if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
        {
            ASSERTION(Allocator::allocator_referenced() && "No allocator given to string!");
        }
    }

    constexpr AllocatorType* allocator() const requires !std::same_as<AllocatorType, SystemMemory>
    {
        return Allocator::get_allocator();
    }

public:


    constexpr BasicString() requires std::same_as<AllocatorType, SystemMemory> :
        m_buffer{ null_terminator<Type>() }, m_capacity{ msb_bit_flag() | SSO_MAX }, m_len{ 0 }
    {}

    constexpr BasicString(AllocatorType& allocator) requires !std::same_as<AllocatorType, SystemMemory> :
        m_buffer{ null_terminator<Type>() }, m_capacity{ msb_bit_flag() | SSO_MAX }, m_len{ 0 }, Allocator{ &allocator }
    {}

    constexpr ~BasicString()
    {
        release();
    }

    constexpr BasicString(size_t size) requires std::same_as<AllocatorType, SystemMemory> :
        BasicString{}
    {
        alloc(size);
    }

    constexpr BasicString(size_t size, AllocatorType& allocator) requires !std::same_as<AllocatorType, SystemMemory> :
        BasicString{ &allocator }
    {
        alloc(size);
    }

    constexpr BasicString(const Type* str) requires std::same_as<AllocatorType, SystemMemory> :
        BasicString{}
    {
        write_fv(str, string_length(str));
    }

    constexpr BasicString(const Type* str, AllocatorType& allocator) requires !std::same_as<AllocatorType, SystemMemory> :
        BasicString{ &allocator }
    {
        write_fv(str, string_length(str));
    }

    constexpr BasicString(const Type* str, size_t length) requires std::same_as<AllocatorType, SystemMemory> :
        BasicString{}
    {
        write_fv(str, length);
    }

    /**
    * Assigns the string literal into the string object.
    */
    constexpr BasicString& operator= (const Type* str)
    {
        size_t length = string_length(str);
        write_fv(str, length);

        return *this;
    }


    /**
    * Copy constructor.
    * Performs a deep copy.
    */
    constexpr BasicString(const BasicString& rhs) { *this = rhs; }


    /**
    * Copy assignment operator.
    * Performs a deep copy.
    */
    constexpr BasicString& operator= (const BasicString& rhs)
    {
        if (this != &rhs)
        {
            Type* pointer = m_buffer;
            const Type* rhsPointer = rhs.pointer_to_string();

            m_capacity = rhs.m_capacity;
            m_len = rhs.m_len;

            // Perform a deep copy if the copied string is not a small string.
            if (!rhs.is_small_string())
            {
                alloc(rhs.m_capacity);
                pointer = m_data;
            }

            //memcpy(pointer, rhsPointer, m_len);
            for (size_t i = 0; i < m_len; i++)
            {
                pointer[i] = rhsPointer[i];
            }

            pointer[m_len] = null_terminator<Type>();
        }

        return *this;
    }

    constexpr BasicString(BasicString&& rhs)
    {
        *this = std::move(rhs);
    }

    constexpr BasicString& operator= (BasicString&& rhs)
    {
        if (this != &rhs)
        {
            Type* rhsPointer = rhs.pointer_to_string();

            if (!rhs.is_small_string())
            {
                bool move = true;
                release();

                if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
                {
                    if (allocator() != rhs.allocator())
                    {
                        reserve(rhs.size());
                        const size_t rhsLength = rhs.length();
                        for (size_t i = 0; i < rhsLength; ++i)
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

                for (size_t i = 0; i < m_len; i++)
                {
                    m_buffer[i] = rhsPointer[i];
                }

                m_buffer[m_len] = null_terminator<Type>();
            }

            if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
            {
                new (&rhs) BasicString{ *rhs.allocator() };
            }
            else
            {
                new (&rhs) BasicString{};
            }
        }

        return *this;
    }


    constexpr bool operator== (const BasicString& rhs) const
    {
        return compare(rhs);
    }


    constexpr bool operator!= (const BasicString& rhs) const
    {
        return !(*this == rhs);
    }


    constexpr bool operator== (const char* str) const
    {
        return compare(str);
    }


    constexpr bool operator!= (const char* str) const
    {
        return !(*this == str);
    }


    /**
    * Square brace overloaded operator.
    * Retrieves an element from the specified index.
    */
    constexpr Type& operator[] (size_t index)
    {
        ASSERTION(index < size() && "The index specified exceeded the capacity of the string!");

        // Necessary for raw pointer accessed string.
        if (m_len < index)
        {
            m_len = (uint32)index + 1;
        }

        Type* pointer = pointer_to_string();
        return pointer[index];
    }


    /**
    * Square brace overloaded operator.
    * Retrieves an element from the specified index.
    */
    constexpr const Type& operator[] (size_t index) const
    {
        ASSERTION(index < length() && "Index specified exceeded the length of the string!");
        Type* pointer = pointer_to_string();
        return pointer[index];
    }


    /**
    * Clears all data from the string and releases it from memory.
    */
    constexpr void release()
    {
        if (!is_small_string())
        {
            check_allocator_referenced();

            if (m_data)
            {
                Allocator::free_storage(m_data);
            }
        }

        m_data = nullptr;
        m_capacity = m_len = 0;

        if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
        {
            Allocator::release_allocator();
        }
    }

    /**
    * Reserves [length] sized memory in the string container.
    */
    constexpr void reserve(size_t length)
    {
        if (length > SSO_MAX)
        {
            alloc(length);
        }
    }

    //constexpr void resize(size_t length)
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

        return m_capacity & msb_bit_flag();
    }

    /**
    * Assign a formatted string into the string object.
    * Uses { fmt } internally.
    */
    template <typename... Args>
    constexpr size_t format(const Type* str, Args&&... args)
    {
        if constexpr (std::is_same_v<AllocatorType, SystemMemory>)
        {
            write_fv(fmt::string_view{ str }, fmt::make_format_args(args...));
        }
        else
        {
            write_fv(Allocator::get_allocator(), fmt::string_view{ str }, fmt::make_format_args(args...));
        }

        return m_len;
    }


    /**
    * Assign a string literal into the string object.
    * It is the same as using the assignment operator to assign the string.
    */
    //constexpr size_t write(const Type* str) { return write_fv(str, string_length(str)); }


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
    constexpr BasicString substr(size_t start, size_t count) const
    {
        ASSERTION(start + count < size());
        const Type* pointer = pointer_to_string();
        return BasicString{ &pointer[start], count };
    }

    /**
    * Clears data from the string container. Memory allocated still in-tact.
    */
    constexpr void empty() 
    { 
        pointer_to_string()[0] = null_terminator<Type>(); 
    }


    /**
    * Pushes a character to the back of the string.
    */
    constexpr size_t push(const Type& ch)
    {
        if (m_len == size() || !size())
        {
            alloc(GrowthPolicy::new_capacity(size()));
        }

        Type* pointer = pointer_to_string();
        store_in_buffer(&pointer[m_len], ch);
        pointer[++m_len] = null_terminator<Type>();

        return m_len;
    }


    /**
    * Pushes a character to the back of the string.
    */
    constexpr size_t push(Type&& ch)
    {
        if (m_len == size())
        {
            alloc(GrowthPolicy::new_capacity(size()));
        }

        Type* pointer = pointer_to_string();
        pointer += m_len;
        store_in_buffer(pointer, std::move(ch));
        pointer[++m_len] = null_terminator<Type>();

        return m_len++;
    }

    constexpr ConstType* c_str()	const { return pointer_to_string(); }

    constexpr size_t length()	const { return static_cast<size_t>(m_len); }
    constexpr size_t size()	const { return static_cast<size_t>(m_capacity & ~msb_bit_flag()); }

    constexpr Type*         first   ()              { return pointer_to_string(); }
    constexpr ConstType*    first   ()	    const   { return pointer_to_string(); }
    constexpr Type*         last    ()              { return pointer_to_string() + m_len; }
    constexpr ConstType*    last    ()	    const   { return pointer_to_string() + m_len; }
    constexpr Type&         front   ()              { return *pointer_to_string(); }
    constexpr ConstType&    front   ()	    const   { return *pointer_to_string(); }
    constexpr Type&         back    ()              { return *(pointer_to_string() + m_len); }
    constexpr ConstType&    back    ()	    const   { return *(pointer_to_string() + m_len); }
    Iterator		        begin   ()              { return Iterator(pointer_to_string()); }
    ConstIterator	        begin   ()       const	{ return ConstIterator(pointer_to_string()); }
    ConstIterator	        cbegin  ()       const	{ return ConstIterator(pointer_to_string()); }
    Iterator		        end     ()              { return Iterator(pointer_to_string() + m_len); }
    ConstIterator	        end     ()	    const	{ return ConstIterator(pointer_to_string() + m_len); }
    ConstIterator	        cend    ()	    const	{ return ConstIterator(pointer_to_string() + m_len); }

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

template <IsCharType StringType, size_t Capacity>
class BaseStaticString : private StringTypeEmptyBase
{
public:
    using Type = StringType;
    using ConstType = const Type;
    using Iterator = ArrayIterator<Type>;
    using ConstIterator = ArrayIterator<ConstType>;

    constexpr BaseStaticString() :
        m_buf{}, m_len(0)
    {}

    constexpr ~BaseStaticString() { flush(); }

    constexpr BaseStaticString(const BaseStaticString& rhs) :
        BaseStaticString{}
    { 
        *this = rhs; 
    }

    constexpr BaseStaticString(BaseStaticString&& rhs) :
        BaseStaticString{}
    {
        *this = std::move(rhs);
    }

    constexpr BaseStaticString& operator=(const BaseStaticString& rhs)
    {
        if (this != &rhs)
        {
            ftl::memcopy(m_buf, rhs.m_buf, rhs.m_len * sizeof(Type));
            m_len = rhs.m_len;
        }
        return *this;
    }

    constexpr BaseStaticString& operator=(BaseStaticString&& rhs)
    {
        if (this != &rhs)
        {
            ftl::memmove(m_buf, rhs.m_buf, rhs.m_len * sizeof(Type));
            m_len = rhs.m_len;
            new (&rhs) BaseStaticString();
        }
        return *this;
    }

    constexpr BaseStaticString(const Type* text) : 
        BaseStaticString{}
    {
        write(text);
    }

    constexpr BaseStaticString(std::basic_string_view<Type> view) :
        BaseStaticString{}
    {
        write(view);
    }

    constexpr bool operator==(const BaseStaticString& rhs) const { return compare(rhs); }
    constexpr bool operator!=(const BaseStaticString& rhs) const { return !compare(rhs); }
    constexpr bool operator!=(const Type* str) const { return !compare(str); }
    constexpr bool operator==(const Type* str) const { return compare(str);  }

    constexpr BaseStaticString& operator=(const Type* str)
    {
        write(str);
        return *this;
    }

    constexpr void flush()
    {
        ftl::memzero(m_buf, m_len);
        m_len = 0;
    }

    constexpr void write(const Type* str)
    {
        size_t len = string_length(str);
        ASSERTION((len + 1) <= Capacity);
        flush();
        while (m_len < len)
        {
            m_buf[m_len] = str[m_len];
            m_len++;
        }
        m_buf[m_len] = null_terminator<Type>();
    }

    constexpr void write(const Type* str, size_t len)
    {
        ASSERTION((len + 1) <= Capacity);
        flush();
        while (m_len < len)
        {
            m_buf[m_len] = str[m_len];
            m_len++;
        }
        m_buf[m_len] = null_terminator<Type>();
    }

    constexpr void write(std::basic_string_view<Type> str)
    {
        write(str.data(), str.length());
    }

    constexpr size_t		diff(const BaseStaticString& rhs)	    const { return static_cast<size_t>(strcmp(m_buf, rhs)); }
    constexpr size_t		diff(const char* str)			        const { return static_cast<size_t>(strcmp(m_buf, str)); }
    constexpr bool		    compare(const BaseStaticString& rhs)	const { return strcmp(m_buf, rhs.m_buf) == 0; }
    constexpr bool		    compare(const char* str)			    const { return strcmp(m_buf, str) == 0; }

    constexpr bool		    is_empty()  const { return !m_len; }
    constexpr size_t		length()    const { return m_len; }
    constexpr size_t        capacity()  const { return Capacity; }
    constexpr ConstType*    c_str()     const { return m_buf; }

    constexpr ConstType*    first() const { return m_buf; }
    constexpr ConstType*    last()  const { return m_buf + m_len; }

    constexpr Type*   first() { return m_buf; }
    constexpr Type*   last()  { return m_buf + m_len; }

    template <typename... ForwardType>
    constexpr void add_ch(ForwardType&&... ch)
    {
        ASSERTION(m_len < Capacity);
        m_buf[m_len++] = Type(std::forward<ForwardType>(ch)...);
        m_buf[m_len] = null_terminator<Type>();
    }

    Iterator	    begin   ()          { return Iterator(m_buf); }
    ConstIterator   begin   ()  const   { return ConstIterator(m_buf); }
    Iterator	    end     ()          { return Iterator(m_buf + m_len); }
    ConstIterator   end     ()  const   { return ConstIterator(m_buf + m_len); }

private:
    Type	m_buf[Capacity];
    size_t	m_len;
};

template <IsCharType U, std::integral T = size_t, class Traits = std::char_traits<U>>
class BasicHashStringView : public std::basic_string_view<U, Traits>
{
private:
    using Super = std::basic_string_view<U>;
    using size_type = typename Super::size_type;
    using hash_value_t = T;
    hash_value_t m_hash;
public:
    constexpr BasicHashStringView() : Super{}, m_hash{} {};
    constexpr BasicHashStringView(BasicHashStringView const& rhs) = default;
    constexpr BasicHashStringView(U const* str, size_type count) :
        Super{ str, count }
    {
        m_hash = static_cast<T>((std::hash<Super>{})(*this));
    };
    constexpr BasicHashStringView(U const* str) :
        Super{ str }
    {
        m_hash = static_cast<T>((std::hash<Super>{})(*this));
    }
    template <typename It, typename End>
    constexpr BasicHashStringView(It first, End last) :
        Super{ first, last }
    {
        m_hash = static_cast<T>((std::hash<Super>{})(*this));
    }

    template <typename StringView_t = Super>
    constexpr BasicHashStringView(StringView_t&& strView) :
        Super{ std::forward<StringView_t>(strView) }
    {
        m_hash = static_cast<T>((std::hash<Super>{})(*this));
    }

    constexpr BasicHashStringView& operator=(BasicHashStringView const& rhs) = default;
    hash_value_t    hash            () const { return m_hash; }
    Super           to_string_view  () const { return *this; }
    // We no longer need to define operator!= & overloads for the other way round in C++20
    // https://brevzin.github.io/c++/2019/07/28/comparisons-cpp20/
    constexpr bool operator==(BasicHashStringView other) noexcept
    {
        return m_hash == other.m_hash;
    }
    constexpr bool operator==(Super other) noexcept
    {
        BasicHashStringView temp{ other };
        return m_hash == temp.m_hash;
    }
};

//template <typename>
using String = BasicString<char>;
using UniString = BasicString<wchar_t>;

//template <typename MemoryAllocator, typename HashFunc = MurmurHash<BasicString<char>>, size_t Slack = 2>
//using HashString = BasicHashString<char, HashFunc, Slack>;

template <size_t Capacity>
using StaticString = BaseStaticString<char, Capacity>;

using String16  = StaticString<16>;
using String32  = StaticString<32>;
using String64  = StaticString<64>;
using String128 = StaticString<128>;
using String256 = StaticString<256>;

template <size_t Capacity>
using WideStaticString = BaseStaticString<wchar_t, Capacity>;

using UniString16  = WideStaticString<16>;
using UniString32  = WideStaticString<32>;
using UniString64  = WideStaticString<64>;
using UniString128 = WideStaticString<128>;
using UniString256 = WideStaticString<256>;

using HashStringView  = BasicHashStringView<char>;
using HashWStringView = BasicHashStringView<wchar_t>;

// Uses { fmt } internally to format string.
template <IsCharType T, typename... Args>
constexpr BasicString<T> format(const T* str, Args&&... arguments)
{
    using FmtAllocator = FmtAllocatorInterface<T, SystemMemory>;

    FmtAllocator fmtAlloc;
    fmt::basic_memory_buffer<T, fmt::inline_buffer_size, FmtAllocator> buffer{ fmtAlloc };

    fmt::vformat_to(std::back_inserter(buffer), fmt::string_view{ str }, fmt::make_format_args(arguments...));

    return BasicString<T>{ buffer.data(), buffer.size() };
}

// Uses { fmt } internally to format string.
template <IsCharType T, CanAllocateMemory Allocator, typename... Args>
constexpr BasicString<T>* format(Allocator& allocator, const T* str, Args&&... arguments)
{
    using FmtAllocator = FmtAllocatorInterface<T, Allocator>;
    using FmtBuffer = typename FmtAllocator::Buffer;

    FmtAllocator fmtAlloc{ &allocator };
    fmt::basic_memory_buffer<T, fmt::inline_buffer_size, FmtAllocator> buffer{ fmtAlloc };

    fmt::vformat_to(std::back_inserter(buffer), fmt::string_view{ str }, fmt::make_format_args(arguments...));

    return BasicString<T>{ buffer.data(), buffer.size() };
}

template <typename T>
concept IsStringType = std::derived_from<T, StringTypeEmptyBase>;

template <size_t POOL_SIZE = 32_KiB>
class BasicStringPool
{
private:
    std::list<std::string> m_pools = {};
    std::string* m_current = nullptr;

    void new_pool()
    {
        std::string _temp;
        _temp.reserve(POOL_SIZE);
        m_pools.push_back(std::move(_temp));
        m_current = &m_pools.back();
    }
public:
    std::string_view append(std::string_view str)
    {
        if (!m_pools.size())
        {
            new_pool();
        }
        else if (m_current && (str.size() + (*m_current).size() > (*m_current).capacity()))
        {
            new_pool();
        }
        std::string& current = *m_current;
        size_t const offset = current.size();
        current.append(str);

        return std::string_view{ current.begin() + offset, current.end() };
    }
};

using StringPool = BasicStringPool<>;

FTLEND

template<>
struct std::hash<ftl::String>
{
    size_t operator()(const ftl::String& str) const noexcept
    {
        using T = ftl::String::Type;
        std::hash<T> hasher;
        size_t result = 0;
        for (const T& c : str)
        {
            result += hasher(c);
        }
        return result;
    }
};

template <>
struct std::hash<ftl::HashStringView>
{
    size_t operator()(ftl::HashStringView const& strView) const noexcept
    {
        return (std::hash<std::string_view>{})(strView.to_string_view());
    }
};

#endif // !FOUNDATION_STRING_H
