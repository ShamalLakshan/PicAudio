#pragma once

namespace PicAudioEngine {

class NetworkAccessManager
{
public:
    enum class Mode : int
    {
        None = 0,
        Manual,
        System
    };
};

} // namespace PicAudioEngine
