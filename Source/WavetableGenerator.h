/**
 * WavetableGenerator 
 *  
 * Some helper functions to generate wavetables.
 */

#pragma once

#include <JuceHeader.h>

#include "Config.h"

#define TWOPI (juce::MathConstants<double>::twoPi)

/**
 * WavetableGenerator provides helpers for generating wavetables.
 */
namespace WavetableGenerator
{
    // Wave "type" ids.
    enum WaveId {
        SINE = 0,
        TRIANGLE,
        SQUARE,
        _LAST_BASIC_WAVE = SQUARE,
    };

    static const double rawWaveformGain = 0.99;

    /**
     * create a sine wave
     */
    template <typename SampleType>
    void createSineWave(juce::AudioBuffer<SampleType> & wave)
    {
        auto* samples = wave.getWritePointer(0);

        // Note we don't calculate the last one in the cycle, which will be the same as the first.
        const double radDelta = TWOPI / static_cast<double>(wave.getNumSamples()-1);

        double currentRadians = 0.0;

        for (int ix = 0; ix < wave.getNumSamples(); ++ix)
        {
            auto sample = static_cast<SampleType>(std::sin(currentRadians));
            samples[ix] = sample * rawWaveformGain;
            currentRadians += radDelta;
        }
    }

    /**
     * Create a wave for placement in a wavetable. 
     *  
     * NOTE numSamples should be equally divisible by 4 to get 
     * a proper pointy triangle wave. 
     *  
     * The waveform generated has a max gain of 0.99 applied.  It is
     * up to oscillators to apply an appropriate gain level 
     * depending on the number of voices, etc. 
     */
    template <typename SampleType>
    juce::AudioBuffer<SampleType> createWave(
        const WaveId waveId, 
        const int numSamples) 
    {
        const int numChannels = 1;
        juce::AudioBuffer<SampleType> wave(numChannels, numSamples);
        const auto oscillatorGain = static_cast<SampleType>( config::oscillatorGain);

        switch (waveId) {
            case SINE: {
                createSineWave(wave);
                break;
            }
            case TRIANGLE: {
                auto* samples = wave.getWritePointer(0);

                const int numSamplesPerQuarter = numSamples / 4;
                jassert(numSamples % 4 == 0);

                double sampleDeltaAbs = rawWaveformGain / numSamplesPerQuarter;

                double currentSample = 0.0;

                for (int ix = 0; ix < numSamples; ++ix)
                {
                    samples[ix] = static_cast<SampleType>(currentSample);

                    // ramp up in first quarter
                    if ( ix < numSamplesPerQuarter ) {
                        currentSample += sampleDeltaAbs;
                    }
                    // ramp down for middle 2 quarters
                    else if ( ix < (numSamplesPerQuarter * 3) ) {
                        currentSample -= sampleDeltaAbs;
                    }
                    // then go back up
                    else {
                        currentSample += sampleDeltaAbs;
                    }
                }

                break;
            }
            case SQUARE: {  
                // first create a sine wave
                createSineWave(wave);
                SampleType * p = wave.getWritePointer(0);
                const SampleType maxValue = 0.99;
                for (int ix=0; ix<wave.getNumSamples(); ++ix, ++p) {
                    // increase the sine wave amplitude by a large factor so 
                    // the rising and falling parts become much steeper:
                    SampleType newValue = *p * 8192;
                    // then flatten off the tops that exceeded the max:
                    if (newValue > maxValue) 
                        newValue = maxValue;
                    else if (newValue < -maxValue ) {
                        newValue = -maxValue;
                    }
                    // write it
                    *p = newValue;
                }

                break;
            }
            default: {
                jassert(false && "Error - unrecognized wave ID in createWave()");
            }
        }

        return wave;
    }

    // old version of square wave:
    //auto* samples = wave.getWritePointer(0);
    //
    //const double flatAmplitude = rawWaveformGain;
    ////const double numSamplesFlatTop =    numSamples * 0.485f;
    ////const double numSamplesFlatBottom = numSamples * 0.485f;
    ////const double numSamplesRising =     numSamples * 0.015f;
    ////const double numSamplesFalling =    numSamples * 0.015f;
    //const double numSamplesFlatTop =    numSamples * 0.495f;
    //const double numSamplesFlatBottom = numSamples * 0.495f;
    //const double numSamplesRising =     numSamples * 0.005f;
    //const double numSamplesFalling =    numSamples * 0.005f;
    //const double risingDeltaAbs = (flatAmplitude * 2) / numSamplesRising;
    //
    //const int numPhases = 5;
    //const double phaseDivisionIncrements[numPhases] = {
    //    numSamplesRising / 2.0,
    //    numSamplesFlatTop,
    //    numSamplesFalling,
    //    numSamplesFlatBottom,
    //    numSamplesRising / 2.0
    //};
    //std::vector<double> pdi;
    //double lastOne = 0.0;
    //for(int ix=0; ix<numPhases; ++ix) {
    //    pdi.push_back(lastOne + phaseDivisionIncrements[ix]);
    //    lastOne = pdi.back();
    //}
    //const std::vector<double> & phaseDivisionIndexes = pdi;
    //
    //double currentSample = 0.0;
    //
    //for (int ix = 0; ix < numSamples; ++ix)
    //{
    //    samples[ix] = static_cast<SampleType>(currentSample);
    //
    //    // rising half
    //    if (ix < phaseDivisionIndexes[0]) {
    //        currentSample += risingDeltaAbs;
    //    }
    //    // flat 
    //    else if (ix < phaseDivisionIndexes[1]) {
    //        currentSample = flatAmplitude;
    //    }
    //    // falling
    //    else if (ix < phaseDivisionIndexes[2]) {
    //        currentSample -= risingDeltaAbs;
    //    }
    //    // flat 
    //    else if (ix < phaseDivisionIndexes[3]) {
    //        currentSample = -flatAmplitude;
    //    }
    //    // rising half
    //    else {
    //        currentSample += risingDeltaAbs;
    //    }
    //}

    /**
     * Create a wavetable with all the 'basic' waveforms (first 3 
     * WaveIds). 
     */
    template <typename SampleType>
    std::deque<juce::AudioBuffer<SampleType>> createBasicWavetable(
        const int numSamples)
    {
        std::deque<juce::AudioBuffer<SampleType>> d;
        for (int e = 0; e <= _LAST_BASIC_WAVE; ++e) {
            d.push_back(createWave<SampleType>(static_cast<WaveId>(e), numSamples));
        }
        return d;
    }


}


