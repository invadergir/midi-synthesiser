#include "Profiler.h"

#include "MTLogger.h"

using namespace juce_igutil;

/**
 * Construct. 
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
Profiler::Profiler(
    std::string nameOfProfiler, 
    std::shared_ptr<MTLogger> _pMTL,
    const unsigned long long numWarmupCycles,
    const unsigned long long numLogMessagesToBuffer
): 
    name(nameOfProfiler), 
    pMTL(_pMTL),
    maxWarmups(numWarmupCycles),
    countOfWarmups(0LL),
    outputModulo(numLogMessagesToBuffer)
{
    // empty
}

/**
 * Destruct.
 */
Profiler::~Profiler() {
    // empty
}

/**
 * Create stats string.
 */
const juce::String Profiler::toString() const {
    using namespace juce;
    return String("Perf Stats:  minNanos=") + String(minNanos) + String(", maxNanos=") + String(maxNanos) +
        String(", nanosAvg=") + String(static_cast<unsigned long>(nanosAvg)) + String(", totalSamples=") + String(totalSamples);
}

/**
 * Log start time
 */
void Profiler::start() {
    if (countOfWarmups >= maxWarmups) {
        sw.start();
    }
}

/**
 * Get elapsed time, store stats, and maybe output to the log
 * based on the modulo.
 */
void Profiler::stop() {
    if (countOfWarmups >= maxWarmups) {
        //const std::chrono::duration<__int64, std::nano> nanos
        const long long nanos = sw.stop().count();

        if (minNanos < 0 || nanos < minNanos) minNanos = nanos;
        if (nanos > maxNanos) maxNanos = nanos;
        if ( nanosAvg < 0.0 ) nanosAvg = (double)nanos;
        else nanosAvg = ((nanosAvg * totalSamples) + nanos) / (totalSamples + 1);
        totalSamples += 1;

        if (totalSamples % outputModulo == 0) {
            pMTL->debug(this->toString());
        }
    }
    else {
        ++countOfWarmups;
    }
}


