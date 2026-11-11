/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QBasicTimer>
#include <QObject>

namespace PicAudioEngine {
class PICAUDIO_UTILS_EXPORT SignalThrottler : public QObject
{
    Q_OBJECT

public:
    explicit SignalThrottler(QObject* parent = nullptr);
    ~SignalThrottler() override;

    [[nodiscard]] bool isActive() const;

    [[nodiscard]] int timeout() const;
    void setTimeout(int timeout);
    void setTimeout(std::chrono::milliseconds timeout);

    [[nodiscard]] Qt::TimerType timerType() const;
    void setTimerType(Qt::TimerType timerType);

Q_SIGNALS:
    void triggered();
    void timeoutChanged(int timeout);
    void timerTypeChanged(Qt::TimerType timerType);

public Q_SLOTS:
    void throttle();

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    void maybeEmitTriggered();
    void emitTriggered();

    QBasicTimer m_timer;
    int m_timeout;
    Qt::TimerType m_timerType;
    bool m_pendingEmit;
};
} // namespace PicAudioEngine
