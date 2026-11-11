/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/track.h>

#include <QString>

#include <vector>

namespace PicAudioEngine::PlaybackOrder {
using IndexGroup  = std::vector<int>;
using IndexGroups = std::vector<IndexGroup>;

std::vector<int> shuffledTrackIndexes(int trackCount, int anchorIndex = -1);
IndexGroups groupedTrackIndexes(const TrackList& tracks, const QString& groupScript, const QString& sortScript);
IndexGroups shuffledTrackGroups(IndexGroups groups, int anchorIndex = -1);
} // namespace PicAudioEngine::PlaybackOrder
