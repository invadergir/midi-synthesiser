#include "WavetableSynth.h"

#include <JuceHeader.h>

#include "juce_igutil/ConfigurableSynthAudioSource.h"
#include "juce_igutil/EffectProcessor.h"
#include "juce_igutil/Processor.h"
#include "WavetableSynthVoice.h"
#include "Debug.h"
#include "DelayProcessor.h"
#include "EffectCreator.h"

using namespace config;
using namespace effect_creator;
using namespace juce;
using namespace juce_igutil;
using namespace std;

/**
 * Constructor that assumes you want a wavetable sound factory. 
 * You just have to pass in the wavetable.
 */
WavetableSynth::WavetableSynth(
    std::shared_ptr<juce_igutil::MTLogger> _pMTL,
    std::shared_ptr<juce::AudioProcessorValueTreeState> pSynthParameters,
    juce::SynthesiserSound::Ptr pSynthSound,
    std::deque<juce::AudioBuffer<WTSampleType>> wavetableToUse,
    const int numVoices,
    juce::MidiKeyboardState & keyState
): 
    SynthAudioSource(),
    pMTL(_pMTL),
    pParams(pSynthParameters),
    wavetable(move(wavetableToUse)),
    fxSetters(maxEffects)
{
    // set up parameter links
    pMTL->info("WavetableSynth: Connecting parameters...");
    pWavetableIndexParam = pParams->getRawParameterValue("waveIndex");
    jassert(pWavetableIndexParam != nullptr);
    for (int ix = 0; ix < maxEffects; ++ix) {
        fxParams.push_back( FxParamGroup{
            pParams->getRawParameterValue(getEffectPN(typeSelectorPN, ix)),
            pParams->getRawParameterValue(getEffectPN(fxLevelPN, ix))
        });
    }

    pMTL->info("WavetableSynth: Creating synth...");

    // Create the voices 
    vector<juce::SynthesiserVoice*> synthVoices;
    for (int i=0; i<numVoices; ++i) {
        synthVoices.push_back( new WavetableSynthVoice(
            pMTL
            ,wavetable 
            ,pParams
        ));
    }

    pFxSequence = make_shared<ProcessorSequence>();
    createEffects();
    // the effects sequence is set later.

    pSynth.reset( new ConfigurableSynthAudioSource(
        pMTL, 
        pParams, 
        pSynthSound, 
        synthVoices,
        keyState,
        pFxSequence
    ));
}

/**
 * create and set the effects.  This should only be called once from the 
 * constructor. 
 */
void WavetableSynth::createEffects() 
{
    jassert(processorPool.empty() && "don't call this more than once.");

    // Init the pool with an empty deque for every key.
    for (int ifx = FIRST_EFFECT; ifx <= LAST_EFFECT; ++ifx ) {  
        processorPool[static_cast<EffectType>(ifx)] = deque<ProcessorAndFxSetter>();
    }

    // Create shared null FX setter which doesn't do anything, this prevents an 
    // if check in setGain()
    static FxSetterFunc nullFxSetter = [](int index){};
    auto pNullFxSetter = make_shared<FxSetter>();
    pNullFxSetter->fxGainSetter = nullFxSetter;

    for (int ix = 0; ix < maxEffects; ++ix) {
        // Null effects (passthrough)
        processorPool[NULL_EFFECT].push_back( 
            ProcessorAndFxSetter { make_shared<NullProcessor>(), pNullFxSetter } );

        processorPool[CHORUS_EFFECT].push_back( createChorus(fxParams) );
        processorPool[DELAY_EFFECT].push_back( createDelay(fxParams) );
        processorPool[REVERB_EFFECT].push_back( createReverb(fxParams) );
        processorPool[DISTORTION_EFFECT].push_back( createDistortion(fxParams) );

        // initialize this too, this is used in setEffectsSequence() to detect
        // the first time we init this.
        lastSelectedFxTypes.push_back(INVALID_EFFECT);
    }
}

/**
 * Get the effective (no pun intended) FxType.  Ie. bump it to NULL_EFFECT if 
 * the param value is anything below the first real effect. 
 */
const EffectType WavetableSynth::getEffectiveFxType(const int index) 
{
    int t = static_cast<int>(*(fxParams.at(index).pTypeSelector));

    if (t < FIRST_REAL_EFFECT ) {
        t = NULL_EFFECT;
    }
    jassert(t <= LAST_EFFECT);

    return static_cast<EffectType>(t);
}

/** 
 *  Set the effects according to the order in the effect type selectors.
 */
void WavetableSynth::setEffectsSequence()
{
    //// Log the current effect type selections.  This is very verbose, but save 
    ///  in case it's needed for debugging:
    //stringstream debugSS;
    //debugSS << "Effect type params:  ";
    //for (int ix=0; ix<maxEffects; ++ix) {
    //    auto paramName = getEffectPN(typeSelectorPN, ix);
    //    const int val = *(pParams->getRawParameterValue(paramName));
    //    debugSS << paramName << "("<<val<<"), ";
    //}
    //pMTL->debug(debugSS.str());

    // check for changed effects and put in place the correct processor.
    for ( int ix = 0; ix < maxEffects; ++ix ) 
    {
        const EffectType fxType = getEffectiveFxType(ix);
        const EffectType lastFxType = lastSelectedFxTypes[ix];

        // If it was never set, or if the effect type changed from previous, update it
        if (INVALID_EFFECT == lastFxType || fxType != lastFxType) 
        {
            // get an unused one from the pool and reset it
            deque<ProcessorAndFxSetter> pnf = processorPool.at(fxType);
            //int size = pProcs->size();
            shared_ptr<Processor> pNewEffect(pnf.front().pProcessor);
            shared_ptr<FxSetter> pFxSetter(pnf.front().pFxSetter);
            pnf.pop_front();
            pNewEffect->reset();
            // put the new items in place
            auto pOldProc = 
                pFxSequence->replaceProcessor(ix, pNewEffect);
            auto pOldFxSetter = fxSetters[ix];
            fxSetters[ix] = pFxSetter;
            // and add the replaced ones back to the pool.
            // (only if we replaced one)
            if (pOldProc || pOldFxSetter) {
                jassert(pOldProc);
                jassert(pOldFxSetter);
                processorPool[fxType].push_back(
                    ProcessorAndFxSetter{pOldProc, pOldFxSetter}
                );
            }

            // save the new type as the last one
            lastSelectedFxTypes[ix] = fxType;
        }
    }
}

/**
 * Prepare the synth for audio generation
 */
void WavetableSynth::prepareToPlay(const juce::dsp::ProcessSpec & spec)
{
    setEffectsSequence();

    // Call prepare() on all the effects in the pool, so they know what's up
    // even though they might not be set in the FX processor yet.
    for ( auto mapItem : processorPool ) {
        for ( auto pfx : mapItem.second ) {
            pfx.pProcessor->prepare(spec);
        }
    }

    processSpec = spec;
    pSynth->prepareToPlay(processSpec);
}

/**
 * Set the level for all the gain processors according to the parameters.
 */
void WavetableSynth::setGain() 
{
    for ( int ix = 0; ix < maxEffects; ++ix ) {
        auto setter = fxSetters[ix]->fxGainSetter;
        setter(ix);
    }
}

/**
 * Render the next block of audio
 */
void WavetableSynth::renderNextBlock(
    juce::AudioBuffer<float> & outputAudio,
    juce::MidiBuffer & inputMidi,
    int startSample)
{
    setEffectsSequence();

    setGain();

    pSynth->renderNextBlock(outputAudio, inputMidi, startSample);
    clampOutput(outputAudio);
}

/**
 * release resources
 */
void WavetableSynth::releaseResources()
{
    pSynth->releaseResources();
}

// Manually enforce the output to be within normal limits in case the resonance 
// gets out of hand.  TODO a better way would be to simulate some analog clipping.
void WavetableSynth::clampOutput(juce::AudioBuffer<float> & outputAudio, bool silent)
{
    static const float highestAllowed = 0.97f;
    std::stringstream errors;

    for (int chan = 0; chan < outputAudio.getNumChannels(); ++chan) {
        const float * p = outputAudio.getReadPointer(chan);
        for (int ix=0; ix<outputAudio.getNumSamples(); ++ix, ++p) {
            if ( *p < -highestAllowed || *p > highestAllowed ) {
                if ( !silent ) {
                    errors << "ERROR: sample out of bounds (" << *p << "), channel (" << chan << "), index (" << ix << ")" << endl;
                }
                float newValue = highestAllowed;
                if (*p < 0) {
                    newValue = -highestAllowed;
                }
                *(outputAudio.getWritePointer(chan, ix)) = newValue;
            }
        }
    }
    std::string errorsStr = errors.str();
    if ( ! errorsStr.empty() ) {
        pMTL->debug(String(errorsStr));
    }
}



