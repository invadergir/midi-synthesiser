/**
 * Stopwatch helper class. Used by the Profiler.
 */

#pragma once

#include <chrono>

namespace juce_igutil {

class Stopwatch {

public:

    // construct
    Stopwatch() :
        startTime(std::chrono::high_resolution_clock::now())
    {}

    // destruct
    virtual ~Stopwatch() {}

    // start() and reset() both reset the start time.
    inline void start() {
        startTime = std::chrono::high_resolution_clock::now();
    }

    inline void reset() {
        start();
    }

    // read() and stop() return now - startTime.
    // Really there's no difference between read() and stop().
    // One or the other may look semantically better in your program.
    inline std::chrono::duration<__int64, std::nano> read() {
        return std::chrono::high_resolution_clock::now() - startTime;
    }

    inline std::chrono::duration<__int64, std::nano> stop() {
        return read();
    }

private:

    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

};

}

