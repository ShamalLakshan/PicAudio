#pragma once

#include <QtCore/qlogging.h>

namespace Fooyin::MessageHandler {

using Level = QtMsgType;

inline void setLevel(Level /*level*/) {}
inline Level level() { return QtInfoMsg; }

} // namespace Fooyin::MessageHandler
