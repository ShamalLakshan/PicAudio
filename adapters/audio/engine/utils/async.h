/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <QtConcurrentRun>

namespace PicAudioEngine::Utils {
template <typename Func>
auto asyncExec(Func&& func)
{
    return QtConcurrent::run(std::forward<Func>(func));
}
} // namespace PicAudioEngine::Utils
