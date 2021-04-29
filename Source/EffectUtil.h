// Some useful helpers for effects usage.

#pragma once

#include "Config.h"

using FxSetterFunc = std::function<void(int)>;
struct FxSetter {
    FxSetterFunc fxGainSetter;
    // todo add more when new effects controls are added
};

struct ProcessorAndFxSetter {
    std::shared_ptr<juce_igutil::Processor> pProcessor;
    std::shared_ptr<FxSetter> pFxSetter = nullptr;
};

struct FxParamGroup {
    std::atomic<float> * pTypeSelector = nullptr;
    std::atomic<float> * pGain = nullptr;
};

using ChorusType = juce::dsp::Chorus<SAMPLE_TYPE>;
using ReverbType = juce::dsp::Reverb;
using GainType = juce::dsp::Gain<SAMPLE_TYPE>;
using DistortionType = juce::dsp::LadderFilter<SAMPLE_TYPE>;

