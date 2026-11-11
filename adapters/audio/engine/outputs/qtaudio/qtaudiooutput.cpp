#include "qtaudiooutput.h"

#include <cmath>
#include <cstring>

namespace Fooyin {

namespace {
QAudioFormat::SampleFormat toQtSampleFormat(SampleFormat fmt)
{
    switch(fmt) {
    case SampleFormat::U8:
        return QAudioFormat::UInt8;
    case SampleFormat::S16:
        return QAudioFormat::Int16;
    case SampleFormat::S32:
    case SampleFormat::S24In32:
        return QAudioFormat::Int32;
    case SampleFormat::F32:
    case SampleFormat::F64:
        return QAudioFormat::Float;
    default:
        return QAudioFormat::Unknown;
    }
}
} // namespace

QtAudioOutput::QtAudioOutput() = default;

QtAudioOutput::~QtAudioOutput()
{
    uninit();
}

bool QtAudioOutput::init(const AudioFormat& format)
{
    uninit();
    m_format = format;

    m_qtFormat.setSampleRate(format.sampleRate());
    m_qtFormat.setChannelCount(format.channelCount());
    m_qtFormat.setSampleFormat(toQtSampleFormat(format.sampleFormat()));

    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if(!m_deviceName.isEmpty()) {
        for(const auto& dev : QMediaDevices::audioOutputs()) {
            if(dev.id() == m_deviceName.toUtf8() || dev.description() == m_deviceName) {
                device = dev;
                break;
            }
        }
    }

    if(!device.isFormatSupported(m_qtFormat)) {
        m_qtFormat = device.preferredFormat();
    }

    m_sink = std::make_unique<QAudioSink>(device, m_qtFormat);
    m_bufferSizeFrames = std::max(2048, format.sampleRate() / 10);
    m_sink->setBufferSize(m_bufferSizeFrames * format.bytesPerFrame());
    m_ioDevice = m_sink->start();
    m_initialised = (m_ioDevice != nullptr);

    return m_initialised;
}

void QtAudioOutput::uninit()
{
    if(m_sink) {
        m_sink->stop();
        m_sink.reset();
    }
    m_ioDevice = nullptr;
    m_initialised = false;
}

void QtAudioOutput::reset()
{
    if(m_sink) {
        m_sink->reset();
        m_ioDevice = m_sink->start();
    }
}

void QtAudioOutput::drain()
{
    // QAudioSink drains naturally when bytes are consumed
}

void QtAudioOutput::start()
{
    if(m_sink && m_sink->state() == QAudio::SuspendedState) {
        m_sink->resume();
    }
}

bool QtAudioOutput::initialised() const
{
    return m_initialised;
}

QString QtAudioOutput::device() const
{
    return m_deviceName;
}

int QtAudioOutput::bufferSize() const
{
    return m_bufferSizeFrames;
}

OutputState QtAudioOutput::currentState()
{
    OutputState state;
    if(!m_sink || !m_initialised) {
        return state;
    }

    const int bytesFree = m_sink->bytesFree();
    const int bpf = m_format.bytesPerFrame();
    if(bpf > 0) {
        state.freeFrames = bytesFree / bpf;
        const int queuedBytes = m_sink->bufferSize() - bytesFree;
        state.queuedFrames = std::max(0, queuedBytes / bpf);
        state.delay = static_cast<double>(state.queuedFrames) / m_format.sampleRate();
    }
    return state;
}

OutputDevices QtAudioOutput::getAllDevices(bool /*isCurrentOutput*/)
{
    OutputDevices devices;
    const auto list = QMediaDevices::audioOutputs();
    for(const auto& dev : list) {
        devices.push_back({dev.description(), dev.description()});
    }
    return devices;
}

int QtAudioOutput::write(std::span<const std::byte> data, int frameCount)
{
    if(!m_ioDevice || !m_sink || frameCount <= 0) {
        return 0;
    }

    const int bpf = m_format.bytesPerFrame();
    const qint64 bytesToWrite = static_cast<qint64>(frameCount) * bpf;
    const qint64 bytesWritten = m_ioDevice->write(reinterpret_cast<const char*>(data.data()), bytesToWrite);

    if(bytesWritten > 0 && bpf > 0) {
        return static_cast<int>(bytesWritten / bpf);
    }
    return 0;
}

void QtAudioOutput::setPaused(bool pause)
{
    if(!m_sink) {
        return;
    }
    if(pause && m_sink->state() == QAudio::ActiveState) {
        m_sink->suspend();
    } else if(!pause && m_sink->state() == QAudio::SuspendedState) {
        m_sink->resume();
    }
}

bool QtAudioOutput::supportsVolumeControl() const
{
    return true;
}

void QtAudioOutput::setVolume(double volume)
{
    if(m_sink) {
        m_sink->setVolume(static_cast<float>(std::clamp(volume, 0.0, 1.0)));
    }
}

void QtAudioOutput::setDevice(const QString& device)
{
    m_deviceName = device;
}

} // namespace Fooyin
