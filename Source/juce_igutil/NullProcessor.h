/** 
 * A do-nothing Processor implementation. 
 */

#pragma once

#include <JuceHeader.h>

#include "juce_igutil/Processor.h"

namespace juce_igutil {

class NullProcessor : public Processor
{
public:
    /** Hidden constructor.  This is an interface. */
    NullProcessor() 
    {
        // empty
    }

    /** Destructor. */
    virtual ~NullProcessor() override = default;

    /** Prepare to process audio.  */
    void prepare(const juce::dsp::ProcessSpec& spec) override
    {
        // empty
    }

    /** 
     * Process audio. 
     */ 
    void process(
        juce::dsp::ProcessContextReplacing<float> & context) noexcept override
    {
        // empty
    }

    /**
     * Reset the internal state of the processor, with smoothing if 
     * necessary.
     */
    void reset() override
    {
        // empty
    }
};

}



