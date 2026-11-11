/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <utils/starrating.h>

#include <QWidget>

namespace PicAudioEngine {
class PICAUDIO_UTILS_EXPORT StarEditor : public QWidget
{
    Q_OBJECT

public:
    explicit StarEditor(Qt::Alignment align, QWidget* parent = nullptr);

    [[nodiscard]] StarRating rating() const;
    void setRating(const StarRating& rating);

    [[nodiscard]] float ratingAtPosition(int x) const;
    [[nodiscard]] static float ratingAtPosition(const QPoint& pos, const QRect& rect, const StarRating& rating,
                                                Qt::Alignment align = Qt::AlignLeft);

    [[nodiscard]] QSize sizeHint() const override;

Q_SIGNALS:
    void editingFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    StarRating m_rating;
    float m_originalRating;
    Qt::Alignment m_align;
};
} // namespace PicAudioEngine
