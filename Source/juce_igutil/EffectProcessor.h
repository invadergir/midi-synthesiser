/**
 * Effects processor for the synth that uses juce::dsp modules.
 */

#pragma once

#include <JuceHeader.h>

#include "juce_igutil/MTLogger.h"
#include "juce_igutil/Processor.h"

namespace juce_igutil {

/**
 * An Effect Processor which can process any effect you like. Can be used as a
 * wrapper for any juce::dsp effect or a ProcessorChain. 
 */
template <typename ProcessorType>
class EffectProcessor: public juce_igutil::Processor
{
public:
    
    // Constructor 
    EffectProcessor(
        //std::shared_ptr<juce_igutil::MTLogger> _pMTL,
        std::shared_ptr<ProcessorType> pConcreteProcessor
    ):
        Processor(),
        //pMTL(_pMTL),
        pProcessor(pConcreteProcessor)
    {
        jassert(pProcessor);

        //pMTL->info("EffectProcessor: Constructing...");
    }

    // destructor
    virtual ~EffectProcessor() override = default;

    /** Prepare to process audio.  */
    void prepare(const juce::dsp::ProcessSpec& spec) override
    {
        processSpec = spec;
        pProcessor->prepare(spec);
    }

    /** 
     * Process audio. 
     */ 
    void process(
        juce::dsp::ProcessContextReplacing<float> & context
    ) noexcept override
    {
        pProcessor->process(context);
    }

    /**
     * Reset the internal state of the processor. 
     */
    void reset() override
    {
        pProcessor->reset();
    }

    // Return the exact processor type, casted appropriately
    template <typename T> 
    std::shared_ptr<T> getExactProcessor() 
    {
        return std::dynamic_pointer_cast<T>(pProcessor);
    }

private:

    //// logger
    //std::shared_ptr<juce_igutil::MTLogger> pMTL;

    // ProcessSpec, set in prepare()
    juce::dsp::ProcessSpec processSpec{0,0,0};

    // Processor
    std::shared_ptr<ProcessorType> pProcessor;
};

}

