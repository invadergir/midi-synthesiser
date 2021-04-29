/**
 * ConfigurableSynthAudioSource
 *  
 * A synth audio source behaves like an AudioSource but takes 
 * midi input into account.  It has configurable sound and 
 * voices, given to it via factories in the constructor. 
 *  
 */

#pragma once

#include <JuceHeader.h>

#include "juce_igutil/Processor.h"
#include "juce_igutil/NullProcessor.h"

#include "SynthAudioSource.h"
#include "MTLogger.h"

namespace juce_igutil {

static const std::string gainPN("gain");

/**
 * ConfigurableSynthAudioSource
 * 
 */
class ConfigurableSynthAudioSource: public SynthAudioSource
{
public:

    ConfigurableSynthAudioSource(
        std::shared_ptr<juce_igutil::MTLogger> pMTL,
        std::shared_ptr<juce::AudioProcessorValueTreeState> pSynthParameters,
        juce::SynthesiserSound::Ptr pSynthSound,
        std::vector<juce::SynthesiserVoice*> synthVoices,
        juce::MidiKeyboardState & keyState, // todo is this the best place for this?
        std::shared_ptr<Processor> pEffectsProcessor = 
            std::make_shared<juce_igutil::NullProcessor>()
    );
 
    // destruct
    virtual ~ConfigurableSynthAudioSource() override = default;

    // prepare 
    void prepareToPlay(const juce::dsp::ProcessSpec & processSpec) override;
 
    // Called to process messages
    void renderNextBlock(
        juce::AudioBuffer<float>& outputAudio,
        juce::MidiBuffer& inputMidi,
        int startSample) override;

    // release resources
    void releaseResources() override;

    // Allow access to the params
    std::shared_ptr<juce::AudioProcessorValueTreeState> getSynthParams() override {
        return pSynthParams;
    }

private:

    // logger
    std::shared_ptr<juce_igutil::MTLogger> pMTL;

    // The synth object
    juce::Synthesiser synth;

    // Synth Parameters
    std::shared_ptr<juce::AudioProcessorValueTreeState> pSynthParams;

    // Generic synth params
    float previousGain = 0.6f;
    std::atomic<float> * pGainParam = nullptr;

    // MidiKeyboardState:  helps merge on-screen keyboard midi
    // with midi from controllers.
    juce::MidiKeyboardState & keyboardState;

    // ProcessSpec, set in prepareToPlay()
    juce::dsp::ProcessSpec processSpec;

    // Optional effects processor.
    std::shared_ptr<juce_igutil::Processor> pFxProcessor;
};

}

