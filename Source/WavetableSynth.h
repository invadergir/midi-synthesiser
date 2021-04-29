/**
 * WavetableSynth 
 *  
 * A ConfigurableSynthAudioSource - based synth that adds 
 * wavetable handling and the ability to track the wavetable 
 * index as a parameter.  Note that it decorates
 * ConfigurableSynthAudioSource rather than using that as a base 
 * class. 
 */

#pragma once

#include <JuceHeader.h>

#include "juce_igutil/ConfigurableSynthAudioSource.h"
#include "juce_igutil/MTLogger.h"
#include "juce_igutil/ProcessorSequence.h"
#include "juce_igutil/SynthAudioSource.h"
#include "Config.h"
#include "EffectUtil.h"

/**
 * ConfigurableSynthAudioSource
 * 
 */
class WavetableSynth: public juce_igutil::SynthAudioSource
{
public:

    // Constructor that assumes you want wavetable voices.
    // You just have to pass in the wavetable.  Note: the wavetable is move-constructed
    WavetableSynth(
        std::shared_ptr<juce_igutil::MTLogger> pMTL,
        std::shared_ptr<juce::AudioProcessorValueTreeState> pSynthParameters,
        juce::SynthesiserSound::Ptr pSynthSound,
        std::deque<juce::AudioBuffer<config::WTSampleType>> wavetableToUse,
        const int numVoices,
        juce::MidiKeyboardState & keyState // todo is this the best place for this?
    );

    // destructor
    virtual ~WavetableSynth() override = default;

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
        return pSynth->getSynthParams();
    }
 
private:

    // manually enforce the output to be within normal limits in case of error.
    void clampOutput(juce::AudioBuffer<float> & outputAudio, bool silent=false);

    // Create efects objects ahead of time so there is no object creation penalty
    // when they are switched.
    void createEffects();

    // Set the effects according to the order in the effect type selectors.
    void setEffectsSequence();

    // get the effective FX type based on the selected (or not selected) type
    inline const config::EffectType getEffectiveFxType(const int index);

    // prior to rendering, set the gain for each proc from the gain params.
    inline void setGain();

    // logger
    std::shared_ptr<juce_igutil::MTLogger> pMTL;

    // cached synth parameters from value tree
    std::shared_ptr<juce::AudioProcessorValueTreeState> pParams;
    std::atomic<float> * pWavetableIndexParam = nullptr;
    std::deque<FxParamGroup> fxParams;
    std::deque<config::EffectType> lastSelectedFxTypes;

    // Wrapped synth:
    std::unique_ptr<juce_igutil::ConfigurableSynthAudioSource> pSynth;

    // Wavetable
    std::deque<juce::AudioBuffer<config::WTSampleType>> wavetable;

    // Process spec
    juce::dsp::ProcessSpec processSpec{0,0,0};

    // The unused processors we have available.
    // FxSetterFunc: the fxSlot (index) is passed in to get the right param.
    std::map<
        config::EffectType, 
        std::deque<ProcessorAndFxSetter>
    > processorPool;

    // FX processor sequence.
    std::shared_ptr<juce_igutil::ProcessorSequence> pFxSequence;

    // FxSetters for every FX slot.  Corresponds to the fX sequence above.
    std::deque<std::shared_ptr<FxSetter>> fxSetters;
};
