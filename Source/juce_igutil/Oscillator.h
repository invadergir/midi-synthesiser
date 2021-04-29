/**
 * Interface for Oscillator types. 
 *  
 * Meant to be used with projects of single-precision audio processing. 
 */

#pragma once

namespace juce_igutil {

class Oscillator
{
protected:
    Oscillator() = default;

public:
    virtual ~Oscillator() = default;

    // prepare
    virtual void prepare (const juce::dsp::ProcessSpec& spec) noexcept = 0;

    // start a note
    virtual void startNote (
        int midiNoteNumber, 
        float velocity,
        int currentPitchWheelPosition
    ) = 0;

    // stop a note
    virtual void stopNote (float velocity, bool allowTailOff) = 0;

    // Returns true when the note is done playing.
    // TODO integrate with a module-wide SAMPLE_TYPE #define (to help avoid 
    // template proliferation).
    virtual bool renderNextBlock (
        juce::AudioBuffer<float> & outputBuffer, 
        int startSample, 
        const int numSamples
    ) noexcept = 0;

    /** Called to let the oscillator know that the pitch wheel has
     *  been moved. This will be called during the rendering
     *  callback, so must be fast and thread-safe.
    */
    virtual void pitchWheelMoved (int newPitchWheelValue) = 0;

    /** Called to let the oscillator know that a midi controller has
     *  been moved. This will be called during the rendering
     *  callback, so must be fast and thread-safe.
    */
    virtual void controllerMoved (int controllerNumber, int newControllerValue) = 0;
};

}
