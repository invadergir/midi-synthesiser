#ifndef __midisynthesiser_debug_h__
#define __midisynthesiser_debug_h__

#include <JuceHeader.h>
#include "juce_igutil/MTLogger.h"

namespace debug {

/**
 * Check if output is in expected bounds.
 */
void checkOutput(const juce::AudioBuffer<float> & outputAudio, std::shared_ptr<juce_igutil::MTLogger> pMTL);

}

#endif