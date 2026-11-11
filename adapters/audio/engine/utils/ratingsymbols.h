/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <QString>

namespace PicAudioEngine {
struct RatingStarSymbols
{
    QString fullStarSymbol;
    QString halfStarSymbol;
    QString emptyStarSymbol;

    friend bool operator==(const RatingStarSymbols&, const RatingStarSymbols&) = default;
};

[[nodiscard]] inline const RatingStarSymbols& defaultRatingStarSymbols()
{
    static const RatingStarSymbols Symbols{
        .fullStarSymbol  = QString{QChar{u'\u2605'}},
        .halfStarSymbol  = QString{QChar{u'\u2BEA'}},
        .emptyStarSymbol = QString{QChar{u'\u2606'}},
    };
    return Symbols;
}

[[nodiscard]] inline const QString& defaultRatingFullStarSymbol()
{
    return defaultRatingStarSymbols().fullStarSymbol;
}

[[nodiscard]] inline const QString& defaultRatingHalfStarSymbol()
{
    return defaultRatingStarSymbols().halfStarSymbol;
}

[[nodiscard]] inline const QString& defaultRatingEmptyStarSymbol()
{
    return defaultRatingStarSymbols().emptyStarSymbol;
}
} // namespace PicAudioEngine
