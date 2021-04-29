/** 
 * A delay effect that uses juce::dsp::DelayLine. 
 */

#pragma once

#include <JuceHeader.h>

#include "juce_igutil/Processor.h"

// rough calculation for this simple delay
static const int maxDelayInSamples = 48000*10;

class DelayProcessor: public juce_igutil::Processor
{
private:

    using DelayLineType = juce::dsp::DelayLine<
        float, 
        juce::dsp::DelayLineInterpolationTypes::Linear
    >;

    float wetMix = 0.25;
    float delayTimeSec = 0.39;
    int numRepeats = 4;
    const int maxRepeats = 4;
    using BufferPtr = std::shared_ptr<juce::AudioBuffer<float>>;
    std::deque<BufferPtr> delayBuffers;
    std::deque<std::shared_ptr<DelayLineType>> delayLines;

public:
    /** Constructor.  */
    DelayProcessor(): 
        juce_igutil::Processor()
    {
        // empty
    }

    /** Destructor. */
    virtual ~DelayProcessor() = default;

    /**
     * set the delay time
     */
    void setDelayTime(const float newDelayInSamples) {
        //delayLine.setDelay(newDelayInSamples);
        std::for_each(delayLines.begin(), delayLines.end(), [=](auto & p){ p->setDelay(newDelayInSamples); });
    }

    /**
     * Set wet/dry mix (1.0 = full wet, 0.0 = full dry)
     */
    void setMix(const float wetToDryRatio) {
        wetMix = wetToDryRatio;
    }

    /** Prepare to process audio.  */
    void prepare(const juce::dsp::ProcessSpec& spec) override 
    {
        using namespace std;
        // create a buffer and delay line for every delay line up to maxRepeats.
        for (int ix=0; ix<maxRepeats; ++ix) {
            // allocate twice the max block size -- I don't think that's *really* a maximum.
            const int numSamplesToAllocate = spec.maximumBlockSize * 2;
            const int numChannels = spec.numChannels;
            auto pDelayBuffer = make_shared<juce::AudioBuffer<float>>(numChannels, numSamplesToAllocate);
            delayBuffers.push_back(pDelayBuffer);

            shared_ptr<DelayLineType> pLine(new DelayLineType(maxDelayInSamples));

            pLine->setDelay(spec.sampleRate * delayTimeSec);

            pLine->prepare(spec);
            delayLines.push_back(pLine);
        }
    }

    /** 
     * Process audio. 
     */ 
    void process(juce::dsp::ProcessContextReplacing<float> & context) noexcept override 
    {
        using namespace juce;

        // copy the block to 'pDelayBuffer' and process delay on it.
        auto & outputBlock = context.getOutputBlock();

        dsp::AudioBlock<float> inputBlock;
        dsp::AudioBlock<float> lastProcessedBlock;
        for (int ix = 0; ix < numRepeats; ++ix) {

            if ( ix == 0 ) 
                inputBlock = outputBlock;
            else 
                inputBlock = lastProcessedBlock;

            // resize the delay buffer without reallocating
            auto pDelayBuffer = delayBuffers[ix];
            pDelayBuffer->setSize(
                outputBlock.getNumChannels(), 
                outputBlock.getNumSamples(),
                true, // keepExistingContent
                false, //clearExtraSpace
                true // avoidReallocating
            );
            jassert(outputBlock.getNumSamples() <= pDelayBuffer->getNumSamples());
            
            // copy the input block to the delay block
            // we use each delay line's to input to the next delay line
            dsp::AudioBlock<float> delayBlock(*pDelayBuffer);
            delayBlock.copyFrom(inputBlock);
            jassert(delayBlock.getNumSamples() == outputBlock.getNumSamples());
            jassert(delayBlock.getNumChannels() == outputBlock.getNumChannels());
            dsp::ProcessContextReplacing<float> delayContext(delayBlock);
            
            // Now process
            auto pDelayLine = delayLines[ix];
            pDelayLine->process(delayContext);
            delayBlock.multiplyBy(wetMix);

            // save the last processed block
            lastProcessedBlock = delayBlock;
        }

        // mix all the delay blocks back with the original block.
        // outputBlock + delayBlocks = new output
        for (int chan=0; chan < outputBlock.getNumChannels(); ++chan) {
            for (int samp = 0; samp < outputBlock.getNumSamples(); ++samp) {
                // todo would using a pointer be more efficient? do clients of AudioBlock even have that ability?
                SAMPLE_TYPE processedTotal = 0.0;
                for (int iBuf = 0; iBuf < numRepeats; ++iBuf) {
                    processedTotal += delayBuffers[iBuf]->getSample(chan, samp);
                }
                SAMPLE_TYPE newSample = outputBlock.getSample(chan, samp) + processedTotal;
                outputBlock.setSample(chan, samp, newSample);
            }
        }
    }

    /**
     * Reset the internal state of the processor, with smoothing if 
     * necessary. 
     */
    void reset() override
    {
        std::for_each(delayLines.begin(), delayLines.end(), [](auto & p){ p->reset(); });
    }

};


