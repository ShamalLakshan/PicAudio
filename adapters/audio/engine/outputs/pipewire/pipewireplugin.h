/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/outputplugin.h>
#include <core/plugins/plugin.h>
#include <gui/plugins/pluginconfigguiplugin.h>
#include <gui/plugins/pluginsettingsprovider.h>

#define PICAUDIO_PLUGIN_IID "com.picaudio.engine.plugin"

namespace PicAudioEngine::Pipewire {
class PipeWirePlugin : public QObject,
                       public Plugin,
                       public OutputPlugin,
                       public PluginConfigGuiPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PICAUDIO_PLUGIN_IID FILE "pipewire.json")
    Q_INTERFACES(PicAudioEngine::Plugin PicAudioEngine::OutputPlugin PicAudioEngine::PluginConfigGuiPlugin)

public:
    [[nodiscard]] QString name() const override;
    [[nodiscard]] OutputCreator creator() const override;
    [[nodiscard]] std::unique_ptr<PluginSettingsProvider> settingsProvider() const override;
};
} // namespace PicAudioEngine::Pipewire
