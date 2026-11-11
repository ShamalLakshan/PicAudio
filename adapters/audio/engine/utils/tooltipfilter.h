/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QObject>

namespace PicAudioEngine {
class PICAUDIO_UTILS_EXPORT ToolTipFilter : public QObject
{
    Q_OBJECT

public:
    explicit ToolTipFilter(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};
} // namespace PicAudioEngine
