#pragma once
#ifndef LIB_ARRAY_H
#define LIB_ARRAY_H

#include <span>
#include <vector>
#include "memory.h"
#include "utility.h"

namespace lib
{

template <typename array_type>
class array_const_iterator
{
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type        = typename array_type::value_type;
	using difference_type   = typename array_type::difference_type;
	using pointer           = typename array_type::const_pointer;
	using reference         = value_type const&;

	using ptr_t = typename array_type::pointer;

	constexpr array_const_iterator() = default;
	constexpr ~array_const_iterator() = default;

	constexpr array_const_iterator(ptr_t data, array_type const& container) : 
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

	constexpr array_const_iterator operator+ (difference_type const offset) const noexcept
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

	constexpr array_const_iterator operator- (difference_type const offset) const noexcept
	{
		array_const_iterator tmp = *this;
		tmp -= offset;
		return *tmp;
	}

	constexpr difference_type operator-(array_const_iterator const& it) const noexcept
	{
		return m_data - it.m_data;
	}

	constexpr bool operator==(array_const_iterator const& rhs) const noexcept
	{
		return  m_data == rhs.m_data;
	}

	constexpr std::strong_ordering operator<=>(array_const_iterator const& rhs) const noexcept
	{
		return m_data <=> rhs.m_data;
	}

protected:
	friend typename array_type;

	constexpr void verify_offset_is_in_range(difference_type const offset) const noexcept
	{
		ASSERTION(((m_data + offset) >= m_container->data()) && "Iterator exceeded past start range of the array.");
		ASSERTION(((m_data + offset) <= m_container->data() + m_container->size()) && "Iterator exceeded past end range of the array.");
	}

	ptr_t               m_data;
	array_type const*   m_container;
};

template <typename array_type>
class array_iterator : public array_const_iterator<array_type>
{
public:
	using super = array_const_iterator<array_type>;

	using iterator_category = std::random_access_iterator_tag;
	using value_type        = typename array_type::value_type;
	using difference_type   = typename array_type::difference_type;
	using pointer           = typename array_type::pointer;
	using reference         = value_type&;

	using super::super;

	constexpr reference operator*() const noexcept
	{
		return *this->m_data;
	}

	constexpr pointer operator->() const noexcept
	{
		return this->m_data;
	}

	constexpr reference operator[](difference_type const index) const noexcept
	{
		return const_cast<reference>(this->operator[](index));
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

	constexpr pointer data() const noexcept { return this->m_data; }

	using super::operator-;

	constexpr array_iterator operator-(difference_type const offset) const noexcept 
	{
		array_iterator tmp = *this;
		tmp -= offset;
		return tmp;
	}
};

template <
	typename T, 
	provides_memory allocator = default_allocator, 
	std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>
>
class array final : protected std::conditional_t<std::is_same_v<allocator, default_allocator>, system_memory_interface, allocator_memory_interface<allocator>>
{
public:

	using value_type                = T;
	using allocator_interface       = std::conditional_t<std::is_same_v<allocator, default_allocator>, system_memory_interface, allocator_memory_interface<allocator>>;
	using allocator_type            = typename allocator_interface::allocator_type;
	using size_type                 = size_t;
	using difference_type           = std::ptrdiff_t;
	using reference                 = value_type&;
	using const_reference           = value_type const&;
	using pointer                   = value_type*;
	using const_pointer             = value_type const*;
	using iterator                  = array_iterator<array>;
	using const_iterator            = array_const_iterator<array>;
	using const_reverse_iterator    = reverse_iterator<const_iterator>;
	using reverse_iterator          = reverse_iterator<iterator>;

	constexpr array() requires std::same_as<allocator_type, default_allocator> :
		m_data{ nullptr }, m_len{}, m_capacity{}
	{}

	constexpr array(allocator_type& allocator) requires !std::same_as<allocator_type, default_allocator> :
		m_data{ nullptr }, m_len{}, m_capacity{}, allocator_interface{ &allocator }
	{}

	constexpr ~array()
	{
		release();
	}

	constexpr array(size_t length) requires std::same_as<allocator_type, default_allocator> :
		array{}
	{
		grow(length);
	}

	constexpr array(size_t length, allocator_type& allocator) requires !std::same_as<allocator_type, default_allocator> :
		array{ allocator }
	{
		grow(length);
	}

	constexpr array(std::initializer_list<value_type> const& list) requires std::same_as<allocator_type, default_allocator> :
		array{}
	{
		append(list);
	}

	constexpr array(array const& rhs) requires std::same_as<allocator_type, default_allocator> :
		array{}
	{ 
		*this = rhs; 
	}

	constexpr array(array const& rhs) requires !std::same_as<allocator_type, default_allocator> :
		array{ *rhs.allocator() }
	{
		*this = rhs;
	}

	constexpr array(array&& rhs) :
		array{}
	{ 
		*this = std::move(rhs); 
	}

	constexpr array& operator= (array const& rhs)
	{
		if (this != &rhs)
		{
			destruct(0, m_len);

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

			if constexpr (!std::is_same_v<allocator_type, default_allocator>)
			{
				if (allocator() != rhs.allocator())
				{
					move = false;

					grow(rhs.capacity());
					size_t const rhsLength = rhs.size();

					for (size_t i = 0; i < rhsLength; ++i)
					{
						m_data[i] = std::move(rhs.m_data[i]);
						++m_len;
					}
					rhs.release();
				}
			}

			if (move)
			{
				m_data = std::move(rhs.m_data);
				m_len = std::move(rhs.m_len);
				m_capacity = std::move(rhs.m_capacity);
			}

			if constexpr (!std::is_same_v<allocator_type, default_allocator>)
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

	constexpr value_type& operator[] (size_t index) const
	{
		ASSERTION(index < m_len && "The index specified exceeded the internal buffer size of the array!");
		return m_data[index];
	}

	constexpr bool operator==(array const& rhs) const
	{
		if (size() != rhs.size())
		{
			return false;
		}
		for (size_t i = 0; i < m_len; ++i)
		{
			if (m_data[i] != rhs.m_data[i])
			{
				return false;
			}
		}
		return true;
	}

	constexpr void reserve(size_t size)
	{
		if (size < m_capacity)
		{
			size_t const numDestroyed = destruct(size - 1, m_capacity);
			m_len -= numDestroyed;
		}
		else
		{
			grow(size);
		}
	}

	constexpr void resize(size_t count)
	{
		if (count < m_len)
		{
			size_t const numDestroyed = destruct(count - 1, m_len - 1);
			m_len -= numDestroyed;
		}
		else if (count > m_capacity)
		{
			grow(count);
		}

		for (size_t i = m_len; i < count; ++i)
		{
			emplace_back(value_type{});
		}
	}

	template <typename... Args>
	constexpr void resize(size_t count, Args&&... args)
	{
		if (count < m_len)
		{
			size_t const numDestroyed = destruct(count - 1, m_len - 1);
			m_len -= numDestroyed;
		}
		else if (count >= m_capacity)
		{
			grow(count);
		}

		for (size_t i = m_len; i < count; i++)
		{
			emplace_back(std::forward<Args>(args)...);
		}
	}

	constexpr void release()
	{
		destruct(0, m_len);

		if (m_data)
		{
			this->free_storage(m_data); 
			m_data = nullptr;
			m_capacity = m_len = 0;
		}
	}

	template <typename... Args>
	constexpr reference emplace_back(Args&&... args)
	{
		if (m_len == m_capacity)
		{
			grow();
		}
		return emplace_internal(m_len++, std::forward<Args>(args)...);
	}

	template <typename... Args>
	constexpr iterator emplace(const_iterator pos, Args&&... args)
	{
		ASSERTION(pos >= cbegin() && pos <= cend() && "Iterator is not within the range of the array.");

		bool const insertAtEnd = pos == cend();
		size_t const position = std::distance(cbegin(), pos);
		size_t const count = 1;

		try_grow_for_insert(count);
		// This is the position that we want to insert out value in.
		iterator it = prepare_for_insert(insertAtEnd, position, count);

		return emplace_at(it, count, std::forward<Args>(args)...);
	}

	constexpr size_t push(value_type const& element)
	{
		const_reference ele = emplace_back(element);
		return index_of(ele);
	}

	constexpr size_t push(value_type&& element)
	{
		const_reference ele = emplace_back(std::move(element));
		return index_of(ele);
	}

	constexpr void push_back(value_type const& element)
	{
		emplace_back(element);
	}

	constexpr void push_back(value_type&& element)
	{
		emplace_back(std::move(element));
	}

	constexpr iterator insert(value_type const& value)
	{
		reference element = emplace_back(value);
		return iterator{ element, *this };
	}

	constexpr iterator insert(value_type&& value)
	{
		reference element = emplace_back(std::move(value));
		return iterator{ element, *this };
	}

	constexpr iterator insert(const_iterator pos, value_type const& value)
	{
		return emplace(pos, value);
	}

	constexpr iterator insert(const_iterator pos, value_type&& value)
	{
		return emplace(pos, std::move(value));
	}

	constexpr iterator insert(const_iterator pos, size_type count, value_type const& value)
	{
		ASSERTION(pos >= cbegin() && pos <= cend() && "Iterator is not within the range of the array.");

		bool const insertAtEnd = pos == cend();
		size_t const position = std::distance(cbegin(), pos);

		try_grow_for_insert(count);

		iterator it = prepare_for_insert(insertAtEnd, position, count);

		return emplace_at(it, count, value);
	}

	template <typename InputIterator>
	constexpr iterator insert(const_iterator pos, InputIterator first, InputIterator last)
	{
		ASSERTION(pos >= cbegin() && pos <= cend() && "Iterator is not within the range of the array.");

		bool const insertAtEnd = pos == cend();
		size_t const position = std::distance(cbegin(), pos);
		size_t const count = std::distance(first, last);

		try_grow_for_insert(count);

		iterator it = prepare_for_insert(insertAtEnd, position, count);

		return insert_at(it, count, &(*first));
	}

	constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> list)
	{
		ASSERTION(pos >= cbegin() && pos <= cend() && "Iterator is not within the range of the array.");

		bool const insertAtEnd = pos == cend();
		size_t const position = std::distance(cbegin(), pos);
		size_t const count = list.size();

		try_grow_for_insert(count);

		iterator it = prepare_for_insert(insertAtEnd, position, count);

		return insert_at(it, count, list.begin());
	}

	constexpr size_t append(std::initializer_list<value_type> list)
	{
		if (list.size() > m_capacity)
		{
			grow(list.size());
		}
		for (value_type const& value : list)
		{
			emplace_back(value);
		}
		return m_len - 1;
	}

	constexpr void assign(size_t count, value_type const& value)
	{
		clear();
		if (count > m_capacity)
		{
			grow(count);
		}
		for (size_t i = 0; i < count; ++i)
		{
			emplace_back(value);
		}
	}

	template <typename input_iterator>
	constexpr void assign(input_iterator first, input_iterator last)
	{
		clear();
		size_t const count = static_cast<size_t>(last - first);
		if (count > m_capacity)
		{
			grow(count);
		}
		while (first != last)
		{
			emplace_back(*first);
			++first;
		}
	}

	constexpr void assign(std::initializer_list<value_type> list)
	{
		clear();
		append(list);
	}

	constexpr void pop(size_t count = 1)
	{
		ASSERTION(m_len >= 0);

		if (m_len) [[likely]]
		{ 
			destruct(m_len - count, m_len);
			m_len -= count;
		}
	}

	constexpr void pop_back()
	{
		pop();
	}

	constexpr void pop_at(size_t index)
	{
		// The element should reside within the container's length.
		ASSERTION(index < m_len);
		// count here means the number of elements to shift forward to close the gap.
		size_t const count = m_len - index;

		destruct(index, index + 1);
		shift_forward(&m_data[index + 1], &m_data[index], count);

		--m_len;
	}

	constexpr void pop_at(const_iterator pos)
	{
		ASSERTION(pos >= cbegin() && pos <= cend() && "Iterator is not within the range of the array.");
		size_t index = std::distance(cbegin(), pos);
		pop_at(index);
	}

	constexpr iterator erase(const_iterator pos)
	{
		ASSERTION(pos >= cbegin() && pos <= cend() && "Iterator is not within the range of the array.");

		size_t const index = std::distance(cbegin(), pos);
		// count here means the number of elements to shift forward to close the gap.
		size_t const count = m_len - index;

		destruct(index, index + 1);
		shift_forward(&m_data[index + 1], &m_data[index], count);

		--m_len;

		return iterator{ m_data + index, *this };
	}

	constexpr iterator erase(const_iterator begin, const_iterator end)
	{
		size_t const from = std::distance(cbegin(), begin);
		size_t const to = std::distance(cbegin(), end);
		size_t const count = m_len - to;

		destruct(from, to);
		shift_forward(&m_data[to], &m_data[to + 1], count);

		m_len -= (to - from);

		return iterator{ m_data + from, *this };
	}

	constexpr bool empty() const
	{
		return m_len == 0;
	}

	constexpr void clear()
	{
		destruct(0, m_len);
		m_len = 0;
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
		ASSERTION(((uint8*)element - (uint8*)m_data) % sizeof(value_type) == 0);

		return static_cast<size_t>(element - m_data);
	}

	/**
	* Number of elements in the array.
	*/
	[[deprecated]] constexpr size_t length() const { return m_len; }

	constexpr size_t    size    () const { return m_len; }
	constexpr size_t    capacity() const { return m_capacity; }
	constexpr pointer   data    () const { return m_data; }
	constexpr size_t    bytes   () const { return m_len * sizeof(value_type); }

	constexpr size_t    size_bytes() const { return bytes(); }

	/**
	* Returns a pointer to the first element in the array.
	*/
	//[[deprecated]] constexpr pointer first   ()          { return m_data; }
	//[[deprecated]] constexpr const_pointer first   () const    { return m_data; }

	/**
	* Returns a pointer to the last element in the array.
	* If length of array is 0, Last() returns the 0th element.
	*/
	//[[deprecated]] constexpr pointer last    ()          { return m_data + (m_len - 1); }
	//[[deprecated]] constexpr const_pointer last    () const    { return m_data + (m_len - 1); }

	constexpr reference front   () { return *m_data; }
	constexpr reference back    () { return *(m_data + (m_len - 1)); }

	constexpr const_reference front ()  const    { return *m_data; }
	constexpr const_reference back  ()  const    { return *(m_data + (m_len - 1)); }

	constexpr iterator                  begin   ()          { return iterator(m_data, *this); }
	constexpr iterator                  end     ()          { return iterator(m_data + m_len, *this); }
	constexpr const_iterator            begin   () const    { return const_iterator(m_data, *this); }
	constexpr const_iterator            end     () const    { return const_iterator(m_data + m_len, *this); }
	constexpr const_iterator            cbegin  () const    { return const_iterator(m_data, *this); }
	constexpr const_iterator            cend    () const    { return const_iterator(m_data + m_len, *this); }
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
		pointer temp = this->allocate_storage<value_type>({ .size = m_capacity });

		if (m_len)
		{
			if constexpr (std::is_trivial_v<value_type> && std::is_trivially_move_assignable_v<value_type>)
			{
				memmove(temp, m_data, m_len * sizeof(value_type));
			}
			else
			{
				for (size_t i = 0; i < m_len; ++i)
				{
					new (temp + i) value_type{ std::move(m_data[i]) };
				}
			}
		}

		this->free_storage(m_data);

		m_data = temp;
	}

	constexpr size_t destruct(size_t from, size_t to)
	{
		for (size_t i = from; i < to; ++i)
		{
			m_data[i].~value_type();
		}
		return to - from;
	}

	template <typename... ForwardType>
	constexpr reference emplace_internal(size_t pos, ForwardType&&... args)
	{
		new (m_data + pos) value_type{ std::forward<ForwardType>(args)... };
		return m_data[pos];
	}

	constexpr void shift_forward(pointer src, pointer dst, size_t count)
	{
		while (count)
		{
			if constexpr (std::is_move_assignable_v<value_type>)
			{
				*dst = std::move(*src);
			}
			else
			{
				new (dst) value_type{ std::move(*src) };
			}
			++dst;
			++src;
			--count;
		}
	}

	constexpr void shift_backwards(pointer src, pointer dst, size_t count)
	{
		dst = dst + count - 1;
		src = src + count - 1;

		while (count)
		{
			if constexpr (std::is_move_assignable_v<value_type>)
			{
				*dst = std::move(*src);
			}
			else
			{
				new (dst) value_type{ std::move(*src) };
			}
			--dst;
			--src;
			--count;
		}
	}

	constexpr void try_grow_for_insert(size_t count)
	{
		if (count > (m_capacity - m_len))
		{
			grow(growth_policy::new_capacity(m_len + count));
		}
	}

	/**
	* \brief "count" is the number of elements being inserted into the container.
	*/
	constexpr iterator prepare_for_insert(bool insertAtEnd, size_t position, size_t count)
	{
		// This is the tail index prior to inserting the elements.
		size_t const tail = m_len - 1;

		// First we resize the container to fit the number of elements that we are inserting.
		resize(m_len + count);

		if (count && !insertAtEnd)
		{
			size_t const elementCount = tail - position + 1;
			pointer src = &m_data[position];
			pointer dst = &m_data[position + count];

			// Shift elements backwards to fill the padding created.
			shift_backwards(src, dst, elementCount);
		}

		return iterator{ m_data + position, *this };
	}

	constexpr iterator insert_at(iterator pos, size_t count, value_type const* data)
	{
		iterator copy = pos;
		if (data)
		{
			while (count)
			{
				new (pos.m_data) value_type{ *(data) };
				++pos;
				++data;
				--count;
			}
		}
		return copy;
	}

	template <typename... Args>
	constexpr iterator emplace_at(iterator pos, size_t count, Args&&... args)
	{
		iterator copy = pos;
		while (count)
		{
			new (pos.m_data) value_type{ std::forward<Args>(args)... };
			++pos;
			--count;
		}
		return copy;
	}

	constexpr allocator_type* allocator() const requires !std::same_as<allocator_type, default_allocator>
	{
		return this->get_allocator();
	}

	pointer m_data;
	size_t m_len;
	size_t m_capacity;
};
}

#endif // !LIB_ARRAY_H
