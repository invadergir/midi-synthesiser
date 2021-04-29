#pragma once

#include <JuceHeader.h>
#include <string>
#include "juce_igutil/ConfigurableSynthAudioSource.h"

// This is used sporadically to make the 64-bit conversion easier later.
#define SAMPLE_TYPE float

namespace config {

// sample type used for wavetables, currently.
using WTSampleType = SAMPLE_TYPE;
using WaveAudioBuffer = juce::AudioBuffer<WTSampleType>;

static const int maxEffects = 6;

static const int maxNumVoices = 8;
static const int numVoices = maxNumVoices;
static const int wavetableNumSamples = 512;

// absolute value of the max gain allowed for the oscillators.  Subject to change.
// Used by the wavetable generator.  The max waveform height is twice this number.
static const double maxOscillatorsGain = 0.5;

static const double oscillatorGain = maxOscillatorsGain / maxNumVoices;

// Note: "PN" is shorthand for "parameter name".

// UI Control Parameter names 
static const std::string waveIndexPN("waveIndex");
static const std::string cutoffPN("cutoff");
static const std::string resonancePN("resonance");

// Per-effect param names
static const std::string typeSelectorPN("typeSelector");
static const std::string fxLevelPN("fxLevel");

// Helper to tack on the index to get the real param name:
static const std::string getEffectPN(const std::string & name, const int index) {
    std::stringstream ss;
    ss << name << index;
    return ss.str();
}

// Effect Type indexes that are received from the parameter.
// Note, that since the parameter doesn't store IDs (only indexes), and the 
// combobox class does not like adding items with a zero id, we have to do some 
// finagling when adding the items to the combobox (essentially adding one to 
// these values).
static const enum EffectType {
    INVALID_EFFECT = -1, // only used in tracking changes to effects (initial value)
    NULL_EFFECT = 0,
    FIRST_EFFECT = NULL_EFFECT,
    DISTORTION_EFFECT = 1,
    FIRST_REAL_EFFECT = DISTORTION_EFFECT,  // first non-null effect
    CHORUS_EFFECT,
    DELAY_EFFECT,
    REVERB_EFFECT,
    LAST_EFFECT = REVERB_EFFECT,
    NUM_REAL_EFFECTS = LAST_EFFECT,
    NUM_EFFECTS // including the null effect
};


} // namespace config
