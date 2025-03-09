#pragma once
#ifndef LIB_ARRAY_HPP
#define LIB_ARRAY_HPP

#include <plf_colony.h>

#include "memory.hpp"
#include "utility.hpp"

namespace lib
{
/**
* TODO(afiq):
* [ ] Make this compatible with std::span. Look into the requirements of contiguous_range and sized_range.
* Refer to this link https://en.cppreference.com/w/cpp/container/span/span, constructor (7).
*/
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

	constexpr void verify_offset_is_in_range([[maybe_unused]] difference_type const offset) const noexcept
	{
		ASSERTION(((m_data + offset) >= m_container->data()), "Iterator exceeded past start range of the array.");
		ASSERTION(((m_data + offset) <= m_container->data() + m_container->size()), "Iterator exceeded past end range of the array.");
	}

	ptr_t               m_data;
	array_type const*   m_container;
};

/**
* TODO(afiq):
* [ ] Tidy up implementation.
*/
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
	typename allocator = allocator<T>,
	std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>
>
class array final
{
public:

	static_assert(std::is_reference_v<T> == false, "array does not support storing reference types.");

	using value_type                = T;
	using allocator_type            = allocator;
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

	constexpr array() :
		m_box{}, m_len{}, m_capacity{}
	{}

	constexpr array(allocator_type const& in_allocator) :
		m_box{ nullptr, in_allocator }, m_len{}, m_capacity{}
	{}

	constexpr ~array()
	{
		release();
	}

	constexpr array(size_t length, allocator_type const& in_allocator = allocator_type{}) :
		array(in_allocator)
	{
		_grow(length);
	}

	constexpr array(size_t count, value_type const& value, allocator_type const& in_allocator = allocator_type{}) :
		array(in_allocator)
	{
		assign(count, value);
	}

	template <typename input_iterator>
	constexpr array(input_iterator first, input_iterator last, allocator_type const& in_allocator = allocator_type{}) :
		array(in_allocator)
	{
		assign(first, last);
	}

	constexpr array(std::initializer_list<value_type> list, allocator_type const& in_allocator = allocator_type{}) :
		array(in_allocator)
	{
		append(list);
	}

	constexpr array(array const& other) :
		array(std::allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_box))
	{
		_deep_copy(other);
	}

	constexpr array(array&& other) :
		m_box{ std::exchange(other.m_box, {}) },
		m_len{ std::exchange(other.m_len, {}) },
		m_capacity{ std::exchange(other.m_capacity, {}) }
	{}

	constexpr array& operator= (array const& other)
	{
		if (this != &other)
		{
			_destruct(0, m_len);
			_deep_copy(other);
		}
		return *this;
	}

	constexpr array& operator= (array&& other)
	{
		if (this != &other)
		{
			release();

			m_box = std::exchange(other.m_box, {});
			m_len = std::exchange(other.m_len, 0);
			m_capacity = std::exchange(other.m_capacity, 0);

			// No placement new is needed to be done on rhs as it is already in an valid but unspecified state.
		}
		return *this;
	}

	constexpr value_type& operator[] (size_t index) const
	{
		ASSERTION(index < m_len, "The index specified exceeded the internal buffer size of the array!");
		return m_box.data[index];
	}

	constexpr bool operator==(array const& rhs) const
	{
		if (size() != rhs.size())
		{
			return false;
		}
		for (size_t i = 0; i < m_len; ++i)
		{
			if (m_box.data[i] != rhs.m_box.data[i])
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
			size_t const numDestroyed = _destruct(size - 1, m_capacity);
			m_len -= numDestroyed;
		}
		else
		{
			_grow(size);
		}
	}

	constexpr void resize(size_t count)
	{
		if (count < m_len)
		{
			size_t const numDestroyed = _destruct(count - 1, m_len - 1);
			m_len -= numDestroyed;
		}
		else if (count > m_capacity)
		{
			_grow(count);
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
			size_t const numDestroyed = _destruct(count - 1, m_len - 1);
			m_len -= numDestroyed;
		}
		else if (count >= m_capacity)
		{
			_grow(count);
		}

		for (size_t i = m_len; i < count; i++)
		{
			emplace_back(std::forward<Args>(args)...);
		}
	}

	constexpr void release()
	{
		_destruct(0, m_len);

		if (m_box.data)
		{
			std::allocator_traits<allocator_type>::deallocate(m_box, m_box.data, m_capacity);
		}
		m_box.data = nullptr;
		m_capacity = m_len = 0;
	}

	template <typename... Args>
	constexpr reference emplace_back(Args&&... args)
	{
		if (m_len == m_capacity)
		{
			_grow();
		}
		return _emplace_internal(m_len++, std::forward<Args>(args)...);
	}

	template <typename... Args>
	constexpr iterator emplace(const_iterator pos, Args&&... args)
	{
		ASSERTION(pos >= cbegin() && pos <= cend(), "Iterator is not within the range of the array.");

		bool const insertAtEnd = pos == cend();
		size_t const position = std::distance(cbegin(), pos);
		size_t const count = 1;

		_try_grow_for_insert(count);
		// This is the position that we want to insert out value in.
		iterator it = _prepare_for_insert(insertAtEnd, position, count);

		return _emplace_at(it, count, std::forward<Args>(args)...);
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
		ASSERTION(pos >= cbegin() && pos <= cend(), "Iterator is not within the range of the array.");

		bool const insertAtEnd = pos == cend();
		size_t const position = std::distance(cbegin(), pos);

		_try_grow_for_insert(count);

		iterator it = _prepare_for_insert(insertAtEnd, position, count);

		return _emplace_at(it, count, value);
	}

	template <typename InputIterator>
	constexpr iterator insert(const_iterator pos, InputIterator first, InputIterator last)
	{
		ASSERTION(pos >= cbegin() && pos <= cend(), "Iterator is not within the range of the array.");

		bool const insertAtEnd = pos == cend();
		size_t const position = std::distance(cbegin(), pos);
		size_t const count = std::distance(first, last);

		_try_grow_for_insert(count);

		iterator it = _prepare_for_insert(insertAtEnd, position, count);

		return _insert_at(it, count, &(*first));
	}

	constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> list)
	{
		ASSERTION(pos >= cbegin() && pos <= cend(), "Iterator is not within the range of the array.");

		bool const insertAtEnd = pos == cend();
		size_t const position = std::distance(cbegin(), pos);
		size_t const count = list.size();

		_try_grow_for_insert(count);

		iterator it = _prepare_for_insert(insertAtEnd, position, count);

		return _insert_at(it, count, list.begin());
	}

	constexpr size_t append(std::initializer_list<value_type> list)
	{
		if (list.size() > m_capacity)
		{
			_grow(list.size());
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
			_grow(count);
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
			_grow(count);
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
			_destruct(m_len - count, m_len);
			m_len -= count;
		}
	}

	constexpr void pop_back(size_t count = 1)
	{
		pop(count);
	}

	constexpr void pop_at(size_t index)
	{
		// The element should reside within the container's length.
		ASSERTION(index < m_len);
		// count here means the number of elements to shift forward to close the gap.
		size_t const count = m_len - index;

		_destruct(index, index + 1);
		_shift_forward(&m_box.data[index + 1], &m_box.data[index], count);

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

		_destruct(index, index + 1);
		_shift_forward(&m_box.data[index + 1], &m_box.data[index], count);

		--m_len;

		return iterator{ m_box.data + index, *this };
	}

	constexpr iterator erase(const_iterator begin, const_iterator end)
	{
		size_t const from = std::distance(cbegin(), begin);
		size_t const to = std::distance(cbegin(), end);
		size_t const count = m_len - to;

		_destruct(from, to);
		_shift_forward(&m_box.data[to], &m_box.data[to + 1], count);

		m_len -= (to - from);

		return iterator{ m_box.data + from, *this };
	}

	constexpr bool empty() const
	{
		return m_len == 0;
	}

	constexpr void clear()
	{
		_destruct(0, m_len);
		m_len = 0;
	}

	constexpr size_t index_of(value_type const& element) const
	{
		return index_of(&element);
	}

	constexpr size_t index_of(value_type const* element) const
	{
		// Object must reside within the boundaries of the array.
		ASSERTION(element >= m_box.data && element < m_box.data + m_capacity);

		// The specified object should reside at an address that is an even multiple of of the Type's size.
		ASSERTION(((uint8*)element - (uint8*)m_box.data) % sizeof(value_type) == 0);

		return static_cast<size_t>(element - m_box.data);
	}

	/**
	* Number of elements in the array.
	*/
	[[deprecated]] constexpr size_t length() const { return m_len; }

	constexpr size_t    size    () const { return m_len; }
	constexpr size_t    capacity() const { return m_capacity; }
	constexpr pointer   data    () const { return *m_box; }
	constexpr size_t    bytes   () const { return m_len * sizeof(value_type); }

	constexpr size_t    size_bytes() const { return bytes(); }

	constexpr reference front   () { return *data(); }
	constexpr reference back    () { return *(data() + (m_len - 1)); }

	constexpr const_reference front ()  const    { return *m_box; }
	constexpr const_reference back  ()  const    { return *(m_box + (m_len - 1)); }

	constexpr iterator                  begin   ()          { return iterator(m_box.data, *this); }
	constexpr iterator                  end     ()          { return iterator(m_box.data + m_len, *this); }
	constexpr const_iterator            begin   () const    { return const_iterator(m_box.data, *this); }
	constexpr const_iterator            end     () const    { return const_iterator(m_box.data + m_len, *this); }
	constexpr const_iterator            cbegin  () const    { return const_iterator(m_box.data, *this); }
	constexpr const_iterator            cend    () const    { return const_iterator(m_box.data + m_len, *this); }
	constexpr reverse_iterator          rbegin  ()          { return reverse_iterator(end()); }
	constexpr reverse_iterator          rend    ()          { return reverse_iterator(begin()); }
	constexpr const_reverse_iterator    rbegin  () const    { return const_reverse_iterator(end()); }
	constexpr const_reverse_iterator    rend    () const    { return const_reverse_iterator(begin()); }
	constexpr const_reverse_iterator    crbegin () const    { return const_reverse_iterator(end()); }
	constexpr const_reverse_iterator    crend   () const    { return const_reverse_iterator(begin()); }

private:
	constexpr void _grow(size_t size = 0)
	{
		size_t const oldCapacity = m_capacity;
		m_capacity = growth_policy::new_capacity(m_capacity);

		if (size)
		{
			m_capacity = size;
		}

		// Basically a realloc.
		pointer temp = std::allocator_traits<allocator_type>::allocate(m_box, m_capacity);

		if (m_len)
		{
			if constexpr (std::is_standard_layout_v<value_type> && std::is_trivially_move_assignable_v<value_type>)
			{
				std::memcpy(temp, m_box.data, m_len * sizeof(value_type));
			}
			else
			{
				for (size_t i = 0; i < m_len; ++i)
				{
					new (temp + i) value_type{ std::move(m_box.data[i]) };
				}
			}
		}

		if (m_box.data)
		{
			std::allocator_traits<allocator_type>::deallocate(m_box, m_box.data, oldCapacity);
		}

		m_box.data = temp;
	}

	constexpr size_t _destruct(size_t from, size_t to)
	{
		for (size_t i = from; i < to; ++i)
		{
			m_box.data[i].~value_type();
		}
		return to - from;
	}

	template <typename... ForwardType>
	constexpr reference _emplace_internal(size_t pos, ForwardType&&... args)
	{
		new (m_box.data + pos) value_type{ std::forward<ForwardType>(args)... };
		return m_box.data[pos];
	}

	constexpr void _shift_forward(pointer src, pointer dst, size_t count)
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

	constexpr void _shift_backwards(pointer src, pointer dst, size_t count)
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

	constexpr void _try_grow_for_insert(size_t count)
	{
		if (count > (m_capacity - m_len))
		{
			_grow(growth_policy::new_capacity(m_len + count));
		}
	}

	/**
	* \brief "count" is the number of elements being inserted into the container.
	*/
	constexpr iterator _prepare_for_insert(bool insertAtEnd, size_t position, size_t count)
	{
		// This is the tail index prior to inserting the elements.
		size_t const tail = m_len - 1;

		// First we resize the container to fit the number of elements that we are inserting.
		resize(m_len + count);

		if (count && !insertAtEnd)
		{
			size_t const elementCount = tail - position + 1;
			pointer src = &m_box.data[position];
			pointer dst = &m_box.data[position + count];

			// Shift elements backwards to fill the padding created.
			_shift_backwards(src, dst, elementCount);
		}

		return iterator{ m_box.data + position, *this };
	}

	constexpr iterator _insert_at(iterator pos, size_t count, value_type const* data)
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
	constexpr iterator _emplace_at(iterator pos, size_t count, Args&&... args)
	{
		iterator copy = pos;
		while (count)
		{
			new (pos.data()) value_type{ std::forward<Args>(args)... };
			++pos;
			--count;
		}
		return copy;
	}

	constexpr auto _deep_copy(array const& other) -> void
	{
		if (m_capacity < other.capacity())
		{
			_grow(other.capacity());
		}

		m_len = other.size();

		if constexpr (std::is_standard_layout_v<value_type> && std::is_trivially_copyable_v<value_type>)
		{
			std::memcpy(m_box.data, other.m_box.data, sizeof(value_type) * other.size());
		}
		else
		{
			for (size_t i = 0; i < other.m_len; i++)
			{
				m_box.data[i] = other.m_box.data[i];
			}
		}
	}

	using box_type = box<pointer, allocator_type>;

	box_type m_box;
	size_t m_len;
	size_t m_capacity;
};

template <typename T>
using hive = plf::colony<T, allocator<T>>;
}

#endif // !LIB_ARRAY_HPP
