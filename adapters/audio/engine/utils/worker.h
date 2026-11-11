/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QObject>

#include <atomic>
#include <mutex>
#include <stop_token>

namespace PicAudioEngine {
class PICAUDIO_UTILS_EXPORT Worker : public QObject
{
    Q_OBJECT

public:
    enum State
    {
        Idle = 0,
        Running,
        Paused
    };

    explicit Worker(QObject* parent = nullptr);

    virtual void initialiseThread();
    virtual void stopThread();
    virtual void pauseThread();
    virtual void closeThread();

    [[nodiscard]] State state() const;
    void setState(State state);

    // Jobs share a cancellation batch until stopThread()/pauseThread()
    [[nodiscard]] std::stop_token prepareWork();
    // Call on the worker thread before entering a queued job
    [[nodiscard]] bool startWork(std::stop_token token);

    [[nodiscard]] bool mayRun() const;
    [[nodiscard]] bool mayRun(std::stop_token token) const;
    [[nodiscard]] bool closing() const;
    [[nodiscard]] std::stop_token stopToken() const;
    [[nodiscard]] bool stopRequested() const;

Q_SIGNALS:
    void finished();

protected:
    void requestStop(State state, bool close = false);

private:
    std::atomic<State> m_state;
    std::atomic<bool> m_closing;
    mutable std::mutex m_stopSourceMutex;
    std::stop_source m_stopSource;
    std::stop_token m_activeStopToken;
};
} // namespace PicAudioEngine
