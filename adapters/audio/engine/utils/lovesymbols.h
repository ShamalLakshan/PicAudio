/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <QString>

namespace PicAudioEngine {
struct LoveSymbols
{
    QString lovedSymbol;
    QString unlovedSymbol;

    friend bool operator==(const LoveSymbols&, const LoveSymbols&) = default;
};

inline const LoveSymbols& defaultLoveSymbols()
{
    static const LoveSymbols Symbols{
        .lovedSymbol   = QString{QChar{u'\u2665'}},
        .unlovedSymbol = QString{QChar{u'\u2661'}},
    };
    return Symbols;
}
} // namespace PicAudioEngine
