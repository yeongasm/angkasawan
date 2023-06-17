#pragma once
#ifndef FOUNDATION_ARRAY_H
#define FOUNDATION_ARRAY_H

#include "memory/memory.h"
#include "utility.h"

FTLBEGIN

template <typename T>
class ArrayIterator
{
private:
    template <typename T>
    using Raw_t = std::remove_reference_t<T>;
public:
    using Type_t = T;
    using Reference_t = Raw_t<Type_t>&;
    using Pointer_t   = Raw_t<Type_t>*;

    // To make it compatible with std algorithms.
    using value_type  = std::remove_cv_t<T>;
    using reference   = Reference_t;

    constexpr ArrayIterator() = default;
    constexpr ~ArrayIterator() = default;

    constexpr ArrayIterator(Pointer_t data) : m_data{ data } {}
    constexpr ArrayIterator(const ArrayIterator& rhs) : m_data{ rhs.m_data } {}

    constexpr ArrayIterator& operator=(const ArrayIterator& rhs)
    {
        m_data = rhs.m_data;
        return *this;
    }

    constexpr Pointer_t data() const { return m_data; }

    constexpr ArrayIterator& operator++()
    {
        ++m_data;
        return *this;
    }

    constexpr ArrayIterator operator++(int)
    {
        ArrayIterator tmp = *this;
        ++m_data;
        return tmp;
    }

    constexpr ArrayIterator& operator--()
    {
        --m_data;
        return *this;
    }

    constexpr ArrayIterator operator--(int)
    {
        ArrayIterator tmp = *this;
        --m_data;
        return tmp;
    }

    constexpr ArrayIterator& operator+ (size_t offset)
    {
        m_data += offset;
        return *this;
    }

    constexpr ArrayIterator& operator- (size_t offset)
    {
        m_data -= offset;
        return *this;
    }

    constexpr size_t operator-(ArrayIterator it) const
    {
        return m_data - it.m_data;
    }

    constexpr bool operator==(const ArrayIterator& rhs) const
    {
        return  m_data == rhs.m_data;
    }

    constexpr bool operator!=(const ArrayIterator& rhs) const
    {
        return m_data != rhs.m_data;
    }

    constexpr bool operator<(const ArrayIterator& rhs) const
    {
        return m_data < rhs.m_data;
    }

    constexpr bool operator<=(const ArrayIterator& rhs) const
    {
        return m_data <= rhs.m_data;
    }

    constexpr bool operator>(const ArrayIterator& rhs) const
    {
        return m_data < rhs.m_data;
    }

    constexpr bool operator>=(const ArrayIterator& rhs) const
    {
        return m_data >= rhs.m_data;
    }

    constexpr Reference_t operator[](size_t index) const
    {
        return *(m_data + index);
    }

    constexpr Reference_t   operator* () const { return *m_data; }
    constexpr Pointer_t     operator->() const { return m_data; }
private:
    mutable Pointer_t m_data;
};

template <typename T>
struct ArrayBase
{
    size_t m_len;
    size_t m_capacity;

    ArrayBase() :
        m_len{}, m_capacity{}
    {}

    ArrayBase(size_t capacity) :
        m_len{}, m_capacity{ capacity }
    {}

    constexpr void destruct(T* data, size_t from = 0, size_t to = m_len, bool reconstruct = false)
    {
        for (size_t i = from; i < to; i++)
        {
            data[i].~T();
            if (reconstruct)
            {
                new (data + i) T{};
            }
            --m_len;
        }
    }

    template <typename... ForwardType>
    size_t emplace_internal(T* data, ForwardType&&... args)
    {
        new (data + m_len) T{ std::forward<ForwardType>(args)... };
        return m_len++;
    }

    template <typename... ForwardType>
    decltype(auto) emplace_at_internal(T* data, size_t index, ForwardType&&... args)
    {
        destruct(data, index, index + 1);
        new (data + index) T{ std::forward<ForwardType>(args)... };
        return data[index];
    }
};


template <typename T, ProvidesMemory AllocatorType = SystemMemory, std::derived_from<ContainerGrowthPolicy> GrowthPolicy = ShiftGrowthPolicy<4>>
class Array final : 
    protected ArrayBase<T>,
    protected std::conditional_t<std::is_same_v<AllocatorType, SystemMemory>, SystemMemoryInterface<T>, AllocatorMemoryInterface<T, AllocatorType>>
{
private:

    using Type = T;
    using ConstType = const std::remove_cv_t<Type>;
    using Super = ArrayBase<T>;
    using Allocator = std::conditional_t<std::is_same_v<AllocatorType, SystemMemory>, SystemMemoryInterface<T>, AllocatorMemoryInterface<T, AllocatorType>>;

    Type* m_data;

    using Super::m_capacity;
    using Super::m_len;

    constexpr void grow(size_t size = 0)
    {
        check_allocator_referenced();

        m_capacity = GrowthPolicy::new_capacity(m_capacity);

        if (size) 
        { 
            m_capacity = size; 
        }

        // Basically a realloc.
        Type* temp = Allocator::allocate_storage(m_capacity);

        if (m_len)
        {
            ftl::memmove(temp, m_data, m_len * sizeof(Type));
        }

        Allocator::free_storage(m_data);

        // Doing this require the objects that is being stored in this container have a default constructor.
        // Not all objects necessarily have a default constructor.
        //for (size_t i = m_len; i < m_capacity; i++)
        //{
        //    new (temp + i) Type();
        //}

        m_data = temp;
    }

    constexpr void check_allocator_referenced() const
    {
        if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
        {
            ASSERTION(Allocator::allocator_referenced() && "No allocator given to array!");
        }
    }

    constexpr AllocatorType* allocator() const requires !std::same_as<AllocatorType, SystemMemory>
    {
        return Allocator::get_allocator();
    }
public:
    using Iterator              = ArrayIterator<Type>;
    using ConstIterator         = ArrayIterator<ConstType>;
    using ConstReverseIterator  = ReverseIterator<ConstIterator>;
    using ReverseIterator       = ReverseIterator<Iterator>;
    using Reference_t           = std::remove_reference_t<Type>&;
    using Pointer_t             = std::remove_reference_t<Type>*;
    using ConstReference_t      = const std::remove_cv_t<Reference_t>;
    using ConstPointer_t        = const std::remove_cv_t<Pointer_t>;

    constexpr Array() requires std::same_as<AllocatorType, SystemMemory> :
        Super{}, m_data{ nullptr }
    {}

    constexpr Array(AllocatorType& allocator) requires !std::same_as<AllocatorType, SystemMemory> :
        Super{}, m_data{ nullptr }, Allocator{ &allocator }
    {}

    constexpr ~Array()
    {
        release();
    }

    constexpr Array(size_t length) requires std::same_as<AllocatorType, SystemMemory> :
        Array{}
    {
        grow(length);
    }

    constexpr Array(size_t length, AllocatorType& allocator) requires !std::same_as<AllocatorType, SystemMemory> :
        Super{}, m_data{ nullptr }, Allocator{ &allocator }
    {
        grow(length);
    }

    constexpr Array(const std::initializer_list<Type>& list) requires std::same_as<AllocatorType, SystemMemory> :
        Array()
    {
        append(list);
    }

    constexpr Array(const Array& rhs) requires std::same_as<AllocatorType, SystemMemory> :
        Super{}, m_data{ nullptr }
    { 
        *this = rhs; 
    }

    constexpr Array(const Array& rhs) requires !std::same_as<AllocatorType, SystemMemory> :
        Super{ *rhs.allocator() }, m_data{nullptr}
    {
        *this = rhs;
    }

    constexpr Array(Array&& rhs) :
        Super{}, m_data{ nullptr }
    { 
        *this = std::move(rhs); 
    }

    constexpr Array& operator= (const Array& rhs)
    {
        if (this != &rhs)
        {
            Super::destruct(m_data, 0, m_len);

            if (m_capacity < rhs.m_capacity)
            {
                grow(rhs.m_capacity);
            }

            m_len = rhs.m_len;
            for (size_t i = 0; i < rhs.m_len; i++)
            {
                m_data[i] = rhs[i];
            }
        }

        return *this;
    }

    constexpr Array& operator= (Array&& rhs)
    {
        if (this != &rhs)
        {
            bool move = true;
            release();

            if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
            {
                if (allocator() != rhs.allocator())
                {
                    grow(rhs.capacity());
                    const size_t rhsLength = rhs.size();

                    for (size_t i = 0; i < rhsLength; ++i)
                    {
                        emplace(std::move(rhs[i]));
                    }
                    rhs.release();
                    move = false;
                }
            }

            if (move)
            {
                m_data = rhs.m_data;
                m_len = rhs.m_len;
                m_capacity = rhs.m_capacity;
            }

            if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
            {
                new (&rhs) Array{ *rhs.allocator() };
            }
            else
            {
                new (&rhs) Array{};
            }
        }

        return *this;
    }

    constexpr Type& operator[] (size_t index)
    {
        ASSERTION(index < m_len && "The index specified exceeded the internal buffer size of the Array!");
        return m_data[index];
    }

    constexpr const Type& operator[] (size_t index) const
    {
        ASSERTION(index < m_len && "The index specified exceeded the internal buffer size of the Array.");
        return m_data[index];
    }

    constexpr void reserve(size_t size)
    {
        if (size < m_capacity)
        {
            Super::destruct(m_data, size, m_capacity);
        }
        else
        {
            grow(size);
        }
    }

    template <typename... ForwardType>
    constexpr void resize(size_t count, ForwardType&&... args)
    {
        if (count < m_len)
        {
            Super::destruct(m_data, count - 1, m_len - 1);
        }
        else
        {
            for (size_t i = m_len; i < count; i++)
            {
                emplace(std::forward<ForwardType>(args)...);
            }
        }
    }

    constexpr void resize(size_t count)
    {
        resize(count, Type());
    }

    constexpr void release()
    {
        check_allocator_referenced();

        Super::destruct(m_data, 0, m_len);

        if (m_data)
        {
            Allocator::free_storage(m_data);

            m_data = nullptr;
            m_capacity = m_len = 0;

            /*if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
            {
                Allocator::release_allocator();
            }*/
        }
    }

    template <typename... ForwardType>
    constexpr size_t emplace(ForwardType&&... args)
    {
        if (m_len == m_capacity) 
        { 
            grow(); 
        }
        return Super::emplace_internal(m_data, std::forward<ForwardType>(args)...);
    }

    template <typename... ForwardType>
    constexpr decltype(auto) emplace_at(size_t index, ForwardType&&... args)
    {
        ASSERTION((index < m_len) && "index specified exceeded array's length");
        return Super::emplace_at_internal(m_data, index, std::forward<ForwardType>(args)...);
    }

    constexpr size_t push(const Type& element)
    {
        return emplace(element);
    }

    constexpr size_t push(Type&& element)
    {
        return emplace(std::move(element));
    }

    constexpr decltype(auto) insert(const Type& element)
    {
        size_t index = emplace(element);
        return m_data[index];
    }

    constexpr decltype(auto) insert(Type&& element)
    {
        size_t index = emplace(std::move(element));
        return m_data[index];
    }

    constexpr size_t append(const Array& rhs)
    {
        append(rhs.begin(), rhs.m_len);
        return m_len;
    }

    constexpr size_t append(const std::initializer_list<Type>& list)
    {
        for (size_t i = 0; i < list.size(); i++)
        {
            push(*(list.begin() + i));
        }
        return m_len - 1;
    }

    constexpr size_t pop(size_t count = 1)
    {
        ASSERTION(m_len >= 0);

        if (!m_len) 
        { 
            return 0; 
        }

        Super::destruct(m_data, m_len - count, m_len, true);

        return m_len;
    }

    constexpr void pop_at(size_t index, bool move = true)
    {
        // The element should reside within the container's length.
        ASSERTION(index < m_len);

        m_data[index].~Type();

        if (move)
        {
            ftl::memmove(m_data + index, m_data + index + 1, sizeof(Type) * (--m_len - index));
            new (m_data + m_len) Type();
        }
        else
        {
            new (m_data + index) Type();
        }
    }

    constexpr size_t erase(Iterator begin, Iterator end)
    {
        size_t from = index_of(*begin);
        size_t to = index_of(*end);
        Super::destruct(m_data, from, to, true);
        return to - from;
    }

    constexpr void empty(bool reconstruct = false)
    {
        Super::destruct(m_data, 0, m_len, reconstruct);
    }

    constexpr size_t index_of(const Type& element) const
    {
        return index_of(&element);
    }

    constexpr size_t index_of(const Type* element) const
    {
        // Object must reside within the boundaries of the Array.
        ASSERTION(element >= m_data && element < m_data + m_capacity);

        // The specified object should reside at an address that is an even multiple of of the Type's size.
        ASSERTION(((uint8*)element - (uint8*)m_data) % sizeof(Type) == 0);

        return static_cast<size_t>(element - m_data);
    }

    /**
    * Number of elements in the array.
    */
    [[deprecated]] constexpr size_t length() const { return m_len; }

    constexpr size_t size() const { return m_len; }

    /**
    * Maximum capacity of the array.
    */
    constexpr size_t capacity() const { return m_capacity; }

    constexpr Pointer_t             data    ()          { return m_data; }
    constexpr ConstPointer_t        data    () const    { return m_data; }

    /**
    * Returns a pointer to the first element in the Array.
    */
    constexpr Pointer_t             first   ()          { return m_data; }
    constexpr ConstPointer_t        first   () const    { return m_data; }

    /**
    * Returns a pointer to the last element in the Array.
    * If length of array is 0, Last() returns the 0th element.
    */
    constexpr Pointer_t             last    ()          { return m_data + (m_len - 1); }
    constexpr ConstPointer_t        last    () const    { return m_data + (m_len - 1); }

    /**
    * Returns a reference to the first element in the Array.
    */
    constexpr Reference_t           front   ()          { return *first(); }
    constexpr ConstReference_t      front   () const    { return *first(); }

    /**
    * Returns a reference to the first element in the Array.
    * If the length of the Array is 0, Back() returns the 0th element.
    */
    constexpr Reference_t           back    ()          { return *last(); }
    constexpr ConstReference_t      back    () const    { return *last(); }

    constexpr Iterator              begin   ()          { return Iterator(m_data); }
    constexpr Iterator              end     ()          { return Iterator(m_data + m_len); }
    constexpr ConstIterator         begin   () const    { return ConstIterator(m_data); }
    constexpr ConstIterator         end     () const    { return ConstIterator(m_data + m_len); }
    constexpr ConstIterator         cbegin  () const    { return ConstIterator(m_data); }
    constexpr ConstIterator         cend    () const    { return ConstIterator(m_data + m_len); }
    constexpr ReverseIterator       rbegin  ()          { return ReverseIterator(end()); }
    constexpr ReverseIterator       rend    ()          { return ReverseIterator(begin()); }
    constexpr ConstReverseIterator  rbegin  () const    { return ConstReverseIterator(end()); }
    constexpr ConstReverseIterator  rend    () const    { return ConstReverseIterator(begin()); }
    constexpr ConstReverseIterator  crbegin () const    { return ConstReverseIterator(end()); }
    constexpr ConstReverseIterator  crend   () const    { return ConstReverseIterator(begin()); }
};


// --- Static Array ---


template <typename T, size_t Size>
class StaticArray final : protected ArrayBase<T>
{
private:

    using Type                  = T;
    using Super                 = ArrayBase<T>;
    using Iterator              = ArrayIterator<Type>;
    using ConstIterator         = ArrayIterator<const Type>;
    using ConstReverseIterator  = ReverseIterator<const Type>;
    using ReverseIterator       = ReverseIterator<Type>;

    Type m_data[Size];
    using Super::m_len;
    using Super::m_capacity;

public:

    constexpr StaticArray() :
        m_data{}, Super{ Size }
    {}

    constexpr ~StaticArray() { empty(); }

    constexpr StaticArray(const StaticArray& rhs) { *this = rhs; }
    constexpr StaticArray(StaticArray&& rhs)      { *this = std::move(rhs); }

    constexpr StaticArray& operator= (const StaticArray& rhs)
    {
        if (this != &rhs)
        {
            Super::destruct(m_data, 0, m_capacity);

            m_len = rhs.m_len;

            for (size_t i = 0; i < rhs.m_len(); i++)
            {
                m_data[i] = rhs.m_data[i];
            }
        }
        return *this;
    }

    constexpr StaticArray& operator= (StaticArray&& rhs)
    {
        if (this != &rhs)
        {
            Super::destruct(m_data, 0, m_capacity);

            m_len = rhs.m_len;

            for (size_t i = 0; i < rhs.m_len; i++)
            {
                m_data[i] = rhs.m_data[i];
            }

            new (&rhs) StaticArray();
        }
        return *this;
    }

    constexpr Type& operator[] (size_t index)
    {
        ASSERTION("The index specified exceeded the internal buffer size of the Array!" && index < m_len);
        return m_data[index];
    }

    constexpr const Type& operator[] (size_t index) const
    {
        ASSERTION("The index specified exceeded the internal buffer size of the Array." && index < m_len);
        return m_data[index];
    }

    constexpr Type& at(size_t index)
    {
        ASSERTION("The index specified exceeded the internal buffer size of the Array!" && index < m_len);
        return m_data[index];
    }

    constexpr const Type& at(size_t index) const
    {
        ASSERTION("The index specified exceeded the internal buffer size of the Array." && index < m_len);
        return m_data[index];
    }

    /*template <typename... ForwardType>
    constexpr void resize(size_t count, ForwardType&&... args)
    {
        ASSERTION((count < m_capacity) && "count can not exceed capacity for containers of this type");
        if (count < m_len)
        {
            Super::destruct(m_data, count - 1, m_len - 1);
        }
        else
        {
            for (size_t i = m_len; i < count; i++)
            {
                emplace(std::forward<ForwardType>(args)...);
            }
        }
    }*/

    /*constexpr void resize(size_t count) { resize(count, Type{}); }*/

    template <typename... ForwardType>
    constexpr size_t emplace(ForwardType&&... args)
    {
        ASSERTION((m_len < m_capacity) && "Array is full");

        return Super::emplace_internal(m_data, std::forward<ForwardType>(args)...);
    }

    template <typename... ForwardType>
    constexpr decltype(auto) emplace_at(size_t index, ForwardType&&... args)
    {
        ASSERTION((index < m_len) && "index specified exceeded array's length");

        Super::destruct(m_data, index, index + 1);
        return Super::emplace_at_internal(m_data, index, std::forward<ForwardType>(args)...);
    }

    constexpr size_t push(const Type& element)  { return emplace(element); }
    constexpr size_t push(Type&& element)       { return emplace(std::move(element)); }

    constexpr decltype(auto) insert(const Type& element)
    {
        size_t index = emplace(element);
        return m_data[index];
    }

    constexpr decltype(auto) insert(Type&& element)
    {
        size_t index = emplace(std::move(element));
        return m_data[index];
    }

    constexpr size_t pop(size_t count = 1)
    {
        ASSERTION((m_len >= 0) || (m_len - count > 0));

        Super::destruct(m_data, m_len - count, m_len, true);
        //m_len -= count;
        //m_len = (m_len < 0) ? 0 : m_len;

        return m_len;
    }

    constexpr void pop_at(size_t index, bool move = true)
    {
        // The element should reside within the container's length.
        ASSERTION(index < m_len);
        m_data[index].~Type();

        if (move)
        {
            ftl::memmove(m_data + index, m_data + index + 1, sizeof(Type) * (--m_len - index));
            new (m_data + m_len) Type{};
        }
        else
        {
            new (m_data + index) Type{};
        }
    }

    constexpr void empty(bool reconstruct = false)
    {
        Super::destruct(m_data, 0, m_len, reconstruct);
    }

    constexpr inline size_t index_of(const Type& element) const
    {
        return index_of(&element);
    }

    constexpr inline size_t index_of(const Type* element) const
    {
        const Type* base = &m_data[0];

        ASSERTION(element >= base && element < base + m_capacity);
        ASSERTION(((uint8*)element - (uint8*)base) % sizeof(Type) == 0);

        return static_cast<size_t>(element - base);
    }

    constexpr size_t length()  const { return m_len; }
    constexpr size_t size()    const { return m_capacity; }

    constexpr Type*         data()          { return m_data; }
    constexpr const Type*   data() const    { return m_data; }

    constexpr Type* first() { return m_data; }
    constexpr const Type* first() const { return m_data; }

    constexpr Type* last() { return &m_data[m_len - 1]; }
    constexpr const Type* last() const { return &m_data[m_len - 1]; }

    constexpr Type& front() { return *first(); }
    constexpr const Type& front() const { return *first(); }
    constexpr Type& back() { return *last(); }
    constexpr const Type& back() const { return *last(); }

    constexpr Iterator              begin   ()          { return Iterator(m_data); }
    constexpr Iterator              end     ()          { return Iterator(&m_data[m_len]); }
    constexpr ConstIterator         begin   () const    { return ConstIterator(m_data); }
    constexpr ConstIterator         end     () const    { return ConstIterator(&m_data[m_len]); }
    constexpr ConstIterator         cbegin  () const    { return ConstIterator(m_data); }
    constexpr ConstIterator         cend    () const    { return ConstIterator(&m_data[m_len]); }
    constexpr ReverseIterator       rbegin  ()          { return ReverseIterator(end()); }
    constexpr ReverseIterator       rend    ()          { return ReverseIterator(begin()); }
    constexpr ConstReverseIterator  rbegin  () const    { return ConstReverseIterator(end()); }
    constexpr ConstReverseIterator  rend    () const    { return ConstReverseIterator(begin()); }
    constexpr ConstReverseIterator  crbegin () const    { return ConstReverseIterator(end()); }
    constexpr ConstReverseIterator  crend   () const    { return ConstReverseIterator(begin()); }
};

FTLEND

#endif // !FOUNDATION_ARRAY_H
