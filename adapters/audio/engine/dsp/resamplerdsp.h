/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/dsp/dspnode.h>
#include <core/engine/dsp/processingbuffer.h>
#include <core/engine/dsp/resamplersettings.h>

#include <cstdint>
#include <set>

struct SwrContext;

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT ResamplerDsp : public DspNode
{
public:
    ResamplerDsp();
    ~ResamplerDsp() override;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString id() const override;

    [[nodiscard]] AudioFormat outputFormat(const AudioFormat& input) const override;
    void prepare(const AudioFormat& format) override;
    void process(ProcessingBufferList& chunks) override;
    void reset() override;
    void flush(ProcessingBufferList& chunks, FlushMode mode) override;
    [[nodiscard]] int latencyFrames() const override;

    [[nodiscard]] QByteArray saveSettings() const override;
    bool loadSettings(const QByteArray& preset) override;

    bool setTargetSampleRate(int sampleRate) override;
    [[nodiscard]] int targetSampleRate() const;

    void setUseSoxr(bool useSoxr);
    [[nodiscard]] bool useSoxr() const;

    void setSoxrPrecision(double precision);
    [[nodiscard]] double soxrPrecision() const;

    void setSampleRateFilterMode(ResamplerSettings::SampleRateFilterMode mode);
    [[nodiscard]] ResamplerSettings::SampleRateFilterMode sampleRateFilterMode() const;

    void setFilteredRatesText(const QString& text);
    [[nodiscard]] QString filteredRatesText() const;

private:
    void recreateContext();
    bool createContext();
    void destroyContext();

    void processBuffer(const ProcessingBuffer& buffer, ProcessingBufferList& output);
    [[nodiscard]] bool shouldResampleInputRate(int inputRate) const;
    void advanceOutputCursor(int outFrames, uint64_t sourceFrameDurationNs);

    SwrContext* m_swr;
    AudioFormat m_inputFormat;
    AudioFormat m_outputFormat;
    bool m_hasOutputCursor;
    uint64_t m_outputCursorNs;
    double m_outputCursorRemainderNs;
    uint64_t m_outputSourceFrameDurationNs;

    int m_targetRate;
    bool m_useSoxr;
    double m_soxrPrecision;
    ResamplerSettings::SampleRateFilterMode m_sampleRateFilterMode;
    std::set<int> m_filteredRates;
    QString m_filteredRatesText;
};
} // namespace PicAudioEngine
