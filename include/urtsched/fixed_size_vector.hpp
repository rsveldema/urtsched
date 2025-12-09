#pragma once

#include <array>
#include <memory>
#include <stdexcept>

#include <iterator>

namespace realtime
{

/** a vector with a maximum size so we're sure that its not resized even when
 * pushing back */
template <typename T, size_t N> class fixed_size_vector
{
public:
    // Iterator type definitions
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = typename std::array<T, N>::iterator;
    using const_iterator = typename std::array<T, N>::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    size_t size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return m_size == 0;
    }

    size_t capacity() const
    {
        return N;
    }

    void push_back(const T& value)
    {
        if (m_size >= N)
        {
            throw std::runtime_error("fixed_size_vector overflow");
        }
        m_data[m_size++] = value;
    }

    void clear()
    {
        m_size = 0;
    }

    // Element access
    reference operator[](size_type pos)
    {
        return m_data[pos];
    }

    const_reference operator[](size_type pos) const
    {
        return m_data[pos];
    }

    reference at(size_type pos)
    {
        if (pos >= m_size)
        {
            throw std::out_of_range("fixed_size_vector::at");
        }
        return m_data[pos];
    }

    const_reference at(size_type pos) const
    {
        if (pos >= m_size)
        {
            throw std::out_of_range("fixed_size_vector::at");
        }
        return m_data[pos];
    }

    reference front()
    {
        return m_data[0];
    }

    const_reference front() const
    {
        return m_data[0];
    }

    reference back()
    {
        return m_data[m_size - 1];
    }

    const_reference back() const
    {
        return m_data[m_size - 1];
    }

    pointer data()
    {
        return m_data.data();
    }

    const_pointer data() const
    {
        return m_data.data();
    }

    // Iterator support
    iterator begin()
    {
        return m_data.begin();
    }

    const_iterator begin() const
    {
        return m_data.begin();
    }

    const_iterator cbegin() const
    {
        return m_data.cbegin();
    }

    iterator end()
    {
        return m_data.begin() + m_size;
    }

    const_iterator end() const
    {
        return m_data.begin() + m_size;
    }

    const_iterator cend() const
    {
        return m_data.cbegin() + m_size;
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(cend());
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(cbegin());
    }

private:
    std::array<T, N> m_data;
    size_t m_size = 0;
};

} // namespace realtime
