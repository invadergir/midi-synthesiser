/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "juce_igutil/MTLogger.h"
#include "juce_igutil/Profiler.h"
#include "juce_igutil/ConfigurableSynthAudioSource.h"

#include "Config.h"
#include "Debug.h"
#include "WavetableGenerator.h"
#include "WavetableSynth.h"
#include "WavetableSynthVoice.h"

using namespace juce;
using namespace juce_igutil;
using namespace std;

using namespace config;

using EditorType = MidisynthesiserAudioProcessorEditor;

//#define AUTO_PLAY_CHORD
//#define LOG_MIDI_NOTES

//==============================================================================
/**
 * Processor constructor
 */
MidisynthesiserAudioProcessor::MidisynthesiserAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    pLogger(std::shared_ptr<juce::FileLogger>(
        FileLogger::createDefaultAppLogger(
            "midi-synthesiser", 
            "midi-synthesiser.txt", 
            "Processor started."))),
    pMTL(std::make_shared<MTLogger>(pLogger))

#endif
{
    Logger::setCurrentLogger(pLogger.get());
    pLogger->logMessage("Audio Processor CONSTRUCTOR.");

    pProfiler.reset(new Profiler("MidisynthesizerAudioProcessor_Profiler", pMTL, 1000));

    pLogger->logMessage("Creating wavetables...");
    deque<juce::AudioBuffer<WTSampleType>> wavetable = 
        WavetableGenerator::createBasicWavetable<WTSampleType>(wavetableNumSamples);
    for ( int ix=0; ix<wavetable.size(); ++ix ) {
        pMTL->info(String("Checking wavetable #") + String(ix));
        debug::checkOutput(wavetable[ix], pMTL);
    }

    pLogger->logMessage("Creating audio parameter layout...");
    // standard "always-on" params:  
    AudioProcessorValueTreeState::ParameterLayout paramLayout(
        make_unique<juce::AudioParameterFloat>(
            gainPN,            // parameterID
            "Gain",            // parameter name
            0.0f,              // minimum value
            1.0f,              // maximum value
            0.5f               // default value
        )
        ,make_unique<juce::AudioParameterFloat>(
            waveIndexPN,            // parameterID
            "Wave Index",            // parameter name
            0.0f,              // minimum value
            2.0f,              // maximum value
            0.0                // default value
        )
        ,make_unique<juce::AudioParameterFloat>(
            resonancePN,            // parameterID
            "Resonance",            // parameter name
            0.0f,              // minimum value
            1.0f,              // maximum value
            0.0                // default value
        )
        ,make_unique<juce::AudioParameterFloat>(
            cutoffPN,            // parameterID
            "Cutoff Frequency",            // parameter name
            NormalisableRange<float>(
                20.0 //ValueType rangeStart,        
                ,20'000 //ValueType rangeEnd,          
                ,0.0001 //ValueType intervalValue,     
                ,0.25 //ValueType skewFactor,        
                //bool useSymmetricSkew = false
            ),
            20'000.0                // default value
        )
    );
    // for each effect:
    for (int ix=0; ix<maxEffects; ++ix) {
        // fx type selector

        // The drop down choices list has to be the same number of values as 
        // expected.  This does NOT overwrite anything you add to the dropdown widget:
        StringArray ddChoices;
        for ( int ix=0; ix<NUM_EFFECTS; ++ix ) ddChoices.add("");

        paramLayout.add(make_unique<juce::AudioParameterChoice>(
            getEffectPN(typeSelectorPN, ix), // parameterID
            "fx type",  // parameter name
            ddChoices, //const StringArray& choices,
            0, //int defaultItemIndex,                                                                 
            "" //const String& parameterLabel = String(), not sure what this does
        ));
        // fx level 
        paramLayout.add(make_unique<juce::AudioParameterFloat>(
            getEffectPN(fxLevelPN, ix),  // parameterID
            "Level",            // parameter name
            0.0f,              // minimum value
            1.0f,              // maximum value
            0.5                // default value
        ));
    }

    pLogger->logMessage("Creating Synth...");
    pSynthAudioSource.reset(new WavetableSynth(
        pMTL,
        // parameters
        std::shared_ptr<juce::AudioProcessorValueTreeState>( new juce::AudioProcessorValueTreeState(
            *this, // needed for visitation.  We get a call back directly when calling.
                   // TODO it would be nice to not give away this until the constructor is done;
                   // is there somewhere else we can init this?  prepare() maybe?
            nullptr, // todo - add an UndoManager
            juce::Identifier("MidisynthesiserAudioProcessor_VTS"),
            move(paramLayout)
        )),
        UnlimitedSynthSound::Ptr(new UnlimitedSynthSound),
        move(wavetable),
        config::numVoices,
        keyboardState
    ));

    pLogger->logMessage("Audio Processor instantiated.");
}

/**
 * Processor destructor
 */
MidisynthesiserAudioProcessor::~MidisynthesiserAudioProcessor()
{
    // I guess the logger, that does not own this pointer, can't abide
    // being destructed if it is the current logger ...shrug...
    Logger::setCurrentLogger(nullptr);
}

//==============================================================================
const juce::String MidisynthesiserAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidisynthesiserAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MidisynthesiserAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MidisynthesiserAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MidisynthesiserAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidisynthesiserAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MidisynthesiserAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MidisynthesiserAudioProcessor::setCurrentProgram (int /*index*/)
{
    // empty - todo what is this method for?
}

const juce::String MidisynthesiserAudioProcessor::getProgramName (int /*index*/)
{
    // empty - todo what is this method for?
    return {};
}

void MidisynthesiserAudioProcessor::changeProgramName (int /*index*/, const juce::String & /*newName*/)
{
    // empty - todo what is this method for?
}

//==============================================================================
void MidisynthesiserAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::uint32 numChannels = getTotalNumOutputChannels(); 
    auto processSpec = dsp::ProcessSpec{
        sampleRate, 
        static_cast<uint32>(samplesPerBlock),
        numChannels
    };

    pSynthAudioSource->prepareToPlay(processSpec);
}

void MidisynthesiserAudioProcessor::releaseResources()
{
    pSynthAudioSource->releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidisynthesiserAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

/**
 * Play a chord to enable testing knobs without a midi keyboard:
 */
static enum ChordType {
    NO_CHORD
    ,BIG_CHORD
    ,LOW_FIFTH
};
void playChord(juce::MidiBuffer & incomingMidi, const ChordType = LOW_FIFTH)
{
    //------------------------------------------------------------
    // For testing knobs without a midi keyboard:
    static ChordType initialChord = LOW_FIFTH;
    if (initialChord > NO_CHORD ) {
        const int channel = 1;
        const float velocity = 0.90f;
        int sampleNum = 0;
        incomingMidi.clear();
        incomingMidi.ensureSize(12);
        if (initialChord == BIG_CHORD) {
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 36, velocity), sampleNum);
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 48, velocity), sampleNum);
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 60, velocity), sampleNum);
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 67, velocity), sampleNum);
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 72, velocity), sampleNum);
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 76, velocity), sampleNum);
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 79, velocity), sampleNum);
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 84, velocity), sampleNum);
            initialChord = NO_CHORD;
        }
        else if (initialChord == LOW_FIFTH) {
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 52, velocity), sampleNum);
            incomingMidi.addEvent(MidiMessage::noteOn(channel, 59, velocity), sampleNum);
            initialChord = NO_CHORD;
        }
    }
}

/**
 * Process the next midi messages and output audio.
 */
void MidisynthesiserAudioProcessor::processBlock(
    juce::AudioBuffer<float> & buffer, 
    juce::MidiBuffer & midiMessages
)
{
    juce::ScopedNoDenormals noDenormals;

    //juce::MidiBuffer& incomingMidi(midiMessages);

    // Enable to play a chord once on startup.
#ifdef AUTO_PLAY_CHORD
    playChord(midiMessages, LOW_FIFTH);
#endif // AUTO_PLAY_CHORD

#ifdef LOG_MIDI_NOTES
    for (auto it = midiMessages.cbegin(); it != midiMessages.cend(); ++it) {
        const auto & msg = (*it).getMessage();
        pMTL->info(String("Midi message received: noteNumber ") + String(msg.getNoteNumber()) + ", " + msg.getDescription());
    }
#endif //LOG_MIDI_NOTES

    //double getSampleRate()

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    //pMTL->info(String("totalNumInputChannels = ") + String(totalNumInputChannels) );   // 0
    //pMTL->info(String("totalNumOutputChannels = ") + String(totalNumOutputChannels) ); // 2
    jassert(totalNumInputChannels == 0);
    jassert(totalNumOutputChannels == 2);

    //pProfiler->start();

    pSynthAudioSource->renderNextBlock(buffer, midiMessages, 0);

    scopeDataCollector.process(buffer.getReadPointer(0), static_cast<size_t>(buffer.getNumSamples()));

    //pProfiler->stop();
}

//------------------------------------------------------------------------------
// Performance Test Results:
//
// With RealtimeSineSynthVoice (1st time with warmup):
// Perf Stats:  minNanos=79'200, maxNanos=514'000, nanosAvg=144'791, nanosCount=5000
// Perf Stats:  minNanos=79'400, maxNanos=519'400, nanosAvg=146'738, nanosCount=5000
// 
// With WavetableSynthVoice (1st time with warmup):
// Perf Stats:  minNanos=92'800, maxNanos=650'600, nanosAvg=176'455, nanosCount=5000
// Perf Stats:  minNanos=98'200, maxNanos=587'500, nanosAvg=185'936, nanosCount=5000
//
//   Before mods to use waveSampleIndex rather than currentRadians (similar to above):
//   Perf Stats:  minNanos=93'300, maxNanos=626'200, nanosAvg=181'661, nanosCount=5000
// 
//   After mods to use waveSampleIndex rather than currentRadians (WINNER!  lower realtime requirements than the realtime sine calculation):
//   Perf Stats:  minNanos=70'100, maxNanos=338'100, nanosAvg=123'156, nanosCount=5000
//   Perf Stats:  minNanos=69'500, maxNanos=367'100, nanosAvg=119'454, nanosCount=5000
// 
//   Removed double-array lookup:
//   Perf Stats:  minNanos=93'800, maxNanos=646'800, nanosAvg=194'920, nanosCount=5000
// 
//   Not calculating difference between samples, just picking closest one:
//   Perf Stats:  minNanos=114'000, maxNanos=681'700, nanosAvg=205'656, nanosCount=5000
//   Perf Stats:  minNanos=104'300, maxNanos=605'200, nanosAvg=195'247, nanosCount=5000
//   Perf Stats:  minNanos=104'900, maxNanos=675'100, nanosAvg=195'891, nanosCount=5000
// 
// ---- Old perf stats - pre-warmup ----
//
//   With RealtimeSineSynthVoice (1st time):
//   Perf Stats:  minNanos=80'000, maxNanos=546'800, nanosAvg=141'654, nanosCount=5000
//   Perf Stats:  minNanos=80'200, maxNanos=2'181'100, nanosAvg=143'928, nanosCount=5000
//   
//   With RealtimeSineSynthVoice (2nd time):
//   Perf Stats:  minNanos=153'100, maxNanos=666'300, nanosAvg=233'263, nanosCount=5000
//   Perf Stats:  minNanos=85'100, maxNanos=477'100, nanosAvg=150'639, nanosCount=5000
//   
//   With WavetableSynthVoice (initial):
//   Perf Stats:  minNanos=93'700, maxNanos=1'188'400, nanosAvg=177'086, nanosCount=5000
//   Perf Stats:  minNanos=101'800, maxNanos=2'235'300, nanosAvg=182'671, nanosCount=5000
//------------------------------------------------------------------------------

// Double-precision version
void MidisynthesiserAudioProcessor::processBlock (
    juce::AudioBuffer<double> & buffer, 
    juce::MidiBuffer & midiMessages
)
{
    pMTL->error("ERROR: can't process double precision audio at the present."); // TODO implement juce-double-precision-poc
    jassert(false);
}

//==============================================================================
bool MidisynthesiserAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MidisynthesiserAudioProcessor::createEditor()
{
    return new MidisynthesiserAudioProcessorEditor(*this, pSynthAudioSource->getSynthParams());
}

//==============================================================================
// serialize state from memory block (save params) - this virtual function name is 
// confusing - "getting" the state implies reading or deserializing, but this actually 
// serializes the data into a memory block.
void MidisynthesiserAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{   
    auto pParams = pSynthAudioSource->getSynthParams();

    // TODO this could be generalized somewhere
    ValueTree state = pParams->copyState();

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

// DEserialize state into the VTS (set the params)
void MidisynthesiserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto pParams = pSynthAudioSource->getSynthParams();

    // TODO this could be generalized somewhere
    std::unique_ptr<juce::XmlElement> xmlState( getXmlFromBinary(data, sizeInBytes) );

    if( xmlState.get() != nullptr )
    {
        if( xmlState->hasTagName( pParams->state.getType()) )
        {
            pParams->replaceState( juce::ValueTree::fromXml( *xmlState ) );
        }

    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidisynthesiserAudioProcessor();
}
