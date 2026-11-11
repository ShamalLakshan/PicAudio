/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audiobuffer.h>
#include <core/engine/conversion/conversiondefs.h>

#include <QString>

#include <functional>
#include <vector>

namespace PicAudioEngine {
struct EncoderParameterRange
{
    double minimum{0.0};
    double maximum{0.0};
    double step{1.0};

    [[nodiscard]] bool isValid() const
    {
        return maximum > minimum && step > 0.0;
    }
};

struct EncoderCapabilities
{
    std::vector<EncoderMode> modes;
    EncoderParameterRange bitrateKbps;
    EncoderParameterRange quality;
    EncoderParameterRange compressionLevel;
    std::vector<SampleFormat> sampleFormats;
};

struct AudioEncoderInfo
{
    QString id;
    QString backendId;
    QString backendName;
    int backendPriority{0};
    QString name;
    QString description;
    EncoderProfile profile;
    EncoderCapabilities capabilities{};
    std::function<int(const EncoderProfile&)> estimateBitrateKbps;
    bool supportsMetadata{false};
    bool supportsPictures{false};
};

class PICAUDIO_ENGINE_EXPORT AudioEncoder
{
public:
    struct Result
    {
        bool ok{false};
        QString error;

        static Result success()
        {
            return {.ok = true, .error = {}};
        }

        static Result failure(QString message)
        {
            return {.ok = false, .error = std::move(message)};
        }
    };

    virtual ~AudioEncoder() = default;

    [[nodiscard]] virtual std::vector<AudioEncoderInfo> availableEncoders() const = 0;

    virtual Result init(const QString& outputPath, const AudioFormat& inputFormat, const AudioEncoderSettings& settings)
        = 0;
    virtual Result write(const AudioBuffer& buffer) = 0;
    virtual Result finish()                         = 0;
};

using EncoderCreator = std::function<std::unique_ptr<AudioEncoder>()>;
} // namespace PicAudioEngine
