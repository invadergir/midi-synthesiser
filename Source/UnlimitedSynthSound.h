/**
 * UnlimitedSynthSound: A SynthesiserSound that is not limited 
 * to any notes or channels. 
 */

#pragma once

#include <JuceHeader.h>

/**
 * UnlimitedSynthSound: A SynthesiserSound that is not limited 
 * to any notes or channels. 
 */
struct UnlimitedSynthSound   : public juce::SynthesiserSound
{
    UnlimitedSynthSound() {}
 
    bool appliesToNote    (int) override        { return true; }
    bool appliesToChannel (int) override        { return true; }
};


