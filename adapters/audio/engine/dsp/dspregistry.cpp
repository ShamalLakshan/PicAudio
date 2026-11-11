/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "dspregistry.h"

#include "downmixtomonodsp.h"
#include "downmixtostereodsp.h"
#include "monotostereodsp.h"
#include "resamplerdsp.h"
#include "reversestereodsp.h"
#include "skipsilencedsp.h"

#include <algorithm>
#include <mutex>

using namespace Qt::StringLiterals;

namespace PicAudioEngine {
DspRegistry::DspRegistry()
{
    registerBuiltins();
}

void DspRegistry::registerDsp(const DspNode::Entry& entry)
{
    const std::scoped_lock lock{m_mutex};

    const auto it = std::ranges::find_if(m_entries, [&entry](const auto& regEntry) { return regEntry.id == entry.id; });
    if(it != m_entries.end()) {
        return;
    }

    m_entries.push_back(entry);
}

std::vector<DspNode::Entry> DspRegistry::entries() const
{
    const std::scoped_lock lock{m_mutex};
    return m_entries;
}

std::unique_ptr<DspNode> DspRegistry::create(const QString& id) const
{
    const std::scoped_lock lock{m_mutex};

    const auto it = std::ranges::find_if(m_entries, [&id](const auto& entry) { return entry.id == id; });
    if(it != m_entries.end() && it->factory) {
        return it->factory();
    }

    return nullptr;
}

void DspRegistry::registerBuiltins()
{
    {
        auto dsp           = std::make_unique<SkipSilenceDsp>();
        const QString id   = dsp->id();
        const QString name = dsp->name();
        registerDsp({.id = id, .name = name, .factory = []() { return std::make_unique<SkipSilenceDsp>(); }});
    }
    {
        auto dsp           = std::make_unique<ResamplerDsp>();
        const QString id   = dsp->id();
        const QString name = u"Resampler (FFmpeg)"_s;
        registerDsp({.id = id, .name = name, .factory = []() { return std::make_unique<ResamplerDsp>(); }});
    }
    {
        auto dsp           = std::make_unique<MonoToStereoDsp>();
        const QString id   = dsp->id();
        const QString name = dsp->name();
        registerDsp({.id = id, .name = name, .factory = []() { return std::make_unique<MonoToStereoDsp>(); }});
    }
    {
        auto dsp           = std::make_unique<ReverseStereoDsp>();
        const QString id   = dsp->id();
        const QString name = dsp->name();
        registerDsp({.id = id, .name = name, .factory = []() { return std::make_unique<ReverseStereoDsp>(); }});
    }
    {
        auto dsp           = std::make_unique<DownmixToStereoDsp>();
        const QString id   = dsp->id();
        const QString name = dsp->name();
        registerDsp({.id = id, .name = name, .factory = []() { return std::make_unique<DownmixToStereoDsp>(); }});
    }
    {
        auto dsp           = std::make_unique<DownmixToMonoDsp>();
        const QString id   = dsp->id();
        const QString name = dsp->name();
        registerDsp({.id = id, .name = name, .factory = []() { return std::make_unique<DownmixToMonoDsp>(); }});
    }
}
} // namespace PicAudioEngine
