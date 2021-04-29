#pragma once

#include "JuceHeader.h"

#include "Config.h"

#include "juce_igutil/Oscillator.h"

#define TWOPI (juce::MathConstants<double>::twoPi)

/**
 * This class is meant to be similar to juce::dsp::Oscillator 
 * except that it allows for global wavetables to be used 
 * instead of locally generated copies.  This saves a lot of 
 * memory for synths with a lot of voices or oscillators. 
 */
class WavetableOscillator : public juce_igutil::Oscillator
{
public:

    /**
     * constructor
     */
    WavetableOscillator(
        std::shared_ptr<juce_igutil::MTLogger> _pMTL,
        std::shared_ptr<juce::AudioProcessorValueTreeState> pSynthParams,
        const std::deque<juce::AudioBuffer<SAMPLE_TYPE>> & waveTableInUse
    ): 
        Oscillator(),
        pMTL(_pMTL),
        wavetable(waveTableInUse)
    {
        using namespace config;

        // empty
        jassert( !wavetable.empty() );

        pMTL->info("Oscillator: Connecting parameters...");
        pWavetableIndexParam = pSynthParams->getRawParameterValue(waveIndexPN);

        setWaves(pWavetableIndexParam);
    }

    // Default destructor
    virtual ~WavetableOscillator() = default;

    /**
     * Prepare to play some audio.  Sets the sample rate etc.
     */
    void prepare (const juce::dsp::ProcessSpec& spec) noexcept override
    {
        sampleRate = static_cast<float>(spec.sampleRate);
    }

    /**
     * start a note 
     *  
     * velocity comes in here as a float between 0 and 1. 
     */
    void startNote (
        int midiNoteNumber, 
        float velocity,
        int /*currentPitchWheelPosition*/
    ) override
    {
        waveSampleIndex = 0.0;
        level = velocity;
        tailOff = 0.0;

        // for this note, this is the number of cycles per second.
        const double noteHertz = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        //const double cyclesPerSecond = noteHertz;

        // note:   cycles / second     /        samples / second
        // for this note, this is the fraction of a cycle per sample:
        const double cyclesPerSample = noteHertz             / sampleRate;
                 //                    27 - 440 (A) - 4186     >= 44,100 (48,000 etc)

        // The fraction of a cycle (in terms of number of samples in the wave,
        // rather than 2pi.
        const juce::AudioBuffer<SAMPLE_TYPE> & wave = wavetable[static_cast<int>(*pWavetableIndexParam)];
        waveCycleDelta = cyclesPerSample * (wave.getNumSamples());

        /* 
        noteHertz = 2 
         
        1 second = 
        |------+------| 
         
        wave samples (4=size of wave): 
        0 1 2 3
         
        sample rate / samples per second (8): 
        0 1 2 3 4 5 6 7
         
        cyclesPerSample = 0.25 = 2 / 8 
        waveCycleDelta = cyclesPerSample * sizeOfWave = 0.25 * 4 = 1
        */
    }

    /**
     * Stop the current note and start the tail-off.
     * Tail-off is always enabled except when calling stopAll().
     */
    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff) 
        {
            if (tailOff < 0.000000000000001)
                tailOff = 1.0;
        }
        else
        {
            waveCycleDelta = 0.0;
        }
    }

    // performance notes:
    //inline double chooseBetweenSamples(
    //    const double & low, 
    //    const double & high,
    //    const double & fractionBetween) 
    //{
    //    // first way:
    //    return (((high - low) * fractionBetween) + low);
    //    //Perf Stats:  minNanos=93'000, maxNanos=630'700, nanosAvg=181'624, nanosCount=5000
    //
    //    // 2nd way - actually slower:
    //    //if (fractionBetween < 0.5) return low;
    //    //else return high;
    //    //Perf Stats:  minNanos=116'200, maxNanos=719'700, nanosAvg=214'725, nanosCount=5000
    //}

    /**
     * Render the next block of audio. 
     *  
     * Returns true when the note has finished (tail off or release 
     * envelope finished) 
     */
    bool renderNextBlock (
        juce::AudioBuffer<SAMPLE_TYPE> & outputBuffer, 
        int startSample, 
        const int numSamples
    ) noexcept override
    {
        bool noteDone = false;
        int outputSampleIndex = startSample;

        // Only generate waveform if there is a note currently assigned to this oscillator.
        if (waveCycleDelta > 0.0)
        {
            // update wave(s) in use
            setWaves(pWavetableIndexParam);

            if (tailOff > 0.0) // [7]
            {
                for (int n = 0; !noteDone && n < numSamples; ++n ) 
                {
                    outputNextSample(outputBuffer, outputSampleIndex);

                    // TODO add ADSR envelope
                    tailOff *= 0.99;

                    if (tailOff <= 0.005)
                    {
                        waveCycleDelta = 0.0;
                        noteDone = true;
                    }
                }
            }
            else
            {
                for (int n = 0; n < numSamples; ++n )
                {
                    outputNextSample(outputBuffer, outputSampleIndex);
                }
            }
        }
        return noteDone;
    }

    /** Called to let the voice know that the pitch wheel has been moved.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */
    void pitchWheelMoved (int /*newPitchWheelValue*/) override 
    {
        // no pitch wheel impl yet
    }

    /** Called to let the voice know that a midi controller has been moved.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */
    void controllerMoved (int /*controllerNumber*/, int /*newControllerValue*/) override
    {
        // no controllers implemented (yet)
    }

private:

    /**
     *  Set the low and high waves based on the current wavetable index.
     */
    inline void setWaves(const std::atomic<float> * pIndex)
    {
        const float index = *pIndex;
        const int low = static_cast<int>( index );
        const int high = ceil(index);
        jassert(low >= 0);
        jassert(low < wavetable.size());
        jassert(high >= 0);
        jassert(high < wavetable.size());

        pLowWave = &(wavetable[low]);
        pHighWave = &(wavetable[high]);
        ratioHighToLow = index - low;
        jassert( ratioHighToLow > -0.0000000001 );
        jassert( ratioHighToLow < 1.000000000001 );
    }

    // get the sample from a wave
    inline SAMPLE_TYPE getSampleFromWave(const juce::AudioBuffer<SAMPLE_TYPE> & wave) 
    {
        const auto * readPointer = wave.getReadPointer(0);
        const int indexFloor = (int)waveSampleIndex;
        const auto low = readPointer[indexFloor];
        const int indexCeil = (indexFloor + 1) % pLowWave->getNumSamples();
        const auto high = readPointer[indexCeil];
        const double indexDiff = waveSampleIndex - (double)indexFloor;
        const double rawSample = low + ((high - low) * indexDiff);
        return rawSample;
    }

    // calculate the next sample.  Optimized version; doesn't use radians.
    inline SAMPLE_TYPE getNextSample() 
    {
        using namespace juce;

        // bump the index and wrap
        waveSampleIndex += waveCycleDelta;
        if (waveSampleIndex > (double)pLowWave->getNumSamples() ) {
            waveSampleIndex -= (double)pLowWave->getNumSamples();
        }

        jassert(pLowWave);
        jassert(pHighWave);

        // mix the two waves' samples together depending on the ratio
        SAMPLE_TYPE lowSample = getSampleFromWave(*pLowWave);
        SAMPLE_TYPE rawSample;
        if ( pLowWave == pHighWave ) {
            rawSample = lowSample;
        }
        else {
            SAMPLE_TYPE highSample = getSampleFromWave(*pHighWave);
            rawSample = highSample * ratioHighToLow + lowSample * (1.0 - ratioHighToLow);
        }

        double currentSample = 
            // limit to the max gain for an oscillator - TODO make configurable
            rawSample * config::oscillatorGain *
            // and apply the velocity gain
            level;

        if (tailOff > 0.0)
            currentSample *= tailOff;

        if (currentSample > 0.99 || currentSample < -0.99) {
            pMTL->error("ERROR - getNextSample() returned out-of-bounds value "+String(currentSample));
        }

        return static_cast<SAMPLE_TYPE>(currentSample);
    }

    /**
     * calculate the next sample and store it in the output buffer.
     */
    inline void outputNextSample(
        juce::AudioBuffer<SAMPLE_TYPE> & outputBuffer, 
        int & outputSampleIndex)
    {
        const SAMPLE_TYPE currentSample = getNextSample();

        for (int i = 0; i < outputBuffer.getNumChannels(); ++i) 
            outputBuffer.addSample(i, outputSampleIndex, currentSample);

        ++outputSampleIndex;
    }

    // logger
    std::shared_ptr<juce_igutil::MTLogger> pMTL;

    float sampleRate = 48000.0;
    //uint32 numChannels = 2;

    double waveSampleIndex = 0.0;
    double waveCycleDelta = 0.0;

    double level = 0.0;
    double tailOff = 0.0;

    // Wavetable, containing potentially multiple waveforms (waves).
    // Todo a shared_ptr would be better.
    const std::deque<juce::AudioBuffer<SAMPLE_TYPE>> & wavetable;

    // current wave in use - low and high (for x index, low=floor(), high=ceil()
    const juce::AudioBuffer<SAMPLE_TYPE> * pLowWave;
    float ratioHighToLow = 1.0;
    const juce::AudioBuffer<SAMPLE_TYPE> * pHighWave;

    // Params
    const std::atomic<float> * pWavetableIndexParam;
};

