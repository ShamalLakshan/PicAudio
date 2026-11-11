/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audioencoderregistry.h>
#include <core/engine/audioloader.h>
#include <core/engine/conversion/conversionpathresolver.h>

#include <QStringList>

#include <functional>

namespace PicAudioEngine {
class DspRegistry;

enum class ConversionResultStatus : uint8_t
{
    Succeeded = 0,
    Skipped,
    Failed,
    Cancelled,
};

struct ConversionTrackResult
{
    Track sourceTrack;
    QString outputPath;
    ConversionResultStatus status{ConversionResultStatus::Failed};
    QString error;
    QStringList warnings;
};

struct ConversionProgress
{
    int trackIndex{0};
    int trackCount{0};
    QString sourcePath;
    QString outputPath;
    uint64_t sourcePositionMs{0};
    uint64_t sourceDurationMs{0};
    bool finished{false};
};

class PICAUDIO_ENGINE_EXPORT ConversionInputObserver
{
public:
    virtual ~ConversionInputObserver()                                       = default;
    virtual void trackStarted(const Track& track, const AudioFormat& format) = 0;
    virtual void sourceAudio(const Track& track, const AudioBuffer& buffer)  = 0;
    virtual void trackFinished(const Track& track, bool complete)            = 0;
};

namespace ConversionRunner {
using ProgressCallback     = std::function<void(const ConversionProgress&)>;
using CancelCallback       = std::function<bool()>;
using ExistingFileCallback = std::function<ExistingFileMode(const QString&)>;

struct Request
{
    AudioLoader* audioLoader{nullptr};
    AudioEncoderRegistry* encoderRegistry{nullptr};
    DspRegistry* dspRegistry{nullptr};
    ConversionJob job;
    QString askFolder;
    ProgressCallback progressCallback;
    CancelCallback cancelCallback;
    ExistingFileCallback existingFileCallback;
    std::shared_ptr<ConversionInputObserver> sourceObserver;
    int maxParallelConversions{0};
};

PICAUDIO_ENGINE_EXPORT std::vector<ConversionTrackResult> run(const Request& request);
}; // namespace ConversionRunner
} // namespace PicAudioEngine
