/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "juce_igutil/SynthAudioSource.h"
#include "juce_igutil/Stopwatch.h"

#include "juce_igutil/MTLogger.h"
#include "juce_igutil/Profiler.h"

#include "Config.h"
#include "AudioBufferQueue.h"
#include "ScopeDataCollector.h"

// forward decl
class MidisynthesiserAudioProcessorEditor;

//==============================================================================
/**
*/
class MidisynthesiserAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    MidisynthesiserAudioProcessor();
    ~MidisynthesiserAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Allow access to the keyboard state
    // TODO thread safety - this is used from process() thread as well as editor.
    juce::MidiKeyboardState & getKeyboardState() { return keyboardState; }

    // Allow access to the audiobufferqueue.
    // Note ABQ is a lockfree, threadsafe single-reader, single-writer FIFO queue.
    AudioBufferQueue<float>& getAudioBufferQueue() noexcept { return audioBufferQueue; }

private:

    //==============================================================================

    // logger and helper
    std::shared_ptr<juce::FileLogger> pLogger;
    std::shared_ptr<juce_igutil::MTLogger> pMTL;
    
    // profiler
    std::unique_ptr<juce_igutil::Profiler> pProfiler;

    // the synth (audio source)
    std::shared_ptr<juce_igutil::SynthAudioSource> pSynthAudioSource;

    // midi keyboard state tracker, used in conjunction with on-screen keyboard.
    // This could be handled inside the synth - or not.  If you don't need an 
    // on-screen keyboard this is optional.
    juce::MidiKeyboardState keyboardState;

    // collects data for the oscilloscope
    AudioBufferQueue<SAMPLE_TYPE> audioBufferQueue;
    ScopeDataCollector<SAMPLE_TYPE> scopeDataCollector { audioBufferQueue };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidisynthesiserAudioProcessor)
};

