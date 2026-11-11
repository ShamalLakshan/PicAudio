#pragma once

namespace Fooyin {

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

} // namespace Fooyin
