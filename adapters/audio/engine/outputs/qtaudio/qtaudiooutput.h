#pragma once

#include <core/engine/audiooutput.h>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <memory>

namespace PicAudioEngine {

class QtAudioOutput : public AudioOutput
{
    Q_OBJECT

public:
    QtAudioOutput();
    ~QtAudioOutput() override;

    bool init(const AudioFormat& format) override;
    void uninit() override;
    void reset() override;
    void drain() override;
    void start() override;

    [[nodiscard]] bool initialised() const override;
    [[nodiscard]] AudioFormat format() const override { return m_format; }
    [[nodiscard]] QString device() const override;
    [[nodiscard]] int bufferSize() const override;
    OutputState currentState() override;
    [[nodiscard]] OutputDevices getAllDevices(bool isCurrentOutput) override;

    int write(std::span<const std::byte> data, int frameCount) override;
    void setPaused(bool pause) override;
    [[nodiscard]] bool supportsVolumeControl() const override;
    void setVolume(double volume) override;
    void setDevice(const QString& device) override;

private:
    AudioFormat m_format;
    QAudioFormat m_qtFormat;
    QString m_deviceName;
    std::unique_ptr<QAudioSink> m_sink;
    QIODevice* m_ioDevice{nullptr};
    bool m_initialised{false};
    int m_bufferSizeFrames{4096};
};

} // namespace PicAudioEngine
