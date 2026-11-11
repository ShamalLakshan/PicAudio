/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <QDataStream>
#include <QMetaType>

#include <cstdint>

namespace PicAudioEngine::Engine {
//! Fade envelope curve used by output and transition fades.
enum class FadeCurve : uint8_t
{
    Linear = 0,
    Sine,
    Cosine,
    Logarithmic,
    Exponential,
    SmootherStep
};

enum class FadeDirection : uint8_t
{
    In = 0,
    Out
};

//! Returns unit-interval curve shape S(p) where S(0)=0 and S(1)=1.
PICAUDIO_ENGINE_EXPORT double shape01(double progress, FadeCurve curve) noexcept;
//! Applies fade direction policy using same-feel semantics: in=S(p), out=S(1-p).
PICAUDIO_ENGINE_EXPORT double gain01(double progress, FadeCurve curve, FadeDirection direction) noexcept;

//! Fade-in/fade-out durations, envelope curve, and enabled state.
struct FadeSpec
{
    int in{1000};
    int out{1000};
    FadeCurve curve{FadeCurve::Linear};
    bool enabled{true};

    [[nodiscard]] constexpr int effectiveInMs() const noexcept
    {
        return enabled ? in : 0;
    }

    [[nodiscard]] constexpr int effectiveOutMs() const noexcept
    {
        return enabled ? out : 0;
    }

    [[nodiscard]] constexpr bool isConfigured() const noexcept
    {
        return effectiveInMs() > 0 || effectiveOutMs() > 0;
    }
};

//! Fading policy group used for transport operations.
struct FadingValues
{
    FadeSpec pause{.in = 120, .out = 120, .curve = FadeCurve::Linear};
    FadeSpec stop{.in = 120, .out = 300, .curve = FadeCurve::Linear};
    FadeSpec boundary{.in = 700, .out = 700, .curve = FadeCurve::Linear, .enabled = false};
};

//! Crossfade policy for manual/automatic track changes and seek transitions.
struct CrossfadingValues
{
    FadeSpec manualChange{.in = 300, .out = 300, .curve = FadeCurve::Linear};
    FadeSpec autoChange{.in = 700, .out = 700, .curve = FadeCurve::Linear};
    FadeSpec seek{.in = 120, .out = 120, .curve = FadeCurve::Linear};
};

PICAUDIO_ENGINE_EXPORT QDataStream& operator<<(QDataStream& stream, const FadeSpec& fading);
PICAUDIO_ENGINE_EXPORT QDataStream& operator>>(QDataStream& stream, FadeSpec& fading);
PICAUDIO_ENGINE_EXPORT QDataStream& operator<<(QDataStream& stream, const FadingValues& fading);
PICAUDIO_ENGINE_EXPORT QDataStream& operator>>(QDataStream& stream, FadingValues& fading);
PICAUDIO_ENGINE_EXPORT QDataStream& operator<<(QDataStream& stream, const CrossfadingValues& fading);
PICAUDIO_ENGINE_EXPORT QDataStream& operator>>(QDataStream& stream, CrossfadingValues& fading);
} // namespace PicAudioEngine::Engine

Q_DECLARE_METATYPE(PicAudioEngine::Engine::FadeCurve)
Q_DECLARE_METATYPE(PicAudioEngine::Engine::FadeSpec)
Q_DECLARE_METATYPE(PicAudioEngine::Engine::FadingValues)
Q_DECLARE_METATYPE(PicAudioEngine::Engine::CrossfadingValues)
