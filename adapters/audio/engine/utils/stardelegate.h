/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QStyledItemDelegate>

namespace PicAudioEngine {
class PICAUDIO_UTILS_EXPORT StarDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum Role
    {
        MixedValues = Qt::UserRole + 100
    };

    using QStyledItemDelegate::QStyledItemDelegate;

    [[nodiscard]] QModelIndex hoveredIndex() const;
    void setHoverIndex(const QModelIndex& index);
    void setHoverIndex(const QModelIndex& index, const QPoint& pos, const QModelIndexList& selected);

    void setHoveredRow(const QModelIndex& index);
    void setShowEmptyOnlyOnActiveRow(bool enabled);

    static QRect contentRect(const QRect& rect, const QWidget* widget);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    [[nodiscard]] QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
    void finishEditing();

    QPersistentModelIndex m_hoverIndex;
    QPersistentModelIndex m_hoveredRow;
    QPoint m_hoverPos;
    QModelIndexList m_selected;
    bool m_showEmptyOnlyOnActiveRow{false};
};
} // namespace PicAudioEngine
