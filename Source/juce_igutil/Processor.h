/** 
 * Interface for any type of audio-processing code.  This 
 * interface provides isolation and is originally meant to wrap 
 * a templated ProcessorChain, but could be used for anything. 
 */

#pragma once

#include <JuceHeader.h>

namespace juce_igutil {

class Processor
{
protected:
    /** Hidden constructor.  This is an interface. */
    Processor() {}

public:
    /** Destructor. */
    virtual ~Processor() = default;

    /** Prepare to process audio.  */
    virtual void prepare(const juce::dsp::ProcessSpec& spec) = 0;

    /** 
     * Process audio. 
     */ 
    virtual void process(
        juce::dsp::ProcessContextReplacing<float> & context) noexcept = 0;

    /**
     * Reset the internal state of the processor, with smoothing if 
     * necessary. 
     *  
     * TODO this is too similar to [smart pointer].reset(). Consider renaming. 
     */
    virtual void reset() = 0;
};

}


