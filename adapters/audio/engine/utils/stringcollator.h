/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QSortFilterProxyModel>
#include <QString>

namespace PicAudioEngine {
class StringCollatorPrivate;

class PICAUDIO_UTILS_EXPORT StringCollator
{
public:
    StringCollator();
    ~StringCollator();

    [[nodiscard]] Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity sensitivity);

    [[nodiscard]] bool numericMode() const;
    void setNumericMode(bool enabled);

    [[nodiscard]] int compare(QStringView s1, QStringView s2) const;

private:
    std::unique_ptr<StringCollatorPrivate> p;
};

class PICAUDIO_UTILS_EXPORT StringSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit StringSortProxyModel(QObject* parent = nullptr);

protected:
    [[nodiscard]] bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    mutable StringCollator m_collator;
};
} // namespace PicAudioEngine
