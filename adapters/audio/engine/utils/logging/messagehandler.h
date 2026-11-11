#pragma once

#include <QtCore/qlogging.h>

namespace PicAudioEngine::MessageHandler {

using Level = QtMsgType;

inline void setLevel(Level /*level*/) {}
inline Level level() { return QtInfoMsg; }

} // namespace PicAudioEngine::MessageHandler
