/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "pipewireplugin.h"

#include "pipewireoutput.h"
#include "pipewiresettings.h"

using namespace Qt::StringLiterals;

namespace PicAudioEngine::Pipewire {
namespace {
class PipewirePluginSettingsProvider : public PluginSettingsProvider
{
protected:
    QDialog* createSettings(QWidget* parent) override
    {
        return new PipewireSettings(parent);
    }
};
} // namespace

QString PipeWirePlugin::name() const
{
    return u"PipeWire"_s;
}

OutputCreator PipeWirePlugin::creator() const
{
    return []() {
        return std::make_unique<PipeWireOutput>();
    };
}

std::unique_ptr<PluginSettingsProvider> PipeWirePlugin::settingsProvider() const
{
    return std::make_unique<PipewirePluginSettingsProvider>();
}
} // namespace PicAudioEngine::Pipewire

#include "moc_pipewireplugin.cpp"
