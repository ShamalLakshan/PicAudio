/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/enginedefs.h>

#include <QObject>

#include <cstdint>
#include <unordered_map>

namespace PicAudioEngine {
class DspNode;
class DspRegistry;
class EngineHandler;
class SettingsManager;

class PICAUDIO_ENGINE_EXPORT DspChainStore : public QObject
{
    Q_OBJECT

public:
    DspChainStore(SettingsManager* settings, DspRegistry* registry, QObject* parent = nullptr);

    void setEngine(EngineHandler* engine);

    [[nodiscard]] Engine::DspChain availableDsps() const;
    [[nodiscard]] Engine::DspChains activeChain() const;
    [[nodiscard]] QString activePreset() const;
    [[nodiscard]] std::unique_ptr<DspNode> createDsp(const QString& id) const;

    void setActiveChain(const Engine::DspChains& chain, QString preset = {});
    void syncActiveChain(const Engine::DspChains& chain, QString preset = {});
    bool updateLiveDspSettings(Engine::DspChainScope scope, uint64_t instanceId, const QByteArray& settings,
                               bool persist, QObject* source = nullptr);
    bool setDspEnabled(Engine::DspChainScope scope, uint64_t instanceId, bool enabled, QObject* source = nullptr);

Q_SIGNALS:
    void activeChainChanged(const PicAudioEngine::Engine::DspChains& chain);
    void liveDspSettingsChanged(PicAudioEngine::Engine::DspChainScope scope, uint64_t instanceId, const QByteArray& settings,
                                bool persisted, QObject* source);
    void dspEnabledChanged(PicAudioEngine::Engine::DspChainScope scope, uint64_t instanceId, bool enabled, QObject* source);

private:
    [[nodiscard]] uint64_t nextInstanceId();
    void noteExistingInstanceId(uint64_t instanceId);
    [[nodiscard]] uint64_t nextLiveRevision(Engine::DspChainScope scope, uint64_t instanceId);

    void loadFromSettings();
    void persistChain(const Engine::DspChains& chain);
    void persistActivePreset();
    [[nodiscard]] Engine::DspChains normaliseChain(const Engine::DspChains& chain);

    SettingsManager* m_settings;
    DspRegistry* m_registry;
    EngineHandler* m_engine;

    Engine::DspChains m_activeChain;
    QString m_activePreset;
    uint64_t m_nextInstanceId;
    std::unordered_map<uint64_t, uint64_t> m_liveRevisionByKey;
};
} // namespace PicAudioEngine
