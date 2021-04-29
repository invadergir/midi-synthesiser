/**
 * Effects processor for the synth that uses juce::dsp modules.
 */

#pragma once

#include <JuceHeader.h>

#include "juce_igutil/MTLogger.h"
#include "juce_igutil/Processor.h"

namespace juce_igutil {

/**
 * A Processor which can process any number of processors in a sequence. Can be 
 * used as a wrapper for any number of Processors, which themselves wrap 
 * juce::dsp effects or ProcessorChains. 
 */
class ProcessorSequence: public juce_igutil::Processor
{
public:
    
    // Default Constructor 
    ProcessorSequence(): 
        Processor(),
        //pMTL(_pMTL),
        procs()
    {
        // empty
    }

    // Constructor 
    ProcessorSequence(
        //std::shared_ptr<juce_igutil::MTLogger> _pMTL,
        std::shared_ptr<Processor> pProcessor
    ): 
        Processor(),
        //pMTL(_pMTL),
        procs{pProcessor}
    {
        jassert(pProcessor);
    }

    // Constructor with multiple processors.  You can pass a deque of size 1 too, 
    // of course, but there's a constructor above that makes that easier.
    ProcessorSequence(
        std::deque<std::shared_ptr<Processor>> processorSequence
    ):
        Processor(),
        procs(processorSequence)
    {
        jassert( procs.size() > 0 );
        for (auto p : procs) jassert(p);
    }

    // destructor
    virtual ~ProcessorSequence() override = default;

    /** Prepare to process audio.  */
    void prepare(const juce::dsp::ProcessSpec& spec) override
    {
        processSpec = spec;
        for (auto p : procs) p->prepare(spec);
    }

    /** 
     * Process audio. 
     */ 
    void process(
        juce::dsp::ProcessContextReplacing<float> & context
    ) noexcept override
    {
        for (auto p : procs) p->process(context);
    }

    /**
     * Reset the internal state of the processor. 
     */
    void reset() override
    {
        for (auto p : procs) p->reset();
    }

    // Helper to add a processor to the end of the processing chain.
    void addProcessor(std::shared_ptr<Processor> p) 
    {
        procs.push_back(move(p));
    }

    // Helper to replace a processor at an index.
    // The replaced processor, if the index was valid, is returned.
    // If the index was invalid, we just push onto the back of the deque.
    // If no processor specified, we replace it with a NullProcessor.
    std::shared_ptr<Processor> replaceProcessor(
        const int index, 
        std::shared_ptr<Processor> p = std::make_shared<NullProcessor>()) 
    {
        std::shared_ptr<Processor> replaced;
        if (index < procs.size()) {
            replaced = move(procs[index]);
            procs[index] = p;
        }
        else {
            procs.push_back(p);
        }
        return replaced;
    }

    // Helper to remove a processor at a specified index. 
    // Returns the processor if something was removed.
    std::shared_ptr<Processor> removeProcessor(const int index) 
    {
        std::shared_ptr<Processor> removed;
        if (index < procs.size()) {
            auto iter = procs.begin() + index;
            removed = *iter;
            procs.erase(iter);
        }
        return removed;
    }

    // Query the number of processors
    int getProcessorsCount() const 
    {
        return procs.size(); 
    }

    // Remove all processors
    void clear() 
    {
        procs.clear(); 
    }

    // Return the exact processor type, casted appropriately
    template <typename T> 
    std::shared_ptr<T> getExactProcessor(const int index) 
    {
        std::shared_ptr<T> p;
        if (index < procs.size()) {
            p = std::dynamic_pointer_cast<T>(procs[index]);
        }
        return p;
    }

private:

    //// logger
    //std::shared_ptr<juce_igutil::MTLogger> pMTL;

    // ProcessSpec, set in prepare()
    juce::dsp::ProcessSpec processSpec{0,0,0};

    // Processor sequence
    std::deque<std::shared_ptr<Processor>> procs;
};

}


