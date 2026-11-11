/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audioloader.h>
#include <core/engine/conversion/conversionrunner.h>

#include <QByteArray>

#include <functional>
#include <memory>

namespace PicAudioEngine {
enum class AudioVerificationStatus : uint8_t
{
    Succeeded = 0,
    Failed,
    Cancelled,
};

struct AudioVerificationResult
{
    Track track;
    AudioVerificationStatus status{AudioVerificationStatus::Failed};
    AudioFormat format;
    uint64_t decodedFrames{0};
    QByteArray md5;
    uint32_t crc32{0};
    QString error;
    QStringList warnings;
};

struct AudioVerificationProgress
{
    int trackIndex{0};
    int trackCount{0};
    Track track;
    uint64_t positionMs{0};
};

namespace AudioVerifier {
using ProgressCallback = std::function<void(const AudioVerificationProgress&)>;
using CancelCallback   = std::function<bool()>;

struct Request
{
    AudioLoader* audioLoader{nullptr};
    TrackList tracks;
    bool verifyIntegrity{true};
    ProgressCallback progressCallback;
    CancelCallback cancelCallback;
    std::shared_ptr<ConversionInputObserver> observer;
};

[[nodiscard]] PICAUDIO_ENGINE_EXPORT std::vector<AudioVerificationResult> run(const Request& request);
} // namespace AudioVerifier
} // namespace PicAudioEngine
