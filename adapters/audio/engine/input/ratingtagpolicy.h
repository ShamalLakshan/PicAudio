/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <QString>

#include <cstdint>
#include <optional>
#include <span>

namespace PicAudioEngine {
namespace RatingSettings {
constexpr auto ReadTag              = "Rating/ReadTag";
constexpr auto ReadScale            = "Rating/ReadScale";
constexpr auto WriteTag             = "Rating/WriteTag";
constexpr auto WriteScale           = "Rating/WriteScale";
constexpr auto ReadId3Popm          = "Rating/ReadId3Popm";
constexpr auto WriteId3Popm         = "Rating/WriteId3Popm";
constexpr auto PopmOwner            = "Rating/PopmOwner";
constexpr auto PopmMapping          = "Rating/PopmMapping";
constexpr auto ReadAsfSharedRating  = "Rating/ReadAsfSharedRating";
constexpr auto WriteAsfSharedRating = "Rating/WriteAsfSharedRating";

constexpr auto DefaultAutomatic             = "Automatic";
constexpr auto DefaultFmpsRatingTag         = "FMPS_RATING";
constexpr auto DefaultNormalizedRatingScale = "Normalized01";
constexpr auto DefaultPopmMapping           = "Default";
constexpr auto DefaultReadId3Popm           = true;
constexpr auto DefaultWriteId3Popm          = true;
constexpr auto DefaultReadAsfSharedRating   = true;
constexpr auto DefaultWriteAsfSharedRating  = true;
} // namespace RatingSettings

enum class RatingScale : uint8_t
{
    Automatic = 0,
    Normalized01,
    OneToFive,
    OneToTen,
    OneToHundred,
};

enum class PopmMapping : uint8_t
{
    Default = 0,
    CommonFiveStar,
    LinearByte,
};

struct RatingScaleDescriptor
{
    RatingScale scale;
    const char* label;
    bool availableForRead;
    bool availableForWrite;
};

struct PopmMappingDescriptor
{
    PopmMapping mapping;
    const char* label;
};

struct PICAUDIO_ENGINE_EXPORT RatingTagPolicy
{
    QString readTag;
    RatingScale readScale{RatingScale::Automatic};
    QString writeTag;
    RatingScale writeScale{RatingScale::Automatic};
    bool readId3Popm{true};
    bool writeId3Popm{true};
    QString popmOwner;
    PopmMapping popmMapping{PopmMapping::Default};
    bool readAsfSharedRating{true};
    bool writeAsfSharedRating{true};

    [[nodiscard]] bool automaticRead() const;
    [[nodiscard]] bool shouldReadTextTag(const QString& tag, bool currentRatingSet) const;
    [[nodiscard]] bool shouldWriteTextTag() const;
    [[nodiscard]] QString effectiveWriteTag() const;
};

[[nodiscard]] PICAUDIO_ENGINE_EXPORT std::span<const RatingScaleDescriptor> ratingScaleDescriptors();
[[nodiscard]] PICAUDIO_ENGINE_EXPORT std::span<const PopmMappingDescriptor> popmMappingDescriptors();
[[nodiscard]] PICAUDIO_ENGINE_EXPORT QString ratingScaleKey(RatingScale scale);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT QString popmMappingKey(PopmMapping mapping);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT RatingScale ratingScaleFromString(QString scale);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT PopmMapping popmMappingFromString(QString mapping);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT RatingTagPolicy ratingTagPolicy();
[[nodiscard]] PICAUDIO_ENGINE_EXPORT std::optional<float> normalisedTextRating(const QString& rawRating, RatingScale scale,
                                                                      bool ratingTagFallback);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT QString formatTextRating(float rating, RatingScale scale);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT float asfSharedUserRatingToRating(int rating);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT int ratingToAsfSharedUserRating(float rating);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT float popmToRating(int popm, PopmMapping mapping);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT int ratingToPopm(float rating, PopmMapping mapping);
} // namespace PicAudioEngine
