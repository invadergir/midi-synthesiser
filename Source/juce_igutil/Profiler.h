#pragma once

#include <JuceHeader.h>

#include "MTLogger.h"
#include "Stopwatch.h"

namespace juce_igutil {

// Class to aid in profiling code.

class Profiler {

public:
    
    /**
     * Constructor. 
     *  
     * @param nameOfProfiler 
     * @param _pMTL - MT logger
     * @param numWarmupCycles - the number of 'start() & stop()' 
     *                        calls to ignore before starting to
     *                        collect stats. This allows the system
     *                        to warmup first, if desired. The
     *                        default is 0 warmup cycles.
     * @param numLogMessagesToBuffer - default is 50
     */
    Profiler(
        std::string nameOfProfiler, 
        std::shared_ptr<MTLogger> _pMTL,
        const unsigned long long numWarmupCycles = 0LL,
        const unsigned long long numLogMessagesToBuffer = 50LL
    );

    // Destruct
    virtual ~Profiler();

    // Stringify the stats for output.
    const juce::String toString() const;
    
    // Check if we started collecting stats yet.
    inline bool startedCounting() const { return minNanos >= 0; }

    // log start time
    void start();

    // get elapsed time, store stats, and maybe output to the log based on the modulo
    void stop();

    inline unsigned long long getTotalSamples() const { return totalSamples; }

private: 

    Stopwatch sw;

    std::string name;

    __int64 minNanos = -1L;
    __int64 maxNanos = 0L;
    double nanosAvg = -1.0;
    unsigned long long totalSamples = 0LL;

    std::shared_ptr<MTLogger> pMTL;

    unsigned long long maxWarmups;
    unsigned long long countOfWarmups;
    const unsigned long long outputModulo;

};

}
