/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/conversion/conversionrunner.h>

#include <QByteArray>
#include <QUrl>

#include <deque>
#include <expected>
#include <mutex>
#include <optional>
#include <vector>

namespace PicAudioEngine::AccurateRip {
constexpr auto FramesPerSector  = 588;
constexpr auto SectorsPerSecond = 75;
constexpr auto LeadInSectors    = 150;
constexpr auto MaximumTracks    = 99;
constexpr auto MaximumOffset    = (5 * FramesPerSector) - 1;

struct TrackLayout
{
    uint32_t firstSector{0};
    uint32_t endSectorExclusive{0};

    bool operator==(const TrackLayout&) const = default;
};

struct DiscLayout
{
    std::vector<TrackLayout> tracks;
    uint32_t leadoutSector{0};

    bool operator==(const DiscLayout&) const = default;
};

struct DiscId
{
    uint32_t id1{0};
    uint32_t id2{0};
    uint32_t cddbId{0};
    int trackCount{0};

    bool operator==(const DiscId&) const = default;
};

struct Checksum
{
    uint32_t confidence{0};
    uint32_t crc{0};
    uint32_t offsetChecksum{0};
};
using Pressing = std::vector<Checksum>;

enum class VerifyStatus : uint8_t
{
    Verified = 0,
    Mismatch,
    Incomplete,
    InvalidFormat,
};

struct TrackResult
{
    Track track;
    VerifyStatus status{VerifyStatus::Incomplete};
    uint32_t crcV1{0};
    uint32_t crcV2{0};
    int confidence{0};
    int sampleOffset{0};
    std::vector<uint32_t> databaseCrcs;
};

struct OffsetMatch
{
    int sampleOffset{0};
    int matchingTracks{0};
    int confidence{0};
};

PICAUDIO_ENGINE_EXPORT std::expected<void, QString> validateLayout(const DiscLayout& layout);
PICAUDIO_ENGINE_EXPORT std::expected<DiscId, QString> discId(const DiscLayout& layout);
PICAUDIO_ENGINE_EXPORT QUrl discUrl(const DiscId& id);
PICAUDIO_ENGINE_EXPORT std::expected<std::vector<Pressing>, QString> parseResponse(const QByteArray& data,
                                                                          const DiscId& expected);
PICAUDIO_ENGINE_EXPORT std::expected<TrackList, QString> prepareAlbumTracks(const TrackList& tracks);

class PICAUDIO_ENGINE_EXPORT Verifier : public ConversionInputObserver
{
public:
    Verifier(DiscLayout layout, std::vector<Pressing> pressings, TrackList tracks);
    ~Verifier() override;

    void trackStarted(const Track& track, const AudioFormat& format) override;
    void sourceAudio(const Track& track, const AudioBuffer& buffer) override;
    void trackFinished(const Track& track, bool complete) override;

    [[nodiscard]] std::vector<TrackResult> results() const;

private:
    struct State
    {
        Track track;
        size_t discTrackIndex{0};
        uint64_t totalFrames{0};
        uint64_t position{0};
        uint32_t crcV1{0};
        uint32_t crcV2{0};
        bool validFormat{false};
        bool complete{false};
    };

    State* stateFor(const Track& track);
    [[nodiscard]] std::optional<size_t> discTrackIndex(const Track& track) const;

    DiscLayout m_layout;
    std::vector<Pressing> m_pressings;
    TrackList m_tracks;
    std::vector<State> m_states;
    mutable std::mutex m_mutex;
};

//! Collects AccurateRip checksums and exact CD-sector lengths when no physical TOC is available.
class PICAUDIO_ENGINE_EXPORT AlbumVerifier : public ConversionInputObserver
{
public:
    explicit AlbumVerifier(TrackList tracks);
    ~AlbumVerifier() override;

    void trackStarted(const Track& track, const AudioFormat& format) override;
    void sourceAudio(const Track& track, const AudioBuffer& buffer) override;
    void trackFinished(const Track& track, bool complete) override;

    [[nodiscard]] std::expected<DiscLayout, QString> layout() const;
    [[nodiscard]] OffsetMatch bestOffset(const std::vector<Pressing>& pressings) const;
    [[nodiscard]] std::vector<TrackResult> results(const std::vector<Pressing>& pressings) const;

private:
    struct Contribution
    {
        uint32_t crcV1{0};
        uint32_t crcV2{0};
    };
    struct State
    {
        Track track;
        size_t discTrackIndex{0};
        uint64_t position{0};
        uint32_t crcV1{0};
        uint32_t crcV2{0};
        std::deque<Contribution> trailing;
        std::vector<uint32_t> offsetWindow;
        bool validFormat{false};
        bool complete{false};
    };

    State* stateFor(const Track& track);
    [[nodiscard]] std::optional<size_t> discTrackIndex(const Track& track) const;

    TrackList m_tracks;
    std::vector<State> m_states;
    mutable std::mutex m_mutex;
};

//! Calculates full AccurateRip checksums after a pressing offset has been identified.
class PICAUDIO_ENGINE_EXPORT OffsetVerifier : public ConversionInputObserver
{
public:
    OffsetVerifier(DiscLayout layout, std::vector<Pressing> pressings, TrackList tracks, int sampleOffset);
    ~OffsetVerifier() override;

    void trackStarted(const Track& track, const AudioFormat& format) override;
    void sourceAudio(const Track& track, const AudioBuffer& buffer) override;
    void trackFinished(const Track& track, bool complete) override;

    [[nodiscard]] std::vector<TrackResult> results() const;

private:
    struct State
    {
        Track track;
        uint64_t sourcePosition{0};
        uint32_t crcV1{0};
        uint32_t crcV2{0};
        bool validFormat{false};
        bool complete{false};
    };

    [[nodiscard]] std::optional<size_t> trackIndex(const Track& track) const;

    DiscLayout m_layout;
    std::vector<Pressing> m_pressings;
    TrackList m_tracks;
    int m_sampleOffset{0};
    std::vector<State> m_states;
    mutable std::mutex m_mutex;
};
} // namespace PicAudioEngine::AccurateRip
