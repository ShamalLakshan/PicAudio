/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "enginehelpers.h"

#include <core/track.h>

namespace PicAudioEngine {
bool sameTrackIdentity(const Track& lhs, const Track& rhs)
{
    return lhs.sameIdentityAs(rhs);
}

bool sameTrackSegment(const Track& lhs, const Track& rhs)
{
    if(!lhs.isValid() || !rhs.isValid()) {
        return false;
    }

    return lhs.filepath() == rhs.filepath() && lhs.subsong() == rhs.subsong() && lhs.offset() == rhs.offset()
        && lhs.duration() == rhs.duration();
}

bool isContiguousSameFileSegment(const Track& currentTrack, const Track& nextTrack)
{
    if(!currentTrack.isValid() || !nextTrack.isValid()) {
        return false;
    }

    return currentTrack.isSameStreamSegment() && nextTrack.isSameStreamSegment()
        && currentTrack.segmentType() == nextTrack.segmentType() && nextTrack.filepath() == currentTrack.filepath()
        && nextTrack.offset() == currentTrack.offset() + currentTrack.duration();
}

bool isMultiTrackFileTransition(const Track& currentTrack, const Track& nextTrack)
{
    if(!currentTrack.isValid() || !nextTrack.isValid()) {
        return false;
    }

    if(nextTrack.filepath() != currentTrack.filepath()) {
        return false;
    }

    // Same physical file, but different logical segment metadata.
    return !sameTrackSegment(currentTrack, nextTrack);
}

uint64_t relativeTrackPositionMs(uint64_t absolutePosMs, uint64_t offsetMs)
{
    return absolutePosMs >= offsetMs ? absolutePosMs - offsetMs : 0;
}

uint64_t absoluteTrackPositionMs(uint64_t streamPosMs, uint64_t streamOriginMs)
{
    return saturatingAdd(streamOriginMs, streamPosMs);
}

uint64_t relativeTrackPositionMs(uint64_t streamPosMs, uint64_t streamOriginMs, uint64_t trackOffsetMs)
{
    return relativeTrackPositionMs(absoluteTrackPositionMs(streamPosMs, streamOriginMs), trackOffsetMs);
}

bool shouldEmitTrackEndOnce(Track& lastEndedTrack, const Track& currentTrack)
{
    if(sameTrackSegment(lastEndedTrack, currentTrack)) {
        return false;
    }

    lastEndedTrack = currentTrack;
    return true;
}

AudioFormat normaliseChannelLayout(const AudioFormat& format)
{
    if(!format.isValid()) {
        return format;
    }

    const int channels = format.channelCount();
    if(channels <= 0 || channels > 8) {
        return format;
    }

    bool needsInference = !format.hasChannelLayout();
    if(!needsInference) {
        for(int i{0}; i < channels; ++i) {
            if(format.channelPosition(i) == PicAudioEngine::AudioFormat::ChannelPosition::UnknownPosition) {
                needsInference = true;
                break;
            }
        }
    }

    if(!needsInference) {
        return format;
    }

    auto inferred{format};
    const auto fallback = PicAudioEngine::AudioFormat::defaultChannelLayoutForChannelCount(channels);
    inferred.setChannelLayout(fallback);

    return inferred;
}
} // namespace PicAudioEngine
