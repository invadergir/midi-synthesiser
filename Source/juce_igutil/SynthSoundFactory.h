/**
 * An interface for creating SynthSound - derived objects.
 */

#pragma once

#include <JuceHeader.h>
//#include <juce_audio_basics/juce_audio_basics.h>

namespace juce_igutil {

class SynthSoundFactory {

protected:

    SynthSoundFactory() = default;

public: 

    virtual ~SynthSoundFactory() = default;
    
    // create the object
    // TODO can we use unique_ptr in the context of this ref-counted thing:
    // SynthesiserSound* addSound (const SynthesiserSound::Ptr& newSound);
    virtual juce::SynthesiserSound * create() = 0;
};

}
