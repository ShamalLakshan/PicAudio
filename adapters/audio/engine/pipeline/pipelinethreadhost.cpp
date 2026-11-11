/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "pipelinethreadhost.h"

#include <algorithm>

namespace PicAudioEngine {
PipelineThreadHost::PipelineThreadHost()
    : m_audioThreadIdHash{0}
    , m_audioThreadReady{false}
    , m_running{false}
    , m_shutdownRequested{false}
    , m_wakeSequence{0}
    , m_queuedCommandCount{0}
{ }

bool PipelineThreadHost::isRunning() const
{
    return m_running.load(std::memory_order_relaxed);
}

void PipelineThreadHost::setRunning(bool running)
{
    m_running.store(running, std::memory_order_relaxed);
}

bool PipelineThreadHost::isShutdownRequested() const
{
    return m_shutdownRequested.load(std::memory_order_relaxed);
}

void PipelineThreadHost::setShutdownRequested(bool shutdownRequested)
{
    m_shutdownRequested.store(shutdownRequested, std::memory_order_relaxed);
}

void PipelineThreadHost::markAudioThreadReady(uint64_t threadIdHash)
{
    m_audioThreadIdHash.store(threadIdHash, std::memory_order_relaxed);
    m_audioThreadReady.store(true, std::memory_order_relaxed);
}

void PipelineThreadHost::clearAudioThreadIdentity()
{
    m_audioThreadReady.store(false, std::memory_order_relaxed);
    m_audioThreadIdHash.store(0, std::memory_order_relaxed);
}

bool PipelineThreadHost::joinable() const
{
    return m_thread.joinable();
}

void PipelineThreadHost::join()
{
    if(m_thread.joinable()) {
        m_thread.join();
    }
}

bool PipelineThreadHost::enqueueCommand(PipelineCommand cmd, size_t maxQueuedCommands) const
{
    if(!m_running.load(std::memory_order_relaxed) || m_shutdownRequested.load(std::memory_order_relaxed)) {
        return false;
    }

    {
        const std::scoped_lock lock{m_commandMutex};
        if(m_commandQueue.size() >= maxQueuedCommands) {
            return false;
        }
        m_commandQueue.push_back(std::move(cmd));
        m_queuedCommandCount.fetch_add(1, std::memory_order_relaxed);
    }

    wake();
    return true;
}

bool PipelineThreadHost::dequeueCommands(std::deque<PipelineCommand>& commandsOut, size_t maxCommandsPerCycle)
{
    bool hasMoreCommands{false};
    {
        const std::scoped_lock lock{m_commandMutex};
        const size_t commandCount = std::min(m_commandQueue.size(), maxCommandsPerCycle);

        for(size_t i{0}; i < commandCount; ++i) {
            commandsOut.push_back(std::move(m_commandQueue.front()));
            m_commandQueue.pop_front();
        }
        if(commandCount > 0) {
            m_queuedCommandCount.fetch_sub(commandCount, std::memory_order_relaxed);
        }

        hasMoreCommands = !m_commandQueue.empty();
    }

    return hasMoreCommands;
}

void PipelineThreadHost::clearCommands()
{
    const std::scoped_lock lock{m_commandMutex};
    m_commandQueue.clear();
    m_queuedCommandCount.store(0, std::memory_order_relaxed);
}

void PipelineThreadHost::wake() const
{
    m_wakeSequence.fetch_add(1, std::memory_order_relaxed);
    m_wakeCondition.notify_one();
}

uint64_t PipelineThreadHost::currentThreadIdHash()
{
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

bool PipelineThreadHost::isAudioThread() const
{
    if(!m_audioThreadReady.load(std::memory_order_relaxed)) {
        return false;
    }

    return m_audioThreadIdHash.load(std::memory_order_relaxed) == currentThreadIdHash();
}
} // namespace PicAudioEngine
