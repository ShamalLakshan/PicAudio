/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <QTemporaryFile>

namespace PicAudioEngine {
class OutputTransaction
{
public:
    explicit OutputTransaction(QString destination);

    [[nodiscard]] QString path() const;

    bool prepare(QString& error);
    bool commit(QString& error);

private:
    QString m_destination;
    QTemporaryFile m_temporary;
};
} // namespace PicAudioEngine
