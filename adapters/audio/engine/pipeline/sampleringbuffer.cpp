/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "sampleringbuffer.h"

#include <cstring>

namespace PicAudioEngine {
SampleRingBuffer::SampleRingBuffer()
    : m_head{0}
    , m_size{0}
{ }

void SampleRingBuffer::clear()
{
    m_head = 0;
    m_size = 0;
}

size_t SampleRingBuffer::size() const
{
    return m_size;
}

void SampleRingBuffer::ensureCapacity(size_t minCapacity)
{
    if(m_buffer.size() >= minCapacity) {
        return;
    }

    size_t newCap = std::max<size_t>(1, m_buffer.size());
    while(newCap < minCapacity) {
        newCap *= 2;
    }

    std::vector<double> next(newCap);

    const size_t first = std::min(m_size, m_buffer.size() - m_head);
    if(first > 0) {
        std::memcpy(next.data(), m_buffer.data() + m_head, first * sizeof(double));
    }

    const size_t second = m_size - first;
    if(second > 0) {
        std::memcpy(next.data() + first, m_buffer.data(), second * sizeof(double));
    }

    m_buffer.swap(next);
    m_head = 0;
}

void SampleRingBuffer::push(const double* src, size_t count)
{
    if(count == 0) {
        return;
    }

    ensureCapacity(m_size + count);

    const size_t tail  = (m_head + m_size) % m_buffer.size();
    const size_t first = std::min(count, m_buffer.size() - tail);
    std::memcpy(m_buffer.data() + tail, src, first * sizeof(double));

    const size_t second = count - first;
    if(second > 0) {
        std::memcpy(m_buffer.data(), src + first, second * sizeof(double));
    }

    m_size += count;
}

void SampleRingBuffer::pushZeros(size_t count)
{
    if(count == 0) {
        return;
    }

    ensureCapacity(m_size + count);

    const size_t tail  = (m_head + m_size) % m_buffer.size();
    const size_t first = std::min(count, m_buffer.size() - tail);
    std::fill_n(m_buffer.data() + tail, first, 0.0);

    const size_t second = count - first;
    if(second > 0) {
        std::fill_n(m_buffer.data(), second, 0.0);
    }

    m_size += count;
}

size_t SampleRingBuffer::pop(double* dst, size_t count)
{
    if(count == 0 || m_size == 0) {
        return 0;
    }

    const size_t take  = std::min(count, m_size);
    const size_t first = std::min(take, m_buffer.size() - m_head);
    std::memcpy(dst, m_buffer.data() + m_head, first * sizeof(double));

    const size_t second = take - first;
    if(second > 0) {
        std::memcpy(dst + first, m_buffer.data(), second * sizeof(double));
    }

    m_head = (m_head + take) % m_buffer.size();
    m_size -= take;
    return take;
}
} // namespace PicAudioEngine
