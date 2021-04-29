#pragma once

#include <memory>

#include <JuceHeader.h>

#include "juce_igutil/EffectProcessor.h"
#include "juce_igutil/MTLogger.h"
#include "juce_igutil/NullProcessor.h"
#include "juce_igutil/Oscillator.h"
#include "juce_igutil/Processor.h"

#include "Config.h"
#include "UnlimitedSynthSound.h"
#include "WavetableOscillator.h"

/**
 * WavetableSynthVoice - using WavetableOscillator, it renders a synth voice that 
 * plays back a wavetable at the right frequency based on a midi note. 
 */
class WavetableSynthVoice : public juce::SynthesiserVoice
{
public:

    /**
     * constructor
     */
    WavetableSynthVoice(
        std::shared_ptr<juce_igutil::MTLogger> _pMTL,
        const std::deque<juce::AudioBuffer<SAMPLE_TYPE>> & waveTableInUse, 
        std::shared_ptr<juce::AudioProcessorValueTreeState> pSynthParams
    ): 
        juce::SynthesiserVoice(),
        pMTL(_pMTL),
        processSpec{48000.0, 0, 0},
        pOscillator(std::make_unique<WavetableOscillator>(
            _pMTL,
            pSynthParams, 
            waveTableInUse
        ))
    {
        using namespace juce;
        
        pCutoffParam = pSynthParams->getRawParameterValue(config::cutoffPN);
        pResonanceParam = pSynthParams->getRawParameterValue(config::resonancePN);

        // Create the filter processor
        pFilter = std::make_shared<FilterType>();

        // Configure the filter
        pFilter->setCutoffFrequencyHz(1000.0f);
        pFilter->setResonance(0.7f);
        pFilter->setMode(dsp::LadderFilterMode::LPF24);

        pFxProcessor = std::make_unique<juce_igutil::EffectProcessor<FilterType>>(pFilter);
    }

    // Default destructor
    virtual ~WavetableSynthVoice() = default;

    // the tutorial says this is an example of how to 
    // check the type and maybe do something different, I guess?  
    // TODO Why not use appliesToNote/Channel() ?
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<UnlimitedSynthSound*> (sound) != nullptr;
    }

    // Set the current playback sample rate
    virtual void setCurrentPlaybackSampleRate (double newRate) override {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate(newRate);

        // The actual channels are determined by the current output AudioBuffer.
        // The WavetableOscillator does not depend on anything other than the 
        // sample rate.
        processSpec = juce::dsp::ProcessSpec{ newRate, 4096, 2 };
        pOscillator->prepare(processSpec);
        pFxProcessor->prepare(processSpec);
    }

    /**
     * start a note
     */
    void startNote (
        int midiNoteNumber, 
        float velocity,
        juce::SynthesiserSound*, 
        int currentPitchWheelPosition
    ) override 
    {
        pOscillator->startNote(midiNoteNumber, velocity, currentPitchWheelPosition);
    }

    /**
     * stop the current note and maybe start the tail-off
     */
    void stopNote (float velocity, bool allowTailOff) override
    {
        pOscillator->stopNote(velocity, allowTailOff);
        if ( false == allowTailOff ) {
            clearCurrentNote();
        }
    }

    /** Called to let the voice know that the pitch wheel has been moved.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */
    void pitchWheelMoved (int newPitchWheelValue) override {
        pOscillator->pitchWheelMoved(newPitchWheelValue);
    }

    /** Called to let the voice know that a midi controller has been moved.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */
    void controllerMoved (int controllerNumber, int newControllerValue) override {
        pOscillator->controllerMoved(controllerNumber, newControllerValue);
    }

    /**
     * render the next block of audio
     */
    void renderNextBlock(
        juce::AudioBuffer<float>& outputBuffer,
        int startSample,
        int numSamples
    ) override
    {
        using namespace juce;

        // set cutoff and resonance before processing
        pFilter->setCutoffFrequencyHz(*pCutoffParam);
        pFilter->setResonance(*pResonanceParam);

        if (pOscillator->renderNextBlock(outputBuffer, startSample, numSamples))
            clearCurrentNote();

        // Run the effects
        dsp::AudioBlock<float> block(outputBuffer);
        dsp::ProcessContextReplacing<float> context(block);
        pFxProcessor->process(context);
    }

    /** A double-precision version of renderNextBlock() */
    void renderNextBlock(
        juce::AudioBuffer<double>& outputBuffer,
        int startSample,
        int numSamples
    ) override
    {
        // Supporting 64bit is not supported...
        pMTL->error("ERROR - calling 64-bit (double precision) version of WavetableSynthVoice::renderNextBlock() is not supported (yet)."); 
        jassert(false);
    }

private:

    // logger
    std::shared_ptr<juce_igutil::MTLogger> pMTL;

    // The process spec.  Set via setCurrentPlaybackSampleRate() override.  
    // Only samplerate is set.  Size of buffer is determined by the buffer itself.
    juce::dsp::ProcessSpec processSpec;

    // Oscillator
    // TODO add abilitiy for multiple oscillators per voice (a la P12)
    std::unique_ptr<juce_igutil::Oscillator> pOscillator;

    // effects processor (for per-voice effects like filtering)
    std::unique_ptr<juce_igutil::Processor> pFxProcessor;

    // FX references so we can control them via params.
    //using FxType = dsp::ProcessorChain<
    //    dsp::LadderFilter<float>
    //>;
    using FilterType = juce::dsp::LadderFilter<float>;
    std::shared_ptr<FilterType> pFilter;

    // Per-voice Params
    const std::atomic<float> * pCutoffParam;
    const std::atomic<float> * pResonanceParam;
};


