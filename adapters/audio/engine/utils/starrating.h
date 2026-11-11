/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <utils/ratingsymbols.h>

#include <QColor>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QSize>
#include <QVariant>

#include <array>
#include <optional>

class QFont;
class QPainter;
class QPalette;
class QRect;

namespace PicAudioEngine {
using RatingStarColours = std::array<QColor, 5>;

class StarRatingPrivate;

class PICAUDIO_UTILS_EXPORT StarRating
{
public:
    enum class EditMode : uint8_t
    {
        Editable,
        ReadOnly,
    };

    StarRating();
    StarRating(float rating, int maxStarCount);
    StarRating(float rating, int maxStarCount, int scale);
    StarRating(float rating, int maxStarCount, int scale, const RatingStarColours& colours);
    StarRating(float rating, int maxStarCount, int scale, const RatingStarColours& colours, const QColor& unratedColour,
               std::optional<RatingStarSymbols> symbols = {});
    ~StarRating();

    StarRating(const StarRating& other);
    StarRating& operator=(const StarRating& other);
    StarRating(StarRating&& other) noexcept;
    StarRating& operator=(StarRating&& other) noexcept;

    [[nodiscard]] float rating() const;
    [[nodiscard]] int maxStarCount() const;
    [[nodiscard]] int starScale() const;
    [[nodiscard]] Qt::Orientation orientation() const;

    void setRating(float rating);
    void setMaxStarCount(int maxStarCount);
    void setStarScale(int scale);
    void setOrientation(Qt::Orientation orientation);
    void setSymbolFont(const QFont& font);

    void paint(QPainter* painter, const QRect& rect, const QPalette& palette, EditMode mode,
               Qt::Alignment alignment = Qt::AlignLeft, bool selected = false) const;
    [[nodiscard]] QSize sizeHint() const;

    operator QVariant() const
    {
        return QVariant::fromValue(*this);
    }

private:
    QSharedDataPointer<StarRatingPrivate> p;
};
} // namespace PicAudioEngine

Q_DECLARE_METATYPE(PicAudioEngine::StarRating)
