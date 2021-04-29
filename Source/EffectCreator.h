#pragma once

#include <JuceHeader.h>

#include "EffectUtil.h"
#include "juce_igutil/EffectProcessor.h"

namespace effect_creator {

    // create chorus effect
    ProcessorAndFxSetter createChorus(std::deque<FxParamGroup> & fxParams);

    // create delay
    ProcessorAndFxSetter createDelay(std::deque<FxParamGroup> & fxParams);

    // reverb
    ProcessorAndFxSetter createReverb(std::deque<FxParamGroup> & fxParams);
    
    // "distortion"; the only one provided with juce::dsp is this mild overdrive
    // from the ladder filter.  It's actually pretty cool because it makes the 
    // square wave look more like the "horned" wave from the OB-X.  It sounds 
    // pretty good except there's some high pitched tonal noise I'm not sure 
    // what to do about.  Lowering the LPF cutoff doesn't seem to help; it just 
    // lowers the frequency of the noise.
    // TODO the noise only shows up when no notes are sounding.  Consider by-
    // passing it when no notes are playing.
    ProcessorAndFxSetter createDistortion(std::deque<FxParamGroup> & fxParams);
}
