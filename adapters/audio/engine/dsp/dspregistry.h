/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/dsp/dspnode.h>

#include <QString>

#include <mutex>
#include <vector>

namespace PicAudioEngine {
/*!
 * Thread-safe registry of available DSP node factories.
 *
 * Includes built-ins and plugin-provided entries.
 */
class PICAUDIO_ENGINE_EXPORT DspRegistry
{
public:
    DspRegistry();

    //! Register/replace a DSP factory by id.
    void registerDsp(const DspNode::Entry& entry);

    //! Snapshot of registered DSP entries.
    [[nodiscard]] std::vector<DspNode::Entry> entries() const;
    //! Instantiate DSP node by id, or nullptr if unavailable.
    [[nodiscard]] std::unique_ptr<DspNode> create(const QString& id) const;

private:
    void registerBuiltins();

    mutable std::mutex m_mutex;
    std::vector<DspNode::Entry> m_entries;
};
} // namespace PicAudioEngine
