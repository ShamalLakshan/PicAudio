/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include <core/engine/dsp/processingbufferlist.h>

namespace PicAudioEngine {
size_t ProcessingBufferList::count() const
{
    return m_chunks.size();
}

ProcessingBuffer* ProcessingBufferList::item(size_t index)
{
    if(index >= m_chunks.size()) {
        return nullptr;
    }

    return &m_chunks[index];
}

const ProcessingBuffer* ProcessingBufferList::item(size_t index) const
{
    if(index >= m_chunks.size()) {
        return nullptr;
    }

    return &m_chunks[index];
}

void ProcessingBufferList::reserve(size_t count)
{
    m_chunks.reserve(count);
}

void ProcessingBufferList::clear()
{
    m_chunks.clear();
}

void ProcessingBufferList::removeByIdx(size_t index)
{
    if(index >= m_chunks.size()) {
        return;
    }

    m_chunks.erase(m_chunks.begin() + static_cast<ptrdiff_t>(index));
}

ProcessingBuffer* ProcessingBufferList::insertItem(size_t index, const AudioFormat& format, uint64_t startTimeNs,
                                                   size_t sampleCount)
{
    ProcessingBuffer buffer{format, startTimeNs};

    if(sampleCount > 0) {
        buffer.resizeSamples(sampleCount);
    }

    if(index >= m_chunks.size()) {
        m_chunks.push_back(std::move(buffer));
        return &m_chunks.back();
    }

    m_chunks.insert(m_chunks.begin() + static_cast<ptrdiff_t>(index), std::move(buffer));

    return &m_chunks[index];
}

ProcessingBuffer* ProcessingBufferList::addItem(const AudioFormat& format, uint64_t startTimeNs, size_t sampleCount)
{
    return insertItem(m_chunks.size(), format, startTimeNs, sampleCount);
}

void ProcessingBufferList::addChunk(const ProcessingBuffer& buffer)
{
    m_chunks.push_back(buffer);
}

void ProcessingBufferList::addChunk(ProcessingBuffer&& buffer)
{
    m_chunks.push_back(std::move(buffer));
}

void ProcessingBufferList::setToSingle(const ProcessingBuffer& buffer)
{
    if(m_chunks.empty()) {
        m_chunks.push_back(buffer);
        return;
    }

    m_chunks.resize(1);
    m_chunks[0] = buffer;
}
} // namespace PicAudioEngine
