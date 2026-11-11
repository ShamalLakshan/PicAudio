/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "ffmpegutils.h"

#include <core/engine/audioencoder.h>

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT FFmpegEncoder : public AudioEncoder
{
public:
    FFmpegEncoder();
    ~FFmpegEncoder() override;

    [[nodiscard]] std::vector<AudioEncoderInfo> availableEncoders() const override;

    Result init(const QString& outputPath, const AudioFormat& inputFormat,
                const AudioEncoderSettings& settings) override;
    Result write(const AudioBuffer& buffer) override;
    Result finish() override;

private:
    Result configureResampler();
    Result encodeInput(const uint8_t* inputData, int inputFrames);
    Result queueConvertedAudio(AVFrame* frame, int frames);
    Result encodeQueuedFrames(bool final);
    Result flushResampler();
    Result prepareFrame(AVFrame* frame, int frames) const;
    Result encodeFrame(AVFrame* frame);
    Result drainPackets();
    void cleanup();

    OutputFormatContextPtr m_formatContext;
    CodecContextPtr m_codecContext;
    AVStream* m_stream;
    SwrContextPtr m_swr;
    FramePtr m_frame;
    FramePtr m_convertedFrame;
    PacketPtr m_packet;
    AudioFifoPtr m_fifo;
    AudioFormat m_inputFormat;
    AVSampleFormat m_inputSampleFormat;
    AVSampleFormat m_outputSampleFormat;
    int m_outputSampleRate;
    int64_t m_nextPts;
    bool m_dither;
    bool m_finished;
};
} // namespace PicAudioEngine
