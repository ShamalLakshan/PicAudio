/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <utils/lovesymbols.h>

#include <QColor>
#include <QSharedDataPointer>
#include <QWidget>

#include <optional>

class QFont;

namespace PicAudioEngine {
class HeartValuePrivate;

class PICAUDIO_UTILS_EXPORT HeartValue
{
public:
    enum class EditMode : uint8_t
    {
        Editable,
        ReadOnly
    };

    HeartValue();
    explicit HeartValue(bool loved);
    HeartValue(bool loved, int scale);
    HeartValue(bool loved, int scale, const QColor& colour);
    HeartValue(bool loved, int scale, const QColor& lovedColour, const QColor& unlovedColour,
               std::optional<LoveSymbols> symbols = {});
    ~HeartValue();

    HeartValue(const HeartValue& other);
    HeartValue& operator=(const HeartValue& other);
    HeartValue(HeartValue&& other) noexcept;
    HeartValue& operator=(HeartValue&& other) noexcept;

    [[nodiscard]] bool loved() const;
    [[nodiscard]] int scale() const;

    void setLoved(bool loved);
    void setScale(int scale);
    void setSymbolFont(const QFont& font);

    void paint(QPainter* painter, const QRect& rect, const QPalette& palette, EditMode mode,
               Qt::Alignment alignment = Qt::AlignLeft, bool selected = false) const;
    [[nodiscard]] QSize sizeHint() const;

    operator QVariant() const
    {
        return QVariant::fromValue(*this);
    }

private:
    QSharedDataPointer<HeartValuePrivate> p;
};

class PICAUDIO_UTILS_EXPORT HeartEditor : public QWidget
{
    Q_OBJECT

public:
    explicit HeartEditor(Qt::Alignment align, QWidget* parent = nullptr);

    [[nodiscard]] HeartValue loved() const;
    void setValue(const HeartValue& value);
    [[nodiscard]] QSize sizeHint() const override;

Q_SIGNALS:
    void editingFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    HeartValue m_loved;
    bool m_originaLoved;
    Qt::Alignment m_align;
};
} // namespace PicAudioEngine

Q_DECLARE_METATYPE(PicAudioEngine::HeartValue)
