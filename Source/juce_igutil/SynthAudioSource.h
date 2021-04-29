/** 
 * Interface for synth audio sources.  The main
 * difference between this and AudioSource is that 
 * getNextAudioBlock() is replaced with renderNextBlock() which 
 * handles midi input received from the processor. 
 *  
 * The expected usage is pretty standard: 
 *   1. prepareToPlay() first
 *   2. renderNextBlock() from inside processBlock()
 *   3. releaseResources() last
 */

#pragma once

#include <JuceHeader.h>

namespace juce_igutil {

class SynthAudioSource
{
protected:
    /** Hidden constructor.  This is an interface. */
    SynthAudioSource() {}

public:
    /** Destructor. */
    virtual ~SynthAudioSource() = default;

    /** Tells the source to prepare for playing.
     *  
     */
    virtual void prepareToPlay(const juce::dsp::ProcessSpec & processSpec) = 0;

    /**
     * Process the next block of midi messages and emit some 
     * samples. This is meant to be called from 
     * AudioProcessor::processBlock().
     */
    virtual void renderNextBlock(
        juce::AudioBuffer<float> & outputAudio,
        juce::MidiBuffer & inputMidi,
        int startSample) = 0;

    /** Allows the source to release anything it no longer needs after playback
     *  has stopped.
     */
    virtual void releaseResources() = 0;

    /**
     * Allow access to the params - necessary for all synths.
     */
    virtual std::shared_ptr<juce::AudioProcessorValueTreeState> getSynthParams() = 0;
};

}
