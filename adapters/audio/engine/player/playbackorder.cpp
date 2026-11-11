/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "playbackorder.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <ranges>
#include <unordered_map>

namespace PicAudioEngine::PlaybackOrder {
std::vector<int> shuffledTrackIndexes(int trackCount, int anchorIndex)
{
    if(trackCount <= 0) {
        return {};
    }

    std::vector<int> indexes(static_cast<size_t>(trackCount));
    std::iota(indexes.begin(), indexes.end(), 0);
    std::ranges::shuffle(indexes, std::mt19937{std::random_device{}()});

    if(anchorIndex >= 0 && anchorIndex < trackCount) {
        const auto anchor = std::ranges::find(indexes, anchorIndex);
        std::rotate(indexes.begin(), anchor, std::next(anchor));
    }

    return indexes;
}

IndexGroups groupedTrackIndexes(const TrackList& tracks, const QString& groupScript, const QString& sortScript)
{
    if(tracks.empty()) {
        return {};
    }

    IndexGroups groups;
    std::unordered_map<QString, size_t> groupIndexes;
    groupIndexes.reserve(tracks.size());

    for(int index{0}; std::cmp_less(index, tracks.size()); ++index) {
        const auto& tr = tracks.at(index);
        QString group;
        if(!groupScript.isEmpty()) {
            if(groupScript.contains("artist", Qt::CaseInsensitive)) {
                group = tr.artist();
            } else {
                group = tr.album();
            }
        }
        if(group.isEmpty()) {
            group = tr.album();
            if(group.isEmpty()) {
                group = tr.artist();
            }
        }
        auto [it, inserted] = groupIndexes.try_emplace(group, groups.size());
        if(inserted) {
            groups.emplace_back();
        }
        groups.at(it->second).push_back(index);
    }

    if(!sortScript.isEmpty()) {
        for(auto& group : groups) {
            std::ranges::sort(group, [&tracks](int a, int b) {
                const auto& trA = tracks.at(a);
                const auto& trB = tracks.at(b);
                if(trA.discNumber() != trB.discNumber()) {
                    return trA.discNumber() < trB.discNumber();
                }
                if(trA.trackNumber() != trB.trackNumber()) {
                    return trA.trackNumber() < trB.trackNumber();
                }
                return trA.title() < trB.title();
            });
        }
    }

    return groups;
}

IndexGroups shuffledTrackGroups(IndexGroups groups, const int anchorIndex)
{
    std::ranges::shuffle(groups, std::mt19937{std::random_device{}()});

    if(anchorIndex >= 0) {
        const auto anchor = std::ranges::find_if(
            groups, [anchorIndex](const auto& group) { return std::ranges::find(group, anchorIndex) != group.end(); });
        if(anchor != groups.end()) {
            std::rotate(groups.begin(), anchor, std::next(anchor));
        }
    }

    return groups;
}
} // namespace PicAudioEngine::PlaybackOrder
