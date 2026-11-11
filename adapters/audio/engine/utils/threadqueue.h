/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

namespace PicAudioEngine {
template <typename QueueItem>
class ThreadQueue
{
public:
    explicit ThreadQueue(bool blocking = true);

    ThreadQueue(ThreadQueue&& other) noexcept;
    ThreadQueue& operator=(ThreadQueue const& other);
    ThreadQueue& operator=(ThreadQueue&& other) noexcept;

    void clear();

    bool empty() const;
    std::size_t size() const;

    void enqueue(QueueItem item);
    QueueItem& front();
    QueueItem dequeue();

private:
    mutable std::mutex m_mtx;
    std::condition_variable m_cv;
    std::deque<QueueItem> m_queue{};
    bool m_blocking;
};

template <typename QueueItem>
ThreadQueue<QueueItem>::ThreadQueue(bool blocking)
    : m_blocking{blocking}
{ }

template <typename QueueItem>
ThreadQueue<QueueItem>::ThreadQueue(ThreadQueue&& other) noexcept
{
    std::lock_guard lock(other.m_mtx);
    m_queue = std::move(other.m_queue);
}

template <typename QueueItem>
ThreadQueue<QueueItem>& ThreadQueue<QueueItem>::ThreadQueue::operator=(ThreadQueue const& other)
{
    std::lock_guard lock(m_mtx);
    std::lock_guard otherLock(other.m_mtx);

    m_queue = other.m_queue;

    return *this;
}

template <typename QueueItem>
void ThreadQueue<QueueItem>::clear()
{
    std::lock_guard lock(m_mtx);
    m_queue.clear();
}

template <typename QueueItem>
ThreadQueue<QueueItem>& ThreadQueue<QueueItem>::ThreadQueue::operator=(ThreadQueue&& other) noexcept
{
    std::lock_guard lock(m_mtx);
    std::lock_guard otherLock(other.m_mtx);

    m_queue = std::move(other.m_queue);

    return *this;
}

template <typename QueueItem>
bool ThreadQueue<QueueItem>::ThreadQueue::empty() const
{
    std::lock_guard lock(m_mtx);
    return m_queue.empty();
}

template <typename QueueItem>
std::size_t ThreadQueue<QueueItem>::ThreadQueue::size() const
{
    std::lock_guard lock(m_mtx);
    return m_queue.size();
}

template <typename QueueItem>
void ThreadQueue<QueueItem>::ThreadQueue::enqueue(QueueItem item)
{
    {
        std::lock_guard lock(m_mtx);
        m_queue.push_back(std::move(item));
    }

    m_cv.notify_one();
}

template <typename QueueItem>
QueueItem& ThreadQueue<QueueItem>::ThreadQueue::front()
{
    std::unique_lock lock(m_mtx);

    if(m_blocking) {
        while(m_queue.empty()) {
            m_cv.wait(lock);
        }
    }

    return m_queue.front();
}

template <typename QueueItem>
QueueItem ThreadQueue<QueueItem>::ThreadQueue::dequeue()
{
    std::unique_lock lock(m_mtx);

    if(m_blocking) {
        while(m_queue.empty()) {
            m_cv.wait(lock);
        }
    }

    QueueItem item = m_queue.front();
    m_queue.pop_front();
    return item;
}
} // namespace PicAudioEngine
