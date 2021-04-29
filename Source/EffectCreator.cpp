#include "EffectCreator.h"

#include <JuceHeader.h>

#include "DelayProcessor.h"
#include "EffectUtil.h"
#include "juce_igutil/EffectProcessor.h"
#include "juce_igutil/ProcessorSequence.h"

using namespace std;
using namespace juce;
using namespace juce_igutil;

// chorus effect
ProcessorAndFxSetter effect_creator::createChorus(std::deque<FxParamGroup> & fxParams) 
{
    auto pChorusFx = make_shared<ChorusType>();
    pChorusFx->setCentreDelay(20.0);
    pChorusFx->setDepth(0.3);
    pChorusFx->setMix(0.5);
    pChorusFx->setRate(0.75);
    FxSetterFunc chorusGainFunc( [pChorusFx, &fxParams](int fxIndex) { 
        auto pGain = fxParams.at(fxIndex).pGain;
        pChorusFx->setMix(*pGain);
    });
    auto pChorusGainSetter = make_shared<FxSetter>();
    pChorusGainSetter->fxGainSetter = chorusGainFunc;
    return ProcessorAndFxSetter{
        make_shared<EffectProcessor<ChorusType>>(pChorusFx),
        pChorusGainSetter
    };
}

// delay
ProcessorAndFxSetter effect_creator::createDelay(std::deque<FxParamGroup> & fxParams)
{
    auto pDelayFx = make_shared<DelayProcessor>();
    FxSetterFunc delayGainFunc( [pDelayFx, &fxParams](int fxIndex) { 
        auto pGain = fxParams.at(fxIndex).pGain;
        pDelayFx->setMix(*pGain);
    });
    auto pDelayGainSetter = make_shared<FxSetter>();
    pDelayGainSetter->fxGainSetter = delayGainFunc;
    return ProcessorAndFxSetter{
        pDelayFx,
        pDelayGainSetter
    };
}

// reverb
ProcessorAndFxSetter effect_creator::createReverb(std::deque<FxParamGroup> & fxParams)
{
    auto pReverbFx = make_shared<ReverbType>();
    dsp::Reverb::Parameters params{
        0.75, //float roomSize   = 0.5f;     /**< Room size, 0 to 1.0, where 1.0 is big, 0 is small. */
        0.4, //float damping    = 0.5f;     /**< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped. */
        0.30, //float wetLevel   = 0.33f;    /**< Wet level, 0 to 1.0 */
        0.6, //float dryLevel   = 0.4f;     /**< Dry level, 0 to 1.0 */
        0.3, //float width      = 1.0f;     /**< Reverb width, 0 to 1.0, where 1.0 is very wide. */
        0.0 //float freezeMode = 0.0f;     /**< Freeze mode - values < 0.5 are "normal" mode, values > 0.5
    };
    pReverbFx->setParameters(params);
    FxSetterFunc reverbGainFunc( [pReverbFx, &fxParams](int fxIndex) { 
        auto pGain = fxParams.at(fxIndex).pGain;
        dsp::Reverb::Parameters parms = pReverbFx->getParameters();
        parms.wetLevel = *pGain;
        parms.dryLevel = 1.0 - parms.wetLevel;
        pReverbFx->setParameters(parms);
    });
    auto pReverbGainSetter = make_shared<FxSetter>();
    pReverbGainSetter->fxGainSetter = reverbGainFunc;
    return ProcessorAndFxSetter{
        make_shared<EffectProcessor<ReverbType>>(pReverbFx),
        pReverbGainSetter
    };
}

// "distortion"; the only one provided with juce::dsp is this mild overdrive
// from the ladder filter.  It's actually pretty cool because it makes the 
// square wave look more like the "horned" wave from the OB-X.  It sounds 
// pretty good except there's some high pitched tonal noise I'm not sure 
// what to do about.  Lowering the LPF cutoff doesn't seem to help; it just 
// lowers the frequency of the noise.
// TODO the noise only shows up when no notes are sounding.  Consider by-
// passing it when no notes are playing.
ProcessorAndFxSetter effect_creator::createDistortion(std::deque<FxParamGroup> & fxParams)
{
    auto pDistortionFx = make_shared<DistortionType>();
    const float drive = 650.0f;
    pDistortionFx->setDrive(drive);
    pDistortionFx->setMode(dsp::LadderFilterMode::LPF12);
    pDistortionFx->setCutoffFrequencyHz(30'000.0);
    pDistortionFx->setResonance(0.0);
    auto pDistLevel = make_shared<GainType>();
    auto pDistSeq = make_shared<ProcessorSequence>();
    pDistSeq->addProcessor(make_shared<EffectProcessor<DistortionType>>(pDistortionFx));
    pDistSeq->addProcessor(make_shared<EffectProcessor<GainType>>(pDistLevel));
    
    FxSetterFunc distGainFunc( [pDistLevel, &fxParams](int fxIndex) { 
        auto pGain = fxParams.at(fxIndex).pGain;
        pDistLevel->setGainLinear(*pGain * 0.15); // hard code a reduction because there's a LOT of gain
    });
    auto pDistGainSetter = make_shared<FxSetter>();
    pDistGainSetter->fxGainSetter = distGainFunc;
    return ProcessorAndFxSetter{pDistSeq, pDistGainSetter};
}


