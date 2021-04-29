/**
 * RealtimeSineSynthVoice 
 *  
 * A synth audio source that calculates the sine wave in real 
 * time. 
 *  
 * TODO this needs to be refactored to be an Oscillator not 
 * SynthesiserVoice, and a RealtimeSineSynthVoice probably needs to be created, 
 * or another Voice implementation could be made more generic so this oscillator 
 * can plug into that. 
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "UnlimitedSynthSound.h"
#include "juce_igutil/MTLogger.h"

/**
 * RealtimeSineWaveVoice - renders a voice in a synth based on midi 
 * notes 
 */
class RealtimeSineSynthVoice: public juce::SynthesiserVoice
{
public:
    
    RealtimeSineSynthVoice(
        std::shared_ptr<juce_igutil::MTLogger> _pMTL
    ): 
        juce::SynthesiserVoice(),
        pMTL(_pMTL)
    {
        // empty
    }

    virtual ~RealtimeSineSynthVoice() = default;

    // the tutorial says this is an example of how to 
    // check the type and maybe do something different, I guess?  
    // TODO Why not use appliesToNote/Channel() ?
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<UnlimitedSynthSound*> (sound) != nullptr;
    }

    /** 
     * startNote 
     *  
     * velocity is the midi velocity 0-127 mapped to 0.0 - 1.0
     */ 
    void startNote (
        int midiNoteNumber, 
        float velocity,
        juce::SynthesiserSound*, 
        int /*currentPitchWheelPosition*/
    ) override 
    {
        currentRadians = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;
        pMTL->debug(String("startNote:  velocity is ") + String(velocity) + String(", level is ") + String(level));

        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();

        radiansDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            radiansDelta = 0.0;
        }
    }

    void pitchWheelMoved (int) override      {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (
        juce::AudioSampleBuffer& outputBuffer, 
        int startSample, 
        int numSamples) 
    override
    {
        if (radiansDelta > 0.0)
        {
            if (tailOff > 0.0) // [7]
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float) (std::sin (currentRadians) * level * tailOff);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentRadians += radiansDelta;
                    ++startSample;

                    tailOff *= 0.99; // [8]
                    //tailOff *= 0.9999; // [8]

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote(); // [9]

                        radiansDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0) // [6]
                {
                    auto currentSample = (float) (std::sin (currentRadians) * level);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentRadians += radiansDelta;
                    ++startSample;
                }
            }
        }
    }
             
private:

    double currentRadians = 0.0;
    double radiansDelta = 0.0;
    double level = 0.0;
    double tailOff = 0.0;

    // logger
    std::shared_ptr<MTLogger> pMTL;
};

