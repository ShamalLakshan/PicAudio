/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <cmath>
#include <cstdlib>

namespace PicAudioEngine::Math {
#if (defined(__GNUC__) && defined(__x86_64__))
#include <emmintrin.h>
inline int32_t fltToInt(float flt)
{
    return _mm_cvtss_si32(_mm_load_ss(&flt));
}

inline int32_t fltToInt(double flt)
{
    return _mm_cvtsd_si32(_mm_load_sd(&flt));
}
#else
inline int32_t fltToInt(float flt)
{
    return static_cast<int32_t>(std::floor(flt + 0.5));
}

inline int32_t fltToInt(double flt)
{
    return static_cast<int32_t>(std::floor(flt + 0.5));
}
#endif
} // namespace PicAudioEngine::Math
