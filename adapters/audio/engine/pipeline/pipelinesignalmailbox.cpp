/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "pipelinesignalmailbox.h"

#include <QCoreApplication>
#include <QEvent>
#include <QObject>

namespace PicAudioEngine {
PipelineSignalMailbox::PipelineSignalMailbox()
    : m_pendingSignalBits{0}
    , m_signalWakePosted{false}
    , m_signalWakeTarget{nullptr}
    , m_signalWakeEventType{signalWakeEventType()}
{ }

void PipelineSignalMailbox::setWakeTarget(QObject* target, int wakeEventType)
{
    m_signalWakeTarget.store(target, std::memory_order_relaxed);
    m_signalWakeEventType.store(wakeEventType > 0 ? wakeEventType : signalWakeEventType(), std::memory_order_relaxed);
}

uint32_t PipelineSignalMailbox::drain()
{
    const uint32_t signalBits = m_pendingSignalBits.exchange(0, std::memory_order_relaxed);

    // Allow one new wake after this drain
    m_signalWakePosted.store(false, std::memory_order_relaxed);

    if(m_pendingSignalBits.load(std::memory_order_relaxed) != 0) {
        postWakeIfNeeded();
    }

    return signalBits;
}

void PipelineSignalMailbox::clear(uint32_t signalMask)
{
    m_pendingSignalBits.fetch_and(~signalMask, std::memory_order_relaxed);
}

void PipelineSignalMailbox::mark(uint32_t signalMask)
{
    m_pendingSignalBits.fetch_or(signalMask, std::memory_order_relaxed);
    postWakeIfNeeded();
}

void PipelineSignalMailbox::reset()
{
    m_pendingSignalBits.store(0, std::memory_order_relaxed);
    m_signalWakePosted.store(false, std::memory_order_relaxed);
}

int PipelineSignalMailbox::signalWakeEventType()
{
    static const int eventType = QEvent::registerEventType();
    return eventType;
}

void PipelineSignalMailbox::postWakeIfNeeded()
{
    if(m_signalWakePosted.exchange(true, std::memory_order_relaxed)) {
        return;
    }

    auto* target = m_signalWakeTarget.load(std::memory_order_relaxed);
    if(!target) {
        m_signalWakePosted.store(false, std::memory_order_relaxed);
        return;
    }

    const int wakeEventType = m_signalWakeEventType.load(std::memory_order_relaxed);
    QCoreApplication::postEvent(target, new QEvent(static_cast<QEvent::Type>(wakeEventType)));
}
} // namespace PicAudioEngine
