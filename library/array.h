#pragma once
#ifndef ARRAY_H
#define ARRAY_H

#include "memory.h"
#include "utility.h"

namespace lib
{

template <typename array_type>
class array_const_iterator
{
public:
    using value_type        = typename array_type::value_type;
    using difference_type   = typename array_type::difference_type;
    using pointer           = typename array_type::const_pointer;
    using reference         = value_type const&;

    using ptr_t = typename array_type::pointer;

    constexpr array_const_iterator() = default;
    constexpr ~array_const_iterator() = default;

    constexpr array_const_iterator(ptr_t data, array_type& container) : 
        m_data{ data },
        m_container{ &container }
    {}

    constexpr array_const_iterator(array_const_iterator const& rhs) : 
        m_data{ rhs.m_data },
        m_container{ rhs.m_container }
    {}

    constexpr array_const_iterator& operator=(array_const_iterator const& rhs)
    {
        m_data = rhs.m_data;
        m_container = rhs.m_container;
        return *this;
    }

    constexpr reference operator[](difference_type index) const noexcept
    {
        verify_offset_is_in_range(index);
        return *(m_data + index);
    }

    constexpr reference operator* () const noexcept { return *m_data; }
    constexpr pointer   operator->() const noexcept { return m_data; }

    constexpr pointer data() const noexcept { return m_data; }

    constexpr array_const_iterator& operator++() noexcept
    {
        verify_offset_is_in_range(1);
        ++m_data;
        return *this;
    }

    constexpr array_const_iterator operator++(int) noexcept
    {
        array_const_iterator tmp = *this;
        tmp.verify_offset_is_in_range(1);
        ++tmp;
        return tmp;
    }

    constexpr array_const_iterator& operator--() noexcept
    {
        verify_offset_is_in_range(-1);
        --m_data;
        return *this;
    }

    constexpr array_const_iterator operator--(int) noexcept
    {
        array_const_iterator tmp = *this;
        tmp.verify_offset_is_in_range(-1);
        --m_data;
        return tmp;
    }

    constexpr array_const_iterator& operator+=(difference_type const offset) noexcept
    {
        verify_offset_is_in_range(offset);
        m_data += offset;
        return *this;
    }

    constexpr array_const_iterator operator+ (difference_type const offset) noexcept
    {
        array_const_iterator tmp = *this;
        tmp += offset;
        return tmp;
    }

    constexpr array_const_iterator& operator-=(difference_type const offset) noexcept
    {
        verify_offset_is_in_range(-offset);
        m_data -= offset;
        return *this;
    }

    constexpr array_const_iterator operator- (difference_type const offset) noexcept
    {
        array_const_iterator tmp = *this;
        tmp -= offset;
        return *tmp;
    }

    constexpr different_type operator-(array_const_iterator const& it) const noexcept
    {
        return m_data - it.m_data;
    }

    constexpr bool operator==(const array_const_iterator& rhs) const noexcept
    {
        return  m_data == rhs.m_data;
    }

    constexpr std::strong_ordering operator<=>(array_const_iterator& rhs) const noexcept
    {
        return m_data <=> rhs.m_data;
    }
protected:

    constexpr void verify_offset_is_in_range(difference_type const offset) const noexcept
    {
#if _DEBUG
        ASSERTION(((m_data + offset) >= m_container.data()) && "Iterator exceeded past start range of the array.");
        ASSERTION(((m_data + offset) < m_container.data() + m_container.size()) && "Iterator exceeded past end range of the array.");
#endif
    }

    ptr_t       m_data;
    array_type* m_container;
};

template <typename array_type>
class array_iterator : public array_const_iterator<array_type>
{
public:
    using super = array_const_iterator<array_type>;

    using value_type        = typename array_type::value_type;
    using difference_type   = typename array_type::difference_type;
    using pointer           = typename array_type::pointer;
    using reference         = typename array_type::reference;

    using super::super;

    constexpr reference operator*() const noexcept
    {
        return *super::m_data;
    }

    constexpr pointer operator->() const noexcept
    {
        return super::m_data;
    }

    constexpr reference operator[](difference_type const index) const noexcept
    {
        return const_cast<reference>(super::operator[](index));
    }

    constexpr array_iterator& operator++() noexcept 
    {
        super::operator++();
        return *this;
    }

    constexpr array_iterator operator++(int) noexcept 
    {
        array_iterator tmp = *this;
        super::operator++();
        return tmp;
    }

    constexpr array_iterator& operator--() noexcept 
    {
        super::operator--();
        return *this;
    }

    constexpr array_iterator operator--(int) noexcept 
    {
        array_iterator tmp = *this;
        super::operator--();
        return tmp;
    }

    constexpr array_iterator& operator+=(difference_type const offset) noexcept 
    {
        super::operator+=(offset);
        return *this;
    }

    constexpr array_iterator operator+(difference_type const offset) const noexcept 
    {
        array_iterator tmp = *this;
        tmp += offset;
        return tmp;
    }

    constexpr array_iterator& operator-=(difference_type const offset) noexcept 
    {
        super::operator-=(offset);
        return *this;
    }

    using super::operator-;

    constexpr array_iterator operator-(difference_type const offset) const noexcept 
    {
        array_iterator tmp = *this;
        tmp -= offset;
        return tmp;
    }
};

template <typename T>
class array_base
{
public:
    size_t m_len;
    size_t m_capacity;
    
    array_base() :
        m_len{}, m_capacity{}
    {}

    array_base(size_t capacity) :
        m_len{}, m_capacity{ capacity }
    {}

    constexpr void destruct(T* data, size_t from = 0, size_t to = m_len)
    {
        for (size_t i = from; i < to; i++)
        {
            data[i].~T();
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

template <
    typename T, 
    provides_memory allocator = system_memory, 
    std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>
>
class array final : 
    protected array_base<T>,
    protected std::conditional_t<std::is_same_v<allocator, system_memory>, system_memory_interface, allocator_memory_interface<allocator>>
{
public:
    using value_type                = T;
    using allocator_interface       = std::conditional_t<std::is_same_v<allocator, system_memory>, system_memory_interface, allocator_memory_interface<allocator>>;
    using allocator_type            = typename allocator_interface::allocator_type;
    using size_type                 = size_t;
    using difference_type           = std::ptrdiff_t;
    using reference                 = value_type&;
    using const_reference           = value_type const&;
    using pointer                   = value_type*;
    using const_pointer             = value_type const*;
    using iterator                  = array_iterator<array>;
    using const_iterator            = array_const_iterator<array>;
    using reverse_iterator          = reverse_iterator<iterator>;
    using const_reverse_iterator    = reverse_iterator<const_iterator>;

    constexpr array() requires std::same_as<allocator_type, system_memory> :
        super{}, m_data{ nullptr }
    {}

    constexpr array(allocator_type& allocator) requires !std::same_as<allocator_type, system_memory> :
        super{}, m_data{ nullptr }, allocator_interface{ &allocator }
    {}

    constexpr ~array()
    {
        release();
    }

    constexpr array(size_t length) requires std::same_as<allocator_type, system_memory> :
        array{}
    {
        grow(length);
    }

    constexpr array(size_t length, allocator_type& allocator) requires !std::same_as<allocator_type, system_memory> :
        super{}, m_data{ nullptr }, allocator_interface{ &allocator }
    {
        grow(length);
    }

    constexpr array(std::initializer_list<value_type> const& list) requires std::same_as<allocator_type, system_memory> :
        array()
    {
        append(list);
    }

    constexpr array(array const& rhs) requires std::same_as<allocator_type, system_memory> :
        super{}, m_data{ nullptr }
    { 
        *this = rhs; 
    }

    constexpr array(array const& rhs) requires !std::same_as<allocator_type, system_memory> :
        super{ *rhs.allocator() }, m_data{nullptr}
    {
        *this = rhs;
    }

    constexpr array(array&& rhs) :
        super{}, m_data{ nullptr }
    { 
        *this = std::move(rhs); 
    }

    constexpr array& operator= (array const& rhs)
    {
        if (this != &rhs)
        {
            super::destruct(m_data, 0, m_len);

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

    constexpr array& operator= (array&& rhs)
    {
        if (this != &rhs)
        {
            bool move = true;
            release();

            if constexpr (!std::is_same_v<allocator_type, system_memory>)
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

            if constexpr (!std::is_same_v<allocator_type, system_memory>)
            {
                new (&rhs) array{ *rhs.allocator() };
            }
            else
            {
                new (&rhs) array{};
            }
        }

        return *this;
    }

    constexpr value_type& operator[] (size_t index)
    {
        ASSERTION(index < m_len && "The index specified exceeded the internal buffer size of the array!");
        return m_data[index];
    }

    constexpr value_type const& operator[] (size_t index) const
    {
        ASSERTION(index < m_len && "The index specified exceeded the internal buffer size of the array.");
        return m_data[index];
    }

    constexpr void reserve(size_t size)
    {
        if (size < m_capacity)
        {
            super::destruct(m_data, size, m_capacity);
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
            super::destruct(m_data, count - 1, m_len - 1);
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
        resize(count, value_type{});
    }

    constexpr void release()
    {
        super::destruct(m_data, 0, m_len);

        if (m_data)
        {
            allocator_interface::free_storage(m_data);

            m_data = nullptr;
            m_capacity = m_len = 0;
        }
    }

    template <typename... ForwardType>
    [[deprecated]] constexpr size_t emplace(ForwardType&&... args)
    {
        if (m_len == m_capacity) 
        { 
            grow(); 
        }
        return super::emplace_internal(m_data, std::forward<ForwardType>(args)...);
    }

    template <typename... forward_type>
    constexpr void emplace_back(forward_type&&... args)
    {
        if (m_len == m_capacity)
        {
            grow();
        }
        return super::emplace_internal(m_data, std::forward<forward_type>(args)...);
    }

    template <typename... ForwardType>
    [[deprecated]] constexpr decltype(auto) emplace_at(size_t index, ForwardType&&... args)
    {
        ASSERTION((index < m_len) && "index specified exceeded array's length");
        super::emplace_at_internal(m_data, index, std::forward<ForwardType>(args)...);
    }

    [[deprecated]] constexpr size_t push(value_type const& element)
    {
        return emplace(element);
    }

    [[deprecated]] size_t push(value_type&& element)
    {
        return emplace(std::move(element));
    }

    constexpr void push_back(value_type const& element)
    {
        super::emplace_internal(m_data, element);
    }

    constexpr void push_back(value_type&& element)
    {
        super::emplace_internal(m_data, std::move(element));
    }

    constexpr decltype(auto) insert(value_type const& element)
    {
        size_t index = emplace(element);
        return m_data[index];
    }

    constexpr decltype(auto) insert(value_type&& element)
    {
        size_t index = emplace(std::move(element));
        return m_data[index];
    }

    constexpr size_t append(array const& rhs)
    {
        append(rhs.begin(), rhs.m_len);
        return m_len;
    }

    constexpr size_t append(std::initializer_list<value_type> const& list)
    {
        for (size_t i = 0; i < list.size(); i++)
        {
            push(*(list.begin() + i));
        }
        return m_len - 1;
    }

    constexpr void pop(size_t count = 1)
    {
        ASSERTION(m_len >= 0);

        if (m_len) 
        { 
            super::destruct(m_data, m_len - count, m_len);
        }
    }

    constexpr void pop_back()
    {
        pop();
    }

    [[deprecated]] constexpr void pop_at(size_t index, bool move = true)
    {
        // The element should reside within the container's length.
        ASSERTION(index < m_len);

        m_data[index].~Type();

        if (move)
        {
            memmove(m_data + index, m_data + index + 1, sizeof(Type) * (--m_len - index));
            new (m_data + m_len) Type();
        }
        else
        {
            new (m_data + index) Type();
        }
    }

    constexpr size_t erase(iterator begin, iterator end)
    {
        size_t from = index_of(*begin);
        size_t to = index_of(*end);
        super::destruct(m_data, from, to);
        return to - from;
    }

    constexpr bool empty() const
    {
        return m_len == 0;
    }

    constexpr void clear()
    {
        super::destruct(m_data, 0, m_len);
    }

    constexpr size_t index_of(value_type const& element) const
    {
        return index_of(&element);
    }

    constexpr size_t index_of(value_type const* element) const
    {
        // Object must reside within the boundaries of the array.
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

    constexpr pointer       data()          { return m_data; }
    constexpr const_pointer data() const    { return m_data; }

    /**
    * Returns a pointer to the first element in the array.
    */
    [[deprecated]] constexpr pointer first   ()          { return m_data; }
    [[deprecated]] constexpr const_pointer first   () const    { return m_data; }

    /**
    * Returns a pointer to the last element in the array.
    * If length of array is 0, Last() returns the 0th element.
    */
    [[deprecated]] constexpr pointer last    ()          { return m_data + (m_len - 1); }
    [[deprecated]] constexpr const_pointer last    () const    { return m_data + (m_len - 1); }

    constexpr reference front   ()  { return *m_data; }
    constexpr reference back    ()  { return *(m_data + (m_len - 1)); }

    constexpr const_reference front ()  const    { return *m_data; }
    constexpr const_reference back  ()  const    { return *(m_data + (m_len - 1)); }

    constexpr iterator                  begin   ()          { return iterator(m_data, this); }
    constexpr iterator                  end     ()          { return iterator(m_data + m_len, this); }
    constexpr const_iterator            begin   () const    { return const_iterator(m_data, this); }
    constexpr const_iterator            end     () const    { return const_iterator(m_data + m_len, this); }
    constexpr const_iterator            cbegin  () const    { return const_iterator(m_data, this); }
    constexpr const_iterator            cend    () const    { return const_iterator(m_data + m_len, this); }
    constexpr reverse_iterator          rbegin  ()          { return reverse_iterator(end()); }
    constexpr reverse_iterator          rend    ()          { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator    rbegin  () const    { return const_reverse_iterator(end()); }
    constexpr const_reverse_iterator    rend    () const    { return const_reverse_iterator(begin()); }
    constexpr const_reverse_iterator    crbegin () const    { return const_reverse_iterator(end()); }
    constexpr const_reverse_iterator    crend   () const    { return const_reverse_iterator(begin()); }

private:

    constexpr void grow(size_t size = 0)
    {
        m_capacity = growth_policy::new_capacity(m_capacity);

        if (size)
        {
            m_capacity = size;
        }

        // Basically a realloc.
        pointer temp = allocator_interface::allocate_storage<value_type>({ .count = m_capacity, .location = std::source_location::current() });

        if (m_len)
        {
            memmove(temp, m_data, m_len * sizeof(value_type));
        }

        allocator_interface::free_storage(m_data);

        m_data = temp;
    }

    constexpr allocator_type* allocator() const requires !std::same_as<allocator_type, system_memory>
    {
        return allocator_interface::get_allocator();
    }

    using super = array_base<T>;

    pointer* m_data;

    using super::m_capacity;
    using super::m_len;
};


// --- Static Array ---

// Use std::array instead.
/*template <typename T, size_t Size>
class static_array final : protected array_base<T>
{
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = value_type const*;

    using iterator = ArrayIterator<value_type>;
    using const_iterator = ArrayIterator<value_type const>;
    using reverse_iterator = ReverseIterator<Iterator>;
    using const_reverse_iterator = ReverseIterator<ConstIterator>;

    constexpr static_array() :
        m_data{}, super{ Size }
    {}

    constexpr ~static_array() { empty(); }

    constexpr static_array(const static_array& rhs) { *this = rhs; }
    constexpr static_array(static_array&& rhs)      { *this = std::move(rhs); }

    constexpr static_array& operator= (const static_array& rhs)
    {
        if (this != &rhs)
        {
            super::destruct(m_data, 0, m_capacity);

            m_len = rhs.m_len;

            for (size_t i = 0; i < rhs.m_len(); i++)
            {
                m_data[i] = rhs.m_data[i];
            }
        }
        return *this;
    }

    constexpr static_array& operator= (static_array&& rhs)
    {
        if (this != &rhs)
        {
            super::destruct(m_data, 0, m_capacity);

            m_len = rhs.m_len;

            for (size_t i = 0; i < rhs.m_len; i++)
            {
                m_data[i] = rhs.m_data[i];
            }

            new (&rhs) static_array{};
        }
        return *this;
    }

    constexpr reference operator[] (size_t index)
    {
        ASSERTION("The index specified exceeded the internal buffer size of the Array!" && index < m_len);
        return m_data[index];
    }

    constexpr const reference operator[] (size_t index) const
    {
        ASSERTION("The index specified exceeded the internal buffer size of the Array." && index < m_len);
        return m_data[index];
    }

    constexpr reference at(size_t index)
    {
        ASSERTION("The index specified exceeded the internal buffer size of the Array!" && index < m_len);
        return m_data[index];
    }

    constexpr const reference at(size_t index) const
    {
        ASSERTION("The index specified exceeded the internal buffer size of the Array." && index < m_len);
        return m_data[index];
    }

    template <typename... ForwardType>
    constexpr void resize(size_t count, ForwardType&&... args)
    {
        ASSERTION((count < m_capacity) && "count can not exceed capacity for containers of this type");
        if (count < m_len)
        {
            super::destruct(m_data, count - 1, m_len - 1);
        }
        else
        {
            for (size_t i = m_len; i < count; i++)
            {
                emplace(std::forward<ForwardType>(args)...);
            }
        }
    }

    constexpr void resize(size_t count) { resize(count, Type{}); }

    template <typename... ForwardType>
    constexpr size_type emplace(ForwardType&&... args)
    {
        ASSERTION((m_len < m_capacity) && "Array is full");

        return super::emplace_internal(m_data, std::forward<ForwardType>(args)...);
    }

    template <typename... ForwardType>
    constexpr decltype(auto) emplace_at(size_type index, ForwardType&&... args)
    {
        ASSERTION((index < m_len) && "index specified exceeded array's length");

        super::destruct(m_data, index, index + 1);
        return super::emplace_at_internal(m_data, index, std::forward<ForwardType>(args)...);
    }

    constexpr size_type push(const Type& element)  { return emplace(element); }
    constexpr size_type push(Type&& element)       { return emplace(std::move(element)); }

    constexpr decltype(auto) insert(const Type& element)
    {
        size_type index = emplace(element);
        return m_data[index];
    }

    constexpr decltype(auto) insert(Type&& element)
    {
        size_type index = emplace(std::move(element));
        return m_data[index];
    }

    constexpr size_type pop(size_type count = 1)
    {
        ASSERTION((m_len >= 0) || (m_len - count > 0));

        super::destruct(m_data, m_len - count, m_len, true);
        //m_len -= count;
        //m_len = (m_len < 0) ? 0 : m_len;

        return m_len;
    }

    constexpr void pop_at(size_type index, bool move = true)
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
        super::destruct(m_data, 0, m_len, reconstruct);
    }

    constexpr inline size_type index_of(const Type& element) const
    {
        return index_of(&element);
    }

    constexpr inline size_type index_of(const Type* element) const
    {
        const Type* base = &m_data[0];

        ASSERTION(element >= base && element < base + m_capacity);
        ASSERTION(((uint8*)element - (uint8*)base) % sizeof(Type) == 0);

        return static_cast<size_type>(element - base);
    }

    [[deprecated]] constexpr size_type length()  const { return m_len; }
    constexpr size_type size()    const { return m_capacity; }

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

private:
    using super = array_base<value_type>;

    value_type m_data[Size];
    using super::m_len;
    using super::m_capacity;
};*/

}

#endif // !ARRAY_H
