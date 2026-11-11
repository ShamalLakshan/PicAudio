/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include <core/engine/dsp/dspnode.h>

namespace PicAudioEngine {
DspNode::DspNode()
    : m_instanceId{0}
    , m_enabled{true}
{ }

bool DspNode::supportsLiveSettings() const
{
    return false;
}

AudioFormat DspNode::outputFormat(const AudioFormat& input) const
{
    return input;
}

void DspNode::reset() { }

void DspNode::flush(ProcessingBufferList& /*chunks*/, FlushMode /*mode*/) { }

QByteArray DspNode::saveSettings() const
{
    return {};
}

bool DspNode::loadSettings(const QByteArray& /*preset*/)
{
    return false;
}

bool DspNode::setTargetSampleRate(int /*sampleRate*/)
{
    return false;
}

int DspNode::latencyFrames() const
{
    return 0;
}

uint64_t DspNode::instanceId() const
{
    return m_instanceId;
}

void DspNode::setInstanceId(uint64_t instanceId)
{
    m_instanceId = instanceId;
}

bool DspNode::isEnabled() const
{
    return m_enabled;
}

void DspNode::setEnabled(bool enabled)
{
    m_enabled = enabled;
}
} // namespace PicAudioEngine
